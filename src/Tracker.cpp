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
#include <osmscout/util/Geometry.h>

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

  connect(this, &Tracker::closeTrackRequest,
          storage, &Storage::closeTrack,
          Qt::QueuedConnection);

  connect(this, &Tracker::createTrackRequest,
          storage, &Storage::createTrack,
          Qt::QueuedConnection);

  connect(this, &Tracker::appendNodesRequest,
          storage, &Storage::appendNodes,
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
  // update accumulator by current track statistics
  accumulator = TrackStatisticsAccumulator(track.statistics);
  emit trackingChanged();
}

void Tracker::closeOpen(QString trackId){
  bool ok;
  qint64 trkId = trackId.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << trackId << "to number";
    return;
  }

  if (trkId != recentOpenTrack.id){
    qWarning() << "Track " << trkId << " was not reported as recent open!";
    return;
  }
  emit closeTrackRequest(recentOpenTrack.collectionId, recentOpenTrack.id);
  recentOpenTrack=Track{};
  emit openTrackLoaded("-1", "");
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

  emit trackingChanged();

  emit createTrackRequest(collId, trackName, trackDescription, true);
}

void Tracker::flushBatch(bool createNewSegment){
  emit appendNodesRequest(track.id,
                          batch,
                          track.statistics,
                          createNewSegment);

  batch = std::make_shared<std::vector<osmscout::gpx::TrackPoint>>();
}

void Tracker::stopTracking(){
  if (!isTracking()){
    qWarning() << "Tracking is not active";
    return;
  }

  flushBatch(false);
  emit closeTrackRequest(track.collectionId, track.id);

  track.id = -1;
  track.statistics = TrackStatistics{};
  accumulator = TrackStatisticsAccumulator{};
  emit trackingChanged();
}

void Tracker::locationChanged(bool locationValid,
                              double lat, double lon,
                              bool horizontalAccuracyValid, double horizontalAccuracy,
                              bool elevationValid, double elevation,
                              bool verticalAccuracyValid, double verticalAccuracy){
  if (!isTracking()){
    return;
  }

  using namespace osmscout;
  gpx::TrackPoint point(GeoCoord(lat, lon));
  point.time=gpx::Optional<Timestamp>::of(Timestamp::clock::now());
  if (elevationValid) {
    point.elevation=gpx::Optional<double>::of(elevation);
  }
  if (horizontalAccuracyValid) {
    point.hdop=gpx::Optional<double>::of(horizontalAccuracy);
  }
  if (verticalAccuracyValid){
    point.vdop=gpx::Optional<double>::of(verticalAccuracy);
  }

  assert(batch);
  Timestamp::duration diffFromLast;
  Timestamp::duration diffFromFirst;
  Distance distanceFromLast;
  if (!batch->empty()){
    assert(batch->back().time.hasValue());
    assert(point.time.hasValue());
    diffFromLast = point.time.get() - batch->back().time.get();
    diffFromFirst = point.time.get() - batch->front().time.get();
    distanceFromLast = GetEllipsoidalDistance(point.coord, batch->back().coord);
    if (diffFromLast < Timestamp::duration::zero()){
      qWarning() << "Clock move to the past by " <<
        std::chrono::duration_cast<std::chrono::seconds>(diffFromLast).count() <<
        " seconds!";
    }
  } else {
    diffFromLast = Timestamp::duration::zero();
    diffFromFirst = Timestamp::duration::zero();
  }

  // create new segment when:
  bool closeSegment = diffFromLast > std::chrono::minutes(10) // big time gap
                   || diffFromLast < Timestamp::duration::zero() // time shift
                   || distanceFromLast > Kilometers(1); // big distance gap

  if (closeSegment || batch->size() > 100 || diffFromFirst > std::chrono::minutes(1)) {
    // append point batch to track (with flag for creating new segment)
    flushBatch(closeSegment);
  }

  // update track statistics
  if (closeSegment){
    accumulator.segmentEnd();
  }
  accumulator.update(point);
  track.statistics = accumulator.accumulate();
  emit statisticsUpdated();

  assert(batch);
  batch->push_back(point);
}

void Tracker::onTrackCreated(qint64 collectionId, qint64 trackId, QString name){
  if (!creationRequested || isTracking() || collectionId != track.collectionId || name != track.name){
    return;
  }
  creationRequested = false;
  track.id = trackId;
  emit trackingChanged();
}

QString Tracker::getName() const {
  return track.name;
}

QString Tracker::getDescription() const {
  return track.description;
}

QDateTime Tracker::getFrom() const
{
  return track.statistics.from;
}

QDateTime Tracker::getTo() const
{
  return track.statistics.to;
}

double Tracker::getDistance() const
{
  return track.statistics.distance.AsMeter();
}

double Tracker::getRawDistance() const
{
  return track.statistics.rawDistance.AsMeter();
}

qint64 Tracker::getDuration() const
{
  return track.statistics.durationMillis();
}

qint64 Tracker::getMovingDuration() const
{
  return track.statistics.movingDurationMillis();
}

double Tracker::getMaxSpeed() const
{
  return track.statistics.maxSpeed;
}

double Tracker::getAverageSpeed() const
{
  return track.statistics.averageSpeed;
}

double Tracker::getMovingAverageSpeed() const
{
  return track.statistics.movingAverageSpeed;
}

double Tracker::getAscent() const
{
  return track.statistics.ascent.AsMeter();
}

double Tracker::getDescent() const
{
  return track.statistics.descent.AsMeter();
}

double Tracker::getMinElevation() const
{
  if (track.statistics.minElevation.hasValue())
    return track.statistics.minElevation.get().AsMeter();
  return -1000000; // JS numeric limits may be different from C++
}

double Tracker::getMaxElevation() const
{
  if (track.statistics.maxElevation.hasValue())
    return track.statistics.maxElevation.get().AsMeter();
  return -1000000; // JS numeric limits may be different from C++
}
