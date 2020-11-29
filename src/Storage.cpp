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

#include "Storage.h"
#include "QVariantConverters.h"

#include <osmscout/OSMScoutQt.h>
#include <osmscout/gpx/GpxFile.h>
#include <osmscout/gpx/Import.h>
#include <osmscout/gpx/Export.h>

#include <QDebug>
#include <QThread>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

namespace {
  static constexpr int DbSchema = 2;
  static constexpr int TrackPointBatchSize = 10000;
  static constexpr int WayPointBatchSize = 100;

  double durationSeconds(const osmscout::Timestamp::duration &d)
  {
    using namespace std::chrono;
    return duration_cast<duration<double,std::ratio<1,1>>>(d).count();
  }
}

using namespace osmscout;
using namespace converters;

static Storage* storage = nullptr;

void ErrorCallback::Error(const std::string &err)
{
  gpx::ProcessCallback::Error(err);
  emit error(QString::fromStdString(err));
}

void MaxSpeedBuffer::flush()
{
  lastPoint.reset();
  bufferTime.zero();
  bufferDistance = Distance::Of<Meter>(0);
}

void MaxSpeedBuffer::insert(const gpx::TrackPoint &p)
{
  if (!p.time){
    return;
  }
  if (lastPoint){
    Timestamp::duration timeDiff = *(p.time) - *(lastPoint->time);
    if (timeDiff.count() < 0){
      qWarning() << "Traveling in time is not supported";
      return;
    }
    Distance distanceDiff = GetEllipsoidalDistance(lastPoint->coord, p.coord);
    distanceFifo.push_back(distanceDiff);
    timeFifo.push_back(timeDiff);
    bufferDistance += distanceDiff;
    bufferTime += timeDiff;

    while (bufferTime > std::chrono::seconds(5) && !distanceFifo.empty()){
      double speed = bufferDistance.AsMeter() / durationSeconds(bufferTime);
      maxSpeed = std::max(maxSpeed, speed);
      bufferDistance = bufferDistance - distanceFifo.front(); // it can be inaccurate!
      bufferTime -= timeFifo.front();
      distanceFifo.pop_front();
      timeFifo.pop_front();
    }
  }
  lastPoint=std::make_shared<osmscout::gpx::TrackPoint>(p);
}

double MaxSpeedBuffer::getMaxSpeed() const
{
  return maxSpeed;
}

void MaxSpeedBuffer::setMaxSpeed(double speed)
{
  maxSpeed=speed;
}

namespace {
QString sqlCreateCollection() {
  QString sql("CREATE TABLE `collection`");
  sql.append("(").append( "`id` INTEGER PRIMARY KEY");
  sql.append(",").append( "`name` varchar(255) NOT NULL ");
  sql.append(",").append( "`description` varchar(255) NULL ");
  sql.append(",").append( "`visible` tinyint(1) NOT NULL");
  sql.append(");");
  return sql;
}

QString sqlCreateTrack(){
  QString sql("CREATE TABLE `track`");
  sql.append("(").append( "`id` INTEGER PRIMARY KEY");
  sql.append(",").append( "`collection_id` INTEGER NOT NULL REFERENCES collection(id) ON DELETE CASCADE");
  sql.append(",").append( "`name` varchar(255) NOT NULL");
  sql.append(",").append( "`description` varchar(255) NULL");
  sql.append(",").append( "`open` tinyint(1) NOT NULL");
  sql.append(",").append( "`creation_time` datetime NOT NULL");
  sql.append(",").append( "`modification_time` datetime NOT NULL");

  // statistics
  sql.append(",").append( "`from_time` datetime NULL");
  sql.append(",").append( "`to_time` datetime NULL");
  sql.append(",").append( "`distance` DOUBLE NOT NULL");
  sql.append(",").append( "`raw_distance` DOUBLE NOT NULL");
  sql.append(",").append( "`duration` INTEGER NOT NULL");
  sql.append(",").append( "`moving_duration` INTEGER NOT NULL");
  sql.append(",").append( "`max_speed` DOUBLE NOT NULL");
  sql.append(",").append( "`average_speed` DOUBLE NOT NULL");
  sql.append(",").append( "`moving_average_speed` DOUBLE NOT NULL");
  sql.append(",").append( "`ascent` DOUBLE NOT NULL");
  sql.append(",").append( "`descent` DOUBLE NOT NULL");
  sql.append(",").append( "`min_elevation` DOUBLE NULL");
  sql.append(",").append( "`max_elevation` DOUBLE NULL");

  // bbox
  sql.append(",").append( "`bbox_min_lat` DOUBLE NOT NULL");
  sql.append(",").append( "`bbox_min_lon` DOUBLE NOT NULL");
  sql.append(",").append( "`bbox_max_lat` DOUBLE NOT NULL");
  sql.append(",").append( "`bbox_max_lon` DOUBLE NOT NULL");

  sql.append(");");

  return sql;
}

QString sqlCreateTrackSegment(){
  QString sql("CREATE TABLE `track_segment`");
  sql.append("(").append( "`id` INTEGER PRIMARY KEY");
  sql.append(",").append( "`track_id` INTEGER NOT NULL REFERENCES track(id) ON DELETE CASCADE");
  sql.append(",").append( "`open` tinyint(1) NOT NULL");
  sql.append(",").append( "`creation_time` datetime NOT NULL");
  sql.append(",").append( "`distance` double NOT NULL");
  sql.append(");");
  return sql;
}

QString sqlCreateTrackPoint(){
  QString sql("CREATE TABLE `track_point`");
  sql.append("(").append( "`segment_id` INTEGER NOT NULL REFERENCES track_segment(id) ON DELETE CASCADE");
  sql.append(",").append( "`timestamp` datetime NULL");
  sql.append(",").append( "`latitude` double NOT NULL");
  sql.append(",").append( "`longitude` double NOT NULL");
  sql.append(",").append( "`elevation` double NULL ");
  sql.append(",").append( "`horiz_accuracy` double NULL ");
  sql.append(",").append( "`vert_accuracy` double NULL ");
  sql.append(");");

  // TODO: what satelites and compas?

  return sql;
}

QString sqlCreateWaypoint(){
  QString sql("CREATE TABLE `waypoint`");
  sql.append("(").append( "`id` INTEGER PRIMARY KEY");
  sql.append(",").append( "`collection_id` INTEGER NOT NULL REFERENCES collection(id) ON DELETE CASCADE");
  sql.append(",").append( "`modification_time` datetime NOT NULL");
  sql.append(",").append( "`timestamp` datetime NULL");
  sql.append(",").append( "`latitude` double NOT NULL");
  sql.append(",").append( "`longitude` double NOT NULL");
  sql.append(",").append( "`elevation` double NULL");
  sql.append(",").append( "`name` varchar(255) NOT NULL ");
  sql.append(",").append( "`description` varchar(255) NULL ");
  sql.append(",").append( "`symbol` varchar(255) NULL ");
  sql.append(");");

  return sql;
}

QString sqlCreateSearchHistory(){
  QString sql("CREATE TABLE `search_history` ");
  sql.append("(").append( "`pattern` varchar(255) NOT NULL PRIMARY KEY");
  sql.append(",").append( "`last_usage` datetime NOT NULL");
  sql.append(");");

  return sql;
}
}

Storage::Storage(QThread *thread,
                 const QDir &directory)
  :thread(thread),
   directory(directory)
{
}

Storage::~Storage()
{
  if (thread != QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread != nullptr){
    thread->quit();
  }

  if (db.isValid()) {
    if (db.isOpen()) {
      db.close();
    }
    db = QSqlDatabase(); // invalidate instance
    QSqlDatabase::removeDatabase("storage");
  }
  qDebug() << "Storage is closed";
}

