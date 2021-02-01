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

#include <osmscout/gpx/Track.h>
#include <osmscout/gpx/Waypoint.h>
#include <osmscout/gpx/Utils.h>
#include <osmscout/gpx/GpxFile.h>
#include <osmscout/util/GeoBox.h>

#include <QObject>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QDir>
#include <QtCore/QDateTime>

#include <atomic>
#include <optional>

class ErrorCallback: public QObject, public osmscout::gpx::ProcessCallback
{
  Q_OBJECT
  Q_DISABLE_COPY(ErrorCallback)

signals:
  void error(QString);

public:
  ErrorCallback() = default;
  virtual ~ErrorCallback() = default;

  virtual void Error(const std::string &error) override;
};

class TrackStatistics
{
public:
  TrackStatistics() = default;

  TrackStatistics(const QDateTime &from,
                  const QDateTime &to,
                  const osmscout::Distance &distance,
                  const osmscout::Distance &rawDistance,
                  const osmscout::Timestamp::duration &duration,
                  const osmscout::Timestamp::duration &movingDuration,
                  const double &maxSpeed,
                  const double &averageSpeed,
                  const double &movingAverageSpeed,
                  const osmscout::Distance &ascent,
                  const osmscout::Distance &descent,
                  const std::optional<osmscout::Distance> &minElevation,
                  const std::optional<osmscout::Distance> &maxElevation,
                  const osmscout::GeoBox &bbox):
    from(from),
    to(to),
    distance(distance),
    rawDistance(rawDistance),
    duration(duration),
    movingDuration(movingDuration),
    maxSpeed(maxSpeed),
    averageSpeed(averageSpeed),
    movingAverageSpeed(movingAverageSpeed),
    ascent(ascent),
    descent(descent),
    minElevation(minElevation),
    maxElevation(maxElevation),
    bbox(bbox)
  {};

  TrackStatistics(const TrackStatistics &o) = default;
  TrackStatistics(TrackStatistics &&o) = default;

  ~TrackStatistics() = default;

  TrackStatistics& operator=(const TrackStatistics &o) = default;
  TrackStatistics& operator=(TrackStatistics &&o) = default;

  qint64 durationMillis() const
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  }

  qint64 movingDurationMillis() const
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(movingDuration).count();
  }

  bool operator==(const TrackStatistics &o){
    bool bboxEquals = bbox.IsValid() == o.bbox.IsValid();
    if (bboxEquals && bbox.IsValid()) {
      bboxEquals = bbox.GetMinCoord() == o.bbox.GetMinCoord() && bbox.GetMaxCoord() == o.bbox.GetMaxCoord();
    }

    return from==o.from &&
           to==o.to &&
           distance==o.distance &&
           rawDistance==o.rawDistance &&
           duration==o.duration &&
           movingDuration==o.movingDuration &&
           maxSpeed==o.maxSpeed &&
           averageSpeed==o.averageSpeed &&
           movingAverageSpeed==o.movingAverageSpeed &&
           ascent==o.ascent &&
           descent==o.descent &&
           minElevation==o.minElevation &&
           maxElevation==o.maxElevation &&
           bboxEquals;
  }

public:
  QDateTime from;
  QDateTime to;
  osmscout::Distance distance;
  osmscout::Distance rawDistance;
  osmscout::Timestamp::duration duration{osmscout::Timestamp::duration::zero()};
  osmscout::Timestamp::duration movingDuration{osmscout::Timestamp::duration::zero()};
  double maxSpeed; // m/s
  double averageSpeed; // m/s
  double movingAverageSpeed; // m/s
  osmscout::Distance ascent;
  osmscout::Distance descent;
  std::optional<osmscout::Distance> minElevation;
  std::optional<osmscout::Distance> maxElevation;
  osmscout::GeoBox bbox;
};

class MaxSpeedBuffer{
public:
  MaxSpeedBuffer() = default;
  ~MaxSpeedBuffer() = default;

  void flush();
  void insert(const osmscout::gpx::TrackPoint &p);

