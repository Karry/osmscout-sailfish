
#ifndef TILECACHE_H
#define	TILECACHE_H

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


#include <QObject>
#include <QString>
#include <QMetaType>
#include <QMutex>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QTime>
#include <QImage>

struct TileCacheKey
{
  uint32_t zoomLevel;
  uint32_t xtile; 
  uint32_t ytile;
  
};

bool operator==(const TileCacheKey a, const TileCacheKey b);

uint qHash(const TileCacheKey &key);

Q_DECLARE_METATYPE(TileCacheKey)

struct TileCacheVal
{
  QTime lastAccess;
  QImage image; 
};

Q_DECLARE_METATYPE(TileCacheVal)

/**
 * Cache have to be locked by its mutex() while access.
 * It owns all inserted tiles and it is responsible for its release
 */
class TileCache : public QObject
{
  Q_OBJECT
  
public:
  TileCache();
  virtual ~TileCache();
  
  /**
   * insert new tile request record, return false if this request exists already
   */
  bool request(uint32_t zoomLevel, uint32_t x, uint32_t y);
  bool contains(uint32_t zoomLevel, uint32_t x, uint32_t y);
  QImage get(uint32_t zoomLevel, uint32_t x, uint32_t y);
  
  void removeRequest(uint32_t zoomLevel, uint32_t x, uint32_t y);
  void put(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image);
  
  mutable QMutex                    mutex;
private:
  QHash<TileCacheKey, TileCacheVal> tiles;
  QSet<TileCacheKey>                requests;
  size_t                            cacheSize; // maximum count of elements in cache
  uint32_t                          maximumLivetimeMs;
};

#endif	/* TILECACHE_H */

