#ifndef MAPWIDGET_H
#define MAPWIDGET_H

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

#include <QQuickPaintedItem>

#include <osmscout/GeoCoord.h>
#include <osmscout/util/GeoBox.h>

#include "DBThread.h"
#include "SearchLocationModel.h"
#include "InputHandler.h"

class MapWidget : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(MapView  *view    READ GetView     NOTIFY viewChanged)
  Q_PROPERTY(double   lat      READ GetLat      NOTIFY viewChanged)
  Q_PROPERTY(double   lon      READ GetLon      NOTIFY viewChanged)
  Q_PROPERTY(uint32_t magLevel READ GetMagLevel NOTIFY viewChanged)
  Q_PROPERTY(bool     finished READ IsFinished  NOTIFY finishedChanged)
  Q_PROPERTY(bool     showCurrentPosition READ getShowCurrentPosition WRITE setShowCurrentPosition)

private:

  MapView          *view;
  double           mapDpi;

  InputHandler     *inputHandler;
  TapRecognizer    tapRecognizer;     
  
  bool showCurrentPosition;
  bool finished;
  QTime lastUpdate;
  bool locationValid;
  osmscout::GeoCoord currentPosition;
  bool horizontalAccuracyValid;
  double horizontalAccuracy;
  
  QMap<int, osmscout::GeoCoord> marks;

signals:
  void viewChanged();
  void finishedChanged(bool finished);
  
  void tap(const int sceenX, const int screenY, const double lat, const double lon);
  void doubleTap(const int sceenX, const int screenY, const double lat, const double lon);
  void longTap(const int sceenX, const int screenY, const double lat, const double lon);
  void tapLongTap(const int sceenX, const int screenY, const double lat, const double lon);

public slots:
  void changeView(const MapView &view);
  void redraw();
  
  void zoom(double zoomFactor);
  void zoomIn(double zoomFactor);
  void zoomOut(double zoomFactor);
  
  void zoom(double zoomFactor, const QPoint widgetPosition);
  void zoomIn(double zoomFactor, const QPoint widgetPosition);
  void zoomOut(double zoomFactor, const QPoint widgetPosition);
  
  void move(QVector2D vector);
  void left();
  void right();
  void up();
  void down();
  void rotateLeft();
  void rotateRight();

  void toggleDaylight();
  void reloadStyle();

  void showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification);
  void showCoordinates(double lat, double lon);
  void showCoordinatesInstantly(osmscout::GeoCoord coord, osmscout::Magnification magnification);
  void showCoordinatesInstantly(double lat, double lon);
  void showLocation(Location* location);

  void locationChanged(bool locationValid, double lat, double lon, bool horizontalAccuracyValid, double horizontalAccuracy);
  
  void addPositionMark(int id, double lat, double lon);
  void removePositionMark(int id);

  void onTap(const QPoint p);
  void onDoubleTap(const QPoint p);
  void onLongTap(const QPoint p);
  void onTapLongTap(const QPoint p);
  
  void onMapDPIChange(double dpi);  
  
private:
  void setupInputHandler(InputHandler *newGesture);
  
public:
  MapWidget(QQuickItem* parent = 0);
  virtual ~MapWidget();

  inline MapView* GetView() const
  {
      return view; // We should be owner, parent is set http://doc.qt.io/qt-5/qqmlengine.html#objectOwnership
  }

  inline double GetLat() const
  {
      return view->center.GetLat();
  }

  inline double GetLon() const
  {
      return view->center.GetLon();
  }
  
  inline int GetMagLevel() const
  {
      return view->magnification.GetLevel();
  }
  inline bool IsFinished() const
  {
      return finished;
  }
  
  inline bool getShowCurrentPosition()
  { 
      return showCurrentPosition;
  };
  
  inline void setShowCurrentPosition(bool b)
  { 
      showCurrentPosition = b;
  };
  
  inline osmscout::MercatorProjection getProjection()
  {
    osmscout::MercatorProjection projection;
    projection.Set(osmscout::GeoCoord(GetLat(), GetLon()),
               view->angle,
               view->magnification,
               mapDpi,
               width(),
               height());
    return projection;
  }
  
  void wheelEvent(QWheelEvent* event);
  virtual void touchEvent(QTouchEvent *event);
  
  virtual void focusOutEvent(QFocusEvent *event);

  void translateToTouch(QMouseEvent* event, Qt::TouchPointStates states);
  
  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  
  void paint(QPainter *painter);
};

#endif
