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

import QtQuick 2.0
import QtPositioning 5.2

import Sailfish.Silica 1.0

import "pages"
import harbour.osmscout.map 1.0

ApplicationWindow {
    id: mainWindow

    function reroute(){
        if (!navigationModel.destinationSet){
            return;
        }

        var startLoc = routingModel.locationEntryFromPosition(positionSource.lat, positionSource.lon);
        console.log("We leave route, reroute from " + startLoc.label + " -> " + navigationModel.destination.label);
        routingModel.setStartAndTarget(startLoc, navigationModel.destination);
    }

    PositionSource {
        id: positionSource

        active: true

        property bool valid: false;
        property double lat: 0.0;
        property double lon: 0.0;

        onValidChanged: {
            console.log("Positioning is " + valid)
            console.log("Last error " + sourceError)

            for (var m in supportedPositioningMethods) {
                console.log("Method " + m)
            }
        }

        onPositionChanged: {
            positionSource.valid = position.latitudeValid && position.longitudeValid;
            positionSource.lat = position.coordinate.latitude;
            positionSource.lon = position.coordinate.longitude;

            navigationModel.locationChanged(valid, // valid
                                            lat, lon,
                                            position.horizontalAccuracyValid, position.horizontalAccuracy);
            // console.log("position: " + latitude + " " + longitude);

            if ((!navigationModel.positionOnRoute) &&
                    routingModel.ready &&
                    navigationModel.destinationSet){
                reroute();
            }
        }
    }

    NavigationModel {
        id: navigationModel
        route: routingModel.route

        property LocationEntry destination
        property bool destinationSet: destination != null && destination.type != "none"

        onDestinationChanged: {
            reroute();
        }

        onPositionOnRouteChanged: {
            if (!positionOnRoute){
                reroute();
            }
        }
    }
    RoutingListModel {
        id: routingModel
    }

    property alias globalNavigationModel: navigationModel
    property alias globalRoutingModel: routingModel

    initialPage: Component {
        MapPage{
            navigationModel: globalNavigationModel
            routingModel: globalRoutingModel
        }
    }

    cover: Component {
        Cover{
            navigationModel: globalNavigationModel
            routingModel: globalRoutingModel
        }
    }

    allowedOrientations: Orientation.All
    _defaultPageOrientations: Orientation.All
}
