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

#include "DBThread.h"
#include "OSMTile.h"

#include <cmath>
#include <iostream>

#include <QGuiApplication>
#include <QMutexLocker>
#include <QDebug>
#include <QDir>

#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>
#include <qt4/QtCore/qdebug.h>

QBreaker::QBreaker()
  : osmscout::Breaker(),
    aborted(false)
{
}

bool QBreaker::Break()
{
  QMutexLocker locker(&mutex);
  aborted=true;

  return true;
}

bool QBreaker::IsAborted() const
{
  QMutexLocker locker(&mutex);

  return aborted;
}

void QBreaker::Reset()
{
  QMutexLocker locker(&mutex);

  aborted=false;
}

// TODO: watch system memory and evict caches when system is under pressure
DBThread::DBThread(QStringList databaseLookupDirs, 
                   QString resourceDirectory, 
                   QString tileCacheDirectory,
                   size_t onlineTileCacheSize, 
                   size_t offlineTileCacheSize)
 : databaseLookupDirs(databaseLookupDirs), 
   resourceDirectory(resourceDirectory),
   tileCacheDirectory(tileCacheDirectory),
   mapDpi(-1),
   physicalDpi(-1),
   onlineTileCache(onlineTileCacheSize), // online tiles can be loaded from disk cache easily 
   offlineTileCache(offlineTileCacheSize), // render offline tile is expensive
   tileDownloader(NULL),
   //databases(std::make_shared<osmscout::Database>(databaseParameter)),
   //locationService(std::make_shared<osmscout::LocationService>(databases)),
   //mapService(std::make_shared<osmscout::MapService>(databases)),
   daylight(true),
   //painter(NULL),
   iconDirectory()//,
   //dataLoadingBreaker(std::make_shared<QBreaker>())
{
  osmscout::log.Debug() << "DBThread::DBThread()";

  QScreen *srn=QGuiApplication::screens().at(0);

  physicalDpi = (double)srn->physicalDotsPerInch();
  qDebug() << "Reported screen DPI: " << physicalDpi;
  mapDpi = Settings::GetInstance()->GetMapDPI();
  qDebug() << "Map DPI override: " << mapDpi;

  onlineTilesEnabled = Settings::GetInstance()->GetOnlineTilesEnabled();
  offlineTilesEnabled = Settings::GetInstance()->GetOfflineMap();
  renderSea = Settings::GetInstance()->GetRenderSea();
  
  connect(Settings::GetInstance(), SIGNAL(MapDPIChange(double)),
          this, SLOT(onMapDPIChange(double)),
          Qt::QueuedConnection);
  
  connect(Settings::GetInstance(), SIGNAL(OnlineTileProviderIdChanged(const QString)), 
          this, SLOT(onlineTileProviderChanged()));
  connect(Settings::GetInstance(), SIGNAL(OnlineTilesEnabledChanged(bool)), 
          this, SLOT(onlineTilesEnabledChanged(bool)));  
  connect(Settings::GetInstance(), SIGNAL(OfflineMapChanged(bool)), 
          this, SLOT(onOfflineMapChanged(bool)));  
  connect(Settings::GetInstance(), SIGNAL(RenderSeaChanged(bool)), 
          this, SLOT(onRenderSeaChanged(bool)));  

  connect(this,SIGNAL(TriggerInitialRendering()),
          this,SLOT(HandleInitialRenderingRequest()));

  connect(this,SIGNAL(TileStatusChanged(const osmscout::TileRef&)),
          this,SLOT(HandleTileStatusChanged(const osmscout::TileRef&)));

  // fix Qt signals with uint32_t on x86_64:
  //
  // QObject::connect: Cannot queue arguments of type 'uint32_t'
  // (Make sure 'uint32_t' is registered using qRegisterMetaType().)
  qRegisterMetaType < uint32_t >("uint32_t");

  //
  // Make sure that we always decouple caller and receiver even if they are running in the same thread
  // else we might get into a dead lock
  //
  
  connect(&onlineTileCache,SIGNAL(tileRequested(uint32_t, uint32_t, uint32_t)),
          this,SLOT(onlineTileRequest(uint32_t, uint32_t, uint32_t)),
          Qt::QueuedConnection);
  
  connect(&offlineTileCache,SIGNAL(tileRequested(uint32_t, uint32_t, uint32_t)),
          this,SLOT(offlineTileRequest(uint32_t, uint32_t, uint32_t)),
          Qt::QueuedConnection);

}

DBThread::~DBThread()
{
  osmscout::log.Debug() << "DBThread::~DBThread()";

  if (tileDownloader != NULL){
    delete tileDownloader;
  }
  for (auto db:databases){
    db->mapService->DeregisterTileStateCallback(db->callbackId);
  }
}

bool DBInstance::AssureRouter(osmscout::Vehicle /*vehicle*/, 
                              osmscout::RouterParameter routerParameter)
{
  if (!database->IsOpen()) {
    return false;
  }

  if (!router/* ||
      (router && router->GetVehicle()!=vehicle)*/) {
    if (router) {
      if (router->IsOpen()) {
        router->Close();
      }
      router=NULL;
    }

    router=std::make_shared<osmscout::RoutingService>(database,
                                                      routerParameter,
                                                      osmscout::RoutingService::DEFAULT_FILENAME_BASE);

    if (!router->Open()) {
      return false;
    }
  }

  return true;
}

bool DBThread::AssureRouter(osmscout::Vehicle vehicle)
{
  for (auto db:databases){
    if (!db->AssureRouter(vehicle, routerParameter)){
      return false;
    }
  }  
  return true;
}

