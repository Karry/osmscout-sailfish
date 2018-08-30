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

#ifndef OSMSCOUT_SAILFISH_COLLECTIONLISTMODEL_H
#define OSMSCOUT_SAILFISH_COLLECTIONLISTMODEL_H

#include "Storage.h"

#include <QObject>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QSet>

class CollectionListModel : public QAbstractListModel {

  Q_OBJECT
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)

signals:
  void loadingChanged() const;
  void collectionLoadRequest();
  void updateCollectionRequest(Collection);
  void deleteCollectionRequest(qint64);
  void importCollectionRequest(QString);
  void error(QString message);

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onCollectionsLoaded(std::vector<Collection> collections, bool ok);
  void createCollection(QString name, QString description);
  void deleteCollection(QString id);
  void editCollection(QString id, QString name, QString description);
  void importCollection(QString filePath);

public:
  CollectionListModel();

  virtual ~CollectionListModel();

  enum Roles {
    NameRole = Qt::UserRole,
    DescriptionRole = Qt::UserRole+1,
    IdRole = Qt::UserRole+2
  };

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  bool isLoading() const;

public:
  std::vector<Collection> collections;
  bool collectionsLoaded{false};
};

#endif //OSMSCOUT_SAILFISH_COLLECTIONLISTMODEL_H
