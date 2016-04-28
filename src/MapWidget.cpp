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

//! We rotate in 16 steps
static double DELTA_ANGLE=2*M_PI/16.0;

MapWidget::MapWidget(QQuickItem* parent)
    : QQuickPaintedItem(parent),
      //center(0.0,0.0),
      //angle(0.0),
      //magnification(64),
      //mouseDragging(false),
      //dbInitialized(false),
      //hasBeenPainted(false),
      
      //currentTouchPoints(),
      inputHandler(NULL)
{
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    
    DBThread *dbThread=DBThread::GetInstance();
    dpi = dbThread->GetDpi();

    //setFocusPolicy(Qt::StrongFocus);

    //connect(dbThread,SIGNAL(InitialisationFinished(DatabaseLoadedResponse)),
    //        this,SLOT(initialisationFinished(DatabaseLoadedResponse)));

    //connect(this,SIGNAL(TriggerMapRenderingSignal(RenderMapRequest)),
    //        dbThread,SLOT(TriggerMapRendering(RenderMapRequest)));

    connect(dbThread,SIGNAL(HandleMapRenderingResult()),
            this,SLOT(redraw()));

    connect(dbThread,SIGNAL(Redraw()),
            this,SLOT(redraw()));    
    
    // todo, open last position, move to current position or get as constructor argument...
    view = { osmscout::GeoCoord(0.0, 0.0), 0, osmscout::Magnification::magContinent  };
    setupInputHandler(new InputHandler(view));
        
    // db thread can be initialized before creating this Widget, 
    // to avoid this race condition, check if database is initialized already
    // and call our slot manually
    /*
    if (dbThread->isInitialized()){
        initialisationFinished(dbThread->loadedResponse());
    }
     * */
}

MapWidget::~MapWidget()
{
    delete inputHandler;
}

void MapWidget::setupInputHandler(InputHandler *newGesture)
{
    if (inputHandler != NULL)
        delete inputHandler;
    inputHandler = newGesture;
    
    connect(inputHandler, SIGNAL(viewChanged(const MapView&)), 
            this, SLOT(viewChanged(const MapView&)));
}

void MapWidget::redraw()
{
    update();
}
/*
void MapWidget::initialisationFinished(const DatabaseLoadedResponse& response)
{
    if (dbInitialized) // avoid double initialization
        return;
    
    size_t zoom=1;
    double dlat=360;
    double dlon=180;

    center=response.boundingBox.GetCenter();

    while (dlat>response.boundingBox.GetHeight() &&
           dlon>response.boundingBox.GetWidth()) {
        zoom=zoom*2;
        dlat=dlat/2;
        dlon=dlon/2;
    }

    magnification=zoom;

    dbInitialized=true;

    qDebug() << "hasBeenPainted: " << hasBeenPainted;
    if (hasBeenPainted) {
        TriggerMapRendering();
    }
}

void MapWidget::TriggerMapRendering()
{
    qDebug() << "TriggerMapRendering";
    
    DBThread         *dbThread=DBThread::GetInstance();
    RenderMapRequest request;

    request.lat=center.GetLat();
    request.lon=center.GetLon();
    request.angle=angle;
    request.magnification=magnification;
    request.width=width();
    request.height=height();

    dbThread->CancelCurrentDataLoading();

    emit TriggerMapRenderingSignal(request);
}
*/

void MapWidget::viewChanged(const MapView &updated)
{
    qDebug() << "viewChanged: " << QString::fromStdString(updated.center.GetDisplayText()) << "   level: " << updated.magnification.GetLevel();
    view = updated;
    redraw();
}
/*
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
    //touchEvent(touchEvnt);
    delete touchEvnt;
}
*/
/*
void MapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button()==1) {
        translateToTouch(event, Qt::TouchPointPressed);
    }
}
*/

/*
void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
    translateToTouch(event, Qt::TouchPointMoved);
}
*/

