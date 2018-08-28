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
    connect(storage, SIGNAL(initialised()),
            this, SLOT(storageInitialised()),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(initialisationError(QString)),
            this, SLOT(storageInitialisationError(QString)),
            Qt::QueuedConnection);

    connect(this, SIGNAL(trackDataRequest(Track)),
            storage, SLOT(loadTrackData(Track)),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(trackDataLoaded(Track, bool, bool)),
            this, SLOT(onTrackDataLoaded(Track, bool, bool)),
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

QString CollectionTrackModel::getName() const
{
  return track.name;
}

QString CollectionTrackModel::getDescription() const
{
  return track.description;
}

double CollectionTrackModel::getDistance() const
{
  return track.statistics.distance.AsMeter();
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
