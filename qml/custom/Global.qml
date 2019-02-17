/*
 OSM Scout for Sailfish OS
 Copyright (C) 2019 Lukas Karas

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

pragma Singleton

import QtQuick 2.0
import Sailfish.Silica 1.0
import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "Utils.js" as Utils

Item {
    property var mapPage
    property var mainMap
    property var remorse
    property alias routingModel: routingModel
    property alias navigationModel: navigationModel

    NavigationModel {
        id: navigationModel

        property LocationEntry destination
        property string vehicle: "car"
        property bool destinationSet: false

        onDestinationChanged: {
            destinationSet = destination != null && destination.type != "none";
            if (!destinationSet){
                navigationModel.route = null;
            }
        }
        onVehicleChanged: {
            console.log("vehicle changed to " + vehicle);
            // TODO: make sure that navigation model trigger re-route request
        }
        onRerouteRequest: {
            if (routingModel.ready){
                reroute();
            }
        }
        onTargetReached: {
            //: %1 is distance, %2 is bearing (north, south...)
            remorse.execute(qsTr("Target reached, in %1 %2. Closing navigation.")
                                .arg(Utils.humanDistance(targetDistance))
                                .arg(Utils.humanBearing(targetBearing)),
                            function() {
                                clear();
                            },
                            10 * 1000);
        }

        function setup(vehicle, route, destination){
            navigationModel.vehicle = vehicle;
            navigationModel.route = route;
            navigationModel.destination = destination;
        }

        function reroute(){
            if (!navigationModel.destinationSet){
                return;
            }

            var startLoc = routingModel.locationEntryFromPosition(positionSource.lat, positionSource.lon);
            console.log("Navigation rerouting \"" + Utils.locationStr(startLoc) + "\" -> \"" + Utils.locationStr(navigationModel.destination) + "\" by " + navigationModel.vehicle);
            routingModel.rerouteRequested = true;
            routingModel.setStartAndTarget(startLoc, navigationModel.destination, navigationModel.vehicle);
        }

        function clear(){
            navigationModel.destination = null;
            navigationModel.route = null;
        }
    }

    RoutingListModel {
        id: routingModel

        property bool rerouteRequested: false

        onComputingChanged: {
            if (routingModel.ready && routingModel.rerouteRequested){
                routingModel.rerouteRequested = false;
                navigationModel.route = routingModel.route;
            }
        }
    }

    PositionSource {
        id: positionSource

        active: navigationModel.destinationSet

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

            if (navigationModel.destinationSet){
                navigationModel.locationChanged(valid, // valid
                                                lat, lon,
                                                position.horizontalAccuracyValid,
                                                position.horizontalAccuracy);
            }
            // console.log("position: " + latitude + " " + longitude);

            //if ((!navigationModel.positionOnRoute) &&
            //        routingModel.ready &&
            //        navigationModel.destinationSet){
            //    reroute();
            //}
        }
    }

}
