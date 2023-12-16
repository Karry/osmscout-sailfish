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

#include "CollectionTrackModel.h"

#include <osmscoutclientqt/LocationEntry.h>
#include <osmscoutclientqt/OverlayObject.h>

#include <QDebug>

using namespace osmscout;

CollectionTrackModel::CollectionTrackModel()
{
  Storage *storage = Storage::getInstance();
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &CollectionTrackModel::storageInitialised,
          Qt::QueuedConnection);

  connect(storage, &Storage::initialisationError,
          this, &CollectionTrackModel::storageInitialisationError,
          Qt::QueuedConnection);

  connect(this, &CollectionTrackModel::trackDataRequest,
          storage, &Storage::loadTrackData,
          Qt::QueuedConnection);

  connect(this, &CollectionTrackModel::cropStartRequest,
          storage, &Storage::cropTrackStart,
          Qt::QueuedConnection);

  connect(this, &CollectionTrackModel::cropEndRequest,
          storage, &Storage::cropTrackEnd,
          Qt::QueuedConnection);

  connect(this, &CollectionTrackModel::splitRequest,
          storage, &Storage::splitTrack,
          Qt::QueuedConnection);

  connect(this, &CollectionTrackModel::filterNodesRequest,
          storage, &Storage::filterTrackNodes,
          Qt::QueuedConnection);

  connect(storage, &Storage::trackDataLoaded,
          this, &CollectionTrackModel::onTrackDataLoaded,
          Qt::QueuedConnection);

  connect(this, &CollectionTrackModel::setColorRequest,
          storage, &Storage::setTrackColor,
          Qt::QueuedConnection);
}

void CollectionTrackModel::storageInitialised()
{
  if (track.id > 0) {
    loading = true;
    emit trackDataRequest(track, accuracyFilter);
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

QString CollectionTrackModel::getType() const
{
  return track.type;
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
  if (track.statistics.minElevation.has_value())
    return track.statistics.minElevation->AsMeter();
  return -1000000; // JS numeric limits may be different from C++
}

double CollectionTrackModel::getMaxElevation() const
{
  if (track.statistics.maxElevation.has_value())
    return track.statistics.maxElevation->AsMeter();
  return -1000000; // JS numeric limits may be different from C++
}

double CollectionTrackModel::getAccuracyFilter() const
{
  if (accuracyFilter) {
    return *accuracyFilter;
  }
  return -1;
}

void CollectionTrackModel::setAccuracyFilter(double accuracyFilterDouble)
{
  std::optional<double> accuracyFilterOpt = accuracyFilterDouble <= 0 ?
    std::nullopt : std::make_optional(accuracyFilterDouble);

  if (accuracyFilterOpt==this->accuracyFilter){
    return;
  }

  this->accuracyFilter=accuracyFilterOpt;
  storageInitialised();
}

QObject *CollectionTrackModel::getBBox() const
{
  if (!track.statistics.bbox.IsValid()){
    qWarning() << "Track bounding box is not valid";
  }

  // QML will take ownership
  return new LocationEntry(LocationInfo::Type::typeNone,
                           "bbox",
                           "",
                           "bbox",
                           QList<AdminRegionInfoRef>(),
                           "",
                           track.statistics.bbox.GetCenter(),
                           track.statistics.bbox);
}

void CollectionTrackModel::onTrackDataLoaded(Track track, std::optional<double> accuracyFilter, bool complete, bool /*ok*/)
{
  if (track.id != this->track.id || accuracyFilter != this->accuracyFilter){
    return;
  }
  // TODO: error handling when !ok
  loading = !complete;
  if (complete) {
    GeoBox originalBox = this->track.statistics.bbox;
    this->track = track;
    if (originalBox.IsValid() != track.statistics.bbox.IsValid() ||
        originalBox.GetMinCoord() != track.statistics.bbox.GetMinCoord() ||
        originalBox.GetMaxCoord() != track.statistics.bbox.GetMaxCoord()) {
      emit bboxChanged();
    }
  }
  emit loadingChanged();
}

int CollectionTrackModel::getSegmentCount() const
{
  return track.data ? track.data->segments.size() : 0;
}

quint64 CollectionTrackModel::getPointCount() const
{
  if (!track.data){
    return 0;
  }
  return std::accumulate(track.data->segments.begin(), track.data->segments.end(), (quint64)0,
                         [](qint64 size, const osmscout::gpx::TrackSegment& segment){ return size + segment.points.size(); });
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
  auto trkOverlay = new OverlayWay(points);
  if (track.color.has_value()) {
    trkOverlay->setColorValue(track.color.value());
  }
  return trkOverlay;
}

QPointF CollectionTrackModel::getPoint(quint64 index) const
{
  if (!track.data)
    return QPointF();

  for (auto const &seg:track.data->segments){
    if (index>=seg.points.size()){
      index-=seg.points.size();
    } else {
      const osmscout::GeoCoord &coord=seg.points[index].coord;
      return QPointF(coord.GetLat(), coord.GetLon());
    }
  }
  return QPointF();
}

void CollectionTrackModel::cropStart(quint64 position)
{
  if (track.id < 0){
    return;
  }
  loading = true;
  emit cropStartRequest(track, position);
  emit loadingChanged();
}

void CollectionTrackModel::cropEnd(quint64 position)
{
  if (track.id < 0){
    return;
  }
  loading = true;
  emit cropEndRequest(track, position);
  emit loadingChanged();
}

void CollectionTrackModel::split(quint64 position)
{
  if (track.id < 0){
    return;
  }
  loading = true;
  emit splitRequest(track, position);
  emit loadingChanged();
}

void CollectionTrackModel::filterNodes(double accuracyFilter)
{
  if (track.id < 0){
    return;
  }
  if (accuracyFilter <= 0){
    return;
  }
  loading = true;
  emit filterNodesRequest(track, accuracyFilter);
  emit loadingChanged();
}

void CollectionTrackModel::setupColor(const QString &color)
{
  if (track.id < 0){
    return;
  }
  std::optional<osmscout::Color> colorValue;
  if (!color.isEmpty()){
    Color cv;
    std::string str(color.toLower().toStdString());
    if (Color::FromHexString(str, cv) ||
        Color::FromW3CKeywordString(str, cv)) {
      colorValue = std::optional<Color>(cv);
    } else {
      osmscout::log.Error() << "Cannot parse color " << str;
      return;
    }
  }
  loading = true;
  emit setColorRequest(track, colorValue);
  emit loadingChanged();
}

