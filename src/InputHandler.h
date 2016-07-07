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

#ifndef INPUTHANDLER_H
#define	INPUTHANDLER_H

#include <QObject>
#include <QVector2D>
#include <QTouchEvent>
#include <QTimer>
#include <QTime>
#include <QQueue>

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>

/**
 * Simple class for recognizing some gestures: tap, double tap and long-tap (aka tap-and-hold).
 */
class TapRecognizer : public QObject{
  Q_OBJECT

private:
  enum TapRecState{
    INACTIVE = 0,
    PRESSED = 1, // timer started with hold interval, if expired - long-tap is emited
    RELEASED = 2, // timer started with tap interval, if expired - tap is emited
    PRESSED2 = 3, // timer started with hold interval, if expired - tap-long-tap
  };
  
  int startFingerId;
  int startX;
  int startY;

  QTimer timer;
  TapRecState state;
  int holdIntervalMs;
  int tapIntervalMs;
  int moveTolerance;
  
private slots:
  void onTimeout();
  
public:
  inline TapRecognizer(): state(INACTIVE), holdIntervalMs(500), tapIntervalMs(200), moveTolerance(15)
  {
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
  }
  
  virtual inline ~TapRecognizer()
  {
  }
  
  void touch(QTouchEvent *event);
  
  inline void setPhysicalDpi(double physicalDpi)
  {
    moveTolerance = physicalDpi / 10.0; // ~ 2.5 mm
  }
  
signals:
  void tap(const QPoint p);
  void doubleTap(const QPoint p);
  void longTap(const QPoint p);
  void tapLongTap(const QPoint p);
};

struct AccumulatorEvent
{
  QPointF pos;
  QTime time;
};
Q_DECLARE_METATYPE(AccumulatorEvent)

class MoveAccumulator : public QObject{
  Q_OBJECT

private:
  int memory; // ms
  QQueue<AccumulatorEvent> events;
  double factor;
  double vectorLengthTreshold;
  
public:
    
  /**
   * 
   * @param memory - in milliseconds for finger position points
   * @param factor - momentum movement length (vector returned from collect method) will be equal to recorded length * factor
   * @param vectorLengthTreshold - movement (between two points) have to be longer than treshold (in pixels) for change move vector
   */
  inline MoveAccumulator(int memory = 100, double factor = 4, double vectorLengthTreshold = 5):
    memory(memory), factor(factor), vectorLengthTreshold(vectorLengthTreshold)
  {
  }
  virtual inline ~MoveAccumulator(){}
  
  MoveAccumulator& operator+=(const QPointF p);
  QVector2D collect();
};

struct MapView
{
  osmscout::GeoCoord           center;
  double                       angle;
  osmscout::Magnification      magnification;
};

Q_DECLARE_METATYPE(MapView)

inline bool operator==(const MapView& a, const MapView& b)
{
  return a.center == b.center && a.angle == b.angle && a.magnification == b.magnification;
}
inline bool operator!=(const MapView& a, const MapView& b)
{
  return ! (a == b);
}

/**
 * Input handler retrieve all inputs from user and may change MapView. 
 * If handler don't accept specific action, returns false. In such case, 
 * default handler for this action should be activated.
 * 
 * Input handlers is application of behaviour pattern. It solves problems like: 
 *  - what should happen when finger is on the screen and plus button is pressed
 *  - recognising multitouch gestures
 *      Qt provides api for register custom gesture recognizers, but it is not 
 *      available in QML world and its api don't fit to Map application requierements.
 * 
 * Handler may control map animations in future.
 */

class InputHandler : public QObject{
    Q_OBJECT
public:
    InputHandler(MapView view);
    virtual ~InputHandler();

    virtual void painted();
    virtual bool animationInProgress();
    
    virtual bool showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification);
    virtual bool zoom(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension);
    virtual bool move(QVector2D vector); // move vector in pixels
    virtual bool rotateBy(double angleStep, double angleChange);
    virtual bool touch(QTouchEvent *event);

signals:
    void viewChanged(const MapView &view);

protected:
    MapView view;
};

/**
 * handler with support of animations
 */
class MoveHandler : public InputHandler {
    Q_OBJECT
    
private:
    QTime animationStart;
    QTimer timer;
    MapView startMapView;
    QVector2D _move;
    osmscout::Magnification targetMagnification;
  
    const int ANIMATION_DURATION = 1000; // ms
    const int ANIMATION_TICK = 16;
    
private slots:
    void onTimeout();

public: 
    MoveHandler(MapView view, double dpi);
    virtual ~MoveHandler();
    
    virtual bool animationInProgress();
    
    /**
     * Called from DragHandler or MultitouchHandler when gesture moves with map
     * 
     * @param vector
     * @return 
     */
    bool moveNow(QVector2D vector); // move vector in pixels, without animation

    virtual bool zoom(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension);
    virtual bool move(QVector2D vector); // move vector in pixels
    virtual bool rotateBy(double angleStep, double angleChange);
    virtual bool touch(QTouchEvent *event);
    
private:
    double dpi;
};

class JumpHandler : public InputHandler {
    Q_OBJECT
    
private:
    QTime animationStart;
    QTimer timer;
    MapView startMapView;
    MapView targetMapView;
  
    const int ANIMATION_DURATION = 1000; // ms
    const int ANIMATION_TICK = 16;
    
private slots:
    void onTimeout();

public: 
    JumpHandler(MapView view);
    virtual ~JumpHandler();
    
    virtual bool animationInProgress();
    virtual bool showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification);
};

class DragHandler : public MoveHandler {
    Q_OBJECT
public: 
    DragHandler(MapView view, double dpi);
    virtual ~DragHandler();

    virtual bool animationInProgress();

    virtual bool zoom(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension);
    virtual bool move(QVector2D vector); // move vector in pixels
    virtual bool rotateBy(double angleStep, double angleChange);
    
    virtual bool touch(QTouchEvent *event);

private:
    bool moving;
    MapView startView;
    int fingerId;
    int startX;
    int startY;
    bool ended;
    MoveAccumulator moveAccumulator;
};

class MultitouchHandler : public MoveHandler {
    Q_OBJECT
public: 
    MultitouchHandler(MapView view, double dpi);
    virtual ~MultitouchHandler();

    virtual bool animationInProgress();

    virtual bool zoom(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension);
    virtual bool move(QVector2D vector); // move vector in pixels
    virtual bool rotateBy(double angleStep, double angleChange);
    
    virtual bool touch(QTouchEvent *event);
private:
    bool moving;
    MapView startView;
    bool initialized;
    bool ended;
    MoveAccumulator moveAccumulator;
    
    // we take only first two touch points into account
    QTouchEvent::TouchPoint startPointA;
    QTouchEvent::TouchPoint startPointB;
};



#endif	/* INPUTHANDLER_H */


