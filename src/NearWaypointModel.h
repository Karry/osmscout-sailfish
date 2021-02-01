/*
  OSMScout for SFOS
  Copyright (C) 2021 Lukas Karas

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

class NearWaypointModel : public QAbstractListModel {
  Q_OBJECT

  /**
   * Count of rows in model - count of search results
   */
  Q_PROPERTY(int      count       READ rowCount    NOTIFY countChanged)

  /**
   * True if searching is in progress
   */
  Q_PROPERTY(bool     searching   READ isSearching NOTIFY SearchingChanged)

  /**
   * Lat and lon properties control where is logical search center.
   * Local admin region is used as default region,
   * databases used for search are sorted by distance from this point
   * (local results should be available faster).
   */
  Q_PROPERTY(double   lat         READ GetLat      WRITE SetLat)

  /**
   * \see lat property
   */
  Q_PROPERTY(double   lon         READ GetLon      WRITE SetLon)

  /**
   * Maximal distance of searched objects
   */
  Q_PROPERTY(double   maxDistance READ GetMaxDistance WRITE SetMaxDistance)

public:
  constexpr static double INVALID_COORD = -1000.0;

signals:
  void countChanged(int);

  void SearchingChanged(bool);

  void nearbyWaypointsRequest(const osmscout::GeoCoord &center, const osmscout::Distance &distance);

public slots:
  void storageInitialised();

  void onNearbyWaypoints(const osmscout::GeoCoord &center,
                         const osmscout::Distance &distance,
                         const std::vector<Storage::WaypointNearby> &waypoints);

public:
  NearWaypointModel();

  virtual ~NearWaypointModel() = default;

  enum Roles {
    NameRole = Qt::UserRole,
    LatRole = Qt::UserRole +3,
    LonRole = Qt::UserRole +4,
    DistanceRole = Qt::UserRole +5,
    BearingRole = Qt::UserRole +6,
  };
  Q_ENUM(Roles)

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  inline bool isSearching() const
  {
    return searching;
  }

  inline double GetLat() const
  {
    return searchCenter.GetLat();
  }

  void SetLat(double lat)
  {
    if (lat!=searchCenter.GetLat()) {
      searchCenter.Set(lat, searchCenter.GetLon());
      load();
    }
  }

  inline double GetLon() const
  {
    return searchCenter.GetLon();
  }

  void SetLon(double lon)
  {
    if (lon!=searchCenter.GetLon()){
      searchCenter.Set(searchCenter.GetLat(), lon);
      load();
    }
  }

  inline double GetMaxDistance() const
  {
    return maxDistance.AsMeter();
  }

  void SetMaxDistance(double d)
  {
    if (maxDistance.AsMeter()!=d){
      maxDistance=osmscout::Distance::Of<osmscout::Meter>(d);
      load();
    }
  }

private:
  void load();

private:
  bool searching{false};
  osmscout::Distance maxDistance{osmscout::Distance::Of<osmscout::Kilometer>(1)};
  osmscout::GeoCoord searchCenter{INVALID_COORD,INVALID_COORD};
  std::vector<Storage::WaypointNearby> items;
};

