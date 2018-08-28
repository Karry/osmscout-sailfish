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

#ifndef OSMSCOUT_SAILFISH_COLLECTIONWAYMODEL_H
#define OSMSCOUT_SAILFISH_COLLECTIONWAYMODEL_H


#include "Storage.h"

#include <QObject>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QSet>

class CollectionTrackModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString trackId READ getTrackId WRITE setTrackId NOTIFY loadingChanged)
  Q_PROPERTY(QString name READ getName NOTIFY loadingChanged)
  Q_PROPERTY(QString description READ getDescription NOTIFY loadingChanged)
  Q_PROPERTY(double distance READ getDistance NOTIFY loadingChanged)
  Q_PROPERTY(QObject *boundingBox READ getBBox NOTIFY bboxChanged)
  Q_PROPERTY(int segmentCount READ getSegmentCount NOTIFY loadingChanged)

signals:
  void loadingChanged();
  void bboxChanged();
  void trackDataRequest(Track track);

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onTrackDataLoaded(Track track, bool complete, bool ok);

public:
  CollectionTrackModel();
  ~CollectionTrackModel(){};

  bool isLoading() const;
  QString getTrackId() const;
  void setTrackId(QString id);

  QString getName() const;
  QString getDescription() const;
  double getDistance() const;
  QObject *getBBox() const;
  int getSegmentCount() const;
  Q_INVOKABLE QObject* createOverlayForSegment(int segment);

private:
  bool loading{false};
  Track track;
};

#endif //OSMSCOUT_SAILFISH_COLLECTIONWAYMODEL_H