void DBThread::HandleInitialRenderingRequest()
{
    
}

void DBThread::HandleTileStatusChanged(const osmscout::TileRef& changedTile)
{
    // ignore this event, we are loading tiles synchronously
    
    /*
    QMutexLocker locker(&tileCacheMutex);
    qDebug() << "Invalidate " << QString::fromStdString( changedTile.get()->GetBoundingBox().GetDisplayText() );
    tileCache.invalidate(changedTile.get()->GetBoundingBox());
    emit Redraw();
    */
}

void DBThread::TileStateCallback(const osmscout::TileRef& changedTile)
{
  // We are in the context of one of the libosmscout worker threads
  emit TileStatusChanged(changedTile);
}

bool DBThread::isInitialized(){
  QMutexLocker locker(&mutex);
  for (auto db:databases){
    if (!db->database->IsOpen()){
      return false;
    }
  }
  return true;
}
const double DBThread::GetMapDpi() const
{
    return mapDpi;
}

const double DBThread::GetPhysicalDpi() const
{
    return physicalDpi;
}

const DatabaseLoadedResponse DBThread::loadedResponse() const {
  QMutexLocker locker(&mutex);
  DatabaseLoadedResponse response;
  for (auto db:databases){
    if (response.boundingBox.IsValid()){
      osmscout::GeoBox boundingBox;
      db->database->GetBoundingBox(boundingBox);
      response.boundingBox.Include(boundingBox);
    }else{
      db->database->GetBoundingBox(response.boundingBox);
    }
  }
  return response;
}

void DBThread::Initialize()
{
  QMutexLocker locker(&mutex);
  qDebug() << "Initialize";
  
  // create tile downloader in correct thread
  tileDownloader = new OsmTileDownloader(tileCacheDirectory);
  connect(tileDownloader, SIGNAL(downloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)),
          this, SLOT(tileDownloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)), 
          Qt::QueuedConnection);
  
  connect(tileDownloader, SIGNAL(failed(uint32_t, uint32_t, uint32_t, bool)),
          this, SLOT(tileDownloadFailed(uint32_t, uint32_t, uint32_t, bool)),
          Qt::QueuedConnection);
    
  stylesheetFilename = resourceDirectory + QDir::separator() + "map-styles" + QDir::separator() + "standard.oss";
  // TODO: remove last separator, it should be added by renderer (MapPainterQt.cpp)
  iconDirectory = resourceDirectory + QDir::separator() + "map-icons" + QDir::separator(); // TODO: load icon set for given stylesheet

  DatabaseLoadedResponse response;  
  for (QString lookupDir:databaseLookupDirs){
    QDirIterator dirIt(lookupDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (dirIt.hasNext()) {
      dirIt.next();
      QFileInfo fInfo(dirIt.filePath());
      if (fInfo.isFile() && fInfo.fileName() == osmscout::TypeConfig::FILE_TYPES_DAT){
        qDebug() << "found database: " << fInfo.dir().absolutePath();

        osmscout::DatabaseRef database = std::make_shared<osmscout::Database>(databaseParameter);
        osmscout::StyleConfigRef styleConfig;
        if (database->Open(fInfo.dir().absolutePath().toLocal8Bit().data())) {
          osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

          if (typeConfig) {
            styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

            if (!styleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
              qDebug() << "Cannot load style sheet!";
              styleConfig=NULL;
            }
          }
          else {
            qDebug() << "TypeConfig invalid!";
            styleConfig=NULL;
          }
        }
        else {
          qWarning() << "Cannot open database!";
          continue;
        }

        if (!database->GetBoundingBox(response.boundingBox)) {
          qWarning() << "Cannot read initial bounding box";
          database->Close();
          continue;
        }
        
        osmscout::MapService::TileStateCallback callback=[this](const osmscout::TileRef& tile) {TileStateCallback(tile);};
        osmscout::MapServiceRef mapService = std::make_shared<osmscout::MapService>(database);
        
        databases << std::make_shared<DBInstance>(fInfo.dir().absolutePath(), 
                                                  database, 
                                                  std::make_shared<osmscout::LocationService>(database),
                                                  mapService,
                                                  mapService->RegisterTileStateCallback(callback),
                                                  std::make_shared<QBreaker>(),
                                                  styleConfig);
      }
    }
  }
  
  //mapService->SetCacheSize(10);

  //lastRendering=QTime::currentTime();

  qDebug() << "InitialisationFinished";
  emit InitialisationFinished(response);
  
  // invalidate tile cache and emit Redraw
  {
    QMutexLocker locker(&tileCacheMutex);
    onlineTileCache.invalidate(response.boundingBox);
    offlineTileCache.invalidate(response.boundingBox);
  }
  emit Redraw();
}

void DBThread::Finalize()
{
  qDebug() << "Finalize";
  //FreeMaps();
  for (auto db:databases){
    if (db->router && db->router->IsOpen()) {
      db->router->Close();
    }

    if (db->database->IsOpen()) {
      db->database->Close();
    }
  }
}

void DBThread::CancelCurrentDataLoading()
{
  for (auto db:databases){
    db->dataLoadingBreaker->Break();
  }
}

void DBThread::ToggleDaylight()
{
  {
    QMutexLocker locker(&mutex);

    if (!isInitialized()) {
        return;
    }
    qDebug() << "Toggling daylight from " << daylight << " to " << !daylight << "...";
    daylight=!daylight;
    stylesheetFlags["daylight"] = daylight;
  }

  ReloadStyle();

  qDebug() << "Toggling daylight done.";
}

