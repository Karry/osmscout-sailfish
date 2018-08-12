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

#include <QDebug>
#include <QThread>
#include <QtSql/QSqlQuery>
#include <osmscout/OSMScoutQt.h>

namespace {
  static constexpr int DbSchema = 1;
}

using namespace osmscout;

static Storage* storage = nullptr;

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
          qDebug()<<"last schema: "<<currentSchema;
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
  if (currentSchema > DbSchema) {
    qWarning() << "newer database schema; " << currentSchema << " > " << DbSchema;
  }

  if (!tables.contains("collection")){
    qDebug()<< "creating collection table";

    QString sql("CREATE TABLE `collection`");
    sql.append("(").append( "`id`   int NOT NULL PRIMARY KEY");
    sql.append(",").append( "`name` varchar(255) NOT NULL ");
    sql.append(",").append( "`description` varchar(255) NULL ");
    sql.append(");");

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating collection table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("track")){
    qDebug()<< "creating track table";

    QString sql("CREATE TABLE `track`");
    sql.append("(").append( "`id`   int NOT NULL PRIMARY KEY");
    sql.append(",").append( "`collection_id` int NULL");
    sql.append(",").append( "`name` varchar(255) NOT NULL");
    sql.append(",").append( "`description` varchar(255) NOT NULL");
    sql.append(",").append( "`open` tinyint(1) NOT NULL");
    sql.append(",").append( "`creation_time` datetime NOT NULL");
    sql.append(",").append( "`distance` double NOT NULL");
    sql.append(");");

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating track table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("track_segment")){
    qDebug()<< "creating track_segment table";

    QString sql("CREATE TABLE `track_segment`");
    sql.append("(").append( "`id` int NOT NULL PRIMARY KEY");
    sql.append(",").append( "`track_id` int NOT NULL");
    sql.append(",").append( "`name` varchar(255) NOT NULL");
    sql.append(",").append( "`open` tinyint(1) NOT NULL");
    sql.append(",").append( "`creation_time` datetime NOT NULL");
    sql.append(");");

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating track segment table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("track_node")){
    qDebug()<< "creating track_node table";

    QString sql("CREATE TABLE `track_point`");
    sql.append("(").append( "`segment_id` int NOT NULL");
    sql.append(",").append( "`timestamp` datetime NOT NULL");
    sql.append(",").append( "`latitude` double NOT NULL");
    sql.append(",").append( "`longitude` double NOT NULL");
    sql.append(",").append( "`elevation` double NULL ");
    sql.append(",").append( "`horiz_accuracy` double NULL ");
    sql.append(",").append( "`vert_accuracy` double NULL ");
    sql.append(");");

    // TODO: what satelites and compas?

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating track node table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("waypoint")){
    qDebug()<< "creating waypoints table";

    QString sql("CREATE TABLE `waypoint`");
    sql.append("(").append( "`id` int NOT NULL PRIMARY KEY");
    sql.append(",").append( "`collection_id` int NOT NULL");
    sql.append(",").append( "`timestamp` datetime NOT NULL");
    sql.append(",").append( "`latitude` double NOT NULL");
    sql.append(",").append( "`longitude` double NOT NULL");
    sql.append(",").append( "`elevation` double NULL");
    sql.append(",").append( "`name` varchar(255) NOT NULL ");
    sql.append(",").append( "`description` varchar(255) NULL ");
    sql.append(",").append( "`symbol` varchar(255) NULL ");
    sql.append(");");

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()){
      qWarning() << "Storage: creating waypoints table failed" << q.lastError();
      db.close();
      return false;
    }
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
    qWarning() << "Failed to cretate directory" << directory;
    emit initialisationError(QString("Failed to cretate directory ") + directory.path());
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
  if (updateSchema()){
    ok = db.isValid() && db.isOpen();
    emit initialised();
  } else {
    emit initialisationError("update schema");
  }
}

namespace {
  long VarToLong(const QVariant &var, long def = -1)
  {
    bool ok;
    qlonglong val = var.toLongLong(&ok);
    return ok ? val : def;
  }

  QString VarToString(const QVariant &var, QString def = "")
  {
    if (!var.isNull() &&
        var.isValid() &&
        var.canConvert(QMetaType::QString)) {
      return var.toString();
    }

    return def;
  }

