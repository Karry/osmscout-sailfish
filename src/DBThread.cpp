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


DBThread::DBThread(QString databaseDirectory, QString resourceDirectory, QString tileCacheDirectory)
 : databaseDirectory(databaseDirectory), 
   resourceDirectory(resourceDirectory),
   tileCacheDirectory(tileCacheDirectory),
   tileDownloader(NULL),
   database(std::make_shared<osmscout::Database>(databaseParameter)),
   locationService(std::make_shared<osmscout::LocationService>(database)),
   mapService(std::make_shared<osmscout::MapService>(database)),
   daylight(true),
   painter(NULL),
   iconDirectory(),
   dataLoadingBreaker(std::make_shared<QBreaker>())
{
  osmscout::log.Debug() << "DBThread::DBThread()";

  QScreen *srn=QGuiApplication::screens().at(0);

  dpi=(double)srn->physicalDotsPerInch();

  connect(this,SIGNAL(TriggerInitialRendering()),
          this,SLOT(HandleInitialRenderingRequest()));

  connect(this,SIGNAL(TileStatusChanged(const osmscout::TileRef&)),
          this,SLOT(HandleTileStatusChanged(const osmscout::TileRef&)));

  //
  // Make sure that we always decouple caller and receiver even if they are running in the same thread
  // else we might get into a dead lock
  //
  
  connect(this,SIGNAL(triggerTileRequest(uint32_t, uint32_t, uint32_t)),
          this,SLOT(tileRequest(uint32_t, uint32_t, uint32_t)),
          Qt::QueuedConnection);


  osmscout::MapService::TileStateCallback callback=[this](const osmscout::TileRef& tile) {TileStateCallback(tile);};

  callbackId=mapService->RegisterTileStateCallback(callback);
}

DBThread::~DBThread()
{
  osmscout::log.Debug() << "DBThread::~DBThread()";

  if (painter!=NULL) {
    delete painter;
  }
  if (tileDownloader != NULL){
    delete tileDownloader;
  }
  mapService->DeregisterTileStateCallback(callbackId);
}

bool DBThread::AssureRouter(osmscout::Vehicle /*vehicle*/)
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

void DBThread::HandleInitialRenderingRequest()
{
    
}

