#ifndef DBTHREAD_H
#define DBTHREAD_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings
 Copyright (C) 2016  Lukáš Karas

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

#include <QtGui>
#include <QThread>
#include <QMetaType>
#include <QMutex>
#include <QTime>
#include <QTimer>

#include <osmscout/Database.h>
#include <osmscout/LocationService.h>
#include <osmscout/MapService.h>
#include <osmscout/RoutingService.h>
#include <osmscout/RoutePostprocessor.h>

#include <osmscout/MapPainterQt.h>

#include <osmscout/util/Breaker.h>

#include "Settings.h"
#include "TileCache.h"
#include "OsmTileDownloader.h"

struct RenderMapRequest
{
  double                  lon;
  double                  lat;
  double                  angle;
  osmscout::Magnification magnification;
  size_t                  width;
  size_t                  height;
};

Q_DECLARE_METATYPE(RenderMapRequest)

struct DatabaseLoadedResponse
{
    osmscout::GeoBox boundingBox;
};

Q_DECLARE_METATYPE(DatabaseLoadedResponse)

class QBreaker : public osmscout::Breaker
{
private:
  mutable QMutex mutex;
  bool           aborted;

public:
  QBreaker();

  bool Break();
  bool IsAborted() const;
  void Reset();
};

class DBInstance : public QObject
{
  Q_OBJECT

public:
  QString                       path;
  osmscout::DatabaseRef         database;
  
  osmscout::LocationServiceRef  locationService;
  osmscout::MapServiceRef       mapService;
  osmscout::MapService::CallbackId callbackId;
  osmscout::BreakerRef          dataLoadingBreaker;  
  
  osmscout::RoutingServiceRef   router;

  osmscout::StyleConfigRef      styleConfig;
  osmscout::MapPainterQt        *painter;
  
  inline DBInstance(QString path,
                    osmscout::DatabaseRef database,
                    osmscout::LocationServiceRef locationService,
                    osmscout::MapServiceRef mapService,
                    osmscout::MapService::CallbackId callbackId,
                    osmscout::BreakerRef dataLoadingBreaker,
                    osmscout::StyleConfigRef styleConfig):
    path(path),
    database(database),
    locationService(locationService),
    mapService(mapService),
    callbackId(callbackId),
    dataLoadingBreaker(dataLoadingBreaker),
    styleConfig(styleConfig),
    painter(NULL)
  {
    if (styleConfig){
      painter = new osmscout::MapPainterQt(styleConfig);
    }   
  };
  
  inline ~DBInstance()
  {
    if (painter!=NULL) {
      delete painter;
    }
  };

  void LoadStyle(QString stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags);
  
  bool AssureRouter(osmscout::Vehicle vehicle, 
                    osmscout::RouterParameter routerParameter);  

};

typedef std::shared_ptr<DBInstance> DBInstanceRef;

class DBThread : public QObject
{
  Q_OBJECT

signals:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void TriggerInitialRendering();
  void TriggerDrawMap();
  void Redraw();
  void TileStatusChanged(const osmscout::TileRef& tile);
  void locationDescription(const osmscout::GeoCoord location, 
                           const QString database,
                           const osmscout::LocationDescription description);

public slots:
  void ToggleDaylight();
  void ReloadStyle();
  void LoadStyle(QString stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags);
  void HandleInitialRenderingRequest();
  void HandleTileStatusChanged(const osmscout::TileRef& changedTile);
  void DrawTileMap(QPainter &p, const osmscout::GeoCoord center, uint32_t z, 
        size_t width, size_t height, size_t lookupWidth, size_t lookupHeight, bool drawBackground);
  void Initialize();
  void Finalize();
  void onlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);
  void offlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);
  void tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray downloadedData);
  void tileDownloadFailed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange);  
  void requestLocationDescription(const osmscout::GeoCoord location);
  
  void onMapDPIChange(double dpi);
  void onlineTileProviderChanged();
  void onlineTilesEnabledChanged(bool);
  
  void onOfflineMapChanged(bool);
  void onRenderSeaChanged(bool);  

