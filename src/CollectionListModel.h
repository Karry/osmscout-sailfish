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

#pragma once

#include "Storage.h"

#include <QObject>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QSet>

class CollectionListModel : public QAbstractListModel {

  Q_OBJECT
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(Ordering ordering  READ getOrdering WRITE setOrdering NOTIFY orderingChanged)

signals:
  void loadingChanged() const;
  void collectionLoadRequest();
  void updateCollectionRequest(Collection);
  void deleteCollectionRequest(qint64);
  void importCollectionRequest(QString);
  void error(QString message);
  void orderingChanged();

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onCollectionsLoaded(std::vector<Collection> collections, bool ok);
  void createCollection(QString name, QString description);
  void deleteCollection(QString id);
  void editCollection(QString id, bool visible, QString name, QString description);
  void importCollection(QString filePath);

public:
  CollectionListModel();

  virtual ~CollectionListModel();

  enum Ordering {
    DateAscent = 0, // older first
    DateDescent = 1, // newer first
    NameAscent = 2, // A-Z
    NameDescent = 3 // Z-A
  };
  Q_ENUM(Ordering)

  enum Roles {
    NameRole = Qt::UserRole,
    DescriptionRole = Qt::UserRole+1,
    IdRole = Qt::UserRole+2,
    VisibleRole = Qt::UserRole+3
  };
  Q_ENUM(Roles)

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  bool isLoading() const;

  inline Ordering getOrdering() const
  {
    return ordering;
  }

  void setOrdering(Ordering ordering);

private:
  void sort(std::vector<Collection> &items) const;

public:
  std::vector<Collection> collections;
  bool collectionsLoaded{false};
  Ordering ordering{DateAscent};
};