void DBThread::HandleTileStatusChanged(const osmscout::TileRef& changedTile)
{
    // ignore this event, we are loading tiles synchronously
    
    /*
    QMutexLocker locker(&tileCache.mutex);
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
  return database->IsOpen();
}
const double DBThread::GetDpi() const
{
    return dpi;
}

const DatabaseLoadedResponse DBThread::loadedResponse() const {
  QMutexLocker locker(&mutex);
  DatabaseLoadedResponse response;
  database->GetBoundingBox(response.boundingBox);
  return response;
}

void DBThread::Initialize()
{
  QMutexLocker locker(&mutex);
  qDebug() << "Initialize";

  // create tile downloader in correct thread
  tileDownloader = new OsmTileDownloader(tileCacheDirectory);
  connect(tileDownloader, SIGNAL(downloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)),
          this, SLOT(tileDownloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)));
  
  connect(tileDownloader, SIGNAL(failed(uint32_t, uint32_t, uint32_t)),
          this, SLOT(tileDownloadFailed(uint32_t, uint32_t, uint32_t)));
    
  stylesheetFilename = resourceDirectory + QDir::separator() + "map-styles" + QDir::separator() + "standard.oss";
  iconDirectory = resourceDirectory + QDir::separator() + "map-icons";

  if (database->Open(databaseDirectory.toLocal8Bit().data())) {
    osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

    if (typeConfig) {
      styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

      delete painter;
      painter=NULL;

      if (styleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
          painter=new osmscout::MapPainterQt(styleConfig);
      }
      else {
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
    return;
  }


  DatabaseLoadedResponse response;

  if (!database->GetBoundingBox(response.boundingBox)) {
    qDebug() << "Cannot read initial bounding box";
    return;
  }

  //lastRendering=QTime::currentTime();

  qDebug() << "InitialisationFinished";
  emit InitialisationFinished(response);
  
  // invalidate tile cache and emit Redraw
  tileCache.invalidate(response.boundingBox);
  emit Redraw();
}

void DBThread::Finalize()
{
  qDebug() << "Finalize";
  //FreeMaps();

  if (router && router->IsOpen()) {
    router->Close();
  }

  if (database->IsOpen()) {
    database->Close();
  }
}

void DBThread::CancelCurrentDataLoading()
{
  dataLoadingBreaker->Break();
}

void DBThread::ToggleDaylight()
{
  QMutexLocker locker(&mutex);

  qDebug() << "Toggling daylight from " << daylight << " to " << !daylight << "...";

  if (!database->IsOpen()) {
      return;
    }

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

  if (!typeConfig) {
    return;
  }

  osmscout::StyleConfigRef newStyleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  newStyleConfig->AddFlag("daylight",!daylight);

  qDebug() << "Loading new stylesheet with daylight = " << newStyleConfig->GetFlagByName("daylight");

  if (newStyleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
    // Tear down
    delete painter;
    painter=NULL;

    // Recreate
    styleConfig=newStyleConfig;
    painter=new osmscout::MapPainterQt(styleConfig);

    daylight=!daylight;

    qDebug() << "Toggling daylight done.";
  }
}

void DBThread::ReloadStyle()
{
  qDebug() << "Reloading style...";

  QMutexLocker locker(&mutex);

  if (!database->IsOpen()) {
    return;
  }

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

  if (!typeConfig) {
    return;
  }

  osmscout::StyleConfigRef newStyleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  newStyleConfig->AddFlag("daylight",daylight);

  if (newStyleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
    // Tear down
    delete painter;
    painter=NULL;

    // Recreate
    styleConfig=newStyleConfig;
    painter=new osmscout::MapPainterQt(styleConfig);

    mapService->FlushTileCache();

    qDebug() << "Reloading style done.";
  }
}

/**
 * Actual map drawing into the back buffer
 * 
 * have to be called with acquiered mutex
 */
void DBThread::DrawTileMap(QPainter &p, const osmscout::GeoCoord center, uint32_t z, size_t width, size_t height, bool drawBackground)
{
  qDebug() << "DrawTileMap()" ;
  // TODO: rewrite to tile rendering
  
  {
    //QMutexLocker locker(&mutex);

    if (!database->IsOpen() || (!styleConfig)) {
        qWarning() << "Not initialized!";
        return;
    }
    qDebug() << "Render tile offline " << QString::fromStdString(center.GetDisplayText()) << " zoom " << z << " WxH: " << width << " x " << height;
          
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
    drawParameter.SetOptimizeWayNodes(osmscout::TransPolygon::quality);
    drawParameter.SetOptimizeAreaNodes(osmscout::TransPolygon::quality);
    
    drawParameter.SetRenderBackground(drawBackground);
    drawParameter.SetRenderSeaLand(false);
    
    // setup projection for this tile
    osmscout::MercatorProjection projection;
    osmscout::Magnification magnification;
    magnification.SetLevel(z);
    projection.Set(center.lon, center.lat, 0, magnification, dpi, width, height);
    
    //searchParameter.SetBreaker(dataLoadingBreaker);
    if (magnification.GetLevel() >= 15) {
      searchParameter.SetMaximumAreaLevel(6);
    }
    else {
      searchParameter.SetMaximumAreaLevel(4);
    }
    searchParameter.SetUseMultithreading(true);
    searchParameter.SetUseLowZoomOptimization(true);
    
    
    mapService->LookupTiles(projection,tiles);
    /*
    if (!mapService->LoadMissingTileDataAsync(searchParameter,*styleConfig,tiles)) {
      qDebug() << "*** Loading of data has error or was interrupted";
      return;
    }
     */
    // load tiles synchronous
    mapService->LoadMissingTileData(searchParameter,*styleConfig,tiles);
    mapService->ConvertTilesToMapData(tiles,data);

    if (drawParameter.GetRenderSeaLand()) {
      mapService->GetGroundTiles(projection, data.groundTiles);
    }

    //QPainter p;

    //p.begin(currentImage);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    bool success=painter->DrawMap(projection,
                                  drawParameter,
                                  data,
                                  &p);

    //p.end();

    if (!success)  {
      qWarning() << "*** Rendering of data has error or was interrupted";
      return;
    }

  }
}

