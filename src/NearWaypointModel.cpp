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

#include "NearWaypointModel.h"

#include <osmscout/util/Geometry.h>
#include <osmscoutclientqt/LocationEntry.h>
#include <osmscoutclientqt/AdminRegionInfo.h>

NearWaypointModel::NearWaypointModel()
{
  Storage *storage = Storage::getInstance();
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &NearWaypointModel::storageInitialised,
          Qt::QueuedConnection);

  connect(storage, &Storage::nearbyWaypoints,
          this, &NearWaypointModel::onNearbyWaypoints,
          Qt::QueuedConnection);

  connect(this, &NearWaypointModel::nearbyWaypointsRequest,
          storage, &Storage::loadNearbyWaypoints,
          Qt::QueuedConnection);

  storageInitialised();
}

void NearWaypointModel::storageInitialised()
{
  load();
}

void NearWaypointModel::load()
{
  if (searchCenter.GetLon() == INVALID_COORD || searchCenter.GetLat() == INVALID_COORD) {
    searching=false;
    emit SearchingChanged(searching);
    return;
  }
  searching=true;
  emit SearchingChanged(searching);
  emit nearbyWaypointsRequest(searchCenter, maxDistance);
}

void NearWaypointModel::onNearbyWaypoints(const osmscout::GeoCoord &center,
                                          const osmscout::Distance &distance,
                                          const std::vector<Storage::WaypointNearby> &waypoints)
{
  if (center != searchCenter || distance != maxDistance){
    return;
  }
  beginResetModel();
  items=waypoints; // Result is sorted by distance from center already.
  endResetModel();
  searching=false;
  emit SearchingChanged(searching);
}

int NearWaypointModel::rowCount(const QModelIndex &) const
{
  return items.size();
}

QVariant NearWaypointModel::data(const QModelIndex &index, int role) const
{
  if(index.row() < 0 || index.row() >= (int)items.size()) {
    return QVariant();
  }

  const auto& [distance, waypoint]=items.at(index.row());

  switch (role) {
    case Qt::DisplayRole:
    case NameRole:
      return waypoint.data.name ? QString::fromStdString(waypoint.data.name.value()) : QVariant();
    case LatRole:
      return QVariant::fromValue(waypoint.data.coord.GetLat());
    case LonRole:
      return QVariant::fromValue(waypoint.data.coord.GetLon());
    case DistanceRole:
      if (searchCenter.GetLat()!=INVALID_COORD && searchCenter.GetLon()!=INVALID_COORD) {
        return distance.AsMeter();
      }else{
        return 0;
      }
    case BearingRole:
      if (searchCenter.GetLat()!=INVALID_COORD && searchCenter.GetLon()!=INVALID_COORD) {
        return QString::fromStdString(
          osmscout::GetSphericalBearingInitial(searchCenter, waypoint.data.coord).LongDisplayString());
      }else{
        return "";
      }
    default:
      break;
  }

  return QVariant();
}

QObject* NearWaypointModel::get(int row) const
{
  if(row < 0 || row >= (int)items.size()) {
    return nullptr;
  }

  const auto& [distance, waypoint]=items.at(row);

  // QML will take ownerhip
  return new osmscout::LocationEntry(
    osmscout::LocationEntry::typeObject,
    QString::fromStdString(waypoint.data.name ? waypoint.data.name.value() : waypoint.data.coord.GetDisplayText()),
    "",
    "",
    QList<osmscout::AdminRegionInfoRef>(),
    "",
    waypoint.data.coord,
    osmscout::GeoBox::BoxByCenterAndRadius(waypoint.data.coord, osmscout::Meters(2.0)));
}

QHash<int, QByteArray> NearWaypointModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[NameRole]="name";
  roles[LatRole]="lat";
  roles[LonRole]="lon";
  roles[DistanceRole]="distance";
  roles[BearingRole]="bearing";

  return roles;
}

Qt::ItemFlags NearWaypointModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
