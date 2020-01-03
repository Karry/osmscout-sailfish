/*
  OSMScout for SFOS
  Copyright (C) 2019 Lukas Karas

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

#include "Tracker.h"

#include <QDebug>

Tracker::Tracker() {
  Storage *storage = Storage::getInstance();
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &Tracker::init,
          Qt::QueuedConnection);

  connect(storage, &Storage::error,
          this, &Tracker::error,
          Qt::QueuedConnection);

  connect(this, &Tracker::openTrackRequested,
          storage, &Storage::loadRecentOpenTrack,
          Qt::QueuedConnection);

  connect(storage, &Storage::openTrackLoaded,
          this, &Tracker::onOpenTrackLoaded,
          Qt::QueuedConnection);

  connect(storage, &Storage::trackCreated,
          this, &Tracker::onTrackCreated,
          Qt::QueuedConnection);

  init();
}

void Tracker::init(){
  emit openTrackRequested();
}

void Tracker::onOpenTrackLoaded(Track track, bool ok){
  if (!ok || track.id < 0){
    return;
  }

  recentOpenTrack = track;
  emit openTrackLoaded(QString::number(track.id), track.name);
}

void Tracker::resumeTrack(QString trackId){
  bool ok;
  qint64 trkId = trackId.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << trackId << "to number";
    return;
  }

  if (isTracking()){
    qWarning() << "Tracking already, track id: " << track.id;
    return;
  }
  if (creationRequested){
    qWarning() << "Tracking requested already";
    return;
  }
  if (trkId != recentOpenTrack.id){
    qWarning() << "Track " << trkId << " was not reported as recent open!";
    return;
  }

  track = recentOpenTrack;
  emit trackingChanged();
}

void Tracker::startTracking(QString collectionId, QString trackName, QString trackDescription){

  bool ok;
  qint64 collId = collectionId.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << collectionId << "to number";
    return;
  }

  if (isTracking()){
    qWarning() << "Tracking already, track id: " << track.id;
    return;
  }
  if (creationRequested){
    qWarning() << "Tracking requested already";
    return;
  }

  track.id = -1;
  track.collectionId = collId;
  track.name = trackName;
  creationRequested = true;

  emit createTrackRequest(collId, trackName, trackDescription, true);
}

void Tracker::stopTracking(){
  if (!isTracking()){
    qWarning() << "Tracking is not active";
    return;
  }
  // TODO: close track

  track.id = -1;
  track.statistics = TrackStatistics{};
  emit trackingChanged();
}

void Tracker::locationChanged(bool locationValid,
                              double lat, double lon,
                              bool horizontalAccuracyValid, double horizontalAccuracy,
                              bool elevationValid, double elevation){
  if (!isTracking()){
    return;
  }

  // TODO:
  // - update track statistics
  // - enqueue point to batch
  // - possibly append point batch to track (with flag for creating new segment)
}

void Tracker::onTrackCreated(qint64 collectionId, qint64 trackId, QString name){
  if (!creationRequested || isTracking() || collectionId != track.collectionId || name != track.name){
    return;
  }
  creationRequested = false;
  track.id = trackId;
  emit trackingChanged();
}

