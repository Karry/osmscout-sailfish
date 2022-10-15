/*
  OSMScout for SFOS
  Copyright (C) 2022 Lukas Karas

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

#include "CollectionStatisticsModel.h"
#include "QVariantConverters.h"



namespace {
std::optional<osmscout::Distance> maxOpt(const std::optional<osmscout::Distance> &a,
                                         const std::optional<osmscout::Distance> &b) {
  if (!a) {
    return b;
  }
  if (!b) {
    return a;
  }
  return std::make_optional<osmscout::Distance>(std::max(*a, *b));
}

std::optional<osmscout::Distance> minOpt(const std::optional<osmscout::Distance> &a,
                                         const std::optional<osmscout::Distance> &b) {
  if (!a) {
    return b;
  }
  if (!b) {
    return a;
  }
  return std::make_optional<osmscout::Distance>(std::min(*a, *b));
}

double distanceOpt(const std::optional<osmscout::Distance> &d) {
  if (d.has_value())
    return d->AsMeter();
  return -1000000; // JS numeric limits may be different from C++
}
} // anonymous namespace

CollectionStatisticsModel::CollectionStatisticsModel()
{
  Storage *storage = Storage::getInstance();
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &CollectionStatisticsModel::storageInitialised,
          Qt::QueuedConnection);

  connect(storage, &Storage::initialisationError,
          this, &CollectionStatisticsModel::storageInitialisationError,
          Qt::QueuedConnection);

  connect(this, &CollectionStatisticsModel::collectionDetailRequest,
          storage, &Storage::loadCollectionDetails,
          Qt::QueuedConnection);

  connect(storage, &Storage::collectionDetailsLoaded,
          this, &CollectionStatisticsModel::onCollectionDetailsLoaded,
          Qt::QueuedConnection);

  connect(storage, &Storage::error,
          this, &CollectionStatisticsModel::error,
          Qt::QueuedConnection);

}

bool CollectionStatisticsModel::isLoading() const
{
  return !collectionLoaded;
}

QString CollectionStatisticsModel::getCollectionName() const
{
  return collectionLoaded? collection.name : "";
}

QString CollectionStatisticsModel::getCollectionDescription() const
{
  return collectionLoaded? collection.description : "";
}

void CollectionStatisticsModel::setCollectionId(QString id)
{
  bool ok;
  collection.id = id.toLongLong(&ok);
  if (!ok)
    collection.id = -1;

  emit collectionDetailRequest(collection);
}

void CollectionStatisticsModel::storageInitialised()
{
  beginResetModel();
  collectionLoaded = false;
  endResetModel();
  if (collection.id > 0) {
    emit collectionDetailRequest(collection);
  }
}

void CollectionStatisticsModel::storageInitialisationError(QString)
{
  storageInitialised();
}

void CollectionStatisticsModel::onCollectionDetailsLoaded(Collection collection, bool ok)
{
  if (this->collection.id != collection.id) {
    return;
  }
  collectionLoaded = true;
  if (ok && collection.tracks) {
    this->collection = collection;
    QMap<QString, CollectionStatisticsItem> typeMap;
    for (const Track &track: *(collection.tracks)) {
      CollectionStatisticsItem &stat = typeMap[track.type];

      stat.type = track.type;
      stat.trackCount++;
      stat.longestTrack = std::max(stat.longestTrack, track.statistics.distance);

      stat.statistics.distance += track.statistics.distance;
      stat.statistics.rawDistance += track.statistics.rawDistance;
      stat.statistics.duration += track.statistics.duration;
      stat.statistics.movingDuration += track.statistics.movingDuration;
      stat.statistics.maxSpeed = std::max(stat.statistics.maxSpeed, track.statistics.maxSpeed);
      stat.statistics.ascent += track.statistics.ascent;
      stat.statistics.descent += track.statistics.descent;
      stat.statistics.minElevation = minOpt(stat.statistics.minElevation, track.statistics.minElevation);
      stat.statistics.maxElevation = maxOpt(stat.statistics.minElevation, track.statistics.minElevation);
    }

    beginResetModel();
    for (const CollectionStatisticsItem &stat: typeMap.values()) {
      items.emplace_back(stat);
    }
    std::sort(items.begin(), items.end(), [](const CollectionStatisticsItem &a, const CollectionStatisticsItem &b) -> bool {
      return a.trackCount < b.trackCount;
    });
    endResetModel();
  }
  emit loadingChanged();
}

int CollectionStatisticsModel::rowCount(const QModelIndex &) const
{
  return items.size();
}

QVariant CollectionStatisticsModel::data(const QModelIndex &index, int role) const
{
  using namespace converters;
  using namespace std::string_literals;

  int row = index.row();
  if(row < 0 || row >= (int)items.size()) {
    return QVariant();
  }

  const CollectionStatisticsItem &item = items[row];
  switch (role) {
    case TypeRole: return item.type;
    case DistanceRole: return item.statistics.distance.AsMeter();
    case RawDistanceRole: return item.statistics.rawDistance.AsMeter();
    case DurationRole: return  item.statistics.durationMillis();
    case MovingDurationRole: return item.statistics.movingDurationMillis();
    case MaxSpeedRole: return item.statistics.maxSpeed;
    case AscentRole: return item.statistics.ascent.AsMeter();
    case DescentRole: return item.statistics.descent.AsMeter();
    case MinElevationRole: return distanceOpt(item.statistics.minElevation);
    case MaxElevationRole: return distanceOpt(item.statistics.maxElevation);
    case LongestTrackRole: return item.longestTrack.AsMeter();
    case TrackCountRole: return item.trackCount;
  }
  assert(false);
}

QHash<int, QByteArray> CollectionStatisticsModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[TypeRole] = "type";
  roles[DistanceRole] = "distance";
  roles[RawDistanceRole] = "rawDistance";
  roles[DurationRole] = "duration";
  roles[MovingDurationRole] = "movingDuration";
  roles[MaxSpeedRole] = "maxSpeed";
  roles[AscentRole] = "ascent";
  roles[DescentRole] = "descent";
  roles[MinElevationRole] = "minElevation";
  roles[MaxElevationRole] = "maxElevation";
  roles[LongestTrackRole] = "longestTrack";
  roles[TrackCountRole] = "trackCount";

  return roles;
}

Qt::ItemFlags CollectionStatisticsModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
