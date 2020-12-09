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
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &CollectionMapBridge::init,
          Qt::QueuedConnection);

  connect(storage, &Storage::initialisationError,
          this, &CollectionMapBridge::storageInitialisationError,
          Qt::QueuedConnection);

  connect(this, &CollectionMapBridge::collectionLoadRequest,
          storage, &Storage::loadCollections,
          Qt::QueuedConnection);

  connect(storage, &Storage::collectionsLoaded,
          this, &CollectionMapBridge::onCollectionsLoaded,
          Qt::QueuedConnection);

  connect(storage, &Storage::error,
          this, &CollectionMapBridge::error,
          Qt::QueuedConnection);

  connect(this, &CollectionMapBridge::collectionDetailRequest,
          storage, &Storage::loadCollectionDetails,
          Qt::QueuedConnection);

  connect(storage, &Storage::collectionDetailsLoaded,
          this, &CollectionMapBridge::onCollectionDetailsLoaded,
          Qt::QueuedConnection);

  connect(this, &CollectionMapBridge::trackDataRequest,
          storage, &Storage::loadTrackData,
          Qt::QueuedConnection);

  connect(storage, &Storage::trackDataLoaded,
          this, &CollectionMapBridge::onTrackDataLoaded,
          Qt::QueuedConnection);

  init();
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
  using namespace std::string_literals;
  if (!collection.visible || delegatedMap == nullptr){
    return;
  }

  qDebug() << "Display collection" << collection.name << "(" << collection.id << ") on map" << delegatedMap;

  DisplayedCollection &dispColl = displayedCollection[collection.id];

  QMap<qint64, DisplayedWaypoint> wptToHide = dispColl.waypoints;
  QMap<qint64, DisplayedWaypoint> &wptVisible = dispColl.waypoints;

  QMap<qint64, DisplayedTrack> trkToHide = dispColl.tracks;
  QMap<qint64, DisplayedTrack> &trkVisible = dispColl.tracks;

  if (collection.tracks){
    for (const auto &trk: *(collection.tracks)){
      trkToHide.remove(trk.id);
      if (!trkVisible.contains(trk.id) || trkVisible[trk.id].lastModification != trk.lastModification) {
        qDebug() << "Request track data (" << trk.id << ")"
                 << trkVisible[trk.id].lastModification << "/" << trk.lastModification;
        emit trackDataRequest(trk, std::nullopt);
      }
    }
  }

  if (collection.waypoints){
    for (const auto &wpt: *(collection.waypoints)){
      wptToHide.remove(wpt.id);
      if (!wptVisible.contains(wpt.id) || wptVisible[wpt.id].lastModification != wpt.lastModification) {
        if (!wptVisible.contains(wpt.id)){
          wptVisible[wpt.id]=DisplayedWaypoint{wpt.lastModification, nextObjectId++};
        }else{
          wptVisible[wpt.id].lastModification=wpt.lastModification;
        }
        qDebug() << "Adding overlay waypoint"
                 << QString::fromStdString(wpt.data.name.value_or("<empty>"s))
                 << "(" << wpt.id << ")"
                 << wpt.lastModification;

        osmscout::OverlayNode wptOverlay;
        wptOverlay.setTypeName(waypointTypeName);
        wptOverlay.addPoint(wpt.data.coord.GetLat(), wpt.data.coord.GetLon());
        wptOverlay.setName(QString::fromStdString(wpt.data.name.value_or(""s)));
        delegatedMap->addOverlayObject(wptVisible[wpt.id].id, &wptOverlay);
      }
    }
  }

  for (const auto &id :wptToHide.keys()){
    qDebug() << "Removing overlay waypoint" << id << wptToHide[id].lastModification;
    delegatedMap->removeOverlayObject(wptToHide[id].id);
    wptVisible.remove(id);
  }
  for (const auto &id :trkToHide.keys()){
    qDebug() << "Removing overlay track" << id << trkToHide[id].lastModification;
    for (const auto &did: trkToHide[id].ids) {
      delegatedMap->removeOverlayObject(did);
    }
    trkVisible.remove(id);
  }
}

void CollectionMapBridge::onTrackDataLoaded(Track track, std::optional<double> accuracyFilter, bool complete, bool ok)
{
  if (delegatedMap == nullptr ||
      accuracyFilter != std::nullopt ||
      !complete ||
      !ok ||
      !displayedCollection.contains(track.collectionId) ||
      displayedCollection[track.collectionId].tracks[track.id].lastModification == track.lastModification
      ){
    return;
  }

  qDebug() << "Adding overlay track"
           << track.name
           << "(" << track.id << ")"
           << displayedCollection[track.collectionId].tracks[track.id].lastModification << "/" << track.lastModification
           << "to map" << delegatedMap;

  std::vector<qint64> ids;
  // if track is displayed already...
  if (displayedCollection.contains(track.collectionId) &&
      displayedCollection[track.collectionId].tracks.contains(track.id)){
    ids = displayedCollection[track.collectionId].tracks[track.id].ids;
  }
  if (ids.size() < track.data->segments.size()) {
    // generate ids for new segments
    ids.reserve(track.data->segments.size());
    while (ids.size() < track.data->segments.size()) {
      ids.push_back(nextObjectId++);
    }
  }
  if (ids.size() > track.data->segments.size()) {
    // hide segments from tail
    for (size_t i=track.data->segments.size(); i < ids.size(); i++){
      delegatedMap->removeOverlayObject(ids[i]);
    }
    ids.resize(track.data->segments.size());
  }

  assert(ids.size() == track.data->segments.size());
  for (size_t i=0; i < track.data->segments.size(); i++) {
    const osmscout::gpx::TrackSegment &seg = track.data->segments[i];
    std::vector<osmscout::Point> points;
    points.reserve(seg.points.size());
    for (auto const &p:seg.points) {
      points.emplace_back(0, p.coord);
    }
    osmscout::OverlayWay trkOverlay(points);
    trkOverlay.setTypeName(trackTypeName);
    trkOverlay.setName(track.name);
    delegatedMap->addOverlayObject(ids[i],&trkOverlay);
  }
  displayedCollection[track.collectionId].tracks[track.id]=DisplayedTrack{
    track.lastModification,
    ids
  };
}

void CollectionMapBridge::onCollectionsLoaded(std::vector<Collection> collections, bool /*ok*/)
{
  qDebug() << "Loaded" << collections.size() << "collections for map" << delegatedMap;

  // clear deleted collections on map
  if (delegatedMap == nullptr) {
    displayedCollection.clear();
  }else{
    QMap<qint64, DisplayedCollection> collectionToHide = displayedCollection;

    for (const auto &c: collections){
      if (c.visible){
        collectionToHide.remove(c.id);
        collectionDetailRequest(c);
      }
    }

    for (const auto &colId: collectionToHide.keys()) {
      DisplayedCollection col=displayedCollection.take(colId);
      for (const auto &wpt: col.waypoints) {
        delegatedMap->removeOverlayObject(wpt.id);
      }
      for (const auto &trk:col.tracks){
        for (const auto &id:trk.ids){
          delegatedMap->removeOverlayObject(id);
        }
      }
    }
  }
}

void CollectionMapBridge::setMap(QObject *map)
{
  delegatedMap = qobject_cast<osmscout::MapWidget*>(map);
  if (delegatedMap == nullptr){
    return;
  }
  qDebug() << "CollectionMapBridge map:" << delegatedMap;
  init();
}

void CollectionMapBridge::setWaypointType(QString name)
{
  waypointTypeName = name;
  init();
}

void CollectionMapBridge::setTrackType(QString type)
{
  trackTypeName = type;
  init();
}
