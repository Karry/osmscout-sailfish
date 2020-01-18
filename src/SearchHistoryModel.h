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

#pragma once

#include "Storage.h"

#include <QObject>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QSet>

class SearchHistoryModel : public QAbstractListModel {
  Q_OBJECT

signals:
  void requestSearchHistory();
  void addSearchPatternRequest(QString pattern);
  void removeSearchPatternRequest(QString pattern);

public slots:
  void storageInitialised();
  void searchHistoryUpdated(const std::vector<SearchItem> &items);

public:
  SearchHistoryModel();

  virtual ~SearchHistoryModel();

  enum Roles {
    PatternRole = Qt::UserRole,
    LastUsageRole = Qt::UserRole + 1,
  };
  Q_ENUM(Roles)

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  Q_INVOKABLE void addPattern(const QString &pattern);
  Q_INVOKABLE void removePattern(const QString &pattern);

private:
  std::vector<SearchItem> items;
};
