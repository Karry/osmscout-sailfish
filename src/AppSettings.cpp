/*
  OSMScout for SFOS
  Copyright (C) 2017 Lukas Karas

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

#include "AppSettings.h"

#include <osmscout/OSMScoutQt.h>

#include <QDebug>

using namespace osmscout;

AppSettings::AppSettings(): view(nullptr)
{
}

MapView *AppSettings::GetMapView()
{
  if (view == nullptr){
    double lat   = settings.value("settings/map/lat",   0).toDouble();
    double lon   = settings.value("settings/map/lon",   0).toDouble();
    //double angle = settings.value("settings/map/angle", 0).toDouble();
    double mag   = settings.value("settings/map/mag",   osmscout::Magnification(osmscout::Magnification::magContinent).GetMagnification()).toDouble();
    view = new MapView(this,
                       osmscout::GeoCoord(lat, lon),
                       Bearing(),
                       osmscout::Magnification(mag),
                       OSMScoutQt::GetInstance().GetSettings()->GetMapDPI());
  }
  return view;
}

void AppSettings::SetMapView(QObject *o)
{
  MapView *updated = qobject_cast<MapView*>(o);
  if (updated == nullptr){
    qWarning() << "Failed to cast " << o << " to MapView*.";
    return;
  }
  bool changed = false;
  if (view == nullptr){
    view = new MapView(this,
                       osmscout::GeoCoord(updated->GetLat(), updated->GetLon()),
                       Bearing(), // keep angle zero in settings
                       osmscout::Magnification(updated->GetMag()),
                       updated->GetMapDpi());
    changed = true;
  }else{
    changed = *view != *updated;
    if (changed){
        view->operator =( *updated );
    }
  }
  if (changed){
    settings.setValue("settings/map/lat", view->GetLat());
    settings.setValue("settings/map/lon", view->GetLon());
    settings.setValue("settings/map/angle", view->GetAngle());
    settings.setValue("settings/map/mag", view->GetMag());
    emit MapViewChanged(view);
  }
}

const QString AppSettings::GetGpsFormat() const
{
  return settings.value("gpsFormat", "degrees").toString();
}

void AppSettings::SetGpsFormat(const QString formatId)
{
  if (GetGpsFormat() != formatId){
    settings.setValue("gpsFormat", formatId);
    emit GpsFormatChanged(formatId);
  }
}

bool AppSettings::GetHillShades() const
{
  return settings.value("hillShades", false).toBool();
}

void AppSettings::SetHillShades(bool b)
{
  if (b!=GetHillShades()) {
    settings.setValue("hillShades", b);
    emit HillShadesChanged(b);
  }
}

double AppSettings::GetHillShadesOpacity() const
{
  return settings.value("hillShadesOpacity", 0.6).toDouble();
}

void AppSettings::SetHillShadesOpacity(double d)
{
  if (d!=GetHillShadesOpacity()) {
    settings.setValue("hillShadesOpacity", d);
    emit HillShadesOpacityChanged(d);
  }
}

QString AppSettings::GetLastVehicle() const
{
  return settings.value("lastVehicle", "car").toString();
}

void AppSettings::SetLastVehicle(const QString vehicle)
{
  if (GetLastVehicle() != vehicle){
    settings.setValue("lastVehicle", vehicle);
    emit LastVehicleChanged(vehicle);
  }
}

QString AppSettings::GetLastCollection() const
{
  return settings.value("lastCollection", "-1").toString();
}
void AppSettings::SetLastCollection(const QString id)
{
  if (GetLastCollection() != id){
    settings.setValue("lastCollection", id);
    emit LastCollectionChanged(id);
  }
}

QString AppSettings::GetLastMapDirectory() const
{
  return settings.value("lastMapDirectory", "").toString();
}
void AppSettings::SetLastMapDirectory(const QString directory)
{
  if (GetLastCollection() != directory){
    settings.setValue("lastMapDirectory", directory);
    emit LastMapDirectoryChanged(directory);
  }
}

int AppSettings::GetExportAccuracy() const
{
  return settings.value("exportAccuracyIndex", "0").toInt();
}
void AppSettings::SetExportAccuracy(int accuracyIndex)
{
  if (GetExportAccuracy() != accuracyIndex){
    settings.setValue("exportAccuracyIndex", accuracyIndex);
    emit ExportAccuracyChanged(accuracyIndex);
  }
}

bool AppSettings::GetShowTrackerDistance() const
{
  return settings.value("showTrackerDistance", "true").toBool();
}
void AppSettings::SetShowTrackerDistance(bool b)
{
  if (GetShowTrackerDistance() != b){
    settings.setValue("showTrackerDistance", b);
    emit ShowTrackerDistanceChanged(b);
  }
}

bool AppSettings::GetShowElevation() const
{
  return settings.value("showElevation", "false").toBool();
}
void AppSettings::SetShowElevation(bool b)
{
  if (GetShowElevation() != b){
    settings.setValue("showElevation", b);
    emit ShowElevationChanged(b);
  }
}

bool AppSettings::GetShowAccuracy() const
{
  return settings.value("showAccuracy", "false").toBool();
}
void AppSettings::SetShowAccuracy(bool b)
{
  if (GetShowAccuracy() != b){
    settings.setValue("showAccuracy", b);
    emit ShowAccuracyChanged(b);
  }
}

bool AppSettings::GetWaypointFirst() const
{
  return settings.value("waypointFirst", "true").toBool();
}
void AppSettings::SetWaypointFirst(bool b)
{
  if (GetWaypointFirst() != b){
    settings.setValue("waypointFirst", b);
    emit WaypointFirstChanged(b);
  }
}

int AppSettings::GetCollectionOrdering() const
{
  return settings.value("collectionOrdering", "0").toInt();
}
void AppSettings::SetCollectionOrdering(int i)
{
  if (GetCollectionOrdering() != i){
    settings.setValue("collectionOrdering", i);
    emit CollectionOrderingChanged(i);
  }
}

int AppSettings::GetCollectionsOrdering() const
{
  return settings.value("collectionsOrdering", "0").toInt();
}
void AppSettings::SetCollectionsOrdering(int i)
{
  if (GetCollectionsOrdering() != i){
    settings.setValue("collectionsOrdering", i);
    emit CollectionsOrderingChanged(i);
  }
}

bool AppSettings::GetShowMapOrientation() const
{
  return settings.value("showMapOrientation", "true").toBool();
}
void AppSettings::SetShowMapOrientation(bool b)
{
  if (GetShowMapOrientation() != b){
    settings.setValue("showMapOrientation", b);
    emit ShowMapOrientationChanged(b);
  }
}

bool AppSettings::GetShowCurrentPosition() const
{
  return settings.value("showCurrentPosition", "true").toBool();
}
void AppSettings::SetShowCurrentPosition(bool b)
{
  if (GetShowCurrentPosition() != b){
    settings.setValue("showCurrentPosition", b);
    emit ShowCurrentPositionChanged(b);
  }
}

bool AppSettings::GetShowNewPlace() const
{
  return settings.value("showNewPlace", "false").toBool();
}
void AppSettings::SetShowNewPlace(bool b)
{
  if (GetShowNewPlace() != b){
    settings.setValue("showNewPlace", b);
    emit ShowNewPlaceChanged(b);
  }
}

bool AppSettings::GetNavigationKeepAlive() const
{
  return settings.value("navigationKeepAlive", "true").toBool();
}
void AppSettings::SetNavigationKeepAlive(bool b)
{
  if (GetNavigationKeepAlive() != b){
    settings.setValue("navigationKeepAlive", b);
    emit NavigationKeepAliveChanged(b);
  }
}

bool AppSettings::GetRoadBikeAllowMainRoads() const
{
  return settings.value("roadBikeAllowMainRoads", "true").toBool();
}
void AppSettings::SetRoadBikeAllowMainRoads(bool b)
{
  if (GetRoadBikeAllowMainRoads() != b){
    settings.setValue("roadBikeAllowMainRoads", b);
    emit RoadBikeAllowMainRoadsChanged(b);
  }
}

bool AppSettings::GetMountainBikeAllowMainRoads() const
{
  return settings.value("mountainBikeAllowMainRoads", "true").toBool();
}
void AppSettings::SetMountainBikeAllowMainRoads(bool b)
{
  if (GetMountainBikeAllowMainRoads() != b){
    settings.setValue("mountainBikeAllowMainRoads", b);
    emit MountainBikeAllowMainRoadsChanged(b);
  }
}

bool AppSettings::GetFootAllowMainRoads() const
{
  return settings.value("footAllowMainRoads", "false").toBool();
}
void AppSettings::SetFootAllowMainRoads(bool b)
{
  if (GetFootAllowMainRoads() != b){
    settings.setValue("footAllowMainRoads", b);
    emit FootAllowMainRoadsChanged(b);
  }
}

bool AppSettings::GetRoadBikeAllowFootways() const
{
  return settings.value("roadBikeAllowFootways", "true").toBool();
}
void AppSettings::SetRoadBikeAllowFootways(bool b)
{
  if (GetRoadBikeAllowFootways() != b){
    settings.setValue("roadBikeAllowFootways", b);
    emit RoadBikeAllowFootwaysChanged(b);
  }
}

bool AppSettings::GetMountainBikeAllowFootways() const
{
  return settings.value("mountainBikeAllowFootways", "true").toBool();
}
void AppSettings::SetMountainBikeAllowFootways(bool b)
{
  if (GetMountainBikeAllowFootways() != b){
    settings.setValue("mountainBikeAllowFootways", b);
    emit MountainBikeAllowFootwaysChanged(b);
  }
}