bool Storage::updateSchema(){
  int currentSchema=0;
  QStringList tables = db.tables();

  if (tables.contains("version")){
    QString sql("SELECT MAX(`version`) AS `currentVersion` FROM `version`;");

    QSqlQuery q = db.exec(sql);
    if (!q.lastError().isValid()){
      if (q.next()){
        QVariant val = q.value(0);
        if (!val.isNull()){
          bool ok = false;
          currentSchema = val.toInt(&ok);
          if (!ok) {
            qWarning() << "Loading currentSchema value failed" << val;
          }
          qDebug() << "last schema: " << currentSchema;
        }
      } else {
        qWarning() << "Loading currentSchema value failed (no entry)";
      }
    }else{
      qWarning() << "failed to get schema version " << q.lastError();
    }
  }

  if (!tables.contains("version")){
    QString sql("CREATE TABLE `version` ( `version` int NOT NULL);");
    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()){
      qWarning() << "Creating version table failed" << q.lastError();
      db.close();
      return false;
    }

    QSqlQuery sqlInsert(db);
    sqlInsert.prepare("INSERT INTO `version` (`version`) VALUES (:version);");
    sqlInsert.bindValue(":version", DbSchema);
    sqlInsert.exec();
    if (sqlInsert.lastError().isValid()){
      qWarning() << "Creating version table entry failed" << sqlInsert.lastError();
      db.close();
      return false;
    }
    currentSchema = DbSchema;
  }

  // upgrade schema
  QStringList updateQueries;
  if (currentSchema < 2){
    // from schema v2 may be timestamps null

    // alter track_point
    updateQueries << "ALTER TABLE `track_point` RENAME TO `_track_point`";
    updateQueries << sqlCreateTrackPoint();
    updateQueries << "INSERT INTO `track_point` SELECT * FROM `_track_point`";
    updateQueries << "DROP TABLE `_track_point`";

    // alter waypoint
    updateQueries << "ALTER TABLE `waypoint` RENAME TO `_waypoint`";
    updateQueries << sqlCreateWaypoint();
    updateQueries << "INSERT INTO `waypoint` SELECT * FROM `_waypoint`";
    updateQueries << "DROP TABLE `_waypoint`";

    updateQueries << "INSERT INTO `version` (`version`) VALUES (2)";
    currentSchema = 2;
  }

  // do upgrade commands
  if (!updateQueries.empty()) {
    db.transaction();
    updateQueries.prepend("PRAGMA foreign_keys=off");
    updateQueries << "PRAGMA foreign_keys=on";

    for (QString query: updateQueries) {
      QSqlQuery sqlUpdate(db);
      sqlUpdate.prepare(query);
      qDebug() << "Update database:" << query;
      sqlUpdate.exec();
      if (sqlUpdate.lastError().isValid()) {
        qWarning() << "Update database failed" << sqlUpdate.lastError();
        db.rollback();
        db.close();
        return false;
      }
    }
    if (!db.commit()) {
      qWarning() << "Commit database update failed";
      db.close();
      return false;
    }
  }

  if (currentSchema > DbSchema) {
    qWarning() << "newer database schema; " << currentSchema << " > " << DbSchema;
  }

  tables = db.tables();
  if (!tables.contains("collection")){
    qDebug()<< "creating collection table";

    QSqlQuery q = db.exec(sqlCreateCollection());
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating collection table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("track")){
    qDebug()<< "creating track table";

    QSqlQuery q = db.exec(sqlCreateTrack());
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating track table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("track_segment")){
    qDebug()<< "creating track_segment table";

    QSqlQuery q = db.exec(sqlCreateTrackSegment());
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating track segment table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("track_point")){
    qDebug()<< "creating track_point table";

    QSqlQuery q = db.exec(sqlCreateTrackPoint());
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating track node table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("waypoint")){
    qDebug()<< "creating waypoints table";

    QSqlQuery q = db.exec(sqlCreateWaypoint());
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating waypoints table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("search_history")){
    qDebug()<< "creating search_history table";

    QSqlQuery q = db.exec(sqlCreateSearchHistory());
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating search_history table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  QStringList indexes;
  if (!listIndexes(indexes)){
    qWarning() << "Storage: cannot load indexes";
    db.close();
    return false;
  }

  if (!indexes.contains("idx_track_point_segment_id")){
    qDebug() << "creating idx_track_point_segment_id index";

    QSqlQuery q = db.exec("CREATE INDEX idx_track_point_segment_id ON track_point (segment_id)");
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating idx_track_point_segment_id index failed" << q.lastError();
      db.close();
      return false;
    }
  }

  return true;
}

bool Storage::listIndexes(QStringList &indexes)
{
  QString sql("SELECT name FROM sqlite_master WHERE type = 'index';");

  QSqlQuery q = db.exec(sql);
  if (q.lastError().isValid()) {
    qWarning() << "Storage: cannot load indexes" << q.lastError();
    return false;
  }
  while (q.next()) {
    indexes << varToString(q.value("name"));
  }
  return true;
}

void Storage::init()
{
  if (!checkAccess("init", false)){
    return;
  }

  // Find QSLite driver
  db = QSqlDatabase::addDatabase("QSQLITE", "storage");
  if (!db.isValid()){
    qWarning() << "Could not find QSQLITE backend";
    emit initialisationError("Could not find QSQLITE backend");
    return;
  }
  QString path=directory.path();
  if (!directory.mkpath(path)){
    qWarning() << "Failed to create directory" << directory;
    emit initialisationError(QString("Failed to create directory ") + directory.path());
    return;
  }
  path.append(QDir::separator()).append("storage.db");
  path = QDir::toNativeSeparators(path);
  db.setDatabaseName(path);

  if (!db.open()){
    qWarning() << "Open database failed" << db.lastError();
    emit initialisationError(db.lastError().text());
    return;
  }
  qDebug() << "Storage database opened:" << path;
  if (!updateSchema()){
    emit initialisationError("update schema");
    return;
  }

  QSqlQuery q = db.exec("PRAGMA foreign_keys = ON;");
  if (q.lastError().isValid()){
    qWarning() << "Enabling foreign keys fails:" << q.lastError();
  }

  ok = db.isValid() && db.isOpen();
  emit initialised();
}

bool Storage::checkAccess(QString slotName, bool requireOpen)
{
  if (thread != QThread::currentThread()){
    qWarning() << this << "::" << slotName << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
    return false;
  }
  if (requireOpen && !ok){
    qWarning() << "Database is not open, " << this << "::" << slotName << "";
    return false;
  }
  return true;
}

void Storage::loadCollections()
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionsLoaded(std::vector<Collection>(), false);
    return;
  }

  QString sql("SELECT `id`, `visible`, `name`, `description` FROM `collection`;");

  QSqlQuery q = db.exec(sql);
  if (q.lastError().isValid()) {
    emit collectionsLoaded(std::vector<Collection>(), false);
  }
  std::vector<Collection> result;
  while (q.next()) {
    result.emplace_back(
      varToLong(q.value("id")),
      varToBool(q.value("visible")),
      varToString(q.value("name")),
      varToString(q.value("description"))
    );
  }
  emit collectionsLoaded(result, true);
}

Track Storage::makeTrack(QSqlQuery &sqlTrack) const
{
  GeoBox bbox(GeoCoord(varToDouble(sqlTrack.value("bbox_min_lat")),
                       varToDouble(sqlTrack.value("bbox_min_lon"))),
              GeoCoord(varToDouble(sqlTrack.value("bbox_max_lat")),
                       varToDouble(sqlTrack.value("bbox_max_lon"))));

  if (bbox.GetMinCoord().GetLat() < -90 || bbox.GetMinCoord().GetLon() < -180){
    bbox.Invalidate();
  }

  return Track(varToLong(sqlTrack.value("id")),
               varToLong(sqlTrack.value("collection_id")),
               varToString(sqlTrack.value("name")),
               varToString(sqlTrack.value("description")),
               varToBool(sqlTrack.value("open")),
               varToDateTime(sqlTrack.value("creation_time")),
               varToDateTime(sqlTrack.value("modification_time")),
               TrackStatistics(
                 varToDateTime(sqlTrack.value("from_time")),
                 varToDateTime(sqlTrack.value("to_time")),
                 Distance::Of<Meter>(varToDouble(sqlTrack.value("distance"))),
                 Distance::Of<Meter>(varToDouble(sqlTrack.value("raw_distance"))),
                 std::chrono::milliseconds(varToLong(sqlTrack.value("duration"))),
                 std::chrono::milliseconds(varToLong(sqlTrack.value("moving_duration"))),
                 varToDouble(sqlTrack.value("max_speed")),
                 varToDouble(sqlTrack.value("average_speed")),
                 varToDouble(sqlTrack.value("moving_average_speed")),
                 Distance::Of<Meter>(varToDouble(sqlTrack.value("ascent"))),
                 Distance::Of<Meter>(varToDouble(sqlTrack.value("descent"))),
                 varToDistanceOpt(sqlTrack.value("min_elevation")),
                 varToDistanceOpt(sqlTrack.value("max_elevation")),

                 bbox));
}

std::shared_ptr<std::vector<Track>> Storage::loadTracks(qint64 collectionId)
{
  QSqlQuery sqlTrack(db);
  sqlTrack.prepare("SELECT * FROM `track` WHERE collection_id = :collectionId;");
  sqlTrack.bindValue(":collectionId", collectionId);
  sqlTrack.exec();

  if (sqlTrack.lastError().isValid()) {
    qWarning() << "Loading tracks for collection id" << collectionId << "fails";
    emit error(tr("Loading tracks for collection id %1 fails").arg(collectionId));
    return nullptr;
  }

  std::shared_ptr<std::vector<Track>> result = std::make_shared<std::vector<Track>>();
  while (sqlTrack.next()) {
    result->emplace_back(makeTrack(sqlTrack));
  }
  return result;
}