bool DBThread::RenderMap(QPainter& painter,
                         const RenderMapRequest& request)
{
  osmscout::MercatorProjection projection;

  projection.Set(request.lon,
                 request.lat,
                 request.angle,
                 request.magnification,
                 dpi,
                 request.width,
                 request.height);

  osmscout::GeoBox boundingBox;

  projection.GetDimensions(boundingBox);
  
   
  QColor white = QColor::fromRgbF(1.0,1.0,1.0);
  QColor grey = QColor::fromRgbF(0.5,0.5,0.5);
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
  
  double osmTileWidth = (x2 - x1) / osmTileRes; // pixels
  double osmTileHeight = (y2 - y1) / osmTileRes; // pixels
  
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
  QMutexLocker locker(&tileCache.mutex);
  int elapsed = start.elapsed();
  if (elapsed > 1){
      std::cout << "Mutex acquiere took " << elapsed << " ms" << std::endl;
  }

  tileCache.clearPendingRequests();
  for ( uint32_t ty = 0; 
        (ty <= (projection.GetHeight() / (uint32_t)osmTileHeight)+1) && ((osmTileFromY + ty) < osmTileRes); 
        ty++ ){
    
    //for (uint32_t i = 1; i< osmTileRes; i++){
    uint32_t ytile = (osmTileFromY + ty);
    double ytileLatRad = atan(sinh(M_PI * (1 - 2 * (double)ytile / (double)osmTileRes)));
    double ytileLatDeg = ytileLatRad * 180.0 / M_PI;

    for ( uint32_t tx = 0; 
          (tx <= (projection.GetWidth() / (uint32_t)osmTileWidth)+1) && ((osmTileFromX + tx) < osmTileRes); 
          tx++ ){

      uint32_t xtile = (osmTileFromX + tx);
      double xtileDeg = (double)xtile / (double)osmTileRes * 360.0 - 180.0;
      
      projection.GeoToPixel(osmscout::GeoCoord(ytileLatDeg, xtileDeg), x, y);
      //double x = x1 + (double)xtile * osmTileWidth;  

      //std::cout << "  xtile: " << xtile << " ytile: " << ytile << " x: " << x << " y: " << y << "" << std::endl;

      bool repaintRequested = false;
      if (tileCache.contains(zoomLevel, xtile, ytile)){
        TileCacheVal val = tileCache.get(zoomLevel, xtile, ytile);
        // take angle into account
        if (projection.GetAngle() == 0){
            if (painter.testRenderHint(QPainter::Antialiasing)){
                // trick for avoiding white lines between tiles caused by antialiasing
                // http://stackoverflow.com/questions/7332118/antialiasing-leaves-thin-line-between-adjacent-widgets
                painter.drawImage(QRectF(x, y, osmTileWidth+0.5, osmTileHeight+0.5), val.image);
            }else{
                painter.drawImage(QRectF(x, y, osmTileWidth, osmTileHeight), val.image);
            }
        }else{
            painter.drawLine(x,y, x + osmTileWidth, y);      
            painter.drawLine(x,y, x, y + osmTileHeight);                  
        }
        repaintRequested = val.needsRepaint;
      }else{
        repaintRequested = true;
        painter.drawLine(x,y, x + osmTileWidth, y);      
        painter.drawLine(x,y, x, y + osmTileHeight);      
      }      
      
      if (repaintRequested){
        if (tileCache.request(zoomLevel, xtile, ytile)){
          //std::cout << "  tile request: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
          emit triggerTileRequest(zoomLevel, xtile, ytile);
        }else{
          //std::cout << "  requested already: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
        }
      }
    }
  }
  
  painter.setPen(grey);
  painter.drawText(20, 30, QString("%1").arg(projection.GetMagnification().GetLevel()));
  
  double centerLat;
  double centerLon;
  projection.PixelToGeo(projection.GetWidth() / 2.0, projection.GetHeight() / 2.0, centerLon, centerLat);
  painter.drawText(20, 60, QString::fromStdString(osmscout::GeoCoord(centerLat, centerLon).GetDisplayText()));  

  return true;
}