void DBThread::ReloadStyle()
{
  qDebug() << "Reloading style...";
  LoadStyle(stylesheetFilename, stylesheetFlags);
  qDebug() << "Reloading style done.";
}

void DBThread::LoadStyle(QString stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags)
{
  QMutexLocker locker(&mutex);

  this->stylesheetFilename = stylesheetFilename;
  this->stylesheetFlags = stylesheetFlags;
  
  for (auto db: databases){
    db->LoadStyle(stylesheetFilename, stylesheetFlags);
  }
}

void DBInstance::LoadStyle(QString stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags)
{


  if (!database->IsOpen()) {
    return;
  }

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

  if (!typeConfig) {
    return;
  }

  mapService->FlushTileCache();
  osmscout::StyleConfigRef newStyleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  for (auto flag: stylesheetFlags){
    newStyleConfig->AddFlag(flag.first, flag.second);
  }

  if (newStyleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
    // Tear down
    if (painter!=NULL){
      delete painter;
      painter=NULL;
    }

    // Recreate
    styleConfig=newStyleConfig;
    painter=new osmscout::MapPainterQt(styleConfig);
  }
}

/**
 * Actual map drawing into the back buffer
 * 
 * have to be called with acquiered mutex
 */
void DBThread::DrawTileMap(QPainter &p, const osmscout::GeoCoord center, uint32_t z, 
        size_t width, size_t height, size_t lookupWidth, size_t lookupHeight, bool drawBackground)
{  
    QMutexLocker locker(&mutex);
    
    for (auto db:databases){
      if (!db->database->IsOpen() || (!db->styleConfig)) {
          qWarning() << " Not initialized! " << db->path;
          return;
      }
    }
    QTime timer;

    qDebug() << "Render tile offline " << QString::fromStdString(center.GetDisplayText()) << " zoom " << z << " WxH: " << width << " x " << height;
    for (auto db:databases){
      std::cout << "Database " << db->path.toStdString() << " stats:" << std::endl;
      db->database->DumpStatistics();
    }

    osmscout::AreaSearchParameter searchParameter;
    osmscout::MapParameter        drawParameter;
    std::list<std::string>        paths;
    std::list<osmscout::TileRef>  tiles;
    osmscout::MapData             data;

    paths.push_back(iconDirectory.toLocal8Bit().data());

    drawParameter.SetIconPaths(paths);
    drawParameter.SetPatternPaths(paths);
    drawParameter.SetDebugData(false);
    drawParameter.SetDebugPerformance(true);
    
    // optimize process can reduce number of nodes before rendering
    // it helps for slow renderer backend, but it cost some cpu
    // it seems that it is better to disable it for mobile devices with slow cpu
    drawParameter.SetOptimizeWayNodes(osmscout::TransPolygon::none);
    drawParameter.SetOptimizeAreaNodes(osmscout::TransPolygon::none);

    drawParameter.SetRenderBackground(drawBackground || renderSea);
    drawParameter.SetRenderSeaLand(renderSea);

    // see Tiler.cpp example...

    // To get accurate label drawing at tile borders, we take into account labels
    // of other than the current tile, too.
    if (z >= 14) {
        // but not for high zoom levels, it is too expensive
        drawParameter.SetDropNotVisiblePointLabels(true);
    }else{
        drawParameter.SetDropNotVisiblePointLabels(false);        
    }

    // setup projection for this tile
    osmscout::MercatorProjection projection;
    osmscout::Magnification magnification;
    magnification.SetLevel(z);
    projection.Set(center, /* angle */ 0, magnification, mapDpi, width, height);
    projection.SetLinearInterpolationUsage(z >= 10);

    // setup projection for data lookup
    osmscout::MercatorProjection lookupProjection;
    lookupProjection.Set(center, /* angle */ 0, magnification, mapDpi, lookupWidth, lookupHeight);
    lookupProjection.SetLinearInterpolationUsage(z >= 10);

    // https://github.com/Framstag/libosmscout/blob/master/Documentation/RenderTuning.txt
    //searchParameter.SetBreaker(dataLoadingBreaker);
    if (magnification.GetLevel() >= 15) {
      searchParameter.SetMaximumAreaLevel(6);
    }
    else {
      searchParameter.SetMaximumAreaLevel(4);
    }
    searchParameter.SetUseMultithreading(true);
    searchParameter.SetUseLowZoomOptimization(true);

    
    bool success = true;
    for (auto db:databases){
      osmscout::GeoBox dbBox;
      db->database->GetBoundingBox(dbBox);
      osmscout::GeoBox lookupBox;
      lookupProjection.GetDimensions(lookupBox);
      if (!dbBox.Intersects(lookupBox)){
        std::cout << "Skip database" << db->path.toStdString() << std::endl;
        continue;
      }
      std::cout << "Database " << db->path.toStdString() << " draw:" << std::endl;
      qDebug() << "prepare:   " << timer.elapsed();
      timer.restart();

      db->mapService->LookupTiles(lookupProjection,tiles);

      qDebug() << "lookup:    " << timer.elapsed();
      timer.restart();
      /*
      if (!mapService->LoadMissingTileDataAsync(searchParameter,*styleConfig,tiles)) {
        qDebug() << "*** Loading of data has error or was interrupted";
        return;
      }
       */
      // load tiles synchronous
      db->mapService->LoadMissingTileData(searchParameter,*db->styleConfig,tiles);

      qDebug() << "load data: " << timer.elapsed();
      timer.restart();
      //mapService->DumpCacheStatistics();

      db->mapService->ConvertTilesToMapData(tiles,data);

      qDebug() << "convert:   " << timer.elapsed();
      timer.restart();

      if (drawParameter.GetRenderSeaLand()) {
        db->mapService->GetGroundTiles(projection, data.groundTiles);
      }

      //p.begin(currentImage);
      p.setRenderHint(QPainter::Antialiasing);
      p.setRenderHint(QPainter::TextAntialiasing);
      p.setRenderHint(QPainter::SmoothPixmapTransform);

      success|=db->painter->DrawMap(projection,
                                    drawParameter,
                                    data,
                                    &p);

      qDebug() << "draw:      " << timer.elapsed();
      timer.restart();
      //p.end();
    }

    if (!success)  {
        qWarning() << "*** Rendering of data has error or was interrupted";
        return;
    }
}