std::shared_ptr<std::vector<Waypoint>> Storage::loadWaypoints(qint64 collectionId)
{
  QSqlQuery sql(db);
  sql.prepare("SELECT `id`, `name`, `description`, `symbol`, `timestamp`, `modification_time`, `latitude`, `longitude`, `elevation` FROM `waypoint` WHERE collection_id = :collectionId;");
  sql.bindValue(":collectionId", collectionId);
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Loading waypoints for collection id" << collectionId << "fails";
    emit error(tr("Loading waypoints for collection id %1 fails").arg(collectionId));
    return nullptr;
  }

  std::shared_ptr<std::vector<Waypoint>> result = std::make_shared<std::vector<Waypoint>>();
  while (sql.next()) {
    gpx::Waypoint wpt(GeoCoord(
      varToDouble(sql.value("latitude")),
      varToDouble(sql.value("longitude"))
      ));

    wpt.name = varToStringOpt(varToString(sql.value("name")));
    wpt.description = varToStringOpt(sql.value("description"));
    wpt.symbol = varToStringOpt(sql.value("symbol"));

    QVariant timestampVar = sql.value("timestamp");
    if (!timestampVar.isNull()) {
      wpt.time = dateTimeToTimestamp(varToDateTime(timestampVar));
    }
    wpt.elevation = varToDoubleOpt(sql.value("elevation"));

    result->emplace_back(varToLong(sql.value("id")), varToDateTime(sql.value("modification_time")), std::move(wpt));
  }
  return result;
}

bool Storage::loadCollectionDetailsPrivate(Collection &collection)
{
  QSqlQuery sql(db);
  sql.prepare("SELECT `name`, `description`, `visible` FROM `collection` WHERE id = :collectionId;");
  sql.bindValue(":collectionId", collection.id);
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Loading collection id" << collection.id << "fails";
    emit error(tr("Loading collection id %1 fails").arg(collection.id));
    return false;
  }
  std::vector<Collection> result;
  if (!sql.next()) {
    qWarning() << "Collection id" << collection.id << "don't exists";
    emit error(tr("Collection id %1 don't exists").arg(collection.id));
    return false;
  }

  collection.name = varToString(sql.value("name"));
  collection.description = varToString(sql.value("description"));
  collection.visible = varToBool(sql.value("visible"));

  collection.tracks = loadTracks(collection.id);
  collection.waypoints = loadWaypoints(collection.id);
  return true;
}

void Storage::loadCollectionDetails(Collection collection)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionDetailsLoaded(collection, false);
    return;
  }

  if (loadCollectionDetailsPrivate(collection)) {
    emit collectionDetailsLoaded(collection, true);
  }else{
    emit collectionDetailsLoaded(collection, false);
  }
}

// QSqlQuery::size() is not supported with SQLite.
// But you can get the number of rows with a workaround
// https://stackoverflow.com/questions/26495049/qsqlquery-size-always-returns-1
int Storage::querySize(QSqlQuery &query)
{
  int initialPos = query.at();
  int size = 0;
  if (query.last()) {
    size = query.at() + 1;
  } else {
    size = 0;
  }
  // Important to restore initial pos
  query.seek(initialPos);
  return size;
}

void Storage::loadTrackPoints(qint64 segmentId, gpx::TrackSegment &segment)
{
  // QTime timer;
  // timer.start();
  QSqlQuery sql(db);
  sql.prepare("SELECT CAST(STRFTIME('%s',`timestamp`, 'UTC') AS INTEGER) AS `timestamp`, `latitude`, `longitude`, `elevation`, `horiz_accuracy`, `vert_accuracy` FROM `track_point` WHERE segment_id = :segmentId;");
  sql.bindValue(":segmentId", segmentId);
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Loading nodes for segment id" << segmentId << "failed";
    emit error(tr("Loading nodes for segment id %1 failed: %2").arg(segmentId).arg(sql.lastError().text()));
    return;
  }

  // qDebug() << "    segment" << segmentId << "sql:" << timer.elapsed() << "ms";

  auto size = querySize(sql);
  // qDebug() << "    segment" << segmentId << "size:" << timer.elapsed() << "ms";
  assert(size>=0);
  segment.points.reserve(size);

  // qDebug() << "    segment" << segmentId << "alloc:" << timer.elapsed() << "ms";

  QSqlRecord record = sql.record();
  int iLatitude = record.indexOf("latitude");
  int iLongitude = record.indexOf("longitude");
  int iTimestamp = record.indexOf("timestamp");
  int iElevation = record.indexOf("elevation");
  int iHorizAcc = record.indexOf("horiz_accuracy");
  int iVertAcc = record.indexOf("vert_accuracy");

  while (sql.next()) {
    segment.points.emplace_back(GeoCoord(
      varToDouble(sql.value(iLatitude)),
      varToDouble(sql.value(iLongitude))
    ));
    gpx::TrackPoint &point = segment.points.back();

    point.time = varLongToOptTimestamp(sql.value(iTimestamp));
    point.elevation = varToDoubleOpt(sql.value(iElevation));

    // see TrackPoint notes
    point.hdop = varToDoubleOpt(sql.value(iHorizAcc));
    point.vdop = varToDoubleOpt(sql.value(iVertAcc));
  }
  // qDebug() << "    segment" << segmentId << "loading:" << timer.elapsed() << "ms";
}

bool Storage::loadTrackDataPrivate(Track &track)
{
  qDebug() << "Loading track data" << track.id;
  QTime timer;
  timer.start();

  QSqlQuery sqlTrack(db);
  sqlTrack.prepare("SELECT * FROM `track` WHERE id = :trackId;");
  sqlTrack.bindValue(":trackId", track.id);
  sqlTrack.exec();

  // qDebug() << "  track sql" << track.id << ":" << timer.elapsed() << "ms";

  if (sqlTrack.lastError().isValid()) {
    qWarning() << "Loading track id" << track.id << "fails: " << sqlTrack.lastError();
    emit error(tr("Loading track id %1 fails").arg(track.id));
    return false;
  }

  if (!sqlTrack.next()) {
    qWarning() << "Track id" << track.id << "don't exists";
    emit error(tr("Track id %1 don't exists").arg(track.id));
    return false;
  }

  track = makeTrack(sqlTrack);
  // qDebug() << "  make track" << track.id << ":" << timer.elapsed() << "ms";

  emit trackDataLoaded(track, false, true);

  track.data = std::make_shared<gpx::Track>();

  track.data->name = track.name.toStdString();
  if (!track.description.isEmpty()) {
    track.data->desc = track.description.toStdString();
  }

  QSqlQuery sql(db);
  sql.prepare("SELECT `id` FROM `track_segment` WHERE track_id = :trackId;");
  sql.bindValue(":trackId", track.id);
  sql.exec();

  // qDebug() << "  track_segment sql" << track.id << ":" << timer.elapsed() << "ms";
  if (sql.lastError().isValid()) {
    qWarning() << "Loading segments for track id" << track.id << "failed";
    emit error(tr("Loading segments for track id %1 failed: %2").arg(track.id).arg(sql.lastError().text()));
  }else{
    while (sql.next()) {
      track.data->segments.emplace_back();
      long segmentId = varToLong(sql.value("id"));
      // qDebug() << "  track_segment " << segmentId << "before:" << timer.elapsed() << "ms";
      loadTrackPoints(segmentId, track.data->segments.back());
      // qDebug() << "  track_segment " << segmentId << "after:" << timer.elapsed() << "ms";
    }
  }

  qDebug() << "  track" << track.id << "data loading:" << timer.elapsed() << "ms";
  return true;
}

void Storage::loadTrackData(Track track)
{
  if (!checkAccess(__FUNCTION__)){
    emit trackDataLoaded(track, true, false);
    return;
  }

  if (loadTrackDataPrivate(track)) {
    emit trackDataLoaded(track, true, true);
  }else{
    emit trackDataLoaded(track, true, false);
  }
}

void Storage::updateOrCreateCollection(Collection collection)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionsLoaded(std::vector<Collection>(), false);
    return;
  }

  QSqlQuery sql(db);
  if (collection.id < 0){
    sql.prepare(
      "INSERT INTO `collection` (`name`, `description`, `visible`) VALUES (:name, :description, :visible);");
    sql.bindValue(":name", collection.name);
    sql.bindValue(":description", collection.description);
    sql.bindValue(":visible", collection.visible);
  }else {
    sql.prepare(
      "UPDATE `collection` SET `name` = :name, `description` = :description, `visible` = :visible WHERE (`id` = :id);");
    sql.bindValue(":id", collection.id);
    sql.bindValue(":name", collection.name);
    sql.bindValue(":description", collection.description);
    sql.bindValue(":visible", collection.visible);
  }
  sql.exec();
  if (sql.lastError().isValid()){
    if (collection.id < 0) {
      qWarning() << "Creating collection failed: " << sql.lastError();
      emit error(tr("Creating collection failed: %1").arg(sql.lastError().text()));
    }else {
      qWarning() << "Updating collection failed: " << sql.lastError();
      emit error(tr("Updating collection failed: %1").arg(sql.lastError().text()));
    }
  } else {
    if (collection.id < 0){
      collection.id = varToLong(sql.lastInsertId());
    }
  }

  loadCollections();
  loadCollectionDetails(Collection(collection.id));
}