void DBThread::tileRequest(uint32_t zoomLevel, 
  uint32_t xtile, uint32_t ytile)
{
    {    
        QMutexLocker locker(&tileCache.mutex);
        if (!tileCache.containsRequest(zoomLevel, xtile, ytile)) // request was canceled
            return;
        tileCache.startRequestProcess(zoomLevel, xtile, ytile);
    }    

    bool requestedFromWeb = true;
    
    osmscout::GeoBox boundingBox;    
    if (database->GetBoundingBox(boundingBox)) {
        osmscout::GeoBox tileBoundingBox = OSMTile::tileBoundingBox(zoomLevel, xtile, ytile);
        osmscout::GeoCoord tileVisualCenter = OSMTile::tileVisualCenter(zoomLevel, xtile, ytile);
        
        qDebug() << "tileRequest: Database bounding box: " << 
                    QString::fromStdString( boundingBox.GetDisplayText()) << 
                " tile bounding box: " << 
                    QString::fromStdString( tileBoundingBox.GetDisplayText() );
        
        // TODO: render offline map when database area intersects tile and online map download fails
        if (boundingBox.GetMinLat() <= tileBoundingBox.GetMinLat() &&
                boundingBox.GetMinLon() <= tileBoundingBox.GetMinLon() &&
                boundingBox.GetMaxLat() >= tileBoundingBox.GetMaxLat() && 
                boundingBox.GetMaxLon() >= tileBoundingBox.GetMaxLon()) {
            
            // we can render whole tile offline
            
            requestedFromWeb = false;
            double osmTileDimension = (double)OSMTile::osmTileOriginalWidth() * (dpi / OSMTile::tileDPI() ); // pixels  
            QImage canvas(osmTileDimension, osmTileDimension, QImage::Format_RGB32);
            QPainter p;
            p.begin(&canvas);            

            DrawTileMap(p, tileVisualCenter, zoomLevel, canvas.width(), canvas.height(), true);

            p.end();
            {    
                QMutexLocker locker(&tileCache.mutex);
                tileCache.put(zoomLevel, xtile, ytile, canvas);
            }
            Redraw();
            //std::cout << "  put offline: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
        }
    }
    
    if (requestedFromWeb){
        if (tileDownloader == NULL){
            qWarning() << "tile requested but donwloader is not initialized yet";
            emit tileDownloadFailed(zoomLevel, xtile, ytile);
        }else{
            emit tileDownloader->download(zoomLevel, xtile, ytile);
        }
    }
}

void DBThread::tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray downloadedData)
{
    QMutexLocker locker(&mutex);
  
    osmscout::GeoBox boundingBox;    
    if (database->GetBoundingBox(boundingBox)) {
        osmscout::GeoBox tileBoundingBox = OSMTile::tileBoundingBox(zoomLevel, x, y);

        qDebug() << "tileDownloaded: Database bounding box: " << 
                    QString::fromStdString( boundingBox.GetDisplayText()) << 
                " tile bounding box: " << 
                    QString::fromStdString( tileBoundingBox.GetDisplayText() );

        if (boundingBox.Intersects(tileBoundingBox)){
            // if tile bounding box intersects with database bounding box, we render offline data on it
            double osmTileDimension = (double)OSMTile::osmTileOriginalWidth() * (dpi / OSMTile::tileDPI() ); // pixels  
            osmscout::GeoCoord tileVisualCenter = OSMTile::tileVisualCenter(zoomLevel, x, y);
            
            if ((int)osmTileDimension != image.width() || (int)osmTileDimension != image.width()){
                QImage canvas(osmTileDimension, osmTileDimension, QImage::Format_RGB32);
    
                QPainter p;
                p.begin(&canvas);
                p.drawImage(QRectF(0, 0, canvas.width() +0.5, canvas.height()+0.5), image);
                DrawTileMap(p, tileVisualCenter, zoomLevel, canvas.width(), canvas.height(), false);
                              
                p.end();
                image = canvas;
            }else{
                QPainter p;
                p.begin(&image);                
                DrawTileMap(p, tileVisualCenter, zoomLevel, image.width(), image.height(), false);
                p.end();
            }
            
        }
    }
    {
        QMutexLocker locker(&tileCache.mutex);
        tileCache.put(zoomLevel, x, y, image);
    }
    //std::cout << "  put: " << zoomLevel << " xtile: " << x << " ytile: " << y << std::endl;

    emit Redraw();
}

