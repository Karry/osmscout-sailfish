/*
  OSMScout for SFOS
  Copyright (C) 2018 Lukas Karas

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

#include "CollectionMapBridge.h"

CollectionMapBridge::CollectionMapBridge(QObject *parent):
  QObject(parent)
{
  Storage *storage = Storage::getInstance();
  if (storage) {
    connect(storage, SIGNAL(initialised()),
            this, SLOT(init()),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(initialisationError(QString)),
            this, SLOT(storageInitialisationError(QString)),
            Qt::QueuedConnection);

    connect(this, SIGNAL(collectionLoadRequest()),
            storage, SLOT(loadCollections()),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(collectionsLoaded(std::vector<Collection>, bool)),
            this, SLOT(onCollectionsLoaded(std::vector<Collection>, bool)),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(error(QString)),
            this, SIGNAL(error(QString)),
            Qt::QueuedConnection);

    connect(this, SIGNAL(collectionDetailRequest(Collection)),
            storage, SLOT(loadCollectionDetails(Collection)),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(collectionDetailsLoaded(Collection, bool)),
            this, SLOT(onCollectionDetailsLoaded(Collection, bool)),
            Qt::QueuedConnection);

    init();
  }
}

void CollectionMapBridge::init()
{
  if (delegatedMap == nullptr){
    return;
  }

  emit collectionLoadRequest();
}

void CollectionMapBridge::storageInitialisationError(QString e)
{
  emit error(e);
}

void CollectionMapBridge::onCollectionDetailsLoaded(Collection collection, bool /*ok*/)
{
  if (!collection.visible || delegatedMap == nullptr){
    return;
  }

  if (collection.tracks){
    for (const auto &trk: *(collection.tracks)){
      trackDataRequest(trk);
    }
  }

  if (collection.waypoints){
    for (const auto &wpt: *(collection.waypoints)){

      osmscout::OverlayNode wptOverlay;
      wptOverlay.setTypeName(waypointTypeName);
      wptOverlay.addPoint(wpt.data.coord.GetLat(), wpt.data.coord.GetLon());
      wptOverlay.setName(QString::fromStdString(wpt.data.name.getOrElse("")));
      delegatedMap->addOverlayObject(overlayIdBase + wpt.id, &wptOverlay);
    }
  }
}

void CollectionMapBridge::onTrackDataLoaded(Track track, bool complete, bool ok)
{
  // TODO: display track
}

void CollectionMapBridge::onCollectionsLoaded(std::vector<Collection> collections, bool /*ok*/)
{
  qDebug() << "loaded" << collections.size() << "collections";
  for (const auto &c: collections){
    if (c.visible){
      collectionDetailRequest(c);
    }
  }
}

void CollectionMapBridge::setMap(QObject *map)
{
  delegatedMap = dynamic_cast<osmscout::MapWidget*>(map);
  if (delegatedMap == nullptr){
    return;
  }
  qDebug() << "map:" << delegatedMap;
  init();
}

void CollectionMapBridge::setWaypointType(QString name)
{
  waypointTypeName = name;
  init();
}