/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2017 Lukas Karas

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

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>

#include <osmscout/InputHandler.h>

class AppSettings: public QObject{
  Q_OBJECT
  Q_PROPERTY(QObject  *mapView          READ GetMapView           WRITE SetMapView           NOTIFY MapViewChanged)
  Q_PROPERTY(QString  gpsFormat         READ GetGpsFormat         WRITE SetGpsFormat         NOTIFY GpsFormatChanged)
  Q_PROPERTY(bool     hillShades        READ GetHillShades        WRITE SetHillShades        NOTIFY HillShadesChanged)
  Q_PROPERTY(double   hillShadesOpacity READ GetHillShadesOpacity WRITE SetHillShadesOpacity NOTIFY HillShadesOpacityChanged)
  Q_PROPERTY(QString  lastVehicle       READ GetLastVehicle       WRITE SetLastVehicle       NOTIFY LastVehicleChanged)

signals:
  void MapViewChanged(osmscout::MapView *view);
  void GpsFormatChanged(const QString formatId);
  void HillShadesChanged(bool);
  void HillShadesOpacityChanged(double);
  void LastVehicleChanged(const QString vehicle);

public:
  AppSettings();
  inline virtual ~AppSettings(){}

  osmscout::MapView *GetMapView();
  void SetMapView(QObject *o);

  const QString GetGpsFormat() const;
  void SetGpsFormat(const QString formatId);

  bool GetHillShades() const;
  void SetHillShades(bool);

  double GetHillShadesOpacity() const;
  void SetHillShadesOpacity(double);

  QString GetLastVehicle() const;
  void SetLastVehicle(const QString vehicle);

private:
  QSettings         settings;
  osmscout::MapView *view;
};

#endif /* APPSETTINGS_H */

