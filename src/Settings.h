#ifndef SETTINGS_H
#define SETTINGS_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2013  Tim Teulings

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

#include <memory>

#include <QSettings>

#include <osmscout/RoutingProfile.h>
#include "InputHandler.h"
#include "OnlineTileProvider.h"

/**
 * Settings provide global instance that extends Qt's QSettings
 * by properties with signals
 */
class Settings: public QObject
{
  Q_OBJECT
  Q_PROPERTY(double   mapDPI     READ GetMapDPI     WRITE SetMapDPI     NOTIFY MapDPIChange)
  Q_PROPERTY(MapView  *mapView   READ GetMapView    WRITE SetMapView    NOTIFY MapViewChanged)
  Q_PROPERTY(bool     onlineTiles READ GetOnlineTilesEnabled WRITE SetOnlineTilesEnabled NOTIFY OnlineTilesEnabledChanged)
  Q_PROPERTY(QString  onlineTileProviderId READ GetOnlineTileProviderId WRITE SetOnlineTileProviderId NOTIFY OnlineTileProviderIdChanged)
  Q_PROPERTY(bool     offlineMap READ GetOfflineMap WRITE SetOfflineMap NOTIFY OfflineMapChanged)
  Q_PROPERTY(bool     renderSea  READ GetRenderSea  WRITE SetRenderSea  NOTIFY RenderSeaChanged)

signals:
  void MapDPIChange(double dpi);
  void MapViewChanged(MapView *view);
  void OnlineTilesEnabledChanged(bool);
  void OnlineTileProviderIdChanged(const QString id);
  void OfflineMapChanged(bool);
  void RenderSeaChanged(bool);
  
private:
  QSettings settings;
  double    physicalDpi;
  MapView   *view;
  QMap<QString, OnlineTileProvider> onlineProviders;

public:
  Settings();
  ~Settings();

  double GetPhysicalDPI() const;
  
  void SetMapDPI(double dpi);
  double GetMapDPI() const;

  MapView *GetMapView();
  void SetMapView(MapView *view);
  
  osmscout::Vehicle GetRoutingVehicle() const;
  void SetRoutingVehicle(const osmscout::Vehicle& vehicle);
  
  bool GetOnlineTilesEnabled() const;
  void SetOnlineTilesEnabled(bool b);
  
  const QList<OnlineTileProvider> GetOnlineProviders() const;
  const OnlineTileProvider GetOnlineTileProvider() const; 
  
  const QString GetOnlineTileProviderId() const; 
  void SetOnlineTileProviderId(QString id);
  
  bool loadOnlineTileProviders(QString path);
  
  bool GetOfflineMap() const;
  void SetOfflineMap(bool);
  
  bool GetRenderSea() const;
  void SetRenderSea(bool);
  
  static Settings* GetInstance();
  static void FreeInstance();
};

class QmlSettings: public QObject{
  Q_OBJECT
  Q_PROPERTY(double   physicalDPI READ GetPhysicalDPI CONSTANT)
  Q_PROPERTY(double   mapDPI    READ GetMapDPI  WRITE SetMapDPI   NOTIFY MapDPIChange)
  Q_PROPERTY(QObject  *mapView  READ GetMapView WRITE SetMapView  NOTIFY MapViewChanged)
  Q_PROPERTY(bool     onlineTiles READ GetOnlineTilesEnabled WRITE SetOnlineTilesEnabled NOTIFY OnlineTilesEnabledChanged)
  Q_PROPERTY(QString  onlineTileProviderId READ GetOnlineTileProviderId WRITE SetOnlineTileProviderId NOTIFY OnlineTileProviderIdChanged)
  Q_PROPERTY(bool     offlineMap READ GetOfflineMap WRITE SetOfflineMap NOTIFY OfflineMapChanged)
  Q_PROPERTY(bool     renderSea  READ GetRenderSea  WRITE SetRenderSea  NOTIFY RenderSeaChanged)

signals:
  void MapDPIChange(double dpi);
  void MapViewChanged(MapView *view);
  void OnlineTilesEnabledChanged(bool enabled);
  void OnlineTileProviderIdChanged(const QString id);
  void OfflineMapChanged(bool);
  void RenderSeaChanged(bool);

public:
  QmlSettings();
  
  inline ~QmlSettings(){};

  double GetPhysicalDPI() const;

  void SetMapDPI(double dpi);
  double GetMapDPI() const;  
    
  MapView *GetMapView() const;
  void SetMapView(QObject *view);    

  bool GetOnlineTilesEnabled() const;
  void SetOnlineTilesEnabled(bool b);
  
  const QString GetOnlineTileProviderId() const; 
  void SetOnlineTileProviderId(QString id);
  
  Q_INVOKABLE QString onlineProviderCopyright();
  
  bool GetOfflineMap() const;
  void SetOfflineMap(bool);
  
  bool GetRenderSea() const;
  void SetRenderSea(bool);  
};

#endif