bool DBThread::RenderMap(QPainter& painter,
                         const RenderMapRequest& request)
{
  osmscout::MercatorProjection projection;

  projection.Set(osmscout::GeoCoord(request.lat, request.lon),
                 request.angle,
                 request.magnification,
                 mapDpi,
                 request.width,
                 request.height);

  osmscout::GeoBox boundingBox;

  projection.GetDimensions(boundingBox);
  
   
  QColor white = QColor::fromRgbF(1.0,1.0,1.0);
  //QColor grey = QColor::fromRgbF(0.5,0.5,0.5);
  QColor grey2 = QColor::fromRgbF(0.8,0.8,0.8);
  
  painter.fillRect( 0,0,
                    projection.GetWidth(),projection.GetHeight(),
                    white);
    
  // OpenStreetMap render its tiles up to latitude +-85.0511 
  double osmMinLat = OSMTile::minLat();
  double osmMaxLat = OSMTile::maxLat();
  double osmMinLon = OSMTile::minLon();
  double osmMaxLon = OSMTile::maxLon();
  
  uint32_t osmTileRes = OSMTile::worldRes(projection.GetMagnification().GetLevel());
  double x1;
  double y1;
  projection.GeoToPixel(osmscout::GeoCoord(osmMaxLat, osmMinLon), x1, y1);
  double x2;
  double y2;
  projection.GeoToPixel(osmscout::GeoCoord(osmMinLat, osmMaxLon), x2, y2);
  
  double renderTileWidth = (x2 - x1) / osmTileRes; // pixels
  double renderTileHeight = (y2 - y1) / osmTileRes; // pixels
  
  painter.setPen(grey2);
  
  uint32_t osmTileFromX = std::max(0.0, (double)osmTileRes * ((boundingBox.GetMinLon() + (double)180.0) / (double)360.0)); 
  double maxLatRad = boundingBox.GetMaxLat() * GRAD_TO_RAD;                                 
  uint32_t osmTileFromY = std::max(0.0, (double)osmTileRes * ((double)1.0 - (log(tan(maxLatRad) + (double)1.0 / cos(maxLatRad)) / M_PI)) / (double)2.0);

  //std::cout << osmTileRes << " * (("<< boundingBox.GetMinLon()<<" + 180.0) / 360.0) = " << osmTileFromX << std::endl; 
  //std::cout <<  osmTileRes<<" * (1.0 - (log(tan("<<maxLatRad<<") + 1.0 / cos("<<maxLatRad<<")) / "<<M_PI<<")) / 2.0 = " << osmTileFromY << std::endl;
  
  uint32_t zoomLevel = projection.GetMagnification().GetLevel();

  /*
  double osmTileDimension = (double)OSMTile::osmTileOriginalWidth() * (dpi / OSMTile::tileDPI() ); // pixels  
  std::cout << 
    "level: " << zoomLevel << 
    " request WxH " << request.width << " x " << request.height <<
    " osmTileRes: " << osmTileRes <<
    " scaled tile dimension: " << osmTileWidth << " x " << osmTileHeight << " (" << osmTileDimension << ")"<<  
    " osmTileFromX: " << osmTileFromX << " cnt " << (projection.GetWidth() / (uint32_t)osmTileWidth) <<
    " osmTileFromY: " << osmTileFromY << " cnt " << (projection.GetHeight() / (uint32_t)osmTileHeight) <<
    " current thread : " << QThread::currentThread() << 
    std::endl;  
   */
  
  // render available tiles
  double x;
  double y;
  QTime start;
  QMutexLocker locker(&tileCacheMutex);
  int elapsed = start.elapsed();
  if (elapsed > 1){
      std::cout << "Mutex acquiere took " << elapsed << " ms" << std::endl;
  }

  onlineTileCache.clearPendingRequests();
  offlineTileCache.clearPendingRequests();
  for ( uint32_t ty = 0; 
        (ty <= (projection.GetHeight() / (uint32_t)renderTileHeight)+1) && ((osmTileFromY + ty) < osmTileRes); 
        ty++ ){
    
    //for (uint32_t i = 1; i< osmTileRes; i++){
    uint32_t ytile = (osmTileFromY + ty);
    double ytileLatRad = atan(sinh(M_PI * (1 - 2 * (double)ytile / (double)osmTileRes)));
    double ytileLatDeg = ytileLatRad * 180.0 / M_PI;

    for ( uint32_t tx = 0; 
          (tx <= (projection.GetWidth() / (uint32_t)renderTileWidth)+1) && ((osmTileFromX + tx) < osmTileRes); 
          tx++ ){

      uint32_t xtile = (osmTileFromX + tx);
      double xtileDeg = (double)xtile / (double)osmTileRes * 360.0 - 180.0;
      
      projection.GeoToPixel(osmscout::GeoCoord(ytileLatDeg, xtileDeg), x, y);
      //double x = x1 + (double)xtile * osmTileWidth;  

      //std::cout << "  xtile: " << xtile << " ytile: " << ytile << " x: " << x << " y: " << y << "" << std::endl;

      //bool lookupTileFound = false;
      
      bool lookupTileFound = false;
      if (onlineTilesEnabled){
        lookupTileFound |= lookupAndDrawTile(onlineTileCache, painter, 
              x, y, renderTileWidth, renderTileHeight, 
              zoomLevel, xtile, ytile, /* up limit */ 6, /* down limit */ 3
              );
      }

      if (offlineTilesEnabled){
        lookupTileFound |= lookupAndDrawTile(offlineTileCache, painter, 
                x, y, renderTileWidth, renderTileHeight, 
                zoomLevel, xtile, ytile, /* up limit */ 6, /* down limit */ 3
                );
      }
      
      if (!lookupTileFound){
        // no tile found, draw its outline
        painter.drawLine(x,y, x + renderTileWidth, y);      
        painter.drawLine(x,y, x, y + renderTileHeight);
      }
    }
  }
  /*
  painter.setPen(grey);
  painter.drawText(20, 30, QString("%1").arg(projection.GetMagnification().GetLevel()));
  
  double centerLat;
  double centerLon;
  projection.PixelToGeo(projection.GetWidth() / 2.0, projection.GetHeight() / 2.0, centerLon, centerLat);
  painter.drawText(20, 60, QString::fromStdString(osmscout::GeoCoord(centerLat, centerLon).GetDisplayText()));  
  */
  return onlineTileCache.isRequestQueueEmpty() && offlineTileCache.isRequestQueueEmpty();
}