void Storage::deleteCollection(qint64 id)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionsLoaded(std::vector<Collection>(), false);
    return;
  }

  QSqlQuery sql(db);
  sql.prepare(
    "DELETE FROM `collection` WHERE (`id` = :id)");
  sql.bindValue(":id", id);
  sql.exec();
  if (sql.lastError().isValid()){
    qWarning() << "Deleting collection failed: " << sql.lastError();
    emit error(tr("Deleting collection failed: %1").arg(sql.lastError().text()));
  } else {
    emit collectionDeleted(id);
  }

  loadCollections();
}

bool Storage::importWaypoints(const gpx::GpxFile &gpxFile, qint64 collectionId)
{
  using namespace std::string_literals;

  int wptNum = 0;
  db.transaction();
  QSqlQuery sqlWpt(db);
  sqlWpt.prepare(
    "INSERT INTO `waypoint` (`collection_id`, `timestamp`, `modification_time`, `latitude`, `longitude`, `elevation`, `name`, `description`, `symbol`) "
    "VALUES                 (:collection_id,  :timestamp,  :modification_time,  :latitude,  :longitude,  :elevation,  :name,  :description,  :symbol)");
  for (const auto &wpt: gpxFile.waypoints) {
    wptNum++;

    QString wptName = QString::fromStdString(wpt.name.value_or(""s));
    if (wptName.isEmpty())
      wptName = tr("waypoint %1").arg(wptNum);

    sqlWpt.bindValue(":collection_id", collectionId);
    sqlWpt.bindValue(":timestamp", dateTimeToSQL(timestampToDateTime(wpt.time)));
    sqlWpt.bindValue(":modification_time", dateTimeToSQL(QDateTime::currentDateTime()));
    sqlWpt.bindValue(":latitude", wpt.coord.GetLat());
    sqlWpt.bindValue(":longitude", wpt.coord.GetLon());
    sqlWpt.bindValue(":elevation", (wpt.elevation ? *wpt.elevation : QVariant()));
    sqlWpt.bindValue(":name", wptName);
    sqlWpt.bindValue(":description",
                     (wpt.description ? QString::fromStdString(*wpt.description) : QVariant()));
    sqlWpt.bindValue(":symbol", (wpt.symbol ? QString::fromStdString(*wpt.symbol) : QVariant()));

    sqlWpt.exec();
    if (sqlWpt.lastError().isValid()) {
      qWarning() << "Import of waypoints failed" << sqlWpt.lastError();
      emit error(tr("Import of waypoints failed: %1").arg(sqlWpt.lastError().text()));
      if (!db.rollback()) {
        qWarning() << "Transaction rollback failed" << db.lastError();
      }
      return false;
    }
    // commit batch WayPointBatchSize queries
    if (wptNum % WayPointBatchSize == 0) {
      if (!db.commit()) {
        emit error(tr("Transaction commit failed: %1").arg(db.lastError().text()));
        qWarning() << "Transaction commit failed" << db.lastError();
        return false;
      }
      db.transaction();
    }
  }
  if (!db.commit()) {
    emit error(tr("Transaction commit failed: %1").arg(db.lastError().text()));
    qWarning() << "Transaction commit failed" << db.lastError();
    return false;
  }
  return true;
}

TrackStatisticsAccumulator::TrackStatisticsAccumulator(const TrackStatistics &statistics):
  // duration accumulator
  from{dateTimeToTimestampOpt(statistics.from)},
  to{dateTimeToTimestampOpt(statistics.to)},
  // bbox
  bbox{statistics.bbox},
  // distance
  length{statistics.distance},
  // raw distance
  rawLength{statistics.rawDistance},
  // moving duration
  movingDuration{statistics.movingDuration},
  // elevation
  minElevation{statistics.minElevation},
  maxElevation{statistics.maxElevation},
  ascent{statistics.ascent.AsMeter()},
  descent{statistics.descent.AsMeter()}
{
  maxSpeedBuf.setMaxSpeed(statistics.maxSpeed);
}

void TrackStatisticsAccumulator::update(const osmscout::gpx::TrackPoint &p)
{
  // filter inaccurate points
  bool filter=true;
  if (filter && p.hdop.has_value() && *(p.hdop) > maxDilution){
    filter=false;
  }
  if (filter && p.pdop.has_value() && *(p.pdop) > maxDilution){
    filter=false;
  }

  // filter near points
  if (filter && filterLastPoint.has_value()){
    Distance distance=GetEllipsoidalDistance(filterLastPoint->coord, p.coord);
    if (distance < minDistance) {
      filter=false;
    }
  }
  if (filter) {
    filterLastPoint = p;
    filteredCnt++;
  }
  rawCount++;

  // time computation
  if (p.time.has_value()){
    to=p.time;
    if (!from.has_value()){
      from=to;
    }
  }

  // bbox
  bbox.Include(GeoBox(p.coord, p.coord));

  // distance
  if (filter) {
    if (filterLastCoord.has_value()) {
      length+=GetEllipsoidalDistance(*filterLastCoord, p.coord);
    }
    filterLastCoord = p.coord;
  }
  if (lastCoord) {
    rawLength+=GetEllipsoidalDistance(*lastCoord, p.coord);
  }
  lastCoord = p.coord;

  // max speed
  if (filter && p.time) {
    if (previousTime) {
      maxSpeedBuf.insert(p);
      auto diff = *(p.time) - *previousTime;
      if (diff < std::chrono::minutes(5)) {
        movingDuration += diff;
      }
    }
    previousTime=p.time;
  }

  // elevation
  if (filter && p.elevation && (!p.vdop || *(p.vdop) < 50.0)){

    // min/max
    double current = *p.elevation;
    if (!minElevation.has_value() || minElevation->AsMeter() > current){
      minElevation = Meters(current);
    }
    if (!maxElevation.has_value() || maxElevation->AsMeter() < current){
      maxElevation = Meters(current);
    }

    // ascent / descent
    if (!prevElevation.has_value()) {
      prevElevation=p.elevation;
    } else {
      double previous=*prevElevation;
      // when difference is less than 9 meters, don't count to ascent/descent
      // this threshold was experimentally set to get similar ascent like on strava.com :-)
      if (std::abs(current - previous) >= 9.0) {
        if (current > previous) {
          ascent += (current - previous);
        } else {
          descent += (previous - current);
        }
        prevElevation = p.elevation;
      }
    }
  }

}

void TrackStatisticsAccumulator::segmentEnd()
{
  // filter
  filterLastPoint=std::nullopt;

  // distance
  lastCoord = std::nullopt;
  filterLastCoord = std::nullopt;

  // max speed
  maxSpeedBuf.flush();

  // moving duration
  previousTime=std::nullopt;

  // elevation
  prevElevation=std::nullopt;
}

TrackStatistics TrackStatisticsAccumulator::accumulate() const
{
  osmscout::Timestamp::duration duration{0};

  // time accumulator
  if (from.has_value() && to.has_value()){
    duration = *to - *from;
  }

  double durationInSeconds = durationSeconds(duration);
  double movingDurationInSeconds = durationSeconds(movingDuration);

  return TrackStatistics(
    timestampToDateTime(from),
    timestampToDateTime(to),
    length,
    rawLength,
    duration,
    movingDuration,
    maxSpeedBuf.getMaxSpeed(),
    /*averageSpeed*/ durationInSeconds == 0 ? -1 : length.AsMeter() / durationInSeconds,
    /*movingAverageSpeed*/ movingDurationInSeconds == 0 ? -1 : length.AsMeter() / movingDurationInSeconds,
    Distance::Of<Meter>(ascent),
    Distance::Of<Meter>(descent),
    minElevation,
    maxElevation,
    bbox);
}

TrackStatistics Storage::computeTrackStatistics(const gpx::Track &trk) const
{
  QTime timer;
  timer.restart();

  qDebug() << "Computing track statistics...";

  TrackStatisticsAccumulator acc;
  for (const auto &seg:trk.segments){
    for (const auto &point:seg.points){
      acc.update(point);
    }
    acc.segmentEnd();
  }

  qDebug() << "Track statistics computation tooks" << timer.elapsed() << "ms";

  return acc.accumulate();
}

