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

    connect(this, SIGNAL(trackDataRequest(Track)),
            storage, SLOT(loadTrackData(Track)),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(trackDataLoaded(Track, bool, bool)),
            this, SLOT(onTrackDataLoaded(Track, bool, bool)),
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

  qDebug() << "Display collection" << collection.name << "(" << collection.id << ")";

  QMap<qint64, QDateTime> wptToHide = displayedWaypoints[collection.id];
  QMap<qint64, QDateTime> &wptVisible = displayedWaypoints[collection.id];

  QMap<qint64, QDateTime> trkToHide = displayedTracks[collection.id];
  QMap<qint64, QDateTime> &trkVisible = displayedTracks[collection.id];

  if (collection.tracks){
    for (const auto &trk: *(collection.tracks)){
      trkToHide.remove(trk.id);
      if (!trkVisible.contains(trk.id) || trkVisible[trk.id] != trk.lastModification) {
        qDebug() << "Request track data (" << trk.id << ")"
                 << trkVisible[trk.id] << "/" << trk.lastModification;
        emit trackDataRequest(trk);
      }
    }
  }

  if (collection.waypoints){
    for (const auto &wpt: *(collection.waypoints)){
      wptToHide.remove(wpt.id);
      if (!wptVisible.contains(wpt.id) || wptVisible[wpt.id] != wpt.lastModification) {
        qDebug() << "Adding overlay waypoint"
                 << QString::fromStdString(wpt.data.name.getOrElse("<empty>"))
                 << "(" << wpt.id << ")"
                 << wptVisible[wpt.id] << "/" << wpt.lastModification;

        osmscout::OverlayNode wptOverlay;
        wptOverlay.setTypeName(waypointTypeName);
        wptOverlay.addPoint(wpt.data.coord.GetLat(), wpt.data.coord.GetLon());
        wptOverlay.setName(QString::fromStdString(wpt.data.name.getOrElse("")));
        delegatedMap->addOverlayObject(overlayWptIdBase + wpt.id, &wptOverlay);
      }

      wptVisible.insert(wpt.id, wpt.lastModification);
    }
  }

  for (const auto &id :wptToHide.keys()){
    qDebug() << "Removing overlay waypoint" << id << wptToHide[id];
    delegatedMap->removeOverlayObject(id + overlayWptIdBase);
    wptVisible.remove(id);
  }
  for (const auto &id :trkToHide.keys()){
    qDebug() << "Removing overlay track" << id << trkToHide[id];
    delegatedMap->removeOverlayObject(id + overlayTrkIdBase);
    trkVisible.remove(id);
  }
}

void CollectionMapBridge::onTrackDataLoaded(Track track, bool complete, bool ok)
{
  if (delegatedMap == nullptr ||
      !complete ||
      !ok ||
      !displayedTracks.contains(track.collectionId) ||
      displayedTracks[track.collectionId][track.id] == track.lastModification
      ){
    return;
  }

  qDebug() << "Adding overlay track"
           << track.name
           << "(" << track.id << ")"
           << displayedTracks[track.collectionId][track.id] << "/" << track.lastModification;

  for (const osmscout::gpx::TrackSegment &seg : track.data->segments) {
    std::vector<osmscout::Point> points;
    points.reserve(seg.points.size());
    for (auto const &p:seg.points) {
      points.emplace_back(0, p.coord);
    }
    osmscout::OverlayWay trkOverlay(points);
    trkOverlay.setTypeName(trackTypeName);
    trkOverlay.setName(track.name);
    delegatedMap->addOverlayObject(overlayTrkIdBase + track.id, &trkOverlay);
  }
  displayedTracks[track.collectionId][track.id] = track.lastModification;
}

void CollectionMapBridge::onCollectionsLoaded(std::vector<Collection> collections, bool /*ok*/)
{
  qDebug() << "Loaded" << collections.size() << "collections";

  // clear deleted collections on map
  if (delegatedMap == nullptr) {
    displayedTracks.clear();
    displayedWaypoints.clear();
  }else{
    QMap<qint64, QMap<qint64, QDateTime>> tracksToHide = displayedTracks;
    QMap<qint64, QMap<qint64, QDateTime>> waypointsToHide = displayedWaypoints;

    for (const auto &c: collections){
      if (c.visible){
        tracksToHide.remove(c.id);
        waypointsToHide.remove(c.id);
        collectionDetailRequest(c);
      }
    }

    for (const auto &colId: waypointsToHide.keys()) {
      for (const auto &wptId: displayedWaypoints.take(colId).keys()) {
        delegatedMap->removeOverlayObject(overlayWptIdBase + wptId);
      }
    }
    for (const auto &colId: tracksToHide.keys()) {
      for (const auto &trkId: displayedTracks.take(colId).keys()) {
        delegatedMap->removeOverlayObject(overlayTrkIdBase + trkId);
      }
    }
  }
}

void CollectionMapBridge::setMap(QObject *map)
{
  delegatedMap = dynamic_cast<osmscout::MapWidget*>(map);
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