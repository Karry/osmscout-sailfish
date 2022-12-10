/*
  OSMScout for SFOS
  Copyright (C) 2022 Lukas Karas

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

#include <variant>

struct CollectionStatisticsItem {
  QString type;
  int trackCount{0};
  osmscout::Distance longestTrack;
  TrackStatistics statistics;
};

class CollectionStatisticsModel : public QAbstractListModel {

  Q_OBJECT
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString collectionId READ getCollectionId WRITE setCollectionId)
  Q_PROPERTY(QString name READ getCollectionName NOTIFY loadingChanged)
  Q_PROPERTY(QString description READ getCollectionDescription NOTIFY loadingChanged)

signals:
  void loadingChanged();
  void collectionDetailRequest(Collection);
  void error(QString message);

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onCollectionDetailsLoaded(Collection collection, bool ok);

public:
  CollectionStatisticsModel();

  ~CollectionStatisticsModel() override = default;

  enum Roles {
    TypeRole = Qt::UserRole,
    DistanceRole = Qt::UserRole + 1,
    RawDistanceRole = Qt::UserRole + 2,
    DurationRole = Qt::UserRole + 3,
    MovingDurationRole = Qt::UserRole + 4,
    MaxSpeedRole = Qt::UserRole + 5,
    AscentRole = Qt::UserRole + 6,
    DescentRole = Qt::UserRole + 7,
    MinElevationRole = Qt::UserRole + 8,
    MaxElevationRole = Qt::UserRole + 9,
    LongestTrackRole = Qt::UserRole + 10,
    TrackCountRole = Qt::UserRole + 11
  };
  Q_ENUM(Roles)

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  QString getCollectionId() const
  {
    return QString::number(collection.id);
  }

  void setCollectionId(QString id);

  bool isLoading() const;
  QString getCollectionName() const;
  QString getCollectionDescription() const;

private:
  Collection collection;
  std::vector<CollectionStatisticsItem> items;

  bool collectionLoaded{false};
};
