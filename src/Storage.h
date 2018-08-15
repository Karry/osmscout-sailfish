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
#ifndef OSMSCOUT_SAILFISH_STORAGE_H
#define OSMSCOUT_SAILFISH_STORAGE_H

#include <osmscout/gpx/Track.h>
#include <osmscout/gpx/Waypoint.h>
#include <osmscout/gpx/Utils.h>

#include <QObject>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QDir>
#include <QtCore/QDateTime>

#include <atomic>
#include <osmscout/gpx/GpxFile.h>

class ErrorCallback: public QObject, public osmscout::gpx::ProcessCallback
{
  Q_OBJECT
  Q_DISABLE_COPY(ErrorCallback)

signals:
  void error(QString);

public:
  ErrorCallback() = default;
  virtual ~ErrorCallback() = default;

  virtual void Error(std::string error);
};

class TrackStatistics
{
public:
  TrackStatistics() = default;
  TrackStatistics(const osmscout::Distance &distance):
    distance(distance)
  {};

public:
  osmscout::Distance distance;
};

class Track
{
public:
  Track() = default;
  Track(qint64 id, qint64 collectionId,  QString name,  QString description, bool open,  QDateTime creationTime, const osmscout::Distance &distance
  ): id(id), collectionId(collectionId), name(name), description(description), open(open), creationTime(creationTime), statistics(distance)
  {};

public:
  qint64 id{-1};
  qint64 collectionId{-1};
  QString name;
  QString description;
  bool open{false};
  QDateTime creationTime;

  TrackStatistics statistics;
  std::shared_ptr<osmscout::gpx::Track> data;
};

class Waypoint
{
public:
  Waypoint() = default;

  Waypoint(qint64 id, const osmscout::gpx::Waypoint &data):
    id(id), data(data)
  {}

  Waypoint(qint64 id, osmscout::gpx::Waypoint &&data):
    id(id), data(std::move(data))
  {}

public:
  qint64 id{-1};
  osmscout::gpx::Waypoint data{osmscout::GeoCoord()};
};

class Collection
{
public:
  Collection() = default;
  Collection(qint64 id,
             const QString &name,
             const QString &description):
    id(id), name(name), description(description)
  {};

public:
  qint64 id{-1};
  QString name;
  QString description;

  std::shared_ptr<std::vector<Track>> tracks;
  std::shared_ptr<std::vector<Waypoint>> waypoints;
};

class Storage : public QObject{
  Q_OBJECT
  Q_DISABLE_COPY(Storage)

private:
  bool updateSchema();

signals:
  void initialised();
  void initialisationError(QString error);

  void collectionsLoaded(std::vector<Collection> collections, bool ok);
  void collectionDetailsLoaded(Collection collection, bool ok);
  void trackDataLoaded(Track track, bool ok);
  void error(QString);

public slots:
  void init();

  /**
   * load collection list
   * emits collectionsLoaded
   */
  void loadCollections();

  /**
   * load list of tracks and waypoints
   * emits collectionDetailsLoaded
   */
  void loadCollectionDetails(Collection collection);

  /**
   * load track data
   * emits trackDataLoaded
   */
  void loadTrackData(Track track);

  /**
   * update collection or create it (if id < 0)
   * emits collectionsLoaded signal
   */
  void updateOrCreateCollection(Collection collection);

  /**
   * delete collection
   * emits collectionsLoaded signal
   */
  void deleteCollection(qint64 id);

  /**
   * import collection from gpx file
   * emits collectionsLoaded signal
   */
  void importCollection(QString filePath);

public:
  Storage(QThread *thread,
          const QDir &directory);
  virtual ~Storage();

  operator bool() const;

  static void initInstance(const QDir &directory);
  static Storage* getInstance();
  static void clearInstance();

private:
  std::shared_ptr<std::vector<Track>> loadTracks(qint64 collectionId);
  std::shared_ptr<std::vector<Waypoint>> loadWaypoints(qint64 collectionId);
  void loadTrackPoints(qint64 segmentId, osmscout::gpx::TrackSegment &segment);
  bool checkAccess(QString slotName, bool requireOpen = true);
  bool importWaypoints(const osmscout::gpx::GpxFile &file, qint64 collectionId);
  bool importTracks(const osmscout::gpx::GpxFile &file, qint64 collectionId);
  bool importTrackPoints(const std::vector<osmscout::gpx::TrackPoint> &points, qint64 segId);

private :
  QSqlDatabase db;
  QThread *thread;
  QDir directory;
  std::atomic_bool ok{false};
};

#endif //OSMSCOUT_SAILFISH_STORAGE_H