void DBThread::tileDownloadFailed(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
  QMutexLocker locker(&tileCache.mutex);
  tileCache.removeRequest(zoomLevel, x, y);
}

osmscout::TypeConfigRef DBThread::GetTypeConfig() const
{
  return database->GetTypeConfig();
}

bool DBThread::GetNodeByOffset(osmscout::FileOffset offset,
                               osmscout::NodeRef& node) const
{
  return database->GetNodeByOffset(offset,node);
}

bool DBThread::GetAreaByOffset(osmscout::FileOffset offset,
                               osmscout::AreaRef& area) const
{
  return database->GetAreaByOffset(offset,area);
}

bool DBThread::GetWayByOffset(osmscout::FileOffset offset,
                              osmscout::WayRef& way) const
{
  return database->GetWayByOffset(offset,way);
}

bool DBThread::ResolveAdminRegionHierachie(const osmscout::AdminRegionRef& adminRegion,
                                           std::map<osmscout::FileOffset,osmscout::AdminRegionRef >& refs) const
{
  QMutexLocker locker(&mutex);

  return locationService->ResolveAdminRegionHierachie(adminRegion,
                                                      refs);
}

bool DBThread::SearchForLocations(const std::string& searchPattern,
                                  size_t limit,
                                  osmscout::LocationSearchResult& result) const
{
  QMutexLocker locker(&mutex);


  osmscout::LocationSearch search;

  search.limit=limit;

  if (!locationService->InitializeLocationSearchEntries(searchPattern,
                                                        search)) {
      return false;
  }

  return locationService->SearchForLocations(search,
                                             result);
}

bool DBThread::CalculateRoute(osmscout::Vehicle vehicle,
                              const osmscout::RoutingProfile& routingProfile,
                              const osmscout::ObjectFileRef& startObject,
                              size_t startNodeIndex,
                              const osmscout::ObjectFileRef targetObject,
                              size_t targetNodeIndex,
                              osmscout::RouteData& route)
{
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
}

bool DBThread::TransformRouteDataToRouteDescription(osmscout::Vehicle vehicle,
                                                    const osmscout::RoutingProfile& routingProfile,
                                                    const osmscout::RouteData& data,
                                                    osmscout::RouteDescription& description,
                                                    const std::string& start,
                                                    const std::string& target)
{
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
                                                      *database,
                                                      postprocessors)) {
    return false;
  }

  return true;
}

bool DBThread::TransformRouteDataToWay(osmscout::Vehicle vehicle,
                                       const osmscout::RouteData& data,
                                       osmscout::Way& way)
{
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  return router->TransformRouteDataToWay(data,way);
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
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  object.Invalidate();

  if (refObject.GetType()==osmscout::refNode) {
    osmscout::NodeRef node;

    if (!database->GetNodeByOffset(refObject.GetFileOffset(),
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

    if (!database->GetAreaByOffset(refObject.GetFileOffset(),
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

    if (!database->GetWayByOffset(refObject.GetFileOffset(),
                                  way)) {
      return false;
    }

    return router->GetClosestRoutableNode(way->nodes[0].GetLat(),
                                          way->nodes[0].GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else {
    return true;
  }
}

static DBThread* dbThreadInstance=NULL;

bool DBThread::InitializeInstance(QString databaseDirectory, QString resourceDirectory, QString tileCacheDirectory)
{
  if (dbThreadInstance!=NULL) {
    return false;
  }

  dbThreadInstance=new DBThread(databaseDirectory, resourceDirectory, tileCacheDirectory);

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