QSqlQuery Storage::trackInsertSql()
{
  QSqlQuery sqlTrk(db);

  sqlTrk.prepare(QString("INSERT INTO `track` (")
                   .append("`collection_id`, `name`, `description`, `open`, `creation_time`, `modification_time`, ")
                   .append("`from_time`, ")
                   .append("`to_time`, ")
                   .append("`distance`, ")
                   .append("`raw_distance`, ")
                   .append("`duration`, ")
                   .append("`moving_duration`, ")
                   .append("`max_speed`, ")
                   .append("`average_speed`, ")
                   .append("`moving_average_speed`, ")
                   .append("`ascent`, ")
                   .append("`descent`, ")
                   .append("`min_elevation`, ")
                   .append("`max_elevation`, ")
                   .append("`bbox_min_lat`, `bbox_min_lon`, `bbox_max_lat`, `bbox_max_lon`")
                   .append(") ")
                   .append("VALUES (")
                   .append(":collection_id,  :name,  :description,  :open,  :creation_time,  :modification_time, ")
                   .append(":from_time, ")
                   .append(":to_time, ")
                   .append(":distance, ")
                   .append(":raw_distance, ")
                   .append(":duration, ")
                   .append(":moving_duration, ")
                   .append(":max_speed, ")
                   .append(":average_speed, ")
                   .append(":moving_average_speed, ")
                   .append(":ascent, ")
                   .append(":descent, ")
                   .append(":min_elevation, ")
                   .append(":max_elevation, ")
                   .append(":bboxMinLat, :bboxMinLon, :bboxMaxLat, :bboxMaxLon")
                   .append(")"));

  return sqlTrk;
}

void Storage::prepareTrackInsert(QSqlQuery &sqlTrk,
                                 qint64 collectionId,
                                 const QString &trackName,
                                 const QStringOpt &desc,
                                 const TrackStatistics &stat,
                                 bool open)
{
  sqlTrk.bindValue(":collection_id", collectionId);
  sqlTrk.bindValue(":name", trackName);
  sqlTrk.bindValue(":description",
                   (desc.has_value() ? *desc : QVariant()));
  sqlTrk.bindValue(":open", open);

  sqlTrk.bindValue(":creation_time", dateTimeToSQL(QDateTime::currentDateTime()));
  sqlTrk.bindValue(":modification_time", dateTimeToSQL(QDateTime::currentDateTime()));

  sqlTrk.bindValue(":from_time", dateTimeToSQL(stat.from));
  sqlTrk.bindValue(":to_time", dateTimeToSQL(stat.to));
  sqlTrk.bindValue(":distance", stat.distance.AsMeter());
  sqlTrk.bindValue(":raw_distance", stat.rawDistance.AsMeter());
  sqlTrk.bindValue(":duration", stat.durationMillis());
  sqlTrk.bindValue(":moving_duration", stat.movingDurationMillis());
  sqlTrk.bindValue(":max_speed", stat.maxSpeed);
  sqlTrk.bindValue(":average_speed", stat.averageSpeed);
  sqlTrk.bindValue(":moving_average_speed", stat.movingAverageSpeed);
  sqlTrk.bindValue(":ascent", stat.ascent.AsMeter());
  sqlTrk.bindValue(":descent", stat.descent.AsMeter());
  sqlTrk.bindValue(":min_elevation", stat.minElevation.has_value() ? QVariant::fromValue(stat.minElevation->AsMeter()) : QVariant());
  sqlTrk.bindValue(":max_elevation", stat.maxElevation.has_value() ? QVariant::fromValue(stat.maxElevation->AsMeter()) : QVariant());

  sqlTrk.bindValue(":bboxMinLat", stat.bbox.IsValid() ? stat.bbox.GetMinLat() : -1000);
  sqlTrk.bindValue(":bboxMinLon", stat.bbox.IsValid() ? stat.bbox.GetMinLon() : -1000);
  sqlTrk.bindValue(":bboxMaxLat", stat.bbox.IsValid() ? stat.bbox.GetMaxLat() : -1000);
  sqlTrk.bindValue(":bboxMaxLon", stat.bbox.IsValid() ? stat.bbox.GetMaxLon() : -1000);
}

bool Storage::importTracks(const gpx::GpxFile &gpxFile, qint64 collectionId)
{
  using namespace std::string_literals;

  int trkNum = 0;
  QSqlQuery sqlTrk=trackInsertSql();

  QSqlQuery sqlSeg(db);
  sqlSeg.prepare("INSERT INTO `track_segment` (`track_id`, `open`, `creation_time`, `distance`) VALUES (:track_id, :open, :creation_time, :distance)");

  for (const auto &trk: gpxFile.tracks){
    trkNum++;

    QString trackName = QString::fromStdString(trk.name.value_or(""s));
    if (trackName.isEmpty())
      trackName = tr("track %1").arg(trkNum);

    TrackStatistics stat = computeTrackStatistics(trk);
    QStringOpt desc = trk.desc ?
                      QStringOpt(QString::fromStdString(*trk.desc)) :
                      std::nullopt;

    prepareTrackInsert(sqlTrk, collectionId, trackName, desc, stat, false);

    sqlTrk.exec();
    if (sqlTrk.lastError().isValid()) {
      qWarning() << "Import of tracks failed" << sqlTrk.lastError();
      emit error(tr("Import of tracks failed: %1").arg(sqlTrk.lastError().text()));
      return false;
    }

    qint64 trackId = varToLong(sqlTrk.lastInsertId());

    for (auto const &seg: trk.segments){
      sqlSeg.bindValue(":track_id", trackId);
      sqlSeg.bindValue(":open", false);

      // TODO: do we need segment statics?
      sqlSeg.bindValue(":creation_time", dateTimeToSQL(QDateTime::currentDateTime()));
      sqlSeg.bindValue(":distance", seg.GetLength().AsMeter());
      sqlSeg.exec();
      if (sqlSeg.lastError().isValid()) {
        qWarning() << "Import of segments failed" << sqlSeg.lastError();
        emit error(tr("Import of segments failed: %1").arg(sqlSeg.lastError().text()));
        return false;
      }
      qint64 segmentId = varToLong(sqlSeg.lastInsertId());

      if (!importTrackPoints(seg.points, segmentId)){
        qWarning() << "Import of track points failed" << sqlSeg.lastError();
        emit error(tr("Import of track points failed: %1").arg(sqlSeg.lastError().text()));
        return false;
      }
      qDebug() << "Imported" << seg.points.size() << "points to segment" << segmentId << "for track" << trackId;
    }
    qDebug() << "Imported track " << trackId;
  }
  return true;
}

bool Storage::importTrackPoints(const std::vector<gpx::TrackPoint> &points, qint64 segmentId)
{
  if (points.empty()){
    return true;
  }

  size_t pointNum = 0;
  db.transaction();
  QSqlQuery sql(db);
  sql.prepare(QString("INSERT INTO `track_point` ")
               .append("(`segment_id`, `timestamp`, `latitude`, `longitude`, `elevation`, `horiz_accuracy`, `vert_accuracy`) ")
               .append("VALUES ")
               .append("(:segment_id, :timestamp, :latitude, :longitude, :elevation, :horiz_accuracy, :vert_accuracy)"));

  for (const auto &point: points){
    pointNum ++;

    sql.bindValue(":segment_id", segmentId);
    sql.bindValue(":timestamp", dateTimeToSQL(timestampToDateTime(point.time)));
    sql.bindValue(":latitude", point.coord.GetLat());
    sql.bindValue(":longitude", point.coord.GetLon());
    sql.bindValue(":elevation", point.elevation.has_value() ? *point.elevation : QVariant());
    // read TrackPoint notes
    sql.bindValue(":horiz_accuracy", point.hdop.has_value() ? *point.hdop : QVariant());
    sql.bindValue(":vert_accuracy", point.vdop.has_value() ? *point.vdop : QVariant());

    sql.exec();
    if (sql.lastError().isValid()) {
      qWarning() << "Import of track points failed" << sql.lastError();
      emit error(tr("Import of track points failed: %1").arg(sql.lastError().text()));
      if (!db.rollback()) {
        qWarning() << "Transaction rollback failed" << db.lastError();
      }
      return false;
    }
    // commit batch TrackPointBatchSize queries
    if (pointNum % TrackPointBatchSize == 0) {
      if (!db.commit()) {
        emit error(tr("Transaction commit failed: %1").arg(db.lastError().text()));
        qWarning() << "Transaction commit failed" << db.lastError();
        return false;
      }
      db.transaction();
    }
  }
  if (!db.commit()) {
    emit error(tr("Transaction commit failed: %1").arg(db.lastError().text()));
    qWarning() << "Transaction commit failed" << db.lastError();
    return false;
  }
  return true;
}

