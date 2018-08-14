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

#include "CollectionListModel.h"

#include <QDebug>

CollectionListModel::CollectionListModel()
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

    connect(storage, SIGNAL(error(QString)),
            this, SIGNAL(error(QString)),
            Qt::QueuedConnection);

    connect(this, SIGNAL(updateCollectionRequest(Collection)),
            storage, SLOT(updateOrCreateCollection(Collection)),
            Qt::QueuedConnection);

    connect(this, SIGNAL(deleteCollectionRequest(qint64)),
            storage, SLOT(deleteCollection(qint64)),
            Qt::QueuedConnection);

    connect(this, SIGNAL(importCollectionRequest(QString)),
            storage, SLOT(importCollection(QString)),
            Qt::QueuedConnection);
  }

  emit collectionLoadRequest();
}

CollectionListModel::~CollectionListModel()
{
}

void CollectionListModel::storageInitialised()
{
  beginResetModel();
  collections.clear();
  collectionsLoaded = false;
  endResetModel();
  emit collectionLoadRequest();
}

void CollectionListModel::storageInitialisationError(QString)
{
  storageInitialised();
}

void CollectionListModel::onCollectionsLoaded(std::vector<Collection> collections, bool ok)
{
  beginResetModel();
  collectionsLoaded = true;
  this->collections = collections;
  endResetModel();

  if (!ok){
    qWarning() << "Collection load fails";
  }
  emit loadingChanged();
}

int CollectionListModel::rowCount(const QModelIndex &parentIndex) const
{
  return collections.size();
}

QVariant CollectionListModel::data(const QModelIndex &index, int role) const
{
  if(index.row() < 0 || index.row() >= (int)collections.size()) {
    return QVariant();
  }
  const Collection &collection = collections[index.row()];
  switch(role){
    case NameRole: return collection.name;
    case DescriptionRole: return collection.description;
    case IdRole: return collection.id;
  }
  return QVariant();
}

QHash<int, QByteArray> CollectionListModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[NameRole]="name";
  roles[DescriptionRole]="description";
  roles[IdRole]="id";

  return roles;
}

Qt::ItemFlags CollectionListModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool CollectionListModel::isLoading() const
{
  return !collectionsLoaded;
}

void CollectionListModel::createCollection(QString name, QString description)
{
  collectionsLoaded=false;
  emit loadingChanged();
  emit updateCollectionRequest(Collection(-1, name, description));
}

void CollectionListModel::deleteCollection(qint64 id)
{
  collectionsLoaded=false;
  emit loadingChanged();
  emit deleteCollectionRequest(id);
}

void CollectionListModel::editCollection(qint64 id, QString name, QString description)
{
  collectionsLoaded=false;
  emit loadingChanged();
  emit updateCollectionRequest(Collection(id, name, description));
}

void CollectionListModel::importCollection(QString filePath)
{
  collectionsLoaded=false;
  emit loadingChanged();
  emit importCollectionRequest(filePath);
}
