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
#include "QVariantConverters.h"

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

  connect(storage, &Storage::collectionDetailsLoaded,
          this, &Tracker::onCollectionDetailsLoaded,
          Qt::QueuedConnection);

  connect(storage, &Storage::collectionDeleted,
          this, &Tracker::onCollectionDeleted,
          Qt::QueuedConnection);

  connect(storage, &Storage::trackDeleted,
          this, &Tracker::onTrackDeleted,
          Qt::QueuedConnection);

  init();
}

Tracker::~Tracker(){
  if (isTracking() && !batch->empty()){
    flushBatch(false);
  }
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
  emit statisticsUpdated();
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

void Tracker::stopTrackingWithoutSync(){
  batch = std::make_shared<std::vector<osmscout::gpx::TrackPoint>>();
  track.id = -1;
  track.statistics = TrackStatistics{};
  accumulator = TrackStatisticsAccumulator{};
  emit trackingChanged();
  emit statisticsUpdated();
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

void Tracker::locationChanged(const QDateTime &timestamp,
                              bool locationValid,
                              double lat, double lon,
                              bool horizontalAccuracyValid, double horizontalAccuracy,
                              bool elevationValid, double elevation,
                              bool verticalAccuracyValid, double verticalAccuracy){
  if (!isTracking() || !locationValid){
    return;
  }

  using namespace osmscout;
  gpx::TrackPoint point(GeoCoord(lat, lon));
  point.time=converters::dateTimeToTimestampOpt(timestamp); // use time from gps
  assert(point.time.has_value());
  if (elevationValid) {
    point.elevation=elevation;
  }
  if (horizontalAccuracyValid) {
    point.hdop=horizontalAccuracy;
  }
  if (verticalAccuracyValid){
    point.vdop=verticalAccuracy;
  }

  assert(batch);
  Timestamp::duration diffFromLast;
  Timestamp::duration diffFromFirst;
  Distance distanceFromLast;
  if (!batch->empty()){
    assert(batch->back().time.has_value());
    diffFromLast = *(point.time) - *(batch->back().time);
    diffFromFirst = *(point.time) - *(batch->front().time);
    distanceFromLast = GetEllipsoidalDistance(point.coord, batch->back().coord);
    if (diffFromLast < Timestamp::duration::zero()){
      qWarning() << "Clock move to the past by " <<
        std::chrono::duration_cast<std::chrono::seconds>(diffFromLast).count() <<
        " seconds!";
    }
  } else {
    if (accumulator.getTo()){
      diffFromLast = *(point.time) - *(accumulator.getTo());
    }else {
      diffFromLast = Timestamp::duration::zero();
    }
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

void Tracker::onCollectionDetailsLoaded(Collection collection, bool ok) {
  if (ok && isTracking() && collection.tracks){
    for (const auto &t : *(collection.tracks)) {
      if (track.id == t.id) {

        if (track.collectionId != t.collectionId) {
          qDebug() << "Track was moved";
          track.collectionId = t.collectionId;
        }
        if (track.name != t.name) {
          qDebug() << "Track was renamed";
          track.name = t.name;
        }
        if (track.description != t.description) {
          qDebug() << "Track description was changed";
          track.description = t.description;
        }
      }
    }
  }
}

void Tracker::onCollectionDeleted(qint64 collectionId) {
  if (isTracking() && collectionId == track.collectionId){
    qWarning() << "Collection was deleted, stopping tracker";
    stopTrackingWithoutSync();
  }
}

void Tracker::onTrackDeleted(qint64, qint64 trackId) {
  if (isTracking() && trackId == track.id){
    qWarning() << "Track was deleted, stopping tracker";
    stopTrackingWithoutSync();
  }
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
  if (track.statistics.minElevation.has_value())
    return track.statistics.minElevation->AsMeter();
  return -1000000; // JS numeric limits may be different from C++
}

double Tracker::getMaxElevation() const
{
  if (track.statistics.maxElevation.has_value())
    return track.statistics.maxElevation->AsMeter();
  return -1000000; // JS numeric limits may be different from C++
}
