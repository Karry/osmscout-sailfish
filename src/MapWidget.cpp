/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MapWidget.h"
#include "InputHandler.h"

#include <iostream>
#include <sys/socket.h>

//! We rotate in 16 steps
static double DELTA_ANGLE=2*M_PI/16.0;

MapWidget::MapWidget(QQuickItem* parent)
    : QQuickPaintedItem(parent),
      inputHandler(NULL), 
      showCurrentPosition(false)
{
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    
    DBThread *dbThread=DBThread::GetInstance();
    dpi = dbThread->GetMapDpi();
    tapRecognizer.setPhysicalDpi(dbThread->GetPhysicalDpi());

    //setFocusPolicy(Qt::StrongFocus);

    connect(dbThread,SIGNAL(Redraw()),
            this,SLOT(redraw()));    
    
    connect(&tapRecognizer, SIGNAL(tap(const QPoint)),        this, SLOT(onTap(const QPoint)));
    connect(&tapRecognizer, SIGNAL(doubleTap(const QPoint)),  this, SLOT(onDoubleTap(const QPoint)));
    connect(&tapRecognizer, SIGNAL(longTap(const QPoint)),    this, SLOT(onLongTap(const QPoint)));
    connect(&tapRecognizer, SIGNAL(tapLongTap(const QPoint)), this, SLOT(onTapLongTap(const QPoint)));

    // TODO, open last position, move to current position or get as constructor argument...
    view = { osmscout::GeoCoord(0.0, 0.0), 0, osmscout::Magnification::magContinent  };
    setupInputHandler(new InputHandler(view));
    setKeepTouchGrab(true);

    setRenderTarget(RenderTarget::FramebufferObject);
    setPerformanceHints(PerformanceHint::FastFBOResizing);
}

MapWidget::~MapWidget()
{
    delete inputHandler;
}

void MapWidget::translateToTouch(QMouseEvent* event, Qt::TouchPointStates states)
{
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);

    QTouchEvent::TouchPoint touchPoint;
    touchPoint.setPressure(1);
    touchPoint.setPos(mEvent->pos());
    touchPoint.setState(states);
    
    QList<QTouchEvent::TouchPoint> points;
    points << touchPoint;
    QTouchEvent *touchEvnt = new QTouchEvent(QEvent::TouchBegin,0, Qt::NoModifier, 0, points);
    //qDebug() << "translate mouse event to touch event: "<< touchEvnt;
    touchEvent(touchEvnt);
    delete touchEvnt;
}
void MapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button()==1) {
        translateToTouch(event, Qt::TouchPointPressed);
    }
}
void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
    translateToTouch(event, Qt::TouchPointMoved);
}
void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button()==1) {
        translateToTouch(event, Qt::TouchPointReleased);
    }
}
 
void MapWidget::setupInputHandler(InputHandler *newGesture)
{
    if (inputHandler != NULL)
        delete inputHandler;
    inputHandler = newGesture;
    
    connect(inputHandler, SIGNAL(viewChanged(const MapView&)), 
            this, SLOT(viewChanged(const MapView&)));
    
    //qDebug() << "Input handler changed (" << (newGesture->animationInProgress()? "animation": "stationary") << ")";
}

void MapWidget::redraw()
{
    update();
}

void MapWidget::viewChanged(const MapView &updated)
{
    //qDebug() << "viewChanged: " << QString::fromStdString(updated.center.GetDisplayText()) << "   level: " << updated.magnification.GetLevel();
    //qDebug() << "viewChanged (" << (inputHandler->animationInProgress()? "animation": "stationary") << ")";

    view = updated;
    redraw();
}

void MapWidget::touchEvent(QTouchEvent *event)
{
  
    if (!inputHandler->touch(event)){
        if (event->touchPoints().size() == 1){
            QTouchEvent::TouchPoint tp = event->touchPoints()[0];
            setupInputHandler(new DragHandler(view, dpi));
        }else{
            setupInputHandler(new MultitouchHandler(view, dpi));
        }
        inputHandler->touch(event);
    }

    tapRecognizer.touch(event);
    
    event->accept();
  
    /*
    qDebug() << "touchEvent:";
    QList<QTouchEvent::TouchPoint> relevantTouchPoints;
    for (QTouchEvent::TouchPoint tp: event->touchPoints()){
      Qt::TouchPointStates state(tp.state());
      qDebug() << "  " << state <<" " << tp.id() << 
              " pos " << tp.pos().x() << "x" << tp.pos().y() << 
              " @ " << tp.pressure();    
    }
    */
 }

