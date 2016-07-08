
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
    
    qRegisterMetaType<osmscout::GeoCoord>("osmscout::GeoCoord");
    qRegisterMetaType<osmscout::LocationDescription>("osmscout::LocationDescription");
    
    connect(dbThread, SIGNAL(InitialisationFinished(const DatabaseLoadedResponse&)), 
            this, SLOT(dbInitialized(const DatabaseLoadedResponse&)),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(locationDescriptionRequested(const osmscout::GeoCoord)), 
            dbThread, SLOT(requestLocationDescription(const osmscout::GeoCoord)),
            Qt::QueuedConnection);
    
    connect(dbThread, SIGNAL(locationDescription(const osmscout::GeoCoord, const osmscout::LocationDescription)), 
            this, SLOT(onLocationDescription(const osmscout::GeoCoord, const osmscout::LocationDescription)),
            Qt::QueuedConnection);
    
}

void LocationInfoModel::setLocation(const double lat, const double lon)
{
    location = osmscout::GeoCoord(lat, lon);
    setup = true;
    ready = false;
    emit readyChange(ready);

    DBThread *dbThread = DBThread::GetInstance();
    if (dbThread->isInitialized()){
        emit locationDescriptionRequested(location);
    }
}

void LocationInfoModel::dbInitialized(const DatabaseLoadedResponse&)
{
    if (setup){
        emit locationDescriptionRequested(location);
    }
}

Qt::ItemFlags LocationInfoModel::flags(const QModelIndex &index) const
{
    if(!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> LocationInfoModel::roleNames() const
{
    QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

    roles[LabelRole]="label";

    return roles;
}

QVariant LocationInfoModel::data(const QModelIndex &index, int role) const
{
    if (!ready){
        return QVariant();
    }
    if(index.row() < 0 || index.row() >= 1) {
        return QVariant();
    }
    
    //osmscout::LocationCoordDescriptionRef coordDescription=description.GetCoordDescription();
    osmscout::LocationAtPlaceDescriptionRef atAddressDescription=description.GetAtAddressDescription();
    
    if (atAddressDescription) {
        osmscout::Place place = atAddressDescription->GetPlace();
        
        switch (role) {
        case Qt::DisplayRole:
        case LabelRole:
            return QString::fromStdString(place.GetDisplayString());
        case BearingRole:
            return QString::fromStdString(osmscout::BearingDisplayString(atAddressDescription->GetBearing()));
        default:
            break;
        }
    }
    return QVariant();    
}

void LocationInfoModel::onLocationDescription(const osmscout::GeoCoord location, 
                                              const osmscout::LocationDescription description)
{
    if (location != this->location){
        return; // not our request
    }
    this->description = description;
    
    osmscout::LocationAtPlaceDescriptionRef atAddressDescription=description.GetAtAddressDescription();    
    if (atAddressDescription) {
        
        osmscout::Place place = atAddressDescription->GetPlace();
        
        if (atAddressDescription->IsAtPlace()){
            qDebug() << "Place " << QString::fromStdString(location.GetDisplayText()) << " description: " 
                     << QString::fromStdString(place.GetDisplayString()); 
        }else{
            qDebug() << "Place " << QString::fromStdString(location.GetDisplayText()) << " description: " 
                     << atAddressDescription->GetDistance() << " m " 
                     << QString::fromStdString(osmscout::BearingDisplayString(atAddressDescription->GetBearing())) << " from "
                     << QString::fromStdString(place.GetDisplayString());
        }
    }else{
        qWarning() << "No place description found for " << QString::fromStdString(location.GetDisplayText()) << "";
    }
    
    ready = true;
    emit readyChange(ready);
}
