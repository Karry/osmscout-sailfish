
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

#include <iostream>

#include "TileCache.h"
#include "OSMTile.h"

using namespace std;

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
  cacheSize(100), 
  maximumLivetimeMs(5 * 60 * 1000)
{
}

TileCache::~TileCache() 
{
}

void TileCache::clearPendingRequests()
{        
    QMutableHashIterator<TileCacheKey, RequestState> it(requests);
    while (it.hasNext()){
      it.next();
      if (it.value().pending){
          // qDebug() << "remove pending " << QString("z: %1, %2x%3").arg(it.key().zoomLevel).arg(it.key().xtile).arg(it.key().ytile);                
          it.remove();
      }
    }
}

bool TileCache::startRequestProcess(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    if (requests.contains(key)){        
        // qDebug() << "start process " << QString("z: %1, %2x%3").arg(key.zoomLevel).arg(key.xtile).arg(key.ytile);
        RequestState state = requests.value(key); 
        if (state.pending){
            state.pending = false;
            requests.insert(key, state);
            return true;
        }else{
            return false; // started already
        }
    }else{
        return false;
    }
}

bool TileCache::request(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    if (requests.contains(key))
        return false;

    // qDebug() << "request " << QString("z: %1, %2x%3").arg(key.zoomLevel).arg(key.xtile).arg(key.ytile);
    RequestState state = {true};
    requests.insert(key, state);
    return true;
}

bool TileCache::containsRequest(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    return requests.contains(key);
}

bool TileCache::contains(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    return tiles.contains(key);
}

TileCacheVal TileCache::get(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    if (!tiles.contains(key)){
        qWarning() << "No tile in cache for key {" << zoomLevel << ", " << x << ", " << y << "}";
        return {QTime(), QImage(), true}; // throw std::underflow_error ?
    }
    TileCacheVal val = tiles.value(key);
    val.lastAccess.start();
    tiles.insert(key, val);
    return val;
}

void TileCache::removeRequest(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    requests.remove(key);    
    // qDebug() << "remove " << QString("z: %1, %2x%3").arg(key.zoomLevel).arg(key.xtile).arg(key.ytile);
}

void TileCache::put(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image)
{
    removeRequest(zoomLevel, x, y);
    TileCacheKey key = {zoomLevel, x, y};
    QTime now;
    now.start();
    TileCacheVal val = {now, image, false};
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

bool TileCache::invalidate(osmscout::GeoBox box){
    QMutableHashIterator<TileCacheKey, TileCacheVal> it(tiles);
    bool removed = false;
    TileCacheKey key;
    osmscout::GeoBox bbox;
    while (it.hasNext() ){            
        it.next();
        key = it.key();
        bbox = OSMTile::tileBoundingBox(key.zoomLevel, key.xtile, key.ytile);
        if (box.Intersects(bbox)){
            it.remove();
            removed = true;
        }
    }
    return removed;
}

