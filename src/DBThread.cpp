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

#include <cmath>
#include <iostream>

#include <QGuiApplication>
#include <QMutexLocker>
#include <QDebug>
#include <QDir>

#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

// Timeout for the first rendering after rerendering was triggered (render what ever data is available)
static int INITIAL_DATA_RENDERING_TIMEOUT = 10;

// Timeout for the updated rendering after rerendering was triggered (more rendering data is available)
static int UPDATED_DATA_RENDERING_TIMEOUT = 200;

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
   tileDownloader(tileCacheDirectory),
   database(std::make_shared<osmscout::Database>(databaseParameter)),
   locationService(std::make_shared<osmscout::LocationService>(database)),
   mapService(std::make_shared<osmscout::MapService>(database)),
   daylight(true),
   painter(NULL),
   iconDirectory(),
   pendingRenderingTimer(this),
   currentImage(NULL),
   currentLat(0.0),
   currentLon(0.0),
   currentAngle(0.0),
   currentMagnification(0),
   finishedImage(NULL),
   finishedLat(0.0),
   finishedLon(0.0),
   finishedMagnification(0),
   dataLoadingBreaker(std::make_shared<QBreaker>())
{
  osmscout::log.Debug() << "DBThread::DBThread()";

  QScreen *srn=QGuiApplication::screens().at(0);

  dpi=(double)srn->physicalDotsPerInch();

  pendingRenderingTimer.setSingleShot(true);

  connect(this,SIGNAL(TriggerInitialRendering()),
          this,SLOT(HandleInitialRenderingRequest()));

  connect(&pendingRenderingTimer,SIGNAL(timeout()),
          this,SLOT(DrawMap()));

  connect(this,SIGNAL(TileStatusChanged(const osmscout::TileRef&)),
          this,SLOT(HandleTileStatusChanged(const osmscout::TileRef&)));

  connect(&tileDownloader, SIGNAL(downloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)),
          this, SLOT(tileDownloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)));
  
  connect(&tileDownloader, SIGNAL(failed(uint32_t, uint32_t, uint32_t)),
          this, SLOT(tileDownloadFailed(uint32_t, uint32_t, uint32_t)));
  //
  // Make sure that we always decouple caller and receiver even if they are running in the same thread
  // else we might get into a dead lock
  //

  connect(this,SIGNAL(TriggerDrawMap()),
          this,SLOT(DrawMap()),
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

  mapService->DeregisterTileStateCallback(callbackId);
}

void DBThread::FreeMaps()
{
  delete currentImage;
  currentImage=NULL;

  delete finishedImage;
  finishedImage=NULL;
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
  //std::cout << "Triggering initial data rendering timer..." << std::endl;
  pendingRenderingTimer.stop();
  pendingRenderingTimer.start(INITIAL_DATA_RENDERING_TIMEOUT);
}

void DBThread::HandleTileStatusChanged(const osmscout::TileRef& changedTile)
{
  QMutexLocker locker(&mutex);

  std::list<osmscout::TileRef> tiles;

  mapService->LookupTiles(projection,tiles);

  bool relevant=false;

  for (const auto tile : tiles) {
    if (tile==changedTile) {
      relevant=true;
      break;
    }
  }

  if (!relevant) {
    return;
  }

  int elapsedTime=lastRendering.elapsed();

  //std::cout << "Relevant tile changed " << elapsedTime << std::endl;

  if (pendingRenderingTimer.isActive()) {
    //std::cout << "Waiting for timer in " << pendingRenderingTimer.remainingTime() << std::endl;
  }
  else if (elapsedTime>UPDATED_DATA_RENDERING_TIMEOUT) {
    emit TriggerDrawMap();
  }
  else {
    //std::cout << "Triggering updated data rendering timer..." << std::endl;
    pendingRenderingTimer.start(UPDATED_DATA_RENDERING_TIMEOUT-elapsedTime);
  }
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

  stylesheetFilename = databaseDirectory + QDir::separator() + "standard.oss";
  iconDirectory = resourceDirectory + QDir::separator() + "icons";

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
    qDebug() << "Cannot open database!";
    return;
  }


  DatabaseLoadedResponse response;

  if (!database->GetBoundingBox(response.boundingBox)) {
    qDebug() << "Cannot read initial bounding box";
    return;
  }

  lastRendering=QTime::currentTime();

  qDebug() << "InitialisationFinished";
  emit InitialisationFinished(response);
}

void DBThread::Finalize()
{
  qDebug() << "Finalize";
  FreeMaps();

  if (router && router->IsOpen()) {
    router->Close();
  }

  if (database->IsOpen()) {
    database->Close();
  }
}