void MapWidget::wheelEvent(QWheelEvent* event)
{
    int numDegrees=event->delta()/8;
    int numSteps=numDegrees/15;

    if (numSteps>=0) {
        zoomIn(numSteps*1.35, event->pos());
    }
    else {
        zoomOut(-numSteps*1.35, event->pos());
    }

    event->accept();
}

void MapWidget::paint(QPainter *painter)
{
    DBThread *dbThread = DBThread::GetInstance();

    bool animationInProgress = inputHandler->animationInProgress();
    
    painter->setRenderHint(QPainter::Antialiasing, !animationInProgress);
    painter->setRenderHint(QPainter::TextAntialiasing, !animationInProgress);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, !animationInProgress);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, !animationInProgress);
    
    RenderMapRequest request;
    QRectF           boundingBox = contentsBoundingRect();

    request.lat = view.center.GetLat();
    request.lon = view.center.GetLon();
    request.angle = view.angle;
    request.magnification = view.magnification;
    request.width = boundingBox.width();
    request.height = boundingBox.height();

    dbThread->RenderMap(*painter,request);

    // render current position spot
    if (showCurrentPosition && locationValid){
        osmscout::MercatorProjection projection = getProjection();
        
        double x;
        double y;
        projection.GeoToPixel(osmscout::GeoCoord(lat, lon), x, y);
        if (boundingBox.contains(x, y)){
            
            if (horizontalAccuracyValid){
                double diameter = horizontalAccuracy * projection.GetMeterInPixel();
                if (diameter > 25.0 && diameter < std::max(request.width, request.height)){
                    painter->setBrush(QBrush(QColor::fromRgbF(1.0, 1.0, 1.0, 0.4)));
                    painter->setPen(QColor::fromRgbF(1.0, 1.0, 1.0, 0.7));
                    painter->drawEllipse(x - (diameter /2.0), y - (diameter /2.0), diameter, diameter);
                }
            }
            
            painter->setBrush(QBrush(QColor::fromRgbF(0,1,0, .6)));
            painter->setPen(QColor::fromRgbF(0.0, 0.5, 0.0, 0.9));
            painter->drawEllipse(x - 10, y - 10, 20, 20);
        }
    }
}

void MapWidget::zoom(double zoomFactor)
{
    zoom(zoomFactor, QPoint(width()/2, height()/2));
}

void MapWidget::zoomIn(double zoomFactor)
{
    zoomIn(zoomFactor, QPoint(width()/2, height()/2));
}

void MapWidget::zoomOut(double zoomFactor)
{
    zoomOut(zoomFactor, QPoint(width()/2, height()/2));
}

void MapWidget::zoom(double zoomFactor, const QPoint widgetPosition)
{
  if (zoomFactor == 1)
    return;
  
  if (!inputHandler->zoom(zoomFactor, widgetPosition, QRect(0, 0, width(), height()))){
    setupInputHandler(new MoveHandler(view, dpi));
    inputHandler->zoom(zoomFactor, widgetPosition, QRect(0, 0, width(), height()));
  }
}

void MapWidget::zoomIn(double zoomFactor, const QPoint widgetPosition)
{
    zoom(zoomFactor, widgetPosition);
}

void MapWidget::zoomOut(double zoomFactor, const QPoint widgetPosition)
{
    zoom( 1.0/zoomFactor, widgetPosition);
}

void MapWidget::move(QVector2D vector)
{
    if (!inputHandler->move(vector)){
        setupInputHandler(new MoveHandler(view, dpi));
        inputHandler->move(vector);
    }
}

void MapWidget::left()
{
    move(QVector2D( width()/-3, 0 ));
}

void MapWidget::right()
{
    move(QVector2D( width()/3, 0 ));
}

void MapWidget::up()
{
    move(QVector2D( 0, height()/-3 ));
}

void MapWidget::down()
{
    move(QVector2D( 0, height()/+3 ));
}

void MapWidget::rotateLeft()
{
    if (!inputHandler->rotateBy(DELTA_ANGLE, -DELTA_ANGLE)){
        setupInputHandler(new MoveHandler(view, dpi));
        inputHandler->rotateBy(DELTA_ANGLE, -DELTA_ANGLE);
    }
}

void MapWidget::rotateRight()
{
    if (!inputHandler->rotateBy(DELTA_ANGLE, DELTA_ANGLE)){
        setupInputHandler(new MoveHandler(view, dpi));
        inputHandler->rotateBy(DELTA_ANGLE, DELTA_ANGLE);
    }
}

