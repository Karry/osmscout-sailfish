/*
  OSMScout for SFOS
  Copyright (C) 2020 Lukas Karas

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

class Tracker : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Tracker)

  Q_PROPERTY(bool tracking READ isTracking NOTIFY trackingChanged)
  Q_PROPERTY(bool canBeResumed READ canBeResumed NOTIFY openTrackLoaded)
  Q_PROPERTY(qint64 openTrackId READ getOpenTrackId NOTIFY openTrackLoaded)
  Q_PROPERTY(QString openTrackName READ getOpenTrackName NOTIFY openTrackLoaded)

  Q_PROPERTY(bool processing READ isProcessing NOTIFY trackingChanged)

  Q_PROPERTY(QString name READ getName NOTIFY trackingChanged)
  Q_PROPERTY(QString description READ getDescription NOTIFY trackingChanged)

  // track statistics
  Q_PROPERTY(QDateTime from READ getFrom NOTIFY statisticsUpdated)
  Q_PROPERTY(QDateTime to READ getTo NOTIFY statisticsUpdated)
  Q_PROPERTY(double distance READ getDistance NOTIFY statisticsUpdated)
  Q_PROPERTY(double rawDistance READ getRawDistance() NOTIFY statisticsUpdated)
  Q_PROPERTY(qint64 duration /* ms */ READ getDuration() NOTIFY statisticsUpdated)
  Q_PROPERTY(qint64 movingDuration /* ms */ READ getMovingDuration() NOTIFY statisticsUpdated)
  Q_PROPERTY(double maxSpeed /* m/s */ READ getMaxSpeed() NOTIFY statisticsUpdated)
  Q_PROPERTY(double averageSpeed /* m/s */ READ getAverageSpeed() NOTIFY statisticsUpdated)
  Q_PROPERTY(double movingAverageSpeed /* m/s */ READ getMovingAverageSpeed() NOTIFY statisticsUpdated)
  Q_PROPERTY(double ascent READ getAscent() NOTIFY statisticsUpdated)
  Q_PROPERTY(double descent READ getDescent() NOTIFY statisticsUpdated)
  Q_PROPERTY(double minElevation READ getMinElevation() NOTIFY statisticsUpdated)
  Q_PROPERTY(double maxElevation READ getMaxElevation() NOTIFY statisticsUpdated)

signals:
  // for UI
  void openTrackLoaded(QString trackId, QString name);
  void error(QString message);
  void trackingChanged();
  void statisticsUpdated();

  // for storage
  void openTrackRequested();
  void createTrackRequest(qint64 collectionId, QString name, QString description, bool open);
  void closeTrackRequest(qint64 collectionId, qint64 trackId);
  void appendNodesRequest(qint64 trackId,
                          std::shared_ptr<std::vector<osmscout::gpx::TrackPoint>> batch,
                          bool createNewSegment);

public slots:
  // for Storage
  void init();
  void onOpenTrackLoaded(Track track, bool ok);
  void onTrackCreated(qint64 collectionId, qint64 trackId, QString name);

  // slot for UI
  void resumeTrack(QString trackId);
  void closeOpen(QString trackId);
  void startTracking(QString collectionId, QString trackName, QString trackDescription);
  void stopTracking();

  void locationChanged(bool locationValid,
                       double lat, double lon,
                       bool horizontalAccuracyValid, double horizontalAccuracy,
                       bool elevationValid, double elevation,
                       bool verticalAccuracyValid, double verticalAccuracy);

public:
  Tracker();
  virtual ~Tracker() = default;

  bool isTracking() const {
    return track.id >= 0;
  }

  bool canBeResumed() const {
    return recentOpenTrack.id >= 0;
  }

  qint64 getOpenTrackId() const {
    return recentOpenTrack.id;
  }

  QString getOpenTrackName() const {
    return recentOpenTrack.name;
  }

  bool isProcessing() const {
    return creationRequested;
  }

  QString getName() const;
  QString getDescription() const;

  QDateTime getFrom() const;
  QDateTime getTo() const;
  double getDistance() const;
  double getRawDistance() const;
  qint64 getDuration() const;
  qint64 getMovingDuration() const;
  double getMaxSpeed() const;
  double getAverageSpeed() const;
  double getMovingAverageSpeed() const;
  double getAscent() const;
  double getDescent() const;
  double getMinElevation() const;
  double getMaxElevation() const;

private:
  void flushBatch(bool createNewSegment);

private:
  bool creationRequested{false};
  Track track;
  Track recentOpenTrack;
  std::shared_ptr<std::vector<osmscout::gpx::TrackPoint>> batch{std::make_shared<std::vector<osmscout::gpx::TrackPoint>>()};
  TrackStatisticsAccumulator accumulator;
};
