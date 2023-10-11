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

#include <osmscoutclientqt/LocationEntry.h>

#include <QDebug>
#include <QtCore/QStandardPaths>
#include <QStorageInfo>
#include <QtCore/QCollator>

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

  connect(this, &CollectionModel::waypointVisibilityRequest,
          storage, &Storage::waypointVisibility,
          Qt::QueuedConnection);

  connect(this, &CollectionModel::trackVisibilityRequest,
          storage, &Storage::trackVisibility,
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
      return timestampToDateTime(std::get<Waypoint>(item).data.timestamp);
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

  static const QCollator coll;

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
                  return coll.compare(name(lhs), name(rhs)) < 0;
                case NameDescent:
                  return coll.compare(name(lhs), name(rhs)) > 0;
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
  if (showWaypoints) {
    if (collection.waypoints) {
      for (const auto &wpt: *collection.waypoints) {
        newItems.push_back(wpt);
      }
    }
  }
  if (showTracks) {
    if (collection.tracks) {
      for (const auto &trk: *collection.tracks) {
        newItems.push_back(trk);
      }
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

QString CollectionModel::waypointType(const std::optional<std::string> &symbol, const QString &defaultType)
{
  static const QMap<QString, QString> waypointSymbolType {
    {"Red Circle", "_waypoint_red_circle"},
    {"Green Circle", "_waypoint_green_circle"},
    {"Blue Circle", "_waypoint_blue_circle"},
    {"Yellow Circle", "_waypoint_yellow_circle"},
    {"Red Square", "_waypoint_red_square"},
    {"Green Square", "_waypoint_green_square"},
    {"Blue Square", "_waypoint_blue_square"},
    {"Yellow Square", "_waypoint_yellow_square"},
    {"Red Triangle", "_waypoint_red_triangle"},
    {"Green Triangle", "_waypoint_green_triangle"},
    {"Blue Triangle", "_waypoint_blue_triangle"},
    {"Yellow Triangle", "_waypoint_yellow_triangle"},
  };

  QString type = defaultType;
  if (symbol.has_value()) {
    QString symbolStr = QString::fromStdString(symbol.value());
    if (waypointSymbolType.contains(symbolStr)) {
      type = waypointSymbolType[symbolStr];
    }
  }
  return type;
}

QString CollectionModel::waypointColor(const std::optional<std::string> &symbol, const QString &defaultColor)
{
  static const QMap<QString, QString> waypointSymbolColors {
    {"Red Circle", "#b32020"},
    {"Green Circle", "#00b200"},
    {"Blue Circle", "#203bb3"},
    {"Yellow Circle", "#dfed00"},
    {"Red Square", "#b32020"},
    {"Green Square", "#00b200"},
    {"Blue Square", "#203bb3"},
    {"Yellow Square", "#dfed00"},
    {"Red Triangle", "#b32020"},
    {"Green Triangle", "#00b200"},
    {"Blue Triangle", "#203bb3"},
    {"Yellow Triangle", "#dfed00"},
  };

  if (!symbol.has_value()) {
    return defaultColor;
  }
  QString symbolName = QString::fromStdString(symbol.value());
  if (waypointSymbolColors.contains(symbolName)) {
    return waypointSymbolColors[symbolName];
  }
  return defaultColor;
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

      case TimeRole: return timestampToDateTime(waypoint.data.timestamp);
      case LastModificationRole: return waypoint.lastModification;
      case VisibleRole: return waypoint.visible;
      case ColorRole: return waypointColor(waypoint.data.symbol);
      case WaypointTypeRole: return waypointType(waypoint.data.symbol);
      case LocationObjectRole:
        // QML will take ownership
        return QVariant::fromValue(new osmscout::LocationEntry(osmscout::LocationEntry::typeObject,
                                                               QString::fromStdString(waypoint.data.name.value_or(""s)),
                                                               "",
                                                               waypointType(waypoint.data.symbol),
                                                               QList<osmscout::AdminRegionInfoRef>(),
                                                               "",
                                                               waypoint.data.coord,
                                                               osmscout::GeoBox::BoxByCenterAndRadius(waypoint.data.coord, osmscout::Meters(waypoint.data.hdop.value_or(1)))));
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
      case VisibleRole: return track.visible;
      case DistanceRole: return track.statistics.distance.AsMeter();
      case ColorRole: return QString::fromStdString(track.color.has_value() ? track.color.value().ToHexString(): ""s);
      case TrackTypeRole: return track.type;
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
  roles[VisibleRole] = "visible";
  roles[ColorRole] = "color";

  // waypoint
  roles[SymbolRole] = "symbol";
  roles[LatitudeRole] = "latitude";
  roles[LongitudeRole] = "longitude";
  roles[ElevationRole] = "elevation";
  roles[WaypointTypeRole] = "waypointType";
  roles[LocationObjectRole] = "locationObject";

  // track
  roles[DistanceRole] = "distance";
  roles[TrackTypeRole] = "trackType";

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

void CollectionModel::createWaypoint(double lat, double lon, QString name, QString description, QString symbol)
{
  collectionLoaded = true;
  emit loadingChanged();
  emit createWaypointRequest(collection.id, lat, lon, name, description, symbol);
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

void CollectionModel::editWaypoint(QString idStr, QString name, QString description, QString symbol)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit editWaypointRequest(collection.id, id, name, description, symbol);
}

void CollectionModel::editTrack(QString idStr, QString name, QString description, QString type)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }

  collectionLoaded = true;
  emit loadingChanged();
  emit editTrackRequest(collection.id, id, name, description, type);
}

void CollectionModel::exportToFile(QString fileName, QString directory, bool includeWaypoints, int accuracyFilter)
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
  std::optional<double> accuracyFilterOpt = accuracyFilter <= 0 ? std::nullopt : std::make_optional(accuracyFilter);
  emit exportCollectionRequest(collection.id, file.absoluteFilePath(), includeWaypoints, accuracyFilterOpt);
}

void CollectionModel::exportTrackToFile(QString trackIdStr, QString fileName, QString directory, bool includeWaypoints, int accuracyFilter)
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
  std::optional<double> accuracyFilterOpt = accuracyFilter <= 0 ? std::nullopt : std::make_optional(accuracyFilter);
  emit exportTrackRequest(collection.id, trackId, file.absoluteFilePath(), includeWaypoints, accuracyFilterOpt);
}

void CollectionModel::onCollectionExported(qint64 collectionId, QString file, bool success)
{
  if (success) {
    emit exported(collectionId, file);
  }
  collectionExporting = false;
  emit exportingChanged();
}

void CollectionModel::onTrackExported(qint64 trackId, QString file, bool success)
{
  if (success) {
    emit trackExported(trackId, file);
  }
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

void CollectionModel::setWaypointVisibility(QString idStr, bool visible)
{
  bool ok;
  qint64 wptId = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }
  emit loadingChanged();
  emit waypointVisibilityRequest(wptId, visible);
}

void CollectionModel::setTrackVisibility(QString idStr, bool visible)
{
  bool ok;
  qint64 trackId = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }
  emit loadingChanged();
  emit trackVisibilityRequest(trackId, visible);
}

void CollectionModel::setShowTracks(bool b)
{
  showTracks=b;
}

void CollectionModel::setShowWaypoints(bool b)
{
  showWaypoints=b;
}
