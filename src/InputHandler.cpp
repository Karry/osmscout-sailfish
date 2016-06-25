/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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

#include <QDebug>
#include <QPoint>
#include <QVector>

#include "InputHandler.h"
#include "OSMTile.h"
#include "osmscout/util/Projection.h"

void TapRecognizer::onTimeout()
{
    switch(state){
    case PRESSED:
        state = INACTIVE;
        emit longTap(QPoint(startX, startY));
        break;
    case RELEASED:
        state = INACTIVE;
        emit tap(QPoint(startX, startY));
        break;
    case PRESSED2:
        state = INACTIVE;
        emit tapLongTap(QPoint(startX, startY));
        break;
    case INACTIVE: 
    default:
        state = INACTIVE;
        break;
    }
}

void TapRecognizer::touch(QTouchEvent *event)
{
    // discard on multi touch
    if (event->touchPoints().size() > 1){
        state = INACTIVE;
        return;
    }
    
    QTouchEvent::TouchPoint finger = event->touchPoints()[0];    
    Qt::TouchPointStates fingerState(finger.state());
    bool released = fingerState.testFlag(Qt::TouchPointReleased);
    bool pressed = fingerState.testFlag(Qt::TouchPointPressed);
    int fingerId = finger.id();
    int x = finger.pos().x();
    int y = finger.pos().y();
    
    // discard when PRESSED and registered another finger or some bigger movement 
    if ((state == PRESSED || state == PRESSED2) && 
            (fingerId != startFingerId || std::max(std::abs(x - startX), std::abs(y - startY)) > moveTolerance)){
        
        state = INACTIVE;
        return;
    }
    // second touch with too big distance
    if (state == RELEASED && std::max(std::abs(x - startX), std::abs(y - startY)) > moveTolerance){
        state = INACTIVE;
        return;        
    }    
    if ((!pressed) && (!released))
        return;
    
    timer.stop();

    switch(state){
    case INACTIVE: 
        if (pressed){
            startFingerId = fingerId;
            startX = x;
            startY = y;
            state = PRESSED;
            timer.setInterval(holdIntervalMs);
            timer.start();
        }
        break;
    case PRESSED:
        if (released){
            state = RELEASED;
            timer.setInterval(tapIntervalMs);
            timer.start();
        }
        break;
    case RELEASED:
        if (pressed){
            startFingerId = fingerId;
            startX = x;
            startY = y;
            state = PRESSED2;
            timer.setInterval(holdIntervalMs);
            timer.start();
        }
        break;
    case PRESSED2:
        if (released){
            state = INACTIVE;
            emit doubleTap(QPoint(startX, startY));
        }        
        break;
    default:
        state = INACTIVE;
        break;
    }
}

InputHandler::InputHandler(MapView view): view(view) 
{
}
InputHandler::~InputHandler()
{
    // noop    
}
void InputHandler::painted()
{
    // noop
}
bool InputHandler::animationInProgress()
{
    return false;
}
bool InputHandler::showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification)
{
    return false;
}
bool InputHandler::zoomIn(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension)
{
    return false;    
}

bool InputHandler::zoomOut(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension)
{
    return false;    
}
bool InputHandler::move(QVector2D move)
{
    return false;
}
bool InputHandler::rotateBy(double angleStep, double angleChange)
{
    return false;
}
bool InputHandler::touch(QTouchEvent *event)
{
    return false;
}

MoveHandler::MoveHandler(MapView view, double dpi): InputHandler(view), dpi(dpi)
{
    
}

MoveHandler::~MoveHandler()
{
    // noop
}

bool MoveHandler::zoomIn(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension)
{
    osmscout::Magnification maxMag;

    maxMag.SetLevel(20);

    if (view.magnification.GetMagnification()*zoomFactor>maxMag.GetMagnification()) {
        view.magnification.SetMagnification(maxMag.GetMagnification());
    }
    else {
        view.magnification.SetMagnification(view.magnification.GetMagnification()*zoomFactor);
    }
    emit viewChanged(view);
    return true;
}

bool MoveHandler::zoomOut(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension)
{
    if (view.magnification.GetMagnification()/zoomFactor<1) {
        view.magnification.SetMagnification(1);
    }
    else {
        view.magnification.SetMagnification(view.magnification.GetMagnification()/zoomFactor);
    }
    emit viewChanged(view);
    return true;
}
bool MoveHandler::move(QVector2D move)
{
    osmscout::MercatorProjection projection;

    //qDebug() << "move: " << QString::fromStdString(view.center.GetDisplayText()) << "   by: " << move;
    
    projection.Set(view.center, view.magnification, dpi, 1000, 1000);

    if (!projection.IsValid()) {
        //TriggerMapRendering();
        return false;
    }

    projection.Move(move.x(), move.y() * -1.0);

    view.center=projection.GetCenter();
    if (view.center.lon < OSMTile::minLon()){
        view.center.lon = OSMTile::minLon();
    }else if (view.center.lon > OSMTile::maxLon()){
        view.center.lon = OSMTile::maxLon();
    }
    if (view.center.lat > OSMTile::maxLat()){
        view.center.lat = OSMTile::maxLat();
    }else if (view.center.lat < OSMTile::minLat()){
        view.center.lat = OSMTile::minLat();
    }

    emit viewChanged(view);
    return true;
}
bool MoveHandler::rotateBy(double angleStep, double angleChange)
{
    return false; // FIXME: rotation is broken in current version. discard rotation for now
    /*
    view.angle=round(view.angle/angleStep)*angleStep + angleStep;

    if (view.angle < 0) {
        view.angle+=2*M_PI;
    }
    if (view.angle <= 2*M_PI) {
        view.angle-=2*M_PI;
    }

    emit viewChanged(view);
    return true;
    */
}