  // return maximum computed speed in m / s
  double getMaxSpeed() const;

  void setMaxSpeed(double speed);

private:
  QList<osmscout::Distance> distanceFifo;
  QList<osmscout::Timestamp::duration> timeFifo;
  osmscout::Distance bufferDistance;
  osmscout::Timestamp::duration bufferTime{0};
  std::shared_ptr<osmscout::gpx::TrackPoint> lastPoint;
  double maxSpeed{0}; // m / s
};

class TrackStatisticsAccumulator
{
public:
  TrackStatisticsAccumulator() = default;
  TrackStatisticsAccumulator(const TrackStatisticsAccumulator &other) = default;
  TrackStatisticsAccumulator(TrackStatisticsAccumulator &&) = default;

  explicit TrackStatisticsAccumulator(const TrackStatistics &statistics);

  virtual ~TrackStatisticsAccumulator() = default;

  TrackStatisticsAccumulator &operator =(const TrackStatisticsAccumulator &other) = default;
  TrackStatisticsAccumulator &operator =(TrackStatisticsAccumulator &&other) = default;

  void update(const osmscout::gpx::TrackPoint &point);
  void segmentEnd();

  TrackStatistics accumulate() const;

  std::optional<osmscout::Timestamp> getTo() const
  {
    return to;
  };

  osmscout::Distance getLength() const
  {
    return length;
  }

private:
  // filter
  size_t rawCount{0};
  size_t filteredCnt{0};

  // accuracy filter
  double maxDilution{30};

  // distance filter
  std::optional<osmscout::gpx::TrackPoint> filterLastPoint;
  osmscout::Distance minDistance{osmscout::Meters(5)};

  // duration accumulator
  std::optional<osmscout::Timestamp> from;
  std::optional<osmscout::Timestamp> to;

  // bbox
  osmscout::GeoBox bbox;

  // distance
  std::optional<osmscout::GeoCoord> filterLastCoord;
  osmscout::Distance length;

  // raw distance
  std::optional<osmscout::GeoCoord> lastCoord;
  osmscout::Distance rawLength;

  // max speed, moving duration
  MaxSpeedBuffer maxSpeedBuf;
  std::optional<osmscout::Timestamp> previousTime;
  osmscout::Timestamp::duration movingDuration{0};

  // elevation
  std::optional<osmscout::Distance> minElevation;
  std::optional<osmscout::Distance> maxElevation;
  std::optional<double> prevElevation; // m
  double ascent=0;
  double descent=0;
};

class Track
{
public:
  Track() = default;

  Track(qint64 id, qint64 collectionId,  QString name,  QString description, bool open,
        const QDateTime &creationTime, const QDateTime &lastModification,
        const TrackStatistics &statistics):
    id(id), collectionId(collectionId), name(name), description(description), open(open),
    creationTime(creationTime), lastModification(lastModification),
    statistics(statistics)
  {};

  Track(const Track &o) = default;
  Track(Track &&o) = default;

  ~Track() = default;

  Track& operator=(const Track &o) = default;
  Track& operator=(Track &&o) = default;

public:
  qint64 id{-1};
  qint64 collectionId{-1};
  QString name;
  QString description;
  bool open{false};
  QDateTime creationTime;
  QDateTime lastModification;

  TrackStatistics statistics;
  std::shared_ptr<osmscout::gpx::Track> data;
};

class Waypoint
{
public:
  Waypoint() = default;

  Waypoint(qint64 id, const QDateTime &lastModification, const osmscout::gpx::Waypoint &data):
    id(id), lastModification(lastModification), data(data)
  {}

  Waypoint(qint64 id, const QDateTime &lastModification, osmscout::gpx::Waypoint &&data):
    id(id), lastModification(lastModification), data(std::move(data))
  {}

public:
  qint64 id{-1};
  QDateTime lastModification;
  osmscout::gpx::Waypoint data{osmscout::GeoCoord()};
};

class Collection
{
public:
  Collection() = default;
  Collection(qint64 id):
    id(id)
  {};

