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
  Q_PROPERTY(bool exporting READ isExporting NOTIFY exportingChanged)
  Q_PROPERTY(QString collectionId READ getCollectionId WRITE setCollectionId)
  Q_PROPERTY(QString name READ getCollectionName NOTIFY loadingChanged)
  Q_PROPERTY(QString filesystemName READ getCollectionFilesystemName NOTIFY loadingChanged)
  Q_PROPERTY(QString description READ getCollectionDescription NOTIFY loadingChanged)

signals:
  void loadingChanged();
  void exportingChanged();
  void collectionDetailRequest(Collection);
  void deleteWaypointRequest(qint64 collectionId, qint64 id);
  void deleteTrackRequest(qint64 collectionId, qint64 id);
  void createWaypointRequest(qint64 collectionId, double lat, double lon, QString name, QString description);
  void editWaypointRequest(qint64 collectionId, qint64 id, QString name, QString description);
  void editTrackRequest(qint64 collectionId, qint64 id, QString name, QString description);
  void exportCollectionRequest(qint64 collectionId, QString file);
  void error(QString message);

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onCollectionDetailsLoaded(Collection collection, bool ok);
  void createWaypoint(double lat, double lon, QString name, QString description);
  void deleteWaypoint(QString id);
  void deleteTrack(QString id);
  void editWaypoint(QString id, QString name, QString description);
  void editTrack(QString id, QString name, QString description);
  void exportToFile(QString fileName, QString directory);
  void onCollectionExported(bool);

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

  QString getCollectionId() const
  {
    return QString::number(collection.id);
  }

  void setCollectionId(QString id);

  bool isLoading() const;
  QString getCollectionName() const;
  QString getCollectionFilesystemName() const;
  QString getCollectionDescription() const;

  bool isExporting();
  Q_INVOKABLE QStringList getExportSuggestedDirectories();

public:
  Collection collection;
  bool collectionLoaded{false};
  bool collectionExporting{false};
};

#endif //OSMSCOUT_SAILFISH_COLLECTIONMODEL_H
