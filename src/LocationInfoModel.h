
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

#ifndef LOCATIONINFOMODEL_H
#define	LOCATIONINFOMODEL_H

#include <QObject>
#include <QAbstractListModel>

#include <osmscout/GeoCoord.h>
#include <osmscout/util/GeoBox.h>

#include "DBThread.h"

class LocationInfoModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChange)

signals:
    void locationDescriptionRequested(const osmscout::GeoCoord location);
    void readyChange(bool ready);

public slots:
    void setLocation(const double lat, const double lon);
    void dbInitialized(const DatabaseLoadedResponse&);
    void onLocationDescription(const osmscout::GeoCoord location, const osmscout::LocationDescription description);

public:
    enum Roles {
        LabelRole = Qt::UserRole,
        RegionRole = Qt::UserRole+1,
        AddressRole = Qt::UserRole+2,
        InPlaceRole = Qt::UserRole+3,
        DistanceRole = Qt::UserRole+4,
        BearingRole = Qt::UserRole+5,
        PoiRole = Qt::UserRole+6
    };

public:
    LocationInfoModel();
    virtual inline ~LocationInfoModel(){};

    int inline rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        return ready? 1:0;
    };
    
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    
    bool inline isReady() const 
    {
        return ready;
    };
    
    Q_INVOKABLE double distance(double lat1, double lon1, 
                                 double lat2, double lon2);
    Q_INVOKABLE QString bearing(double lat1, double lon1, 
                                double lat2, double lon2);
    
private:
    bool ready;
    bool setup;
    osmscout::GeoCoord location;
    osmscout::LocationDescription description;
    
};

#endif	/* LOCATIONINFOMODEL_H */

