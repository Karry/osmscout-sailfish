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

    connect(this, SIGNAL(collectionLoadRequest()),
            storage, SLOT(loadCollections()),
            Qt::QueuedConnection);

    connect(storage, SIGNAL(collectionsLoaded(std::vector<Collection>, bool)),
            this, SLOT(onCollectionsLoaded(std::vector<Collection>, bool)),
            Qt::QueuedConnection);
  }

  emit collectionLoadRequest();
}

CollectionModel::~CollectionModel()
{
}

void CollectionModel::storageInitialised()
{
  beginResetModel();
  collections.clear();
  collectionLoaded = false;
  loadingCollection.clear();
  endResetModel();
  emit collectionLoadRequest();
}

void CollectionModel::storageInitialisationError(QString)
{
  storageInitialised();
}


void CollectionModel::requestCollectionLoad(long id) const
{
  if (loadingCollection.contains(id))
    return;

  loadingCollection << id;
  emit loadingChanged();
  // TODO: emit collection load
}

void CollectionModel::onCollectionsLoaded(std::vector<Collection> collections,
                                          bool ok)
{
  beginResetModel();
  collectionLoaded = true;
  collections = collections;
  loadingCollection.clear();
  endResetModel();

  if (!ok){
    qWarning() << "Collection load fails";
  }
  emit loadingChanged();
}

int CollectionModel::rowCount(const QModelIndex &parentIndex) const
{
  QStringList dir;
  if (parentIndex.isValid()){
    Collection *parent = static_cast<Collection*>(parentIndex.internalPointer());
    if (parent == nullptr)
      return 0;
    if (parent->tracks && parent->waypoints) {
      return parent->tracks->size() + parent->waypoints->size();
    } else {
      requestCollectionLoad(parent->id);
    }
  } else {
    if (collectionLoaded){
      return collections.size();
    }
  }
  return 0;
}

int CollectionModel::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid()){
    return 1;
  }
  return 0;
}

QModelIndex CollectionModel::index(int row, int column, const QModelIndex &parent) const
{
  // TODO
  return QModelIndex();
}

QModelIndex CollectionModel::parent(const QModelIndex &index) const
{
  // TODO
  return QModelIndex();
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
  // TODO
  return QVariant();
}

QHash<int, QByteArray> CollectionModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[NameRole]="name";
  roles[DescriptionRole]="description";
  roles[TypeRole]="type";
  roles[IdRole]="id";

  return roles;
}

Qt::ItemFlags CollectionModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool CollectionModel::isLoading() const
{
  return !loadingCollection.empty() || !collectionLoaded;
}