bool DBThread::lookupAndDrawTile(TileCache& tileCache, QPainter& painter, 
        double x, double y, double renderTileWidth, double renderTileHeight, 
        uint32_t zoomLevel, uint32_t xtile, uint32_t ytile, 
        uint32_t upLimit, uint32_t downLimit)
{
    bool triggerRequest = true;
    
    // trick for avoiding white lines between tiles caused by antialiasing
    // http://stackoverflow.com/questions/7332118/antialiasing-leaves-thin-line-between-adjacent-widgets
    double overlap = painter.testRenderHint(QPainter::Antialiasing) ? 0.5 : 0.0;

    uint32_t lookupTileZoom = zoomLevel;
    uint32_t lookupXTile = xtile;
    uint32_t lookupYTile = ytile;
    QRectF lookupTileViewport(0, 0, 1, 1); // tile viewport (percent)
    bool lookupTileFound = false;
    
    // lookup upper zoom levels
    //qDebug() << "Need paint tile " << xtile << " " << ytile << " zoom " << zoomLevel;
    while ((!lookupTileFound) && (lookupTileZoom >= 0) && (zoomLevel - lookupTileZoom <= upLimit)){
      //qDebug() << "  - lookup tile " << lookupXTile << " " << lookupYTile << " zoom " << lookupTileZoom << " " << " viewport " << lookupTileViewport;
      if (tileCache.contains(lookupTileZoom, lookupXTile, lookupYTile)){
          TileCacheVal val = tileCache.get(lookupTileZoom, lookupXTile, lookupYTile);
          if (!val.image.isNull()){
            double imageWidth = val.image.width();
            double imageHeight = val.image.height();
            QRectF imageViewport(imageWidth * lookupTileViewport.x(), imageHeight * lookupTileViewport.y(), 
                    imageWidth * lookupTileViewport.width(), imageHeight * lookupTileViewport.height() );

            // TODO: support map rotation
            painter.drawPixmap(QRectF(x, y, renderTileWidth+overlap, renderTileHeight+overlap), val.image, imageViewport);
          }
          lookupTileFound = true;
          if (lookupTileZoom == zoomLevel)
              triggerRequest = false;
      }else{
          // no tile found on current zoom zoom level, lookup upper zoom level
          lookupTileZoom --;
          uint32_t crop = 1 << (zoomLevel - lookupTileZoom);
          double viewportWidth = 1.0 / (double)crop;
          double viewportHeight = 1.0 / (double)crop;
          lookupTileViewport = QRectF(
                  (double)(xtile % crop) * viewportWidth,
                  (double)(ytile % crop) * viewportHeight,
                  viewportWidth,
                  viewportHeight);
          lookupXTile = lookupXTile / 2;
          lookupYTile = lookupYTile / 2;
      }
    }
    
    // lookup bottom zoom levels
    if (!lookupTileFound && downLimit > 0){
        lookupAndDrawBottomTileRecursive(tileCache, painter, 
            x, y, renderTileWidth, renderTileHeight, overlap,
            zoomLevel, xtile, ytile, 
            downLimit -1);        
    }
    
    if (triggerRequest){
       if (tileCache.request(zoomLevel, xtile, ytile)){
         //std::cout << "  tile request: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
        }else{
         //std::cout << "  requested already: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
       }
    }
    return lookupTileFound;
}