DragHandler::DragHandler(MapView view, double dpi): 
    MoveHandler(view, dpi), moving(true), startView(view), fingerId(-1), startX(-1), startY(-1)
{
}
DragHandler::~DragHandler()
{
}
bool DragHandler::touch(QTouchEvent *event)
{
    if (event->touchPoints().size() > 1)
        return false;
    
    QTouchEvent::TouchPoint finger = event->touchPoints()[0];
    Qt::TouchPointStates state(finger.state());
        
    moving = !state.testFlag(Qt::TouchPointReleased);

    if (startX < 0){ // first touch by this point
        if (!state.testFlag(Qt::TouchPointReleased)){
            startX = finger.pos().x();
            startY = finger.pos().y();
            fingerId = finger.id();
        }
    }else{
        if (fingerId != finger.id())
            return false; // should not happen
        
        view = startView;
        MoveHandler::move(QVector2D(
            startX - finger.pos().x(),
            startY - finger.pos().y()
        ));
    }
    
    return !state.testFlag(Qt::TouchPointReleased);
}

bool DragHandler::zoomIn(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension)
{
    return false; 
        // TODO: finger on screen and zoom 
        // => compute geo point under finger, change magnification and then update startView
}

bool DragHandler::zoomOut(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension)
{
    return false; // TODO
}
bool DragHandler::move(QVector2D move)
{
    return false; // finger on screen discard move
}
bool DragHandler::rotateBy(double angleStep, double angleChange)
{
    return false; // finger on screen discard rotation ... TODO like zoom
}
bool DragHandler::animationInProgress()
{
    return moving;
}


MultitouchHandler::MultitouchHandler(MapView view, double dpi): 
    MoveHandler(view, dpi), moving(true), startView(view), initialized(false)
{
}

MultitouchHandler::~MultitouchHandler()
{

}
bool MultitouchHandler::animationInProgress()
{
    return moving;
}
bool MultitouchHandler::zoomIn(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension)
{
    return false;
}
bool MultitouchHandler::zoomOut(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension)
{
    return false;
}
bool MultitouchHandler::move(QVector2D vector)
{
    return false;
}
bool MultitouchHandler::rotateBy(double angleStep, double angleChange)
{
    return false;
}    
bool MultitouchHandler::touch(QTouchEvent *event)
{
    if (!initialized){
        QList<QTouchEvent::TouchPoint> valid;
        QListIterator<QTouchEvent::TouchPoint> it(event->touchPoints());        
        while (it.hasNext() && (valid.size() < 2)){
            QTouchEvent::TouchPoint tp = it.next();
            Qt::TouchPointStates state(tp.state());
            if (!state.testFlag(Qt::TouchPointReleased)){
                valid << tp;
            }
        }
        if (valid.size() >= 2){
            startPointA = valid[0];
            startPointB = valid[1];
            initialized = true;
            return true;
        }
    }else{
        QTouchEvent::TouchPoint currentA;
        QTouchEvent::TouchPoint currentB;
        int assigned = 0;
        
        QListIterator<QTouchEvent::TouchPoint> it(event->touchPoints());        
        while (it.hasNext() && (assigned < 2)){
            QTouchEvent::TouchPoint tp = it.next();
            if (tp.id() == startPointA.id()){
                currentA = tp;
                assigned++;
            }
            if (tp.id() == startPointB.id()){
                currentB = tp;
                assigned++;
            }
        }
        if (assigned == 2){
            view = startView;
            
            // move
            QPointF startCenter(
                (startPointA.pos().x() + startPointB.pos().x()) / 2,
                (startPointA.pos().y() + startPointB.pos().y()) / 2
            );
            QPointF currentCenter(
                (currentA.pos().x() + currentB.pos().x()) / 2,
                (currentA.pos().y() + currentB.pos().y()) / 2
            );
            
            QVector2D move(
                startCenter.x() - currentCenter.x(),
                startCenter.y() - currentCenter.y()
            );
            MoveHandler::move(move);    
            
            // zoom
            QVector2D startVector(startPointA.pos() - startPointB.pos());
            QVector2D currentVector(currentA.pos() - currentB.pos());
            double scale = 1;
            if (startVector.length() > 0){
                
                scale = currentVector.length() / startVector.length();
                
                osmscout::Magnification maxMag;
                osmscout::Magnification minMag;
                maxMag.SetLevel(20);
                minMag.SetLevel(0);

                if (view.magnification.GetMagnification()*scale>maxMag.GetMagnification()) {
                    view.magnification.SetMagnification(maxMag.GetMagnification());
                }else {
                    if (view.magnification.GetMagnification()*scale<minMag.GetMagnification()) {
                        view.magnification.SetMagnification(minMag.GetMagnification());
                    }else {
                        view.magnification.SetMagnification(view.magnification.GetMagnification()*scale);
                    }
                }
            }
            
            if (move.length() > 10 || scale > 1.1 || scale < 0.9){
                startPointA = currentA;
                startPointB = currentB;
                startView = view;
            }
            
            emit viewChanged(view);
            return true;
        }
    }
    moving = false;
    return false;
}
