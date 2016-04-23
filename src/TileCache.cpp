
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

#include "TileCache.h"


uint qHash(const TileCacheKey &key){
  return (key.zoomLevel << 24) ^ (key.xtile << 12) ^ key.ytile;
}

bool operator==(const TileCacheKey a, const TileCacheKey b)
{
    return a.zoomLevel == b.zoomLevel && a.xtile == b.xtile && a.ytile == b.ytile;
}

TileCache::TileCache(): 
  mutex(),
  tiles(), 
  requests(),
  cacheSize(200), 
  maximumLivetimeMs(20000)
{
}

TileCache::~TileCache() {
}

bool TileCache::request(uint32_t zoomLevel, uint32_t x, uint32_t y){
  TileCacheKey key = {zoomLevel, x, y};
  if (requests.contains(key))
    return false;
  
  requests << key;
  return true;
}

bool TileCache::contains(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
  TileCacheKey key = {zoomLevel, x, y};
  return tiles.contains(key);
}

QImage TileCache::get(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
  TileCacheKey key = {zoomLevel, x, y};
  if (!tiles.contains(key)){
    qWarning() << "No tile in cache for key {" << zoomLevel << ", " << x << ", " << y << "}";
    return QImage(); // throw std::underflow_error ?
  }
  TileCacheVal val = tiles.value(key);
  val.lastAccess.start();
  return val.image;
}

void TileCache::removeRequest(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
  TileCacheKey key = {zoomLevel, x, y};
  requests.remove(key);
}

void TileCache::put(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image)
{
  removeRequest(zoomLevel, x, y);
  TileCacheKey key = {zoomLevel, x, y};
  QTime now;
  now.start();
  TileCacheVal val = {now, image};
  tiles.insert(key, val);
  
  if (tiles.size() > (int)cacheSize){
    /** 
     * maximum size reached
     * 
     * first, we will iterate over all entries and remove up to 10% tiles 
     * older than `maximumLivetimeMs`, if no such entry found, remove oldest
     */
    qDebug() << "Cleaning tile cache (" << cacheSize << ")";
    uint32_t removed = 0;
    int oldest = 0;
    TileCacheKey oldestKey = key;
    
    QMutableHashIterator<TileCacheKey, TileCacheVal> it(tiles);
    while (it.hasNext() && removed < (cacheSize / 10)){
      it.next();
      
    //QHash<TileCacheKey, TileCacheVal>::const_iterator it = tiles.constBegin();
    //while (it != tiles.constEnd() && removed < (cacheSize / 10)){

      key = it.key();
      TileCacheVal val = it.value();

      int elapsed = val.lastAccess.elapsed();
      if (elapsed > oldest){
        oldest = elapsed;
        oldestKey = key;
      }
        
      if (elapsed > (int)maximumLivetimeMs){
        qDebug() << "  removing " << key.zoomLevel << " / " << key.xtile << " x " << key.ytile;
        
        //tiles.remove(key);
        it.remove();
        
        removed ++;
      }
      //++it;
    }
    if (removed == 0){
      key = oldestKey;
      qDebug() << "  removing " << key.zoomLevel << " / " << key.xtile << " x " << key.ytile;
      tiles.remove(key);
    }
  }
}
