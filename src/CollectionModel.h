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

#include "Storage.h"

#include <QObject>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QSet>

class CollectionModel : public QAbstractListModel {

  Q_OBJECT
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(qint64 collectionId READ getCollectionId WRITE setCollectionId)

signals:
  void loadingChanged() const;
  void collectionDetailRequest(Collection) const;

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onCollectionDetailsLoaded(Collection collection, bool ok);

public:
  CollectionModel();

  virtual ~CollectionModel();

  enum Roles {
    NameRole = Qt::UserRole,
    DescriptionRole = Qt::UserRole+1,
    TypeRole = Qt::UserRole+2,
    IdRole = Qt::UserRole+3,
    TimeRole = Qt::UserRole+4,

    // type == waypoint
    SymbolRole = Qt::UserRole+5,
    LatitudeRole = Qt::UserRole+6,
    LongitudeRole = Qt::UserRole+7,

    // type == track
    DistanceRole = Qt::UserRole+8
  };

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  qint64 getCollectionId() const
  {
    return collection.id;
  }

  void setCollectionId(qint64 id);

  bool isLoading() const;

public:
  Collection collection;
  bool collectionLoaded{false};
};

#endif //OSMSCOUT_SAILFISH_COLLECTIONMODEL_H