  osmscout::gpx::Optional<std::string> VarToStringOpt(const QVariant &var)
  {
    if (!var.isNull() &&
        var.isValid() &&
        var.canConvert(QMetaType::QString)) {
      return osmscout::gpx::Optional<std::string>::of(var.toString().toStdString());
    }

    return osmscout::gpx::Optional<std::string>();
  }

  bool VarToBool(const QVariant &var, bool def = false)
  {
    if (!var.isNull() &&
        var.isValid() &&
        var.canConvert(QMetaType::Bool)) {
      return var.toBool();
    }

    return def;
  }

  QDateTime VarToDateTime(const QVariant &var, QDateTime def = QDateTime())
  {
    if (!var.isNull() &&
        var.isValid() &&
        var.canConvert(QMetaType::QDateTime)) {
      return var.toDateTime();
    }

    return def;
  }

  Timestamp DateTimeToTimestamp(const QDateTime &datetime)
  {
    int64_t millis = datetime.toMSecsSinceEpoch();
    auto duration = std::chrono::milliseconds(millis);
    return Timestamp(duration);
  }

  double VarToDouble(const QVariant &var, double def = 0)
  {
    if (!var.isNull() &&
        var.isValid() &&
        var.canConvert(QMetaType::Double)) {
      return var.toDouble();
    }

    return def;
  }

  osmscout::gpx::Optional<double> VarToDoubleOpt(const QVariant &var)
  {
    if (!var.isNull() &&
        var.isValid() &&
        var.canConvert(QMetaType::Double)) {
      return osmscout::gpx::Optional<double>::of(var.toDouble());
    }

    return osmscout::gpx::Optional<double>();
  }
}

bool Storage::checkAccess(QString slotName, bool requireOpen)
{
  if (thread != QThread::currentThread()){
    qWarning() << this << "/" << slotName << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
    return false;
  }
  if (requireOpen && !ok){
    qWarning() << "Database is not opened, " << this << "/" << slotName << "";
    return false;
  }
  return true;
}

void Storage::loadCollections()
{
  if (!checkAccess("loadCollections")){
    emit collectionsLoaded(std::vector<Collection>(), false);
    return;
  }

  QString sql("SELECT `id`, `name`, `description` FROM `collection`;");

  QSqlQuery q = db.exec(sql);
  if (q.lastError().isValid()) {
    emit collectionsLoaded(std::vector<Collection>(), false);
  }
  std::vector<Collection> result;
  while (q.next()) {
    result.emplace_back(
      VarToLong(q.value("id")),
      VarToString(q.value("name")),
      VarToString(q.value("description"))
    );
  }
  emit collectionsLoaded(result, true);
}

std::shared_ptr<std::vector<Track>> Storage::loadTracks(long collectionId)
{
  QSqlQuery sqlTrack(db);
  sqlTrack.prepare("SELECT `id`, `name`, `description`, `open`, `creation_time`, `distance` FROM `track` WHERE collection_id = :collectionId;");
  sqlTrack.bindValue(":collectionId", QVariant::fromValue(collectionId));
  sqlTrack.exec();

  if (sqlTrack.lastError().isValid()) {
    qWarning() << "Loading tracks for collection id" << collectionId << "fails";
    return nullptr;
  }

  std::shared_ptr<std::vector<Track>> result = std::make_shared<std::vector<Track>>();
  while (sqlTrack.next()) {
    result->emplace_back(
      VarToLong(sqlTrack.value("id")),
      collectionId,
      VarToString(sqlTrack.value("name")),
      VarToString(sqlTrack.value("description")),
      VarToBool(sqlTrack.value("open")),
      VarToDateTime(sqlTrack.value("creationTime")),
      osmscout::Distance::Of<osmscout::Meter>(VarToDouble(sqlTrack.value("distance")))
    );
  }
  return result;
}

std::shared_ptr<std::vector<osmscout::gpx::Waypoint>> Storage::loadWaypoints(long collectionId)
{
  QSqlQuery sql(db);
  sql.prepare("SELECT `id`, `name`, `description`, `symbol`, `timestamp`, `latitude`, `longitude`, `elevation` FROM `waypoint` WHERE collection_id = :collectionId;");
  sql.bindValue(":collectionId", QVariant::fromValue(collectionId));
  sql.exec();

  if (sql.lastError().isValid()) {
    qWarning() << "Loading waypoints for collection id" << collectionId << "fails";
    return nullptr;
  }

  std::shared_ptr<std::vector<osmscout::gpx::Waypoint>> result = std::make_shared<std::vector<osmscout::gpx::Waypoint>>();
  while (sql.next()) {
    osmscout::gpx::Waypoint wpt(osmscout::GeoCoord(
      VarToDouble(sql.value("latitude")),
      VarToDouble(sql.value("longitude"))
      ));

    wpt.name = VarToStringOpt(VarToString(sql.value("name")));
    wpt.description = VarToStringOpt(sql.value("description"));
    wpt.symbol = VarToStringOpt(sql.value("symbol"));

    wpt.time = osmscout::gpx::Optional<Timestamp>::of(DateTimeToTimestamp(VarToDateTime(sql.value("timestamp"))));
    wpt.elevation = VarToDoubleOpt(sql.value("elevation"));

    result->push_back(std::move(wpt));
  }
  return result;
}