void Storage::importCollection(QString filePath)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionsLoaded(std::vector<Collection>(), false);
    return;
  }

  QTime timer;
  timer.start();
  qDebug() << "Importing collection from" << filePath;

  gpx::GpxFile gpxFile;
  std::shared_ptr<ErrorCallback> callback = std::make_shared<ErrorCallback>();
  connect(callback.get(), &ErrorCallback::error, this, &Storage::error);

  if (!gpx::ImportGpx(filePath.toStdString(),
                      gpxFile,
                      nullptr,
                      std::static_pointer_cast<gpx::ProcessCallback, ErrorCallback>(callback))){

    qWarning() << "Gpx import failed " << filePath;
    loadCollections();
    return;
  }

  // import collection
  QSqlQuery sql(db);
  sql.prepare("INSERT INTO `collection` (`name`, `description`, `visible`) VALUES (:name, :description, 0);");
  sql.bindValue(":name", gpxFile.name.has_value() ?
                         QString::fromStdString(*gpxFile.name) : QFileInfo(filePath).baseName());
  sql.bindValue(":description", gpxFile.desc.has_value() && !gpxFile.desc->empty() ?
                                QString::fromStdString(*gpxFile.desc) :
                                tr("Imported from %1").arg(filePath));

  sql.exec();
  if (sql.lastError().isValid()){
    qWarning() << "Creating collection failed" << sql.lastError();
    emit error(tr("Creating collection failed: %1").arg(sql.lastError().text()));
    loadCollections();
    return;
  }
  qint64 collectionId = varToLong(sql.lastInsertId());
  if (collectionId < 0){
    qWarning() << "Invalid collection id" << collectionId;
    emit error(tr("Invalid collection id: %1").arg(collectionId));
    loadCollections();
    return;
  }

  // import waypoints
  if (!gpxFile.waypoints.empty()) {
    if (!importWaypoints(gpxFile, collectionId)){
      loadCollections();
      return;
    }
  }
  qDebug() << "Imported" << gpxFile.waypoints.size() << "waypoints to collection" << collectionId << "from" << filePath;

  // import tracks
  if (!gpxFile.tracks.empty()) {
    if (!importTracks(gpxFile, collectionId)){
      loadCollections();
      return;
    }
  }
  qDebug() << "Imported" << gpxFile.tracks.size() << "tracks to collection" << collectionId
           << "from" << filePath << "in" << timer.elapsed() << "ms";

  loadCollections();
}

void Storage::deleteWaypoint(qint64 collectionId, qint64 waypointId)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionDetailsLoaded(Collection(collectionId), false);
    return;
  }

  QSqlQuery sql(db);
  sql.prepare("DELETE FROM `waypoint` WHERE `id` = :id AND `collection_id` = :collection_id;");
  sql.bindValue(":id", waypointId);
  sql.bindValue(":collection_id", collectionId);
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Deleting waypoint failed" << sql.lastError();
    emit error(tr("Deleting waypoint failed: %1").arg(sql.lastError().text()));
    loadCollectionDetails(Collection(collectionId));
  } else {
    emit waypointDeleted(collectionId, waypointId);
  }

  loadCollectionDetails(Collection(collectionId));
}

void Storage::createWaypoint(qint64 collectionId, double lat, double lon, QString name, QString description)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionDetailsLoaded(Collection(collectionId), false);
    return;
  }

  QSqlQuery sqlWpt(db);
  sqlWpt.prepare(
    "INSERT INTO `waypoint` (`collection_id`, `timestamp`, `modification_time`, `latitude`, `longitude`, `name`, `description`) "
    "VALUES                 (:collection_id,  :timestamp,  :modification_time,  :latitude,  :longitude,  :name,  :description)");

  sqlWpt.bindValue(":collection_id", collectionId);
  sqlWpt.bindValue(":timestamp", dateTimeToSQL(QDateTime::currentDateTime()));
  sqlWpt.bindValue(":modification_time", dateTimeToSQL(QDateTime::currentDateTime()));
  sqlWpt.bindValue(":latitude", lat);
  sqlWpt.bindValue(":longitude", lon);
  sqlWpt.bindValue(":name", name);
  sqlWpt.bindValue(":description", (description.isEmpty() ? QVariant() : description));

  sqlWpt.exec();
  if (sqlWpt.lastError().isValid()) {
    qWarning() << "Creation of waypoint failed" << sqlWpt.lastError();
    emit error(tr("Creation of waypoint failed: %1").arg(sqlWpt.lastError().text()));
  } else {
    qint64 wptId = varToLong(sqlWpt.lastInsertId());
    emit waypointCreated(collectionId, wptId, name);
  }

  loadCollectionDetails(Collection(collectionId));
}

void Storage::createTrack(qint64 collectionId, QString name, QString description, bool open)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionDetailsLoaded(Collection(collectionId), false);
    return;
  }

  QSqlQuery sqlTrk = trackInsertSql();

  QStringOpt desc = description.isEmpty() ?
                    std::nullopt :
                    QStringOpt(description);

  prepareTrackInsert(sqlTrk, collectionId, name, desc, TrackStatistics{}, open);

  sqlTrk.exec();
  if (sqlTrk.lastError().isValid()) {
    qWarning() << "Creation of track failed" << sqlTrk.lastError();
    emit error(tr("Creation of track failed: %1").arg(sqlTrk.lastError().text()));
  } else {
    qint64 trackId = varToLong(sqlTrk.lastInsertId());
    emit trackCreated(collectionId, trackId, name);
  }

  loadCollectionDetails(Collection(collectionId));
}

void Storage::closeTrack(qint64 collectionId, qint64 trackId){
  if (!checkAccess(__FUNCTION__)){
    emit collectionDetailsLoaded(Collection(collectionId), false);
    return;
  }

  QSqlQuery sql(db);
  sql.prepare("UPDATE `track` SET `open` = 'FALSE' WHERE `id` = :id AND `collection_id` = :collection_id;");
  sql.bindValue(":id", trackId);
  sql.bindValue(":collection_id", collectionId);
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Closing track failed" << sql.lastError();
    emit error(tr("Closing track failed: %1").arg(sql.lastError().text()));
    loadCollectionDetails(Collection(collectionId));
  }

  loadCollectionDetails(Collection(collectionId));
}


void Storage::deleteTrack(qint64 collectionId, qint64 trackId)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionDetailsLoaded(Collection(collectionId), false);
    return;
  }

  QSqlQuery sql(db);
  sql.prepare("DELETE FROM `track` WHERE `id` = :id AND `collection_id` = :collection_id;");
  sql.bindValue(":id", trackId);
  sql.bindValue(":collection_id", collectionId);
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Deleting track failed" << sql.lastError();
    emit error(tr("Deleting track failed: %1").arg(sql.lastError().text()));
    loadCollectionDetails(Collection(collectionId));
  } else {
    emit trackDeleted(collectionId, trackId);
  }

  loadCollectionDetails(Collection(collectionId));
}

void Storage::editWaypoint(qint64 collectionId, qint64 id, QString name, QString description)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionDetailsLoaded(Collection(collectionId), false);
    return;
  }

  QSqlQuery sql(db);
  sql.prepare("UPDATE `waypoint` SET `name` = :name, `description` = :description, `modification_time` = :modification_time WHERE `id` = :id AND `collection_id` = :collection_id;");
  sql.bindValue(":id", id);
  sql.bindValue(":collection_id", collectionId);
  sql.bindValue(":name", name);
  sql.bindValue(":description", description);
  sql.bindValue(":modification_time", dateTimeToSQL(QDateTime::currentDateTime()));
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Edit waypoint failed" << sql.lastError();
    emit error(tr("Edit waypoint failed: %1").arg(sql.lastError().text()));
    loadCollectionDetails(Collection(collectionId));
  }

  loadCollectionDetails(Collection(collectionId));
}

void Storage::editTrack(qint64 collectionId, qint64 id, QString name, QString description)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionDetailsLoaded(Collection(collectionId), false);
    return;
  }

  QSqlQuery sql(db);
  sql.prepare("UPDATE `track` SET `name` = :name, `description` = :description, `modification_time` = :modification_time WHERE `id` = :id AND `collection_id` = :collection_id;");
  sql.bindValue(":id", id);
  sql.bindValue(":collection_id", collectionId);
  sql.bindValue(":name", name);
  sql.bindValue(":description", description);
  sql.bindValue(":modification_time", dateTimeToSQL(QDateTime::currentDateTime()));
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Edit track failed" << sql.lastError();
    emit error(tr("Edit track failed: %1").arg(sql.lastError().text()));
    loadCollectionDetails(Collection(collectionId));
  }

  loadCollectionDetails(Collection(collectionId));
}