void MapWidget::toggleDaylight()
{
    DBThread *dbThread=DBThread::GetInstance();

    dbThread->ToggleDaylight();
    redraw();
}

void MapWidget::reloadStyle()
{
    DBThread *dbThread=DBThread::GetInstance();

    dbThread->ReloadStyle();
    redraw();
}

void MapWidget::showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification)
{
    if (!inputHandler->showCoordinates(coord, magnification)){
        setupInputHandler(new JumpHandler(view));
        inputHandler->showCoordinates(coord, magnification);
        //view = { coord, view.angle, magnification  };
        //viewChanged(view);
    }
}

void MapWidget::showCoordinates(double lat, double lon)
{
    showCoordinates(osmscout::GeoCoord(lat,lon), osmscout::Magnification::magVeryClose);
}

void MapWidget::showCoordinatesInstantly(osmscout::GeoCoord coord, osmscout::Magnification magnification)
{
    view.magnification = magnification;
    view.center = coord;
    setupInputHandler(new InputHandler(view));
    redraw();
}

void MapWidget::showCoordinatesInstantly(double lat, double lon)
{
    showCoordinatesInstantly(osmscout::GeoCoord(lat,lon), osmscout::Magnification::magVeryClose);    
}

void MapWidget::showLocation(Location* location)
{
    if (location==NULL) {
        qDebug() << "MapWidget::showLocation(): no location passed!";
        return;
    }

    qDebug() << "MapWidget::showLocation(\"" << location->getName().toLocal8Bit().constData() << "\")";

    if (location->getType()==Location::typeObject) {
        osmscout::ObjectFileRef reference=location->getReferences().front();

        DBThread* dbThread=DBThread::GetInstance();

        if (reference.GetType()==osmscout::refNode) {
            osmscout::NodeRef node;

            if (dbThread->GetNodeByOffset(reference.GetFileOffset(),node)) {
                showCoordinates(node->GetCoords(), osmscout::Magnification::magVeryClose);    
            }
        }
        else if (reference.GetType()==osmscout::refArea) {
            osmscout::AreaRef area;

            if (dbThread->GetAreaByOffset(reference.GetFileOffset(),area)) {
                osmscout::GeoCoord coord;
                if (area->GetCenter(coord)) {
                    showCoordinates(coord, osmscout::Magnification::magVeryClose);    
                }
            }
        }
        else if (reference.GetType()==osmscout::refWay) {
            osmscout::WayRef way;

            if (dbThread->GetWayByOffset(reference.GetFileOffset(),way)) {
                osmscout::GeoCoord coord;
                if (way->GetCenter(coord)) {
                    showCoordinates(coord, osmscout::Magnification::magVeryClose);    
                }
            }
        }
        else {
            assert(false);
        }
    }
    else if (location->getType()==Location::typeCoordinate) {
        osmscout::GeoCoord coord=location->getCoord();

        qDebug() << "MapWidget: " << coord.GetDisplayText().c_str();

        showCoordinates(coord, osmscout::Magnification::magVeryClose);    

    }
}
void MapWidget::locationChanged(bool locationValid, double lat, double lon, bool horizontalAccuracyValid, double horizontalAccuracy)
{
    // location
    lastUpdate.restart();
    this->locationValid = locationValid;
    this->lat = lat; 
    this->lon = lon;
    this->horizontalAccuracyValid = horizontalAccuracyValid;
    this->horizontalAccuracy = horizontalAccuracy;
    redraw();
}

void MapWidget::onTap(const QPoint p)
{
    qDebug() << "tap " << p;
    double lat;
    double lon;
    getProjection().PixelToGeo(p.x(), p.y(), lon, lat);
    emit tap(p.x(), p.y(), lat, lon);
}
void MapWidget::onDoubleTap(const QPoint p)
{
    qDebug() << "double tap " << p;
    zoomIn(2.0, p);
    double lat;
    double lon;
    getProjection().PixelToGeo(p.x(), p.y(), lon, lat);
    emit doubleTap(p.x(), p.y(), lat, lon);
}
void MapWidget::onLongTap(const QPoint p)
{
    qDebug() << "long tap " << p;
    double lat;
    double lon;
    getProjection().PixelToGeo(p.x(), p.y(), lon, lat);
    emit longTap(p.x(), p.y(), lat, lon);
}
void MapWidget::onTapLongTap(const QPoint p)
{
    qDebug() << "tap, long tap " << p;
    zoomOut(2.0, p);
    double lat;
    double lon;
    getProjection().PixelToGeo(p.x(), p.y(), lon, lat);
    emit tapLongTap(p.x(), p.y(), lat, lon);
}