private:
  QStringList                   databaseLookupDirs; 
  QString                       resourceDirectory;
  QString                       tileCacheDirectory;
  
  double                        mapDpi;
  double                        physicalDpi;

  mutable QMutex                mutex;
  
  // tile caches
  // Rendered tile is combined from both sources.
  //
  // Online cache may contain NULL images (QImage::isNull() is true) for areas
  // covered by offline database and offline cache can contain NULL images 
  // for areas not coverred by database.
  // 
  // Offline tiles should be in ARGB format on database area interface.
  mutable QMutex                tileCacheMutex;
  TileCache                     onlineTileCache;
  TileCache                     offlineTileCache;
  
  OsmTileDownloader             *tileDownloader;

  osmscout::DatabaseParameter   databaseParameter;
  QList<DBInstanceRef>          databases;
  osmscout::RouterParameter     routerParameter;
  osmscout::RoutePostprocessor  routePostprocessor;

  QString                       stylesheetFilename;
  std::unordered_map<std::string,bool>
                                stylesheetFlags;
  bool                          daylight;
  QString                       iconDirectory;
  
  bool                          onlineTilesEnabled;
  bool                          offlineTilesEnabled;
  bool                          renderSea;

private:
  enum DatabaseTileState{
    Outside = 0,
    Covered = 1,
    Intersects = 2,
  };
  
  DBThread(QStringList databaseLookupDirectories, QString resourceDirectory, QString tileCacheDirectory,
           size_t onlineTileCacheSize = 20, size_t offlineTileCacheSize = 50);
  virtual ~DBThread();

  bool AssureRouter(osmscout::Vehicle vehicle);

  void TileStateCallback(const osmscout::TileRef& changedTile);
  
  /**
   * lookup tile in cache, if not found, try upper zoom level for substitute. 
   * (It is better upscaled tile than empty space)
   * Is is repeated up to zoomLevel - upLimit
   */
  bool lookupAndDrawTile(TileCache& tileCache, QPainter& painter, 
        double x, double y, double renderTileWidth, double renderTileHeight, 
        uint32_t zoomLevel, uint32_t xtile, uint32_t ytile, 
        uint32_t upLimit, uint32_t downLimit);

  void lookupAndDrawBottomTileRecursive(TileCache& tileCache, QPainter& painter, 
        double x, double y, double renderTileWidth, double renderTileHeight, double overlap,
        uint32_t zoomLevel, uint32_t xtile, uint32_t ytile, 
        uint32_t downLimit);

  DatabaseTileState databaseTileState(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);
  
public:
  bool isInitialized(); 
  
  const DatabaseLoadedResponse loadedResponse() const;
  
  const double GetMapDpi() const;
  
  const double GetPhysicalDpi() const;
  
  void CancelCurrentDataLoading();

  /**
   * Render map defined by request to painter 
   * @param painter
   * @param request
   * @return true if rendered map is complete (queue of tile render requests is empty)
   */
  bool RenderMap(QPainter& painter,
                 const RenderMapRequest& request);
  
  /*
  osmscout::TypeConfigRef GetTypeConfig() const;

  bool GetNodeByOffset(osmscout::FileOffset offset,
                       osmscout::NodeRef& node) const;
  bool GetAreaByOffset(osmscout::FileOffset offset,
                       osmscout::AreaRef& relation) const;
  bool GetWayByOffset(osmscout::FileOffset offset,
                      osmscout::WayRef& way) const;

  bool ResolveAdminRegionHierachie(const osmscout::AdminRegionRef& adminRegion,
                                   std::map<osmscout::FileOffset,osmscout::AdminRegionRef >& refs) const;
  */

  bool SearchForLocations(const std::string& searchPattern,
                          size_t limit,
                          osmscout::LocationSearchResult& result) const;

  bool CalculateRoute(osmscout::Vehicle vehicle,
                      const osmscout::RoutingProfile& routingProfile,
                      const osmscout::ObjectFileRef& startObject,
                      size_t startNodeIndex,
                      const osmscout::ObjectFileRef targetObject,
                      size_t targetNodeIndex,
                      osmscout::RouteData& route);

  bool TransformRouteDataToRouteDescription(osmscout::Vehicle vehicle,
                                            const osmscout::RoutingProfile& routingProfile,
                                            const osmscout::RouteData& data,
                                            osmscout::RouteDescription& description,
                                            const std::string& start,
                                            const std::string& target);
  bool TransformRouteDataToWay(osmscout::Vehicle vehicle,
                               const osmscout::RouteData& data,
                               osmscout::Way& way);

  bool GetClosestRoutableNode(const osmscout::ObjectFileRef& refObject,
                              const osmscout::Vehicle& vehicle,
                              double radius,
                              osmscout::ObjectFileRef& object,
                              size_t& nodeIndex);

  void ClearRoute();
  void AddRoute(const osmscout::Way& way);

  static bool InitializeInstance(QStringList databaseDirectory, QString resourceDirectory, QString tileCacheDirectory,
                                 size_t onlineTileCacheSize = 20, size_t offlineTileCacheSize = 50);
  static DBThread* GetInstance();
  static void FreeInstance();
};

#endif