bool Storage::exportPrivate(qint64 collectionId, const QString &file, const std::optional<qint64> &trackId, bool includeWaypoints, int accuracyFilter)
{
  QTime timer;
  timer.start();
  qDebug() << "Exporting collection" << collectionId << "to" << file;

  // load data
  Collection collection(collectionId);
  gpx::GpxFile gpxFile;
  if (!loadCollectionDetailsPrivate(collection)){
    emit collectionExported(false);
    return false;
  }

  if (!collection.name.isEmpty()) {
    gpxFile.name = collection.name.toStdString();
  }
  if (!collection.description.isEmpty()) {
    gpxFile.desc = collection.description.toStdString();
  }

  assert(collection.waypoints);
  if (includeWaypoints) {
    gpxFile.waypoints.reserve(collection.waypoints->size());
    for (const Waypoint &w: *(collection.waypoints)) {
      gpxFile.waypoints.push_back(w.data);
    }
  }
  collection.waypoints.reset();

  // load track data
  assert(collection.tracks);
  gpxFile.tracks.reserve(collection.tracks->size());
  for (Track &t : *(collection.tracks)){
    if (!trackId || *trackId == t.id) {
      if (!loadTrackDataPrivate(t)) {
        emit collectionExported(false);
        return false;
      }
      assert(t.data);
      if (accuracyFilter > 0){
        t.data->FilterPoints([accuracyFilter](std::vector<osmscout::gpx::TrackPoint> &points){
          osmscout::gpx::FilterInaccuratePoints(points, accuracyFilter);
        });
      }

      // drop empty segments
      t.data->segments.erase(std::remove_if(t.data->segments.begin(),t.data->segments.end(),
                                            [](const gpx::TrackSegment &s){ return s.points.empty(); }),
                             t.data->segments.end());

      gpxFile.tracks.push_back(std::move(*(t.data)));
    }
  }

  // export
  qDebug() << "Writing gpx file" << file;
  std::shared_ptr<ErrorCallback> callback = std::make_shared<ErrorCallback>();
  connect(callback.get(), &ErrorCallback::error, this, &Storage::error);

  bool success = gpx::ExportGpx(gpxFile,
                                file.toStdString(),
                                nullptr,
                                std::static_pointer_cast<gpx::ProcessCallback, ErrorCallback>(callback));

  qDebug() << "Exported in" << timer.elapsed() << "ms";
  return success;
}

void Storage::exportCollection(qint64 collectionId, QString file, bool includeWaypoints, int accuracyFilter)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionExported(false);
    return;
  }

  bool success = exportPrivate(collectionId, file, std::nullopt, includeWaypoints, accuracyFilter);
  emit collectionExported(success);
}

void Storage::exportTrack(qint64 collectionId, qint64 trackId, QString file, bool includeWaypoints, int accuracyFilter)
{
  if (!checkAccess(__FUNCTION__)){
    emit collectionExported(false);
    return;
  }

  bool success = exportPrivate(collectionId, file, trackId, includeWaypoints, accuracyFilter);
  emit trackExported(success);
}

void Storage::moveWaypoint(qint64 waypointId, qint64 collectionId)
{
  if (!checkAccess(__FUNCTION__)){
    return;
  }

  QSqlQuery sql(db);
  sql.prepare("SELECT `collection_id` FROM `waypoint` WHERE `id` = :id;");
  sql.bindValue(":id", waypointId);
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Loading waypoint id" << waypointId << "fails";
    emit error(tr("Loading waypoint id %1 fails").arg(waypointId));
    return;
  }

  if (!sql.next()) {
    qWarning() << "Waypoint id" << waypointId << "not found";
    emit error(tr("Waypoint id %1 not found").arg(waypointId));
    return;
  }
  qint64 sourceCollectionId = varToLong(sql.value("collection_id"));

  qDebug() << "Moving waypoint" << waypointId << "from collection" << sourceCollectionId << "to" << collectionId;

  QSqlQuery sqlUpdate(db);
  sqlUpdate.prepare("UPDATE `waypoint` SET `collection_id` = :collection_id  WHERE `id` = :id;");
  sqlUpdate.bindValue(":id", waypointId);
  sqlUpdate.bindValue(":collection_id", collectionId);
  sqlUpdate.exec();

  if (sqlUpdate.lastError().isValid()) {
    qWarning() << "Move waypoint id" << waypointId << "fails";
    emit error(tr("Move waypoint id %1 fails").arg(waypointId));
    return;
  }

  Collection collection(sourceCollectionId);
  if (loadCollectionDetailsPrivate(collection)) {
    emit collectionDetailsLoaded(collection, true);
  }else{
    emit collectionDetailsLoaded(collection, false);
  }

  collection.id = collectionId;
  if (loadCollectionDetailsPrivate(collection)) {
    emit collectionDetailsLoaded(collection, true);
  }else{
    emit collectionDetailsLoaded(collection, false);
  }
}

void Storage::moveTrack(qint64 trackId, qint64 collectionId)
{
  if (!checkAccess(__FUNCTION__)){
    return;
  }

  QSqlQuery sql(db);
  sql.prepare("SELECT `collection_id` FROM `track` WHERE `id` = :id;");
  sql.bindValue(":id", trackId);
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Loading track id" << trackId << "fails";
    emit error(tr("Loading track id %1 fails").arg(trackId));
    return;
  }

  if (!sql.next()) {
    qWarning() << "Track id" << trackId << "not found";
    emit error(tr("Track id %1 not found").arg(trackId));
    return;
  }
  qint64 sourceCollectionId = varToLong(sql.value("collection_id"));

  qDebug() << "Moving track" << trackId << "from collection" << sourceCollectionId << "to" << collectionId;

  QSqlQuery sqlUpdate(db);
  sqlUpdate.prepare("UPDATE `track` SET `collection_id` = :collection_id  WHERE `id` = :id;");
  sqlUpdate.bindValue(":id", trackId);
  sqlUpdate.bindValue(":collection_id", collectionId);
  sqlUpdate.exec();

  if (sqlUpdate.lastError().isValid()) {
    qWarning() << "Move track id" << trackId << "fails";
    emit error(tr("Move track id %1 fails").arg(trackId));
    return;
  }

  Collection collection(sourceCollectionId);
  if (loadCollectionDetailsPrivate(collection)) {
    emit collectionDetailsLoaded(collection, true);
  }else{
    emit collectionDetailsLoaded(collection, false);
  }

  collection.id = collectionId;
  if (loadCollectionDetailsPrivate(collection)) {
    emit collectionDetailsLoaded(collection, true);
  }else{
    emit collectionDetailsLoaded(collection, false);
  }
}

void Storage::cropTrackStart(Track track, quint64 position)
{
  if (!checkAccess(__FUNCTION__)){
    return;
  }
  // TODO
}

void Storage::cropTrackEnd(Track track, quint64 position)
{
  if (!checkAccess(__FUNCTION__)){
    return;
  }
  // TODO
}

void Storage::splitTrack(Track track, quint64 position)
{
  if (!checkAccess(__FUNCTION__)){
    return;
  }
  // TODO
}

void Storage::loadRecentOpenTrack(){
  if (!checkAccess(__FUNCTION__)){
    return;
  }

  QSqlQuery sqlTrack(db);
  sqlTrack.prepare("SELECT * FROM `track` WHERE `open` ORDER BY `creation_time` DESC LIMIT 1;");
  sqlTrack.exec();

  if (sqlTrack.lastError().isValid()) {
    qWarning() << "Loading last open track fails";
    emit error(tr("Loading last open track fails"));
    emit openTrackLoaded(Track{}, false);
  }

  if (sqlTrack.next()) {
    openTrackLoaded(makeTrack(sqlTrack), true);
  } else {
    emit openTrackLoaded(Track{}, true);
  }
}

bool Storage::createSegment(qint64 trackId, qint64 &segmentId)
{
  QSqlQuery sqlSeg(db);
  sqlSeg.prepare("INSERT INTO `track_segment` (`track_id`, `open`, `creation_time`, `distance`) VALUES (:track_id, :open, :creation_time, :distance)");

  sqlSeg.bindValue(":track_id", trackId);
  sqlSeg.bindValue(":open", true);

  // TODO: do we need segment statics?
  sqlSeg.bindValue(":creation_time", dateTimeToSQL(QDateTime::currentDateTime()));
  sqlSeg.bindValue(":distance", 0); // ignored right now
  sqlSeg.exec();
  if (sqlSeg.lastError().isValid()) {
    qWarning() << "Segment creation failed" << sqlSeg.lastError();
    emit error(tr("Segment creation failed: %1").arg(sqlSeg.lastError().text()));
    return false;
  }
  segmentId = varToLong(sqlSeg.lastInsertId());
  return true;
}

