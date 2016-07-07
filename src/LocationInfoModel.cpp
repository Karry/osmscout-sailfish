
/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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

#include "LocationInfoModel.h"
LocationInfoModel::LocationInfoModel(): 
ready(false), setup(false)
{
    DBThread *dbThread = DBThread::GetInstance();
    
    connect(dbThread, SIGNAL(InitialisationFinished(const DatabaseLoadedResponse&)), 
            this, SLOT(dbInitialized(const DatabaseLoadedResponse&)),
            Qt::QueuedConnection);
}

void LocationInfoModel::setCoords(const double lat, const double lon)
{
    coord = osmscout::GeoCoord(lat, lon);
    setup = true;
    ready = false;

    DBThread *dbThread = DBThread::GetInstance();
    if (dbThread->isInitialized()){
        // TODO: search
    }
}

void LocationInfoModel::dbInitialized(const DatabaseLoadedResponse&)
{
    if (setup){
        // TODO: search
    }
}