void Storage::loadCollectionDetails(Collection collection)
{
  if (!checkAccess("loadCollectionDetails")){
    emit collectionDetailsLoaded(collection, false);
    return;
  }

  collection.tracks = loadTracks(collection.id);
  collection.waypoints = loadWaypoints(collection.id);

  emit collectionDetailsLoaded(collection, true);
}

void Storage::loadTrackPoints(long segmentId, osmscout::gpx::TrackSegment &segment)
{
  QSqlQuery sql(db);
  sql.prepare("SELECT `timestamp`, `latitude`, `longitude`, `elevation`, `horiz_accuracy`, `vert_accuracy` FROM `track_point` WHERE segment_id = :segmentId;");
  sql.bindValue(":segmentId", QVariant::fromValue(segmentId));
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Loading nodes for segment id" << segmentId << "fails";
  }else{
    while (sql.next()) {
      osmscout::gpx::TrackPoint point(osmscout::GeoCoord(
        VarToDouble(sql.value("latitude")),
        VarToDouble(sql.value("longitude"))
      ));

      point.time = osmscout::gpx::Optional<Timestamp>::of(DateTimeToTimestamp(VarToDateTime(sql.value("timestamp"))));
      point.elevation = VarToDoubleOpt(sql.value("elevation"));

      // see TrackPoint notes
      point.hdop = VarToDoubleOpt(sql.value("horiz_accuracy"));
      point.vdop = VarToDoubleOpt(sql.value("vert_accuracy"));

      segment.points.push_back(std::move(point));
    }
  }
}

void Storage::loadTrackData(Track track)
{
  if (!checkAccess("loadTrackData")){
    emit trackDataLoaded(track, false);
    return;
  }

  track.data = std::make_shared<osmscout::gpx::Track>();

  track.data->name = osmscout::gpx::Optional<std::string>::of(track.name.toStdString());
  track.data->desc = osmscout::gpx::Optional<std::string>::of(track.description.toStdString());

  QSqlQuery sql(db);
  sql.prepare("SELECT `id` FROM `track_segment` WHERE track_id = :trackId;");
  sql.bindValue(":trackId", QVariant::fromValue(track.id));
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Loading segments for track id" << track.id << "fails";
  }else{
    while (sql.next()) {
      osmscout::gpx::TrackSegment segment;
      long segmentId = VarToLong(sql.value("id"));
      loadTrackPoints(segmentId, segment);
      track.data->segments.push_back(std::move(segment));
    }
  }

  emit trackDataLoaded(track, true);
}

void Storage::updateOrCreateCollection(Collection collection)
{
  if (!checkAccess("updateOrCreateCollection")){
    emit collectionsLoaded(std::vector<Collection>(), false);
    return;
  }

  QSqlQuery sql(db);
  if (collection.id < 0){
    sql.prepare(
      "INSERT INTO `collection` (`name`, `description`) VALUES (:name, :description);");
    sql.bindValue(":name", collection.name);
    sql.bindValue(":description", collection.description);
  }else {
    sql.prepare(
      "UPDATE `collection` SET `name` = :name, `description` = :description WHERE (`id` = :id);");
    sql.bindValue(":id", QVariant::fromValue(collection.id));
    sql.bindValue(":name", collection.name);
    sql.bindValue(":description", collection.description);
  }
  sql.exec();
  if (sql.lastError().isValid()){
    qWarning() << "Creating version table entry failed" << sql.lastError();
  }

  loadCollections();
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
    QThread *thread = OSMScoutQt::GetInstance().makeThread("OverlayTileLoader");
    storage = new Storage(thread, directory);
    storage->moveToThread(thread);
    connect(thread, SIGNAL(started()),
            storage, SLOT(init()));
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