void DBThread::lookupAndDrawBottomTileRecursive(TileCache& tileCache, QPainter& painter, 
        double x, double y, double renderTileWidth, double renderTileHeight, double overlap,
        uint32_t zoomLevel, uint32_t xtile, uint32_t ytile, 
        uint32_t downLimit)
{
    if (zoomLevel > 20)
        return;

    //qDebug() << "Need paint tile " << xtile << " " << ytile << " zoom " << zoomLevel;
    uint32_t lookupTileZoom = zoomLevel + 1;
    uint32_t lookupXTile;
    uint32_t lookupYTile;
    uint32_t tileCnt = 2;

    for (uint32_t ty = 0; ty < tileCnt; ty++){
        lookupYTile = ytile *2 + ty;
        for (uint32_t tx = 0; tx < tileCnt; tx++){
            lookupXTile = xtile *2 + tx;
            //qDebug() << "  - lookup tile " << lookupXTile << " " << lookupYTile << " zoom " << lookupTileZoom;
            bool found = false;
            if (tileCache.contains(lookupTileZoom, lookupXTile, lookupYTile)){
                TileCacheVal val = tileCache.get(lookupTileZoom, lookupXTile, lookupYTile);
                if (!val.image.isNull()){
                    double imageWidth = val.image.width();
                    double imageHeight = val.image.height();                    
                    painter.drawPixmap(
                            QRectF(x + tx * (renderTileWidth/tileCnt), y + ty * (renderTileHeight/tileCnt), renderTileWidth/tileCnt + overlap, renderTileHeight/tileCnt + overlap),
                            val.image, 
                            QRectF(0.0, 0.0, imageWidth, imageHeight));
                    found = true;
                }              
            }
            if (!found && downLimit > 0){
                // recursion
                lookupAndDrawBottomTileRecursive(tileCache, painter, 
                    x + tx * (renderTileWidth/tileCnt), y + ty * (renderTileHeight/tileCnt), renderTileWidth/tileCnt, renderTileHeight/tileCnt, overlap,
                    zoomLevel +1, lookupXTile, lookupYTile, 
                    downLimit -1); 
            }
        }            
    }
}


DBThread::DatabaseTileState DBThread::databaseTileState(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
    QMutexLocker locker(&mutex);
  
    // TODO: use database multi-polygon, not bounding box
    osmscout::GeoBox boundingBox;    
    for (auto db:databases){
      if (boundingBox.IsValid()){
        osmscout::GeoBox dbBox;    
        if (db->database->GetBoundingBox(dbBox)){
          boundingBox.Include(dbBox);
        }
      }else{
        db->database->GetBoundingBox(boundingBox);
      }
    }
    if (boundingBox.IsValid()) {
        osmscout::GeoBox tileBoundingBox = OSMTile::tileBoundingBox(zoomLevel, xtile, ytile);
        //osmscout::GeoCoord tileVisualCenter = OSMTile::tileVisualCenter(zoomLevel, xtile, ytile);
        
        /*
        qDebug() << "Database bounding box: " << 
                    QString::fromStdString( boundingBox.GetDisplayText()) << 
                " tile bounding box: " << 
                    QString::fromStdString( tileBoundingBox.GetDisplayText() );
         */
        
        if (boundingBox.GetMinLat() <= tileBoundingBox.GetMinLat() &&
                boundingBox.GetMinLon() <= tileBoundingBox.GetMinLon() &&
                boundingBox.GetMaxLat() >= tileBoundingBox.GetMaxLat() && 
                boundingBox.GetMaxLon() >= tileBoundingBox.GetMaxLon()) {

            return DatabaseTileState::Covered;
        }
        if (boundingBox.Intersects(tileBoundingBox)){
            return DatabaseTileState::Intersects;
        }
    }
    return DatabaseTileState::Outside;
}

void DBThread::onlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
    {    
        QMutexLocker locker(&tileCacheMutex);
        if (!onlineTileCache.startRequestProcess(zoomLevel, xtile, ytile)) // request was canceled or started already
            return;
    }
    
    // TODO: mutex?
    bool requestedFromWeb = !(offlineTilesEnabled && databaseTileState(zoomLevel, xtile, ytile) == DatabaseTileState::Covered);
    if (requestedFromWeb){
        QMutexLocker locker(&mutex);
        if (tileDownloader == NULL){
            qWarning() << "tile requested but donwloader is not initialized yet";
            emit tileDownloadFailed(zoomLevel, xtile, ytile, false);
        }else{
            emit tileDownloader->download(zoomLevel, xtile, ytile);
        }
    } else{
        // put Null image
        {    
            QMutexLocker locker(&tileCacheMutex);
            onlineTileCache.put(zoomLevel, xtile, ytile, QImage());
        }        
    }  
}

