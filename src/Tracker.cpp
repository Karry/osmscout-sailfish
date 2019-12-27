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

  init();
}

void Tracker::init(){
  emit openTrackRequested();
}

void Tracker::onOpenTrackLoaded(Track track, bool ok){
  if (!ok || track.id < 0){
    return;
  }
  emit openTrackLoaded(QString::number(track.id), track.name);
}

void Tracker::resumeTrack(QString trackId){
  // TODO
}

void Tracker::startTracking(QString collectionId, QString trackName, QString trackDescription){
  // TODO
}

void Tracker::stopTracking(){
  // TODO
}

void Tracker::locationChanged(bool locationValid,
                              double lat, double lon,
                              bool horizontalAccuracyValid, double horizontalAccuracy,
                              bool elevationValid, double elevation){
  // TODO
}