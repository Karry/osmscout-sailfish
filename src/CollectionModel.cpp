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

#include "CollectionModel.h"
#include "QVariantConverters.h"

#include <QDebug>
#include <QtCore/QStandardPaths>
#include <QStorageInfo>

namespace {
QString safeFileName(QString name)
{
  return name.replace(QRegExp("[" + QRegExp::escape( "\\/:*?\"<>|" ) + "]"), QString("_"));
}
}

CollectionModel::CollectionModel()
{
  Storage *storage = Storage::getInstance();
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &CollectionModel::storageInitialised,
          Qt::QueuedConnection);

  connect(storage, &Storage::initialisationError,
          this, &CollectionModel::storageInitialisationError,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::collectionDetailRequest,
          storage, &Storage::loadCollectionDetails,
          Qt::QueuedConnection);

  connect(storage, &Storage::collectionDetailsLoaded,
          this, &CollectionModel::onCollectionDetailsLoaded,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::deleteWaypointRequest,
          storage, &Storage::deleteWaypoint,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::deleteTrackRequest,
          storage, &Storage::deleteTrack,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::createWaypointRequest,
          storage, &Storage::createWaypoint,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::editWaypointRequest,
          storage, &Storage::editWaypoint,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::editTrackRequest,
          storage, &Storage::editTrack,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::exportCollectionRequest,
          storage, &Storage::exportCollection,
          Qt::QueuedConnection);

  connect(storage, &Storage::collectionExported,
          this, &CollectionModel::onCollectionExported,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::exportTrackRequest,
          storage, &Storage::exportTrack,
          Qt::QueuedConnection);

  connect(storage, &Storage::trackExported,
          this, &CollectionModel::onTrackExported,
          Qt::QueuedConnection);

  connect(storage, &Storage::error,
          this, &CollectionModel::error,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::moveWaypointRequest,
          storage, &Storage::moveWaypoint,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::moveTrackRequest,
          storage, &Storage::moveTrack,
          Qt::QueuedConnection);

}

void CollectionModel::handleChanges(std::vector<Item> &current, const std::vector<Item> &newItems)
{
  auto id = [](const Item &item) -> qint64 {
    if (std::holds_alternative<Waypoint>(item)){
      qint64 id = std::get<Waypoint>(item).id;
      assert(id >= 0);
      return id * -1; // to distinguish waypoints and track, use negative numbers to waypoints
    } else {
      assert(std::holds_alternative<Track>(item));
      qint64 id = std::get<Track>(item).id;
      assert(id >= 0);
      return id;
    }
  };

  // process removals
  QMap<qint64, Item> currentDirMap;
  for (auto entry: newItems){
    currentDirMap[id(entry)] = entry;
  }

  bool deleteDone=false;
  while (!deleteDone){
    deleteDone=true;
    for (size_t row=0; row < current.size(); row++){
      if (!currentDirMap.contains(id(current.at(row)))){
        beginRemoveRows(QModelIndex(), row, row);
        current.erase(current.begin() + row);
        //old.removeAt(row);
        endRemoveRows();
        deleteDone = false;
        break;
      }
    }
  }

  // process adds
  QMap<qint64, Item> oldDirMap;
  for (auto entry: current){
    oldDirMap[id(entry)] = entry;
  }

  for (size_t row = 0; row < newItems.size(); row++) {
    auto entry = newItems.at(row);
    if (!oldDirMap.contains(id(entry))){
      beginInsertRows(QModelIndex(), row, row);
      current.insert(current.begin() + row, entry);
      endInsertRows();
      oldDirMap[id(entry)] = entry;
    }else{
      current[row] = entry;
      // TODO: check changed roles
      dataChanged(index(row), index(row), roleNames().keys().toVector());
    }
  }
}


void CollectionModel::storageInitialised()
{
  beginResetModel();
  collectionLoaded = false;
  endResetModel();
  if (collection.id > 0) {
    emit collectionDetailRequest(collection);
  }
}

void CollectionModel::storageInitialisationError(QString)
{
  storageInitialised();
}

void CollectionModel::sort(std::vector<Item> &items) const
{
  using namespace converters;
  using namespace std::string_literals;

  auto date = [](const Item &item) -> QDateTime {
    if (std::holds_alternative<Track>(item)){
      return std::get<Track>(item).creationTime;
    } else {
      assert(std::holds_alternative<Waypoint>(item));
      return timestampToDateTime(std::get<Waypoint>(item).data.time);
    }
  };

  auto name = [](const Item &item) -> QString {
    if (std::holds_alternative<Track>(item)){
      return std::get<Track>(item).name;
    } else {
      assert(std::holds_alternative<Waypoint>(item));
      return QString::fromStdString(std::get<Waypoint>(item).data.name.value_or(""s));
    }
  };

  std::sort(items.begin(), items.end(),
            [&](const Item& lhs, const Item& rhs) {
              if (waypointFirst && lhs.index() != rhs.index()){
                return lhs.index() > rhs.index();
              }
              switch (ordering){
                case DateAscent:
                  return date(lhs) < date(rhs);
                case DateDescent:
                  return date(lhs) > date(rhs);
                case NameAscent:
                  return name(lhs) < name(rhs);
                case NameDescent:
                  return name(lhs) > name(rhs);
              }
              assert(false);
              return false;
            });
}

void CollectionModel::onCollectionDetailsLoaded(Collection collection, bool ok)
{
  if (this->collection.id != collection.id){
    return;
  }
  collectionLoaded = true;
  this->collection = collection;

  std::vector<Item> newItems;
  if (collection.waypoints){
    for (const auto &wpt : *collection.waypoints){
      newItems.push_back(wpt);
    }
  }
  if (collection.tracks){
    for (const auto &trk : *collection.tracks){
      newItems.push_back(trk);
    }
  }

  sort(newItems);

  handleChanges(items, newItems);

  if (!ok){
    qWarning() << "Collection load fails";
  }
  emit loadingChanged();
}

int CollectionModel::rowCount([[maybe_unused]] const QModelIndex &parentIndex) const
{
  return items.size();
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
  using namespace converters;
  using namespace std::string_literals;

  int row = index.row();
  if(row < 0 || row >= (int)items.size()) {
    return QVariant();
  }

  const Item &item = items[row];
  if (std::holds_alternative<Waypoint>(item)){
    const Waypoint &waypoint = std::get<Waypoint>(item);
    switch(role){
      case TypeRole: return "waypoint";
      case NameRole: return QString::fromStdString(waypoint.data.name.value_or(""s));
      case FilesystemNameRole: return safeFileName(QString::fromStdString(waypoint.data.name.value_or(""s)));
      case DescriptionRole: return QString::fromStdString(waypoint.data.description.value_or(""s));
      case IdRole: return QString::number(waypoint.id);

      case SymbolRole: return QString::fromStdString(waypoint.data.symbol.value_or(""s));
      case LatitudeRole: return waypoint.data.coord.GetLat();
      case LongitudeRole: return waypoint.data.coord.GetLon();
      case ElevationRole: return waypoint.data.elevation.has_value() ? *(waypoint.data.elevation) : QVariant();

      case TimeRole: return timestampToDateTime(waypoint.data.time);
      case LastModificationRole: return waypoint.lastModification;
      default: return QVariant();
    }
  } else {
    assert(std::holds_alternative<Track>(item));
    const Track &track = std::get<Track>(item);
    switch(role){
      case TypeRole: return "track";
      case NameRole: return track.name;
      case FilesystemNameRole: return safeFileName(track.name);
      case DescriptionRole: return track.description;
      case IdRole: return QString::number(track.id);

      case TimeRole: return track.statistics.from;
      case LastModificationRole: return track.lastModification;
      case DistanceRole: return track.statistics.distance.AsMeter();
      default: return QVariant();
    }
  }

  assert(false);
}

QHash<int, QByteArray> CollectionModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[NameRole] = "name";
  roles[FilesystemNameRole] = "filesystemName";
  roles[DescriptionRole] = "description";
  roles[TypeRole] = "type";
  roles[IdRole] = "id";
  roles[TimeRole] = "time";
  roles[LastModificationRole] = "lastModification";

  // waypoint
  roles[SymbolRole] = "symbol";
  roles[LatitudeRole] = "latitude";
  roles[LongitudeRole] = "longitude";
  roles[ElevationRole] = "elevation";

  // track
  roles[DistanceRole] = "distance";

  return roles;
}

Qt::ItemFlags CollectionModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void CollectionModel::setCollectionId(QString id)
{
  bool ok;
  collection.id = id.toLongLong(&ok);
  if (!ok)
    collection.id = -1;

  emit collectionDetailRequest(collection);
}

bool CollectionModel::isLoading() const
{
  return !collectionLoaded;
}

bool CollectionModel::isExporting()
{
  return collectionExporting;
}

QString CollectionModel::getCollectionName() const
{
  return collectionLoaded? collection.name : "";
}

QString CollectionModel::getCollectionFilesystemName() const
{
  return safeFileName(getCollectionName());
}