void DBThread::offlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
    {
        QMutexLocker locker(&tileCacheMutex);
        if (!offlineTileCache.startRequestProcess(zoomLevel, xtile, ytile)) // request was canceled or started already
            return;
    }

    DatabaseTileState state = databaseTileState(zoomLevel, xtile, ytile);
    bool render = (state != DatabaseTileState::Outside);
    
    if (render) {
        // tile rendering have sub-linear complexity with area size
        // it means that it is advatage to merge more tile requests with same zoom
        // and render bigger area
        uint32_t xFrom;
        uint32_t xTo;
        uint32_t yFrom;
        uint32_t yTo;
        {
            QMutexLocker locker(&tileCacheMutex);
            offlineTileCache.mergeAndStartRequests(zoomLevel, xtile, ytile, xFrom, xTo, yFrom, yTo, 5, 5);
        }
        uint32_t width = (xTo - xFrom + 1);
        uint32_t height = (yTo - yFrom + 1);
        
        //osmscout::GeoBox tileBoundingBox = OSMTile::tileBoundingBox(zoomLevel, xtile, ytile);
        osmscout::GeoCoord tileVisualCenter = OSMTile::tileRelativeCoord(zoomLevel, 
                (double)xFrom + (double)width/2.0, 
                (double)yFrom + (double)height/2.0);
        
        double osmTileDimension = (double)OSMTile::osmTileOriginalWidth() * (mapDpi / OSMTile::tileDPI() ); // pixels  
        
        QImage canvas(
                (double)width * osmTileDimension, 
                (double)height * osmTileDimension, 
                QImage::Format_ARGB32_Premultiplied); // TODO: verify best format with profiler (callgrind)
        
        QColor transparent = QColor::fromRgbF(1, 1, 1, 0.0);
        canvas.fill(transparent);
        
        QPainter p;
        p.begin(&canvas);
        
        // TODO: improve renderer to render background only on database multi-polygon
        bool drawBackground = (state == DatabaseTileState::Covered);
        DrawTileMap(p, tileVisualCenter, zoomLevel, canvas.width(), canvas.height(), 
                canvas.width() + osmTileDimension, canvas.height() + osmTileDimension, 
                drawBackground);

        p.end();
        {
            QMutexLocker locker(&tileCacheMutex);
            if (width == 1 && height == 1){
                offlineTileCache.put(zoomLevel, xtile, ytile, canvas);
            }else{
                for (uint32_t y = yFrom; y <= yTo; ++y){
                    for (uint32_t x = xFrom; x <= xTo; ++x){

                        QImage tile = canvas.copy(
                                (double)(x - xFrom) * osmTileDimension,
                                (double)(y - yFrom) * osmTileDimension,
                                osmTileDimension, osmTileDimension
                                );
                        
                        offlineTileCache.put(zoomLevel, x, y, tile);
                    }
                }
            }
        }
        Redraw();
        //std::cout << "  put offline: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
    }else{
        // put Null image
        {    
            QMutexLocker locker(&tileCacheMutex);
            offlineTileCache.put(zoomLevel, xtile, ytile, QImage());
        }        
    }
}

void DBThread::tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray downloadedData)
{
    //QMutexLocker locker(&mutex);
  
    {
        QMutexLocker locker(&tileCacheMutex);
        onlineTileCache.put(zoomLevel, x, y, image);
    }
    //std::cout << "  put: " << zoomLevel << " xtile: " << x << " ytile: " << y << std::endl;

    emit Redraw();
}

void DBThread::tileDownloadFailed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange)
{
    QMutexLocker locker(&tileCacheMutex);
    onlineTileCache.removeRequest(zoomLevel, x, y);
    
    if (zoomLevelOutOfRange && zoomLevel > 0){
        // hack: when zoom level is too high for online source, 
        // we try to request tile with lower zoom level and put it to cache 
        // as substitute
        uint32_t reqZoom = zoomLevel - 1;
        uint32_t reqX = x / 2;
        uint32_t reqY = y / 2;
        if ((!onlineTileCache.contains(reqZoom, reqX, reqY))
             && onlineTileCache.request(reqZoom, reqX, reqY)){
            qDebug() << "Tile download failed " << x << " " << y << " zoomLevel " << zoomLevel << " try lower zoom";
            //triggerTileRequest(reqZoom, reqX, reqY);
        }
    }
}

/*
osmscout::TypeConfigRef DBThread::GetTypeConfig() const
{
  return databases->GetTypeConfig();
}

bool DBThread::GetNodeByOffset(osmscout::FileOffset offset,
                               osmscout::NodeRef& node) const
{
  return databases->GetNodeByOffset(offset,node);
}

bool DBThread::GetAreaByOffset(osmscout::FileOffset offset,
                               osmscout::AreaRef& area) const
{
  return databases->GetAreaByOffset(offset,area);
}

bool DBThread::GetWayByOffset(osmscout::FileOffset offset,
                              osmscout::WayRef& way) const
{
  return databases->GetWayByOffset(offset,way);
}

bool DBThread::ResolveAdminRegionHierachie(const osmscout::AdminRegionRef& adminRegion,
                                           std::map<osmscout::FileOffset,osmscout::AdminRegionRef >& refs) const
{
  QMutexLocker locker(&mutex);

  return locationService->ResolveAdminRegionHierachie(adminRegion,
                                                      refs);
}
*/
bool DBThread::SearchForLocations(const std::string& searchPattern,
                                  size_t limit,
                                  osmscout::LocationSearchResult& result) const
{
  QMutexLocker locker(&mutex);

  osmscout::LocationSearch search;

  search.limit=limit;
  for (auto db:databases){

    if (!db->locationService->InitializeLocationSearchEntries(searchPattern, search)) {
        return false;
    }

    if (!db->locationService->SearchForLocations(search, result)){
      return false;
    }
  }
  return true;
}

bool DBThread::CalculateRoute(osmscout::Vehicle vehicle,
                              const osmscout::RoutingProfile& routingProfile,
                              const osmscout::ObjectFileRef& startObject,
                              size_t startNodeIndex,
                              const osmscout::ObjectFileRef targetObject,
                              size_t targetNodeIndex,
                              osmscout::RouteData& route)
{
  return false; // TODO: implement multi database routing
  /*
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  return router->CalculateRoute(routingProfile,
                                startObject,
                                startNodeIndex,
                                targetObject,
                                targetNodeIndex,
                                route);
  */
}

