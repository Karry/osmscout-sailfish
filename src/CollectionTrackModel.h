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
#include <QPointF>

class CollectionTrackModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString trackId READ getTrackId WRITE setTrackId NOTIFY loadingChanged)
  Q_PROPERTY(QString collectionId READ getCollectionId NOTIFY loadingChanged)
  Q_PROPERTY(QString name READ getName NOTIFY loadingChanged)
  Q_PROPERTY(QString description READ getDescription NOTIFY loadingChanged)

  Q_PROPERTY(QDateTime from READ getFrom NOTIFY loadingChanged)
  Q_PROPERTY(QDateTime to READ getTo NOTIFY loadingChanged)
  Q_PROPERTY(double distance READ getDistance NOTIFY loadingChanged)
  Q_PROPERTY(double rawDistance READ getRawDistance() NOTIFY loadingChanged)
  Q_PROPERTY(qint64 duration /* ms */ READ getDuration() NOTIFY loadingChanged)
  Q_PROPERTY(qint64 movingDuration /* ms */ READ getMovingDuration() NOTIFY loadingChanged)
  Q_PROPERTY(double maxSpeed /* m/s */ READ getMaxSpeed() NOTIFY loadingChanged)
  Q_PROPERTY(double averageSpeed /* m/s */ READ getAverageSpeed() NOTIFY loadingChanged)
  Q_PROPERTY(double movingAverageSpeed /* m/s */ READ getMovingAverageSpeed() NOTIFY loadingChanged)
  Q_PROPERTY(double ascent READ getAscent() NOTIFY loadingChanged)
  Q_PROPERTY(double descent READ getDescent() NOTIFY loadingChanged)
  Q_PROPERTY(double minElevation READ getMinElevation() NOTIFY loadingChanged)
  Q_PROPERTY(double maxElevation READ getMaxElevation() NOTIFY loadingChanged)

  Q_PROPERTY(QObject *boundingBox READ getBBox NOTIFY bboxChanged)
  Q_PROPERTY(int segmentCount READ getSegmentCount NOTIFY loadingChanged)

  Q_PROPERTY(quint64 pointCount READ getPointCount NOTIFY loadingChanged)

  Q_PROPERTY(double accuracyFilter /* m */ READ getAccuracyFilter WRITE setAccuracyFilter NOTIFY loadingChanged)

signals:
  void loadingChanged();
  void bboxChanged();
  void trackDataRequest(Track track, std::optional<double>);

  // track edits
  void cropStartRequest(Track track, quint64 position);
  void cropEndRequest(Track track, quint64 position);
  void splitRequest(Track track, quint64 position);
  void filterNodesRequest(Track track, std::optional<double> accuracyFilter);
  void setColorRequest(Track track, std::optional<osmscout::Color> colorOpt);

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onTrackDataLoaded(Track track, std::optional<double>, bool complete, bool ok);

public:
  CollectionTrackModel();
  virtual ~CollectionTrackModel() = default;

  bool isLoading() const;
  QString getTrackId() const;
  void setTrackId(QString id);

  QString getCollectionId() const;

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
  double getAccuracyFilter() const;
  void setAccuracyFilter(double accuracyFilter);

  QObject *getBBox() const;
  int getSegmentCount() const;
  quint64 getPointCount() const;
  Q_INVOKABLE QObject* createOverlayForSegment(int segment);
  Q_INVOKABLE QPointF getPoint(quint64 index) const;

  Q_INVOKABLE void cropStart(quint64 position);
  Q_INVOKABLE void cropEnd(quint64 position);
  Q_INVOKABLE void split(quint64 position);
  Q_INVOKABLE void filterNodes(double accuracyFilter);
  Q_INVOKABLE void setupColor(const QString &color);

private:
  bool loading{false};
  std::optional<double> accuracyFilter{std::nullopt};
  Track track;
};