QString CollectionModel::getCollectionDescription() const
{
  return collectionLoaded? collection.description : "";
}

bool CollectionModel::isVisible() const
{
  return collectionLoaded? collection.visible : false;
}

void CollectionModel::setWaypointFirst(bool b)
{
  if (b != waypointFirst){
    waypointFirst=b;

    emit beginResetModel();
    sort(items);
    emit endResetModel();

    emit orderingChanged();
  }
}

void CollectionModel::setOrdering(Ordering ordering)
{
  if (ordering != this->ordering){
    this->ordering = ordering;

    emit beginResetModel();
    sort(items);
    emit endResetModel();

    emit orderingChanged();
  }
}

void CollectionModel::createWaypoint(double lat, double lon, QString name, QString description)
{
  collectionLoaded = true;
  emit loadingChanged();
  emit createWaypointRequest(collection.id, lat, lon, name, description);
}

void CollectionModel::deleteWaypoint(QString idStr)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit deleteWaypointRequest(collection.id, id);
}

void CollectionModel::deleteTrack(QString idStr)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit deleteTrackRequest(collection.id, id);
}

void CollectionModel::editWaypoint(QString idStr, QString name, QString description)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit editWaypointRequest(collection.id, id, name, description);
}

void CollectionModel::editTrack(QString idStr, QString name, QString description)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit editTrackRequest(collection.id, id, name, description);
}

void CollectionModel::exportToFile(QString fileName, QString directory)
{
  QFileInfo dir(directory);
  if (!dir.isDir() || !dir.isWritable()){
    qWarning() << "Invalid export directory:" << dir.absoluteFilePath();
    emit error(tr("Invalid export directory"));
    return;
  }
  QString safeName = safeFileName(fileName);
  if (safeName.isEmpty()){
    qWarning() << "Invalid file name:" << fileName << "/" << safeName;
    emit error(tr("Invalid file name"));
    return;
  }
  QFileInfo file(QDir(dir.absoluteFilePath()), safeName + ".gpx");
  collectionExporting = true;
  emit exportingChanged();
  emit exportCollectionRequest(collection.id, file.absoluteFilePath());
}

void CollectionModel::exportTrackToFile(QString trackIdStr, QString fileName, QString directory)
{
  bool ok;
  qint64 trackId = trackIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << trackIdStr << "to number";
    return;
  }

  QFileInfo dir(directory);
  if (!dir.isDir() || !dir.isWritable()){
    qWarning() << "Invalid export directory:" << dir.absoluteFilePath();
    emit error(tr("Invalid export directory"));
    return;
  }
  QString safeName = safeFileName(fileName);
  if (safeName.isEmpty()){
    qWarning() << "Invalid file name:" << fileName << "/" << safeName;
    emit error(tr("Invalid file name"));
    return;
  }
  QFileInfo file(QDir(dir.absoluteFilePath()), safeName + ".gpx");
  collectionExporting = true;
  emit exportingChanged();
  emit exportTrackRequest(collection.id, trackId, file.absoluteFilePath());
}

void CollectionModel::onCollectionExported(bool)
{
  collectionExporting = false;
  emit exportingChanged();
}

void CollectionModel::onTrackExported(bool)
{
  collectionExporting = false;
  emit exportingChanged();
}

QStringList CollectionModel::getExportSuggestedDirectories()
{
  QStringList result;
  result << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  result << QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

  for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
      if (storage.isValid() &&
          storage.isReady() &&
          !storage.isReadOnly() &&
          ( // Sailfish OS specific mount point base for SD cards!
            storage.rootPath().startsWith("/media") ||
            storage.rootPath().startsWith("/run/media/") /* SFOS >= 2.2 */
            )) {
        result << storage.rootPath();
      }
  }

  return result;
}

void CollectionModel::moveWaypoint(QString waypointIdStr, QString collectionIdStr)
{
  bool ok;
  qint64 waypointId = waypointIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << waypointIdStr << "to number";
    return;
  }
  qint64 collectionId = collectionIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << collectionIdStr << "to number";
    return;
  }

  collectionLoaded = false;
  emit loadingChanged();
  emit moveWaypointRequest(waypointId, collectionId);
}

void CollectionModel::moveTrack(QString trackIdStr, QString collectionIdStr)
{
  bool ok;
  qint64 trackId = trackIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << trackIdStr << "to number";
    return;
  }
  qint64 collectionId = collectionIdStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << collectionIdStr << "to number";
    return;
  }

  collectionLoaded = false;
  emit loadingChanged();
  emit moveTrackRequest(trackId, collectionId);
}
