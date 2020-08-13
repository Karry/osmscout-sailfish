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

#include <variant>

class CollectionModel : public QAbstractListModel {

  Q_OBJECT
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(bool exporting READ isExporting NOTIFY exportingChanged)
  Q_PROPERTY(bool collectionVisible READ isVisible NOTIFY loadingChanged)
  Q_PROPERTY(QString collectionId READ getCollectionId WRITE setCollectionId)
  Q_PROPERTY(QString name READ getCollectionName NOTIFY loadingChanged)
  Q_PROPERTY(QString filesystemName READ getCollectionFilesystemName NOTIFY loadingChanged)
  Q_PROPERTY(QString description READ getCollectionDescription NOTIFY loadingChanged)
  // ordering
  Q_PROPERTY(bool waypointFirst READ getWaypointFirst WRITE setWaypointFirst NOTIFY orderingChanged)
  Q_PROPERTY(Ordering ordering  READ getOrdering      WRITE setOrdering      NOTIFY orderingChanged)

signals:
  void loadingChanged();
  void exportingChanged();
  void collectionDetailRequest(Collection);
  void deleteWaypointRequest(qint64 collectionId, qint64 id);
  void deleteTrackRequest(qint64 collectionId, qint64 id);
  void createWaypointRequest(qint64 collectionId, double lat, double lon, QString name, QString description);
  void editWaypointRequest(qint64 collectionId, qint64 id, QString name, QString description);
  void editTrackRequest(qint64 collectionId, qint64 id, QString name, QString description);
  void exportCollectionRequest(qint64 collectionId, QString file, bool includeWaypoints, int accuracyFilter);
  void exportTrackRequest(qint64 collectionId, qint64 trackId, QString file, bool includeWaypoints, int accuracyFilter);
  void error(QString message);
  void moveWaypointRequest(qint64 waypointId, qint64 collectionId);
  void moveTrackRequest(qint64 trackId, qint64 collectionId);
  void orderingChanged();

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onCollectionDetailsLoaded(Collection collection, bool ok);
  void createWaypoint(double lat, double lon, QString name, QString description);
  void deleteWaypoint(QString id);
  void deleteTrack(QString id);
  void editWaypoint(QString id, QString name, QString description);
  void editTrack(QString id, QString name, QString description);
  void exportToFile(QString fileName, QString directory, bool includeWaypoints, int accuracyFilter);
  void exportTrackToFile(QString id, QString name, QString directory, bool includeWaypoints, int accuracyFilter);
  void onCollectionExported(bool);
  void onTrackExported(bool);
  void moveWaypoint(QString waypointId, QString collectionId);
  void moveTrack(QString trackId, QString collectionId);

public:
  CollectionModel();

  ~CollectionModel() override = default;

  enum Ordering {
    DateAscent = 0, // older first
    DateDescent = 1, // newer first
    NameAscent = 2, // A-Z
    NameDescent = 3 // Z-A
  };
  Q_ENUM(Ordering)

  enum Roles {
    NameRole = Qt::UserRole,
    FilesystemNameRole = Qt::UserRole+1,
    DescriptionRole = Qt::UserRole+2,
    TypeRole = Qt::UserRole+3,
    IdRole = Qt::UserRole+4,
    TimeRole = Qt::UserRole+5,
    LastModificationRole = Qt::UserRole+6,

    // type == waypoint
    SymbolRole = Qt::UserRole+7,
    LatitudeRole = Qt::UserRole+8,
    LongitudeRole = Qt::UserRole+9,
    ElevationRole = Qt::UserRole+10,

    // type == track
    DistanceRole = Qt::UserRole+11
  };
  Q_ENUM(Roles)

  using Item = std::variant<Track, Waypoint>;

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
  QString getCollectionFilesystemName() const;
  QString getCollectionDescription() const;
  bool isVisible() const;

  bool isExporting();
  Q_INVOKABLE QStringList getExportSuggestedDirectories();

  bool getWaypointFirst() const
  {
    return waypointFirst;
  }
  void setWaypointFirst(bool b);

  inline Ordering getOrdering() const
  {
    return ordering;
  }

  void setOrdering(Ordering ordering);

private:
  /**
   * Updates model entries without reseting whole model.
   * Both vectors have to use the same sorting.
   */
  void handleChanges(std::vector<Item> &current, const std::vector<Item> &newItems);

  void sort(std::vector<Item> &items) const;

private:
  Collection collection;
  std::vector<Item> items;

  bool collectionLoaded{false};
  bool collectionExporting{false};

  bool waypointFirst{true};
  Ordering ordering{DateAscent};
};
