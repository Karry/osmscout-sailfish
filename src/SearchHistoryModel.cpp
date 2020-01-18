/*
  OSMScout for SFOS
  Copyright (C) 2020 Lukas Karas

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

#include "SearchHistoryModel.h"

SearchHistoryModel::SearchHistoryModel() {
  Storage *storage = Storage::getInstance();
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &SearchHistoryModel::storageInitialised,
          Qt::QueuedConnection);

  connect(storage, &Storage::searchHistory,
          this, &SearchHistoryModel::searchHistoryUpdated,
          Qt::QueuedConnection);

  connect(this, &SearchHistoryModel::requestSearchHistory,
          storage, &Storage::loadSearchHistory,
          Qt::QueuedConnection);

  connect(this, &SearchHistoryModel::addSearchPatternRequest,
          storage, &Storage::addSearchPattern,
          Qt::QueuedConnection);

  connect(this, &SearchHistoryModel::removeSearchPatternRequest,
          storage, &Storage::removeSearchPattern,
          Qt::QueuedConnection);

  storageInitialised();
}

SearchHistoryModel::~SearchHistoryModel() {}

int SearchHistoryModel::rowCount(const QModelIndex &) const {
  return items.size();
}

QVariant SearchHistoryModel::data(const QModelIndex &index, int role) const {

  int row = index.row();
  if(row < 0 || (size_t)row >= items.size()) {
    return QVariant();
  }

  const SearchItem &item = items[row];
  switch(role) {
    case PatternRole:
      return item.pattern;
    case LastUsageRole :
      return item.lastUsage;
    default:
      return QVariant();
  }
}

QHash<int, QByteArray> SearchHistoryModel::roleNames() const {
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[PatternRole] = "pattern";
  roles[LastUsageRole] = "lastUsage";

  return roles;
}

Qt::ItemFlags SearchHistoryModel::flags(const QModelIndex &index) const {
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void SearchHistoryModel::addPattern(const QString &pattern) {
  emit addSearchPatternRequest(pattern);
}

void SearchHistoryModel::removePattern(const QString &pattern) {
  emit removeSearchPatternRequest(pattern);
}

void SearchHistoryModel::storageInitialised(){
  emit requestSearchHistory();
}

void SearchHistoryModel::searchHistoryUpdated(const std::vector<SearchItem> &items){
  beginResetModel();
  this->items = items;
  endResetModel();
}