void DBThread::GetProjection(osmscout::MercatorProjection& projection)
{
    QMutexLocker locker(&mutex);

    projection=this->projection;
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
 * Triggers the loading of data for the given area and also triggers rendering
 * of the data afterwards.
 */
void DBThread::TriggerMapRendering(const RenderMapRequest& request)
{
  std::cout << ">>> User triggered rendering" << std::endl;
  qDebug() << "magnification: " << request.magnification.GetMagnification() << " level: " << request.magnification.GetLevel();
  dataLoadingBreaker->Reset();

  {
    QMutexLocker locker(&mutex);

    currentWidth=request.width;
    currentHeight=request.height;
    currentLon=request.lon;
    currentLat=request.lat;
    currentAngle=request.angle;
    currentMagnification=request.magnification;

    if (database->IsOpen() &&
        styleConfig) {
      osmscout::MapParameter        drawParameter;
      osmscout::AreaSearchParameter searchParameter;

      searchParameter.SetBreaker(dataLoadingBreaker);

      if (currentMagnification.GetLevel()>=15) {
        searchParameter.SetMaximumAreaLevel(6);
      }
      else {
        searchParameter.SetMaximumAreaLevel(4);
      }

      searchParameter.SetUseMultithreading(true);
      searchParameter.SetUseLowZoomOptimization(true);

      projection.Set(currentLon,
                     currentLat,
                     currentAngle,
                     currentMagnification,
                     dpi,
                     currentWidth,
                     currentHeight);

      std::list<osmscout::TileRef> tiles;

      mapService->LookupTiles(projection,tiles);
      if (!mapService->LoadMissingTileDataAsync(searchParameter,*styleConfig,tiles)) {
        qDebug() << "*** Loading of data has error or was interrupted";
        return;
      }

      emit TriggerInitialRendering();
    }
    else {
      qDebug() << "Cannot draw map: " << database->IsOpen() << " " << styleConfig.get();

      QPainter p;

      RenderMessage(p,request.width,request.height,"Database not open");
    }
  }
}

/**
 * Actual map drawing into the back buffer
 */
void DBThread::DrawMap()
{
  std::cout << "DrawMap()" << std::endl;
  {
    QMutexLocker locker(&mutex);

    if (currentImage==NULL ||
        currentImage->width()!=(int)currentWidth ||
        currentImage->height()!=(int)currentHeight) {
      delete currentImage;

      currentImage=new QImage(QSize(currentWidth,
                                    currentHeight),
                              QImage::Format_RGB32);
    }

    osmscout::MapParameter       drawParameter;
    std::list<std::string>       paths;
    std::list<osmscout::TileRef> tiles;

    paths.push_back(iconDirectory.toLocal8Bit().data());

    drawParameter.SetIconPaths(paths);
    drawParameter.SetPatternPaths(paths);
    drawParameter.SetDebugData(false);
    drawParameter.SetDebugPerformance(true);
    drawParameter.SetOptimizeWayNodes(osmscout::TransPolygon::quality);
    drawParameter.SetOptimizeAreaNodes(osmscout::TransPolygon::quality);
    drawParameter.SetRenderBackground(true);
    drawParameter.SetRenderSeaLand(true);

    mapService->LookupTiles(projection,tiles);

    mapService->ConvertTilesToMapData(tiles,data);

    if (drawParameter.GetRenderSeaLand()) {
      mapService->GetGroundTiles(projection,
                                 data.groundTiles);
    }

    QPainter p;

    p.begin(currentImage);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    bool success=painter->DrawMap(projection,
                                  drawParameter,
                                  data,
                                  &p);

    p.end();

    if (!success)  {
      qDebug() << "*** Rendering of data has error or was interrupted";
      return;
    }

    std::swap(currentImage,finishedImage);

    finishedLon=currentLon;
    finishedLat=currentLat;
    finishedAngle=currentAngle;
    finishedMagnification=currentMagnification;

    lastRendering=QTime::currentTime();
  }

  emit HandleMapRenderingResult();
}

void DBThread::RenderMessage(QPainter& painter, qreal width, qreal height, const char* message)
{
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);

  painter.fillRect(0,0,width,height,
                   QColor::fromRgbF(0.0,0.0,0.0,1.0));

  painter.setPen(QColor::fromRgbF(1.0,1.0,1.0,1.0));

  QString text(message);

  painter.drawText(QRectF(0.0,0.0,width,height),
                   Qt::AlignCenter|Qt::AlignVCenter,
                   text,
                   NULL);
}

 static const double gradtorad=2*M_PI/360;

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
  QColor blue = QColor::fromRgbF(0.0,0.0,1.0);
  QColor grey = QColor::fromRgbF(0.5,0.5,0.5);
  
  painter.fillRect( 0,0,
                    projection.GetWidth(),projection.GetHeight(),
                    white);
    
  // OpenStreetMap render its tiles up to latitude +-85.0511 
  double osmMinLat =  -85.0511;
  double osmMaxLat =   85.0511;
  double osmMinLon = -180.0;
  double osmMaxLon =  180.0;
  uint32_t osmTileOriginalWidth = 256; // pixels
  uint32_t osmTileRes = 1 << projection.GetMagnification().GetLevel();
  double x1;
  double y1;
  projection.GeoToPixel(osmscout::GeoCoord(osmMaxLat, osmMinLon), x1, y1);
  double x2;
  double y2;
  projection.GeoToPixel(osmscout::GeoCoord(osmMinLat, osmMaxLon), x2, y2);
  
  // draw line around whole map
  painter.setPen(blue);
  /*
  painter.drawLine(x1,y1, x2, y1);
  painter.drawLine(x1,y1, x1, y2);
   */
  painter.drawLine(x2,y1, x2, y2);
  painter.drawLine(x1,y2, x2, y2);
  double osmTileWidth = (x2 - x1) / osmTileRes; // pixels
  double osmTileHeight = (y2 - y1) / osmTileRes; // pixels
  double osmTileDimension = osmTileOriginalWidth * (projection.GetMagnification().GetMagnification() / (double)osmTileRes); // pixels
  
  
  painter.setPen(grey);
  // http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
      
  uint32_t osmTileFromX = std::max(0.0, (double)osmTileRes * ((boundingBox.GetMinLon() + (double)180.0) / (double)360.0)); 
  double maxLatRad = boundingBox.GetMaxLat() * gradtorad;                                 
  uint32_t osmTileFromY = std::max(0.0, (double)osmTileRes * ((double)1.0 - (log(tan(maxLatRad) + (double)1.0 / cos(maxLatRad)) / M_PI)) / (double)2.0); // std::max(0, ((int32_t)y1 / (int32_t)osmTileHeight)* -1);

  //std::cout << osmTileRes << " * (("<< boundingBox.GetMinLon()<<" + 180.0) / 360.0) = " << osmTileFromX << std::endl; 
  //std::cout <<  osmTileRes<<" * (1.0 - (log(tan("<<maxLatRad<<") + 1.0 / cos("<<maxLatRad<<")) / "<<M_PI<<")) / 2.0 = " << osmTileFromY << std::endl;
  
  uint32_t zoomLevel = projection.GetMagnification().GetLevel();
  
  std::cout << 
    "level: " << zoomLevel << 
    " osmTileRes: " << osmTileRes <<
    " scaled tile dimension: " << osmTileWidth << " x " << osmTileHeight << " (" << osmTileDimension << ")"<<  
    " osmTileFromX: " << osmTileFromX << " cnt " << (projection.GetWidth() / (uint32_t)osmTileWidth) <<
    " osmTileFromY: " << osmTileFromY << " cnt " << (projection.GetHeight() / (uint32_t)osmTileHeight) <<
    std::endl;  
  
  // render tile net
  double x;
  double y;
  QMutexLocker locker(&tileCache.mutex);
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

      // TODO: take angle into account
      if (tileCache.contains(zoomLevel, xtile, ytile)){
        QImage img = tileCache.get(zoomLevel, xtile, ytile);
        painter.drawImage(QRectF(x, y, osmTileWidth, osmTileHeight), img);
      }else{
        if (tileCache.request(zoomLevel, xtile, ytile)){
          std::cout << "  tile request: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
          emit tileRequest(zoomLevel, xtile, ytile, osmTileWidth, osmTileHeight);
        }else{
          std::cout << "  requested already: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
        }
      }
      painter.drawLine(x,y, x + osmTileWidth, y);      
      painter.drawLine(x,y, x, y + osmTileHeight);      
    }
    //double y = y1 + (double)ytile * osmTileHeight;
    //painter.drawLine(x1,y, x2, y);  
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
  uint32_t xtile, uint32_t ytile, 
  uint32_t expectedRenderedWidth, uint32_t expectedRenderedHeight)
{
  emit tileDownloader.download(zoomLevel, xtile, ytile);
}

void DBThread::tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray downloadedData)
{
  QMutexLocker locker(&tileCache.mutex);
  tileCache.put(zoomLevel, x, y, image);
  std::cout << "  put: " << zoomLevel << " xtile: " << x << " ytile: " << y << std::endl;

  emit HandleMapRenderingResult();
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
  {
    QMutexLocker locker(&mutex);

    data.poiWays.clear();

    FreeMaps();
  }

  emit Redraw();
}

void DBThread::AddRoute(const osmscout::Way& way)
{
  {
    QMutexLocker locker(&mutex);

    data.poiWays.push_back(std::make_shared<osmscout::Way>(way));

    FreeMaps();
  }

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
