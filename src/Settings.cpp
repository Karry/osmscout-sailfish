/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2013  Tim Teulings

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

#include <QScreen>
#include <QGuiApplication>
#include <qt5/QtCore/qobject.h>

#include "Settings.h"

Settings::Settings()
{
    /* Warning: Sailfish OS before version 2.0.1 reports incorrect DPI (100)
     *
     * Some DPI values:
     *
     * ~ 330 - Jolla tablet native
     *   242.236 - Jolla phone native
     *   130 - PC (24" FullHD)
     *   100 - Qt default (reported by SailfishOS < 2.0.1)
     */    
    QScreen *srn=QGuiApplication::screens().at(0);
    physicalDpi = (double)srn->physicalDotsPerInch();
}

Settings::~Settings()
{
    settings.sync();
}

void Settings::SetMapDPI(double dpi)
{
    settings.setValue("settings/map/dpi", (unsigned int)dpi);
    emit MapDPIChange(dpi);
}

double Settings::GetMapDPI() const
{
  // With mobile device user eyes are closer to screen than PC monitor, 
  // we render thinks a bit smaller (0.75)...
  return (size_t)settings.value("settings/map/dpi",physicalDpi * 0.75).toDouble();
}

osmscout::Vehicle Settings::GetRoutingVehicle() const
{
  return (osmscout::Vehicle)settings.value("routing/vehicle",osmscout::vehicleCar).toUInt();
}

void Settings::SetRoutingVehicle(const osmscout::Vehicle& vehicle)
{
  settings.setValue("routing/vehicle", (unsigned int)vehicle);
}

static Settings* settingsInstance=NULL;

Settings* Settings::GetInstance()
{
    if (settingsInstance == NULL){
        settingsInstance = new Settings();
    }
    return settingsInstance;
}

void Settings::FreeInstance()
{
    if (settingsInstance != NULL){
        delete settingsInstance;
        settingsInstance = NULL;
    }
}

QmlSettings::QmlSettings()
{
    connect(Settings::GetInstance(), SIGNAL(MapDPIChange(dpi)), 
            this, SIGNAL(MapDPIChange(dpi)));
}

void QmlSettings::SetMapDPI(double dpi)
{
    Settings::GetInstance()->SetMapDPI(dpi);
}

double QmlSettings::GetMapDPI() const
{
    return Settings::GetInstance()->GetMapDPI();
}