  Collection(qint64 id,
             bool visible,
             const QString &name,
             const QString &description):
    id(id), visible(visible), name(name), description(description)
  {};

public:
  qint64 id{-1};
  bool visible{false};
  QString name;
  QString description;

  std::shared_ptr<std::vector<Track>> tracks;
  std::shared_ptr<std::vector<Waypoint>> waypoints;
};

struct SearchItem {
  QString pattern;
  QDateTime lastUsage;
};

class Storage : public QObject{
  Q_OBJECT
  Q_DISABLE_COPY(Storage)

public:
  using QStringOpt = std::optional<QString>;
  using WaypointNearby = std::tuple<osmscout::Distance, Waypoint>;

private:
  bool updateSchema();

signals:
  void initialised();
  void initialisationError(QString error);

  void collectionsLoaded(std::vector<Collection> collections, bool ok);
  void collectionDetailsLoaded(Collection collection, bool ok);
  void trackDataLoaded(Track track, std::optional<double>, bool complete, bool ok);
  void collectionExported(bool success);
  void trackExported(bool success);

  void trackCreated(qint64 collectionId, qint64 trackId, QString name);
  void waypointCreated(qint64 collectionId, qint64 waypointId, QString name);

  void collectionDeleted(qint64 collectionId);
  void trackDeleted(qint64 collectionId, qint64 trackId);
  void waypointDeleted(qint64 collectionId, qint64 waypointId);

  void openTrackLoaded(Track track, bool ok);

  void searchHistory(std::vector<SearchItem> items);

  void nearbyWaypoints(const osmscout::GeoCoord &center, const osmscout::Distance &distance, const std::vector<WaypointNearby> &waypoints);

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
  void loadTrackData(Track track, std::optional<double> accuracyFilter);

  /**
   * update collection or create it (if id < 0)
   * emits collectionsLoaded signal
   */
  void updateOrCreateCollection(Collection collection);

  /**
   * delete collection
   * emits collectionsLoaded, collectionDeleted
   */
  void deleteCollection(qint64 id);

  /**
   * import collection from gpx file
   * emits collectionsLoaded signal
   */
  void importCollection(QString filePath);

  /**
   * delete waypoint
   * emits collectionDetailsLoaded, waypointDeleted
   */
  void deleteWaypoint(qint64 collectionId, qint64 waypointId);

  /**
   * delete waypoint
   * emits collectionDetailsLoaded, trackDeleted
   */
  void deleteTrack(qint64 collectionId, qint64 trackId);

  /**
   * close track
   * emits collectionDetailsLoaded
   */
  void closeTrack(qint64 collectionId, qint64 trackId);

  /**
   * create waypoint
   * emits waypointCreated (or error), collectionDetailsLoaded
   */
  void createWaypoint(qint64 collectionId, double lat, double lon, QString name, QString description);

  /**
   * create empty track
   * emits trackCreated (or error), collectionDetailsLoaded
   */
  void createTrack(qint64 collectionId, QString name, QString description, bool open);

  /**
   * edit waypoint
   * emits collectionDetailsLoaded
   */
  void editWaypoint(qint64 collectionId, qint64 id, QString name, QString description);

  /**
   * edit track
   * emits collectionDetailsLoaded
   */
  void editTrack(qint64 collectionId, qint64 id, QString name, QString description);

  /**
   * emits collectionExported
   */
  void exportCollection(qint64 collectionId, QString file, bool includeWaypoints, std::optional<double> accuracyFilter);

  /**
   * emits trackExported
   */
  void exportTrack(qint64 collectionId, qint64 trackId, QString file, bool includeWaypoints, std::optional<double> accuracyFilter);

  /**
   * emit collectionDetailsLoaded for source and target collection
   *
   * @param waypointId
   * @param collectionId
   */
  void moveWaypoint(qint64 waypointId, qint64 collectionId);
  void moveTrack(qint64 trackId, qint64 collectionId);

  /**
   * emits collectionDetailsLoaded and trackDataLoaded
   *
   * @param track
   * @param position (exclusive, point on that position, and following is keep)
   */
  void cropTrackStart(Track track, quint64 position);

