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
#include <QThread>

#include "OsmTileDownloader.h"

OsmTileDownloader::OsmTileDownloader(QString diskCacheDir):
  serverNumber(qrand())
{
  connect(&webCtrl, SIGNAL (finished(QNetworkReply*)),  this, SLOT (fileDownloaded(QNetworkReply*)));
 
  /** http://wiki.openstreetmap.org/wiki/Tile_usage_policy
   * 
   * - Valid User-Agent identifying application. Faking another app's User-Agent WILL get you blocked.
   * - If known, a valid HTTP Referer.
   * - DO NOT send no-cache headers. ("Cache-Control: no-cache", "Pragma: no-cache" etc.)
   * - Cache Tile downloads locally according to HTTP Expiry Header, alternatively a minimum of 7 days.
   * - Maximum of 2 download threads. (Unmodified web browsers' download thread limits are acceptable.)
   */
 
  
  diskCache.setCacheDirectory(diskCacheDir);
  webCtrl.setCache(&diskCache); 
}

OsmTileDownloader::~OsmTileDownloader() {
}

void OsmTileDownloader::download(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
  if (zoomLevel > 19){ // TODO: load from configuraion
    emit failed(zoomLevel, x, y, true);
    return;
  }
  
  // TODO: move tile provider url and its maximum zoom level to resources
  QChar ch('a' + char(serverNumber % ('a' - 'c')));  
  QUrl tileUrl(QString("https://%1.tile.openstreetmap.org/%2/%3/%4.png")
    .arg(ch).arg(zoomLevel).arg(x).arg(y));  
  qDebug() << "Download tile " << tileUrl << " (current thread: " << QThread::currentThread() << ")";
  
  TileCacheKey key = {zoomLevel, x, y};
  requests.insert(tileUrl, key);
  
  QNetworkRequest request(tileUrl);
  request.setHeader(QNetworkRequest::UserAgentHeader, QString("OSMScout-Sailfish %1").arg(OSMSCOUT_SAILFISH_VERSION_STRING));
  //request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
  webCtrl.get(request);
}
  
void OsmTileDownloader::fileDownloaded(QNetworkReply* reply)
{ 
  QUrl url = reply->url();
  if (!requests.contains(url)){
    qWarning() << "Response from non-requested url: " << url;
  }else{
    
    TileCacheKey key = requests.value(url);
    requests.remove(url);
    if (reply->error() != QNetworkReply::NoError){
        // TODO: it seems that this code is affected by https://bugreports.qt.io/browse/QTBUG-46323
        // on Jolla phone (Qt 5.2.2), exists some workaround? Can we copy-paste fixed QNetworkAccessManager to project?
      qWarning() << "Downloading " << url << "failed with " << reply->errorString();
      serverNumber = qrand(); // try another server for future requests
      emit failed(key.zoomLevel, key.xtile, key.ytile, false);
    }else{
      QByteArray downloadedData = reply->readAll();

      QImage image;
      if (image.loadFromData(downloadedData, Q_NULLPTR)){    
        qDebug() << "Downloaded tile " << url << " (current thread: " << QThread::currentThread() << ")";
        emit downloaded(key.zoomLevel, key.xtile, key.ytile, image, downloadedData);
      }else{
        qWarning() << "Failed to load image data from " << url;
        emit failed(key.zoomLevel, key.xtile, key.ytile, false);
      }
    }
  }
  reply->deleteLater();
}
