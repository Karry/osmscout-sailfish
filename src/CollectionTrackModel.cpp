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

#include <osmscout/LocationEntry.h>
#include <osmscout/OverlayObject.h>
#include "CollectionTrackModel.h"

using namespace osmscout;

CollectionTrackModel::CollectionTrackModel()
{
  Storage *storage = Storage::getInstance();
  if (storage) {
    connect(storage, &Storage::initialised,
            this, &CollectionTrackModel::storageInitialised,
            Qt::QueuedConnection);

    connect(storage, &Storage::initialisationError,
            this, &CollectionTrackModel::storageInitialisationError,
            Qt::QueuedConnection);

    connect(this, &CollectionTrackModel::trackDataRequest,
            storage, &Storage::loadTrackData,
            Qt::QueuedConnection);

    connect(storage, &Storage::trackDataLoaded,
            this, &CollectionTrackModel::onTrackDataLoaded,
            Qt::QueuedConnection);
  }
}

void CollectionTrackModel::storageInitialised()
{
  if (track.id > 0) {
    loading = true;
    emit trackDataRequest(track);
    emit loadingChanged();
  }
}

void CollectionTrackModel::storageInitialisationError(QString)
{
  storageInitialised();
}

bool CollectionTrackModel::isLoading() const
{
  return loading;
}

QString CollectionTrackModel::getTrackId() const
{
  return QString::number(track.id);
}

void CollectionTrackModel::setTrackId(QString id)
{
  bool ok;
  track.id = id.toLongLong(&ok);
  if (!ok)
    track.id = -1;
  storageInitialised();
}

QString CollectionTrackModel::getCollectionId() const
{
  return QString::number(track.collectionId);
}

QString CollectionTrackModel::getName() const
{
  return track.name;
}

QString CollectionTrackModel::getDescription() const
{
  return track.description;
}

QDateTime CollectionTrackModel::getFrom() const
{
  return track.statistics.from;
}

QDateTime CollectionTrackModel::getTo() const
{
  return track.statistics.to;
}

double CollectionTrackModel::getDistance() const
{
  return track.statistics.distance.AsMeter();
}

double CollectionTrackModel::getRawDistance() const
{
  return track.statistics.rawDistance.AsMeter();
}

qint64 CollectionTrackModel::getDuration() const
{
  return track.statistics.durationMillis();
}

qint64 CollectionTrackModel::getMovingDuration() const
{
  return track.statistics.movingDurationMillis();
}

double CollectionTrackModel::getMaxSpeed() const
{
  return track.statistics.maxSpeed;
}

double CollectionTrackModel::getAverageSpeed() const
{
  return track.statistics.averageSpeed;
}

double CollectionTrackModel::getMovingAverageSpeed() const
{
  return track.statistics.movingAverageSpeed;
}

double CollectionTrackModel::getAscent() const
{
  return track.statistics.ascent.AsMeter();
}

double CollectionTrackModel::getDescent() const
{
  return track.statistics.descent.AsMeter();
}

double CollectionTrackModel::getMinElevation() const
{
  if (track.statistics.minElevation.hasValue())
    return track.statistics.minElevation.get().AsMeter();
  return -1000000; // JS numeric limits may be different from C++
}

double CollectionTrackModel::getMaxElevation() const
{
  if (track.statistics.maxElevation.hasValue())
    return track.statistics.maxElevation.get().AsMeter();
  return -1000000; // JS numeric limits may be different from C++
}

QObject *CollectionTrackModel::getBBox() const
{
  // QML will take ownership
  return new LocationEntry(LocationEntry::Type::typeNone,
                           "bbox",
                           "bbox",
                           QStringList(),
                           "",
                           track.statistics.bbox.GetCenter(),
                           track.statistics.bbox);
}

void CollectionTrackModel::onTrackDataLoaded(Track track, bool complete, bool /*ok*/)
{
  loading = !complete;
  GeoBox originalBox = this->track.statistics.bbox;
  this->track = track;
  if (originalBox.IsValid() != track.statistics.bbox.IsValid() ||
      originalBox.GetMinCoord() != track.statistics.bbox.GetMinCoord() ||
      originalBox.GetMaxCoord() != track.statistics.bbox.GetMaxCoord() ){
    emit bboxChanged();
  }
  emit loadingChanged();
}

int CollectionTrackModel::getSegmentCount() const
{
  return track.data ? track.data->segments.size() : 0;
}

QObject* CollectionTrackModel::createOverlayForSegment(int segment)
{
  if (!track.data)
    return nullptr;
  if (segment < 0 || (size_t)segment >= track.data->segments.size())
    return nullptr;

  gpx::TrackSegment &seg = track.data->segments[segment];
  std::vector<osmscout::Point> points;
  points.reserve(seg.points.size());
  for (auto const &p:seg.points){
    points.emplace_back(0, p.coord);
  }
  return new OverlayWay(points);
}
