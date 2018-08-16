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

CollectionModel::CollectionModel()
{
  Storage *storage = Storage::getInstance();
  if (storage){
    connect(storage, SIGNAL(initialised()),
            this, SLOT(storageInitialised()),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(initialisationError(QString)),
            this, SLOT(storageInitialisationError(QString)),
            Qt::QueuedConnection);

    connect(this, SIGNAL(collectionDetailRequest(Collection)),
            storage, SLOT(loadCollectionDetails(Collection)),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(collectionDetailsLoaded(Collection, bool)),
            this, SLOT(onCollectionDetailsLoaded(Collection, bool)),
            Qt::QueuedConnection);
  }
}

CollectionModel::~CollectionModel()
{
}

void CollectionModel::storageInitialised()
{
  beginResetModel();
  collectionLoaded = false;
  endResetModel();
  if (collection.id > 0)
    emit collectionDetailRequest(collection);
}

void CollectionModel::storageInitialisationError(QString)
{
  storageInitialised();
}

void CollectionModel::onCollectionDetailsLoaded(Collection collection, bool ok)
{
  beginResetModel();
  collectionLoaded = true;
  this->collection = collection;
  endResetModel();

  if (!ok){
    qWarning() << "Collection load fails";
  }
  emit loadingChanged();
}

int CollectionModel::rowCount(const QModelIndex &parentIndex) const
{
  int cnt = 0;
  if (collection.waypoints)
    cnt += collection.waypoints->size();
  if (collection.tracks)
    cnt += collection.tracks->size();
  return cnt;
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
  using namespace converters;

  if(index.row() < 0) {
    return QVariant();
  }
  size_t row = index.row();

  if (collection.waypoints) {
    if (row < collection.waypoints->size()){
      const Waypoint &waypoint = collection.waypoints->at(row);
      switch(role){
        case TypeRole: return "waypoint";
        case NameRole: return QString::fromStdString(waypoint.data.name.getOrElse(""));
        case DescriptionRole: return QString::fromStdString(waypoint.data.description.getOrElse(""));
        case IdRole: return waypoint.id;

        case SymbolRole: return QString::fromStdString(waypoint.data.symbol.getOrElse(""));
        case LatitudeRole: return waypoint.data.coord.GetLat();
        case LongitudeRole: return waypoint.data.coord.GetLon();
        case TimeRole: return timestampToDateTime(waypoint.data.time);
      }
    } else {
      row -= collection.waypoints->size();
    }
  }
  if (collection.tracks) {
    if (row < collection.tracks->size()){
      const Track &track = collection.tracks->at(row);
      switch(role){
        case TypeRole: return "track";
        case NameRole: return track.name;
        case DescriptionRole: return track.description;
        case IdRole: return track.id;

        case TimeRole: return track.creationTime;
        case DistanceRole: return track.statistics.distance.AsMeter();
      }
    }
  }

  return QVariant();
}

QHash<int, QByteArray> CollectionModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[NameRole] = "name";
  roles[DescriptionRole] = "description";
  roles[TypeRole] = "type";
  roles[IdRole] = "id";
  roles[TimeRole] = "time";

  // waypoint
  roles[SymbolRole] = "symbol";
  roles[LatitudeRole] = "latitude";
  roles[LongitudeRole] = "longitude";

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

void CollectionModel::setCollectionId(qint64 id)
{
  collection.id = id;
  emit collectionDetailRequest(collection);
}

bool CollectionModel::isLoading() const
{
  return !collectionLoaded;
}

QString CollectionModel::getCollectionName() const
{
  return collectionLoaded? collection.name : "";
}

QString CollectionModel::getCollectionDescription() const
{
  return collectionLoaded? collection.description : "";
}