/*
void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button()==1) {
        translateToTouch(event, Qt::TouchPointReleased);
    }
}
*/
void MapWidget::touchEvent(QTouchEvent *event)
{
    qDebug() << "touchEvent:";
    //currentTouchPoints.clear();
  
    if (!inputHandler->touch(event)){
        if (event->touchPoints().size() == 1){
            QTouchEvent::TouchPoint tp = event->touchPoints()[0];
            //Qt::TouchPointStates state(tp.state());
            //if (state.testFlag(Qt::TouchPointReleased)){
            //    setupInputHandler(new InputHandler(view)); // return to rendering with Antialiasing
            //}else{
            setupInputHandler(new DragHandler(view, dpi));
            //}
        }else{
            /*
            int remaining = 0;
            for (QTouchEvent::TouchPoint tp: event->touchPoints()){
                Qt::TouchPointStates state(tp.state());
                if (!state.testFlag(Qt::TouchPointReleased))
                    remaining++;
            }
            if (remaining == 0){
                setupInputHandler(new InputHandler(view)); // return to rendering with Antialiasing
            }else{
                setupInputHandler(new DragHandler(view, dpi));
            }
            */
            setupInputHandler(new MultitouchHandler(view, dpi));
        }
        inputHandler->touch(event);
    }
  
    QList<QTouchEvent::TouchPoint> relevantTouchPoints;
    for (QTouchEvent::TouchPoint tp: event->touchPoints()){
      Qt::TouchPointStates state(tp.state());
      qDebug() << "  " << state <<" " << tp.id() << 
              //"- scene " << tp.scenePos().x() << "x" << tp.scenePos().y() << 
              " pos " << tp.pos().x() << "x" << tp.pos().y() << 
              //" screen " << tp.screenPos().x() << "x" << tp.screenPos().y() << 
              " @ " << tp.pressure();    
      /*
      if (relevantTouchPoints.size() < 2 && // we respect only two points
        ( state.testFlag(Qt::TouchPointMoved)
        || state.testFlag(Qt::TouchPointStationary)
        || state.testFlag(Qt::TouchPointReleased )
        )){

        relevantTouchPoints << tp;
        currentTouchPoints << tp.screenPos();
      }
       */
    }
    /*
    if (relevantTouchPoints.size() >= 2){
      // compute zoom
      QVector2D last(relevantTouchPoints[0].lastScreenPos() - relevantTouchPoints[1].lastScreenPos());
      QVector2D current(relevantTouchPoints[0].screenPos() - relevantTouchPoints[1].screenPos());
      if (last.length() == 0)
        last = current;
      if (current.length() != 0){
        zoom( current.length() / last.length() );      
      }
    }
    if (relevantTouchPoints.size() == 1){

    }


    for (QTouchEvent::TouchPoint tp: event->touchPoints()){
      //qDebug() << "  " << tp.id() << "- " << tp.scenePos().x() << "x" << tp.scenePos().y() << " @ " << tp.pressure();    
      currentTouchPoints << tp.pos();
    }
    //update();
    //QQuickPaintedItem::touchEvent(event); 
    */
}

void MapWidget::wheelEvent(QWheelEvent* event)
{
    int numDegrees=event->delta()/8;
    int numSteps=numDegrees/15;

    if (numSteps>=0) {
        zoomIn(numSteps*1.35);
    }
    else {
        zoomOut(-numSteps*1.35);
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
    
    //if (dbInitialized) {
        RenderMapRequest request;
        QRectF           boundingBox = contentsBoundingRect();

        request.lat = view.center.GetLat();
        request.lon = view.center.GetLon();
        request.angle = view.angle;
        request.magnification = view.magnification;
        request.width = boundingBox.width();
        request.height = boundingBox.height();

        if (!dbThread->RenderMap(*painter,request)) {
            /*
            if (!mouseDragging) {
                TriggerMapRendering();
            }
             */
        }
        /*
        painter->setPen(QColor::fromRgbF(1,0,0));
        for (QPointF tp:currentTouchPoints){
          painter->drawEllipse(tp, 40.0, 40.0);
        }
         
        /*
    }
    else {
      dbThread->RenderMessage(*painter,width(),height(),"Database not initialized yet");
    }

    hasBeenPainted=true;
         */
}

void MapWidget::zoom(double zoomFactor)
{
  if (zoomFactor == 1)
    return;
  
  if (zoomFactor < 1.0){
    zoomOut(1/zoomFactor);
  }else{
    zoomIn(zoomFactor);
  }
}

void MapWidget::zoomIn(double zoomFactor)
{
    if (!inputHandler->zoomIn(zoomFactor)){
        setupInputHandler(new MoveHandler(view, dpi));
        inputHandler->zoomIn(zoomFactor);
    }
}

void MapWidget::zoomOut(double zoomFactor)
{
    if (!inputHandler->zoomOut(zoomFactor)){
        setupInputHandler(new MoveHandler(view, dpi));
        inputHandler->zoomOut(zoomFactor);
    }
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
        view = { coord, view.angle, magnification  };
        setupInputHandler(new InputHandler(view));
    }
}

void MapWidget::showCoordinates(double lat, double lon)
{
    showCoordinates(osmscout::GeoCoord(lat,lon), osmscout::Magnification::magVeryClose);
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