bool DBThread::TransformRouteDataToRouteDescription(osmscout::Vehicle vehicle,
                                                    const osmscout::RoutingProfile& routingProfile,
                                                    const osmscout::RouteData& data,
                                                    osmscout::RouteDescription& description,
                                                    const std::string& start,
                                                    const std::string& target)
{
  return false; // TODO: implement multi database routing
  /*
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  if (!router->TransformRouteDataToRouteDescription(data,description)) {
    return false;
  }

  osmscout::TypeConfigRef typeConfig=router->GetTypeConfig();

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors;

  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>(start));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>(target));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>());

  osmscout::RoutePostprocessor::InstructionPostprocessorRef instructionProcessor=std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>();

  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetTypeInfo("highway_motorway_link"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway_trunk"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_trunk"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetTypeInfo("highway_trunk_link"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway_primary"));
  postprocessors.push_back(instructionProcessor);

  if (!routePostprocessor.PostprocessRouteDescription(description,
                                                      routingProfile,
                                                      *databases,
                                                      postprocessors)) {
    return false;
  }

  return true;
   */
}

bool DBThread::TransformRouteDataToWay(osmscout::Vehicle vehicle,
                                       const osmscout::RouteData& data,
                                       osmscout::Way& way)
{
  return false; // TODO: implement multi database routing
  /*
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  return router->TransformRouteDataToWay(data,way);
   */
}


void DBThread::ClearRoute()
{
  emit Redraw();
}

void DBThread::AddRoute(const osmscout::Way& way)
{
  emit Redraw();
}

bool DBThread::GetClosestRoutableNode(const osmscout::ObjectFileRef& refObject,
                                      const osmscout::Vehicle& vehicle,
                                      double radius,
                                      osmscout::ObjectFileRef& object,
                                      size_t& nodeIndex)
{
  return false; // TODO: implement multi database routing
  /*
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  object.Invalidate();

  if (refObject.GetType()==osmscout::refNode) {
    osmscout::NodeRef node;

    if (!databases->GetNodeByOffset(refObject.GetFileOffset(),
                                   node)) {
      return false;
    }

    return router->GetClosestRoutableNode(node->GetCoords().GetLat(),
                                          node->GetCoords().GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else if (refObject.GetType()==osmscout::refArea) {
    osmscout::AreaRef area;

    if (!databases->GetAreaByOffset(refObject.GetFileOffset(),
                                   area)) {
      return false;
    }

    osmscout::GeoCoord center;

    area->GetCenter(center);

    return router->GetClosestRoutableNode(center.GetLat(),
                                          center.GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else if (refObject.GetType()==osmscout::refWay) {
    osmscout::WayRef way;

    if (!databases->GetWayByOffset(refObject.GetFileOffset(),
                                  way)) {
      return false;
    }

    return router->GetClosestRoutableNode(way->GetNodes()[0].GetLat(),
                                          way->GetNodes()[0].GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else {
    return true;
  }
    */
}

void DBThread::requestLocationDescription(const osmscout::GeoCoord location)
{
  if (!isInitialized()){
      return; // ignore request if db is not initialized
  }
  QMutexLocker locker(&mutex);
  
  for (auto db:databases){
    osmscout::LocationDescription description;

    if (!db->locationService->DescribeLocation(location, description)) {
      std::cerr << "Error during generation of location description" << std::endl;
      emit locationDescription(location, description); // TODO: report error
      return;
    }

    // TODO: skip if description is empty
    emit locationDescription(location, description);
  }
}

void DBThread::onMapDPIChange(double dpi)
{
    QMutexLocker locker(&mutex);
    mapDpi = dpi;

    // invalidate tile cache and emit Redraw
    {
        QMutexLocker locker(&tileCacheMutex);
        //onlineTileCache.invalidate(); // DPI change don't affect online tiles
        offlineTileCache.invalidate();
    }
    emit Redraw();
}

void DBThread::onlineTileProviderChanged()
{
    {
        QMutexLocker locker(&tileCacheMutex);
        onlineTileCache.invalidate();
    } 
    emit Redraw();    
}

void DBThread::onlineTilesEnabledChanged(bool b)
{
    {
        QMutexLocker threadLocker(&mutex);
        onlineTilesEnabled = b;

        QMutexLocker cacheLocker(&tileCacheMutex);
        onlineTileCache.invalidate();
        onlineTileCache.clearPendingRequests();
    }
    emit Redraw();    
}

void DBThread::onOfflineMapChanged(bool b)
{
    {
        QMutexLocker threadLocker(&mutex);
        offlineTilesEnabled = b;

        QMutexLocker cacheLocker(&tileCacheMutex);
        onlineTileCache.invalidate(); // overlapp areas will change
        offlineTileCache.invalidate();
        offlineTileCache.clearPendingRequests();
    }
    emit Redraw();    
}
void DBThread::onRenderSeaChanged(bool b)
{
    {
        QMutexLocker threadLocker(&mutex);
        renderSea = b;

        QMutexLocker cacheLocker(&tileCacheMutex);
        offlineTileCache.invalidate();
        offlineTileCache.clearPendingRequests();
    }
    emit Redraw();
}

static DBThread* dbThreadInstance=NULL;

bool DBThread::InitializeInstance(QStringList databaseDirectory, QString resourceDirectory, QString tileCacheDirectory,
                                  size_t onlineTileCacheSize, size_t offlineTileCacheSize)
{
  if (dbThreadInstance!=NULL) {
    return false;
  }

  dbThreadInstance=new DBThread(databaseDirectory, resourceDirectory, tileCacheDirectory,
                                onlineTileCacheSize, offlineTileCacheSize);

  return true;
}

DBThread* DBThread::GetInstance()
{
  return dbThreadInstance;
}

void DBThread::FreeInstance()
{
  delete dbThreadInstance;

  dbThreadInstance=NULL;
}
