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
#include <QtCore/QCollator>

CollectionListModel::CollectionListModel()
{
  Storage *storage = Storage::getInstance();
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &CollectionListModel::storageInitialised,
          Qt::QueuedConnection);

  connect(storage, &Storage::initialisationError,
          this, &CollectionListModel::storageInitialisationError,
          Qt::QueuedConnection);

  connect(this, &CollectionListModel::collectionLoadRequest,
          storage, &Storage::loadCollections,
          Qt::QueuedConnection);

  connect(storage, &Storage::collectionsLoaded,
          this, &CollectionListModel::onCollectionsLoaded,
          Qt::QueuedConnection);

  connect(storage, &Storage::error,
          this, &CollectionListModel::error,
          Qt::QueuedConnection);

  connect(this, &CollectionListModel::updateCollectionRequest,
          storage, &Storage::updateOrCreateCollection,
          Qt::QueuedConnection);

  connect(this, &CollectionListModel::deleteCollectionRequest,
          storage, &Storage::deleteCollection,
          Qt::QueuedConnection);

  connect(this, &CollectionListModel::importCollectionRequest,
          storage, &Storage::importCollection,
          Qt::QueuedConnection);

  connect(this, &CollectionListModel::visibleAllRequest,
          storage, &Storage::visibleAll,
          Qt::QueuedConnection);

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
  collectionsLoaded = true;

  // following process is little bit complicated, but we don't want to call
  // model reset - it breaks UI animations for changes

  // process removals
  QMap<qint64, Collection> currentColMap;
  for (auto col: collections){
    currentColMap[col.id] = col;
  }

  sort(collections);

  bool deleteDone=false;
  while (!deleteDone){
    deleteDone=true;
    for (size_t row=0;row<this->collections.size(); row++){
      if (!currentColMap.contains(this->collections.at(row).id)){
        beginRemoveRows(QModelIndex(), row, row);
        this->collections.erase(this->collections.begin() + row);
        //this->collections.removeAt(row);
        endRemoveRows();
        deleteDone = false;
        break;
      }
    }
  }

  // process adds
  QMap<qint64, Collection> oldColMap;
  for (auto col: this->collections){
    oldColMap[col.id] = col;
  }

  for (size_t row = 0; row < collections.size(); row++) {
    auto col = collections.at(row);
    if (!oldColMap.contains(col.id)){
      beginInsertRows(QModelIndex(), row, row);
      this->collections.insert(this->collections.begin() + row, col);
      endInsertRows();
      oldColMap[col.id] = col;
    }else{
      this->collections[row] = col;
      // TODO: check changed roles
      dataChanged(index(row), index(row), roleNames().keys().toVector());
    }
  }

  if (!ok){
    qWarning() << "Collection load fails";
  }
  emit loadingChanged();
}

int CollectionListModel::rowCount([[maybe_unused]] const QModelIndex &parentIndex) const
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
    case IdRole: return QString::number(collection.id);
    case VisibleRole: return collection.visible;
    case VisibleAllRole: return collection.visibleAll;
  }
  return QVariant();
}

QHash<int, QByteArray> CollectionListModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[NameRole]="name";
  roles[DescriptionRole]="description";
  roles[IdRole]="id";
  roles[VisibleRole]="visible";
  roles[VisibleAllRole]="visibleAll";

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
  emit updateCollectionRequest(Collection(-1, false, false, name, description));
}

void CollectionListModel::deleteCollection(QString idStr)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }
  collectionsLoaded=false;
  emit loadingChanged();
  emit deleteCollectionRequest(id);
}

void CollectionListModel::editCollection(QString idStr, bool visible, QString name, QString description)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }
  collectionsLoaded=false;
  emit loadingChanged();
  emit updateCollectionRequest(Collection(id, visible, false, name, description));
}

void CollectionListModel::visibleAll(QString idStr)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }
  collectionsLoaded=false;
  emit loadingChanged();
  emit visibleAllRequest(id, true);
}

void CollectionListModel::visibleNone(QString idStr)
{
  bool ok;
  qint64 id = idStr.toLongLong(&ok);
  if (!ok){
    qWarning() << "Can't convert" << idStr << "to number";
    return;
  }
  collectionsLoaded=false;
  emit loadingChanged();
  emit visibleAllRequest(id, false);
}

void CollectionListModel::importCollection(QString filePath)
{
  collectionsLoaded=false;
  emit loadingChanged();
  emit importCollectionRequest(filePath);
}

void CollectionListModel::sort(std::vector<Collection> &items) const
{
  using namespace std::string_literals;

  static const QCollator coll;

  std::sort(items.begin(), items.end(),
            [&](const Collection& lhs, const Collection& rhs) {
              switch (ordering){
                case DateAscent:
                  return lhs.id < rhs.id;
                case DateDescent:
                  return lhs.id > rhs.id;
                case NameAscent:
                  return coll.compare(lhs.name, rhs.name) < 0;
                case NameDescent:
                  return coll.compare(lhs.name, rhs.name) > 0;
              }
              assert(false);
              return false;
            });
}

void CollectionListModel::setOrdering(Ordering ordering)
{
  if (ordering != this->ordering){
    this->ordering = ordering;

    emit beginResetModel();
    sort(collections);
    emit endResetModel();

    emit orderingChanged();
  }
}