  /**
   * emits collectionDetailsLoaded and trackDataLoaded
   *
   * @param track
   * @param position (inclusive, point on that position and following is removed)
   */
  void cropTrackEnd(Track track, quint64 position);

  /**
   * emits collectionDetailsLoaded and trackDataLoaded (2x)
   *
   * @param track
   * @param position (exclusive, point on that position is keep)
   */
  void splitTrack(Track track, quint64 position);

  /**
   * emits collectionDetailsLoaded and trackDataLoaded
   *
   * @param track
   * @param accuracyFilter
   */
  void filterTrackNodes(Track track, std::optional<double> accuracyFilter);

  /**
   * emit openTrackLoaded()
   */
  void loadRecentOpenTrack();

  /**
   * Append batch of nodes to last segment in track,
   * update track statistics.
   * Possibly create new segment when "createNewSegment" is true
   *
   * emit collectionDetailsLoaded
   */
  void appendNodes(qint64 trackId,
                   std::shared_ptr<std::vector<osmscout::gpx::TrackPoint>> batch,
                   TrackStatistics statistics,
                   bool createNewSegment);

  /**
   * emit searchHistory
   */
  void loadSearchHistory();

  /**
   * add search pattern to search history or update its lastUsage
   * emit searchHistory
   * @param pattern
   */
  void addSearchPattern(QString pattern);

  /**
   * remove search pattern from search history
   * emit searchHistory
   * @param pattern
   */
  void removeSearchPattern(QString pattern);

  /**
   * request for loading nearby waypoints
   *
   * emit nearbyWaypoints
   * @param center
   * @param distance
   */
  void loadNearbyWaypoints(const osmscout::GeoCoord &center, const osmscout::Distance &distance);

public:
  Storage(QThread *thread,
          const QDir &directory);
  virtual ~Storage();

  operator bool() const;

  static void initInstance(const QDir &directory);
  static Storage* getInstance();
  static void clearInstance();

private:
  QSqlQuery trackInsertSql();

  void prepareTrackInsert(QSqlQuery &sqlTrk,
                          qint64 collectionId,
                          const QString &trackName,
                          const QStringOpt &desc,
                          const TrackStatistics &stat,
                          bool open);

  Track makeTrack(QSqlQuery &sqlTrack) const;
  Waypoint makeWaypoint(QSqlQuery &sql) const;
  std::shared_ptr<std::vector<Track>> loadTracks(qint64 collectionId);
  std::shared_ptr<std::vector<Waypoint>> loadWaypoints(qint64 collectionId);
  void loadTrackPoints(qint64 segmentId, osmscout::gpx::TrackSegment &segment);
  bool checkAccess(QString slotName, bool requireOpen = true);
  bool importWaypoints(const osmscout::gpx::GpxFile &file, qint64 collectionId);
  bool importTracks(const osmscout::gpx::GpxFile &file, qint64 collectionId);
  bool importTrackPoints(const std::vector<osmscout::gpx::TrackPoint> &points, qint64 segId);
  TrackStatistics computeTrackStatistics(const osmscout::gpx::Track &trk) const;
  bool loadCollectionDetailsPrivate(Collection &collection);
  bool loadTrackDataPrivate(Track &track, std::optional<double> accuracyFilter);
  bool createSegment(qint64 trackId, qint64 &segmentId);
  bool exportPrivate(qint64 collectionId,
                     const QString &file,
                     const std::optional<qint64> &trackId,
                     bool includeWaypoints,
                     std::optional<double> accuracyFilter);

  /**
   * obtain collection id from trackId
   */
  bool trackCollection(qint64 trackId, qint64 &collectionId);
  bool listIndexes(QStringList &indexes);
  int querySize(QSqlQuery &query);

  void cropTrackPrivate(qint64 trackId, quint64 count, bool cropStart);
  bool updateTrackStatistics(qint64 trackId, const TrackStatistics &statistics);

private :
  QSqlDatabase db;
  QThread *thread;
  QDir directory;
  std::atomic_bool ok{false};
};
