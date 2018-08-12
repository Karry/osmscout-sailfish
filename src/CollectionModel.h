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

#ifndef OSMSCOUT_SAILFISH_COLLECTIONMODEL_H
#define OSMSCOUT_SAILFISH_COLLECTIONMODEL_H

#include <QObject>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QSet>
#include "Storage.h"

class CollectionModel : public QAbstractItemModel {

  Q_OBJECT
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)

signals:
  void loadingChanged() const;
  void collectionLoadRequest();

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onCollectionsLoaded(std::vector<Collection> collections,
                           bool ok);

public:
  CollectionModel();

  virtual ~CollectionModel();

  enum Roles {
    NameRole = Qt::UserRole,
    DescriptionRole = Qt::UserRole+1,
    TypeRole = Qt::UserRole+2,
    IdRole = Qt::UserRole+3
  };

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QModelIndex parent(const QModelIndex &index) const;

  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  bool isLoading() const;
  void requestCollectionLoad(long id) const;

public:
  std::vector<Collection> collections;
  bool collectionLoaded{false};
  mutable QSet<long> loadingCollection;
};

#endif //OSMSCOUT_SAILFISH_COLLECTIONMODEL_H