void Storage::appendNodes(qint64 trackId,
                          std::shared_ptr<std::vector<osmscout::gpx::TrackPoint>> batch,
                          TrackStatistics statistics,
                          bool createNewSegment)
{
  if (!checkAccess(__FUNCTION__)){
    return;
  }

  assert(batch);

  QSqlQuery sqlSegment(db);
  sqlSegment.prepare("SELECT MAX(`id`) AS `segment_id` FROM `track_segment` WHERE `track_id` = :id;");
  sqlSegment.bindValue(":id", trackId);
  sqlSegment.exec();

  if (sqlSegment.lastError().isValid()) {
    qWarning() << "Evaluating last segment failed" << sqlSegment.lastError();
    emit error(tr("Failed to append nodes to track"));
    return;
  }

  qint64 segmentId;
  if (sqlSegment.next()) {
    QVariant segmentIdVar = sqlSegment.value("segment_id");
    if (segmentIdVar.isNull()){
      if (!createSegment(trackId, segmentId)){
        qWarning() << "Creating segment failed";
        emit error(tr("Failed to append nodes to track"));
        return;
      }
    }else {
      segmentId = varToLong(segmentIdVar);
    }
  } else {
    qWarning() << "Evaluating last segment failed, cannot retrieve row";
    emit error(tr("Failed to append nodes to track"));
    return;
  }

  if (!importTrackPoints(*batch, segmentId)){
    qWarning() << "Failed to append nodes to track";
    emit error(tr("Failed to append nodes to track"));
    return;
  }

  if (createNewSegment){
    if (!createSegment(trackId, segmentId)){
      qWarning() << "Creating segment failed";
    }
  }

  qint64 collectionId;
  if (!trackCollection(trackId, collectionId)){
    return;
  }

  QSqlQuery sql(db);
  sql.prepare(QString("UPDATE `track` SET ")
              .append("`from_time` = :from_time, ")
              .append("`to_time` = :to_time, ")
              .append("`distance` = :distance, ")
              .append("`raw_distance` = :raw_distance, ")
              .append("`duration` = :duration, ")
              .append("`moving_duration` = :moving_duration, ")
              .append("`max_speed` = :max_speed, ")
              .append("`average_speed` = :average_speed, ")
              .append("`moving_average_speed` = :moving_average_speed, ")
              .append("`ascent` = :ascent, ")
              .append("`descent` = :descent, ")
              .append("`min_elevation` = :min_elevation, ")
              .append("`max_elevation` = :max_elevation, ")
              .append("`bbox_min_lat` = :bbox_min_lat, ")
              .append("`bbox_min_lon` = :bbox_min_lon, ")
              .append("`bbox_max_lat` = :bbox_max_lat, ")
              .append("`bbox_max_lon` = :bbox_max_lon, ")
              .append("`modification_time` = :modification_time ")
              .append("WHERE `id` = :id"));

  sql.bindValue(":from_time", dateTimeToSQL(statistics.from));
  sql.bindValue(":to_time", dateTimeToSQL(statistics.to));
  sql.bindValue(":distance", statistics.distance.AsMeter());
  sql.bindValue(":raw_distance", statistics.rawDistance.AsMeter());
  sql.bindValue(":duration", statistics.durationMillis());
  sql.bindValue(":moving_duration", statistics.movingDurationMillis());
  sql.bindValue(":max_speed", statistics.maxSpeed);
  sql.bindValue(":average_speed", statistics.averageSpeed);
  sql.bindValue(":moving_average_speed", statistics.movingAverageSpeed);
  sql.bindValue(":ascent", statistics.ascent.AsMeter());
  sql.bindValue(":descent", statistics.descent.AsMeter());
  sql.bindValue(":min_elevation", statistics.minElevation.has_value() ? QVariant::fromValue(statistics.minElevation->AsMeter()) : QVariant());
  sql.bindValue(":max_elevation", statistics.maxElevation.has_value() ? QVariant::fromValue(statistics.maxElevation->AsMeter()) : QVariant());

  sql.bindValue(":bbox_min_lat", statistics.bbox.IsValid() ? statistics.bbox.GetMinLat() : -1000);
  sql.bindValue(":bbox_min_lon", statistics.bbox.IsValid() ? statistics.bbox.GetMinLon() : -1000);
  sql.bindValue(":bbox_max_lat", statistics.bbox.IsValid() ? statistics.bbox.GetMaxLat() : -1000);
  sql.bindValue(":bbox_max_lon", statistics.bbox.IsValid() ? statistics.bbox.GetMaxLon() : -1000);

  sql.bindValue(":modification_time", dateTimeToSQL(QDateTime::currentDateTime()));

  sql.bindValue(":id", trackId);
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Edit track failed" << sql.lastError();
    emit error(tr("Edit track failed: %1").arg(sql.lastError().text()));
    loadCollectionDetails(Collection(collectionId));
    return;
  }

  loadCollectionDetails(Collection(collectionId));
}

bool Storage::trackCollection(qint64 trackId, qint64 &collectionId)
{
  QSqlQuery sqlSegment(db);
  sqlSegment.prepare("SELECT `collection_id` FROM `track` WHERE `id` = :id;");
  sqlSegment.bindValue(":id", trackId);
  sqlSegment.exec();

  if (sqlSegment.lastError().isValid()) {
    qWarning() << "Cannot obtain collection id" << sqlSegment.lastError();
    return false;
  }

  if (sqlSegment.next()) {
    QVariant segmentIdVar = sqlSegment.value("collection_id");
    if (segmentIdVar.isNull()){
      qWarning() << "Cannot obtain collection id, track don't exits";
      return false;
    }else {
      collectionId = varToLong(segmentIdVar);
    }
  } else {
    qWarning() << "Cannot obtain collection id, cannot retrieve row";
    return false;
  }

  return true;
}

void Storage::loadSearchHistory(){
  if (!checkAccess(__FUNCTION__)){
    return;
  }

  std::vector<SearchItem> items;
  QSqlQuery sqlRecent(db);
  sqlRecent.prepare("SELECT `pattern`, `last_usage` FROM `search_history` ORDER BY `last_usage` DESC LIMIT 50;");
  sqlRecent.exec();
  if (sqlRecent.lastError().isValid()) {
    qWarning() << "Cannot load search history" << sqlRecent.lastError();
    emit error("Cannot load search history");
    emit searchHistory(items);
    return;
  }

  while (sqlRecent.next()) {
    items.push_back(
      SearchItem{
        varToString(sqlRecent.value("pattern")),
        varToDateTime(sqlRecent.value("last_usage"))
      });
  }

  emit searchHistory(items);
}

void Storage::addSearchPattern(QString pattern){
  if (!checkAccess(__FUNCTION__)){
    return;
  }

  QSqlQuery sqlInsert(db);
  sqlInsert.prepare("INSERT OR REPLACE INTO `search_history` (`pattern`, `last_usage`) VALUES(:pattern, :last_usage);");
  sqlInsert.bindValue(":pattern", pattern);
  sqlInsert.bindValue(":last_usage", QDateTime::currentDateTime());
  sqlInsert.exec();

  if (sqlInsert.lastError().isValid()) {
    qWarning() << "Cannot store entry to search history" << sqlInsert.lastError();
    emit error("Cannot store entry to search history");
    return;
  }

  // cleanup 100 oldest entries, keep 50 recent
  QSqlQuery sqlRecent(db);
  sqlRecent.prepare(QString("DELETE FROM `search_history` WHERE `pattern` IN ")
                       .append("(SELECT `pattern` FROM `search_history` ORDER BY `last_usage` DESC LIMIT 50,100);"));
  sqlRecent.exec();

  if (sqlInsert.lastError().isValid()) {
    qWarning() << "Cannot clean search history" << sqlInsert.lastError();
    emit error("Cannot clean search history");
    return;
  }

  loadSearchHistory();
}

void Storage::removeSearchPattern(QString pattern){
  if (!checkAccess(__FUNCTION__)){
    return;
  }

  QSqlQuery sqlInsert(db);
  sqlInsert.prepare("DELETE FROM `search_history` WHERE `pattern` = :pattern;");
  sqlInsert.bindValue(":pattern", pattern);
  sqlInsert.exec();

  if (sqlInsert.lastError().isValid()) {
    qWarning() << "Cannot remove entry from search history" << sqlInsert.lastError();
    emit error("Cannot remove entry from search history");
    return;
  }

  loadSearchHistory();
}

Storage::operator bool() const
{
  return ok;
}

Storage* Storage::getInstance()
{
  return storage;
}

void Storage::initInstance(const QDir &directory)
{
  if (storage == nullptr){
    QThread *thread = OSMScoutQt::GetInstance().makeThread("Storage");
    storage = new Storage(thread, directory);
    storage->moveToThread(thread);
    connect(thread, &QThread::started,
            storage, &Storage::init);
    thread->start();
  }
}

void Storage::clearInstance()
{
  if (storage != nullptr){
    storage->deleteLater();
    storage = nullptr;
  }
}
