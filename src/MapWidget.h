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
  Q_PROPERTY(double lat READ GetLat)
  Q_PROPERTY(double lon READ GetLon)

private:

  MapView                        view;
  double                         dpi;

  InputHandler                   *inputHandler;

signals:
  void latChanged();
  void lonChanged();

public slots:
  void viewChanged(const MapView &view);
  void redraw();
  void zoom(double zoomFactor);
  void zoomIn(double zoomFactor);
  void zoomOut(double zoomFactor);
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
  void showLocation(Location* location);

private:
  void setupInputHandler(InputHandler *newGesture);
  
public:
  MapWidget(QQuickItem* parent = 0);
  virtual ~MapWidget();

  inline double GetLat() const
  {
      return view.center.GetLat();
  }

  inline double GetLon() const
  {
      return view.center.GetLon();
  }
  
  void wheelEvent(QWheelEvent* event);
  virtual void touchEvent(QTouchEvent *event);

  void paint(QPainter *painter);
};

#endif
