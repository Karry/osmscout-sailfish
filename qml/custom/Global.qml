/*
 OSM Scout for Sailfish OS
 Copyright (C) 2020 Lukas Karas

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
// Sailfish.Silica components are not available on desktop, don't import it here...
// import Sailfish.Silica 1.0
import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "Utils.js" as Utils

Item {
    property var mapPage
    property var mainMap
    property var remorse
    property alias routingModel: routingModel
    property alias navigationModel: navigationModel
    property alias positionSource: positionSource
    property alias tracker: tracker

    Settings {
        id: settings
        onUnitsChanged: {
            Utils.distanceUnits = settings.units;
        }
        Component.onCompleted: {
            Utils.distanceUnits = settings.units;
            Utils.gpsFormat = AppSettings.gpsFormat;
            AppSettings.onGpsFormatChanged.connect(function(){
                Utils.gpsFormat = AppSettings.gpsFormat;
            });
        }
    }

    NavigationModel {
        id: navigationModel

        property LocationEntry destination
        property string vehicle: "car"
        property RoutingProfile profile: RoutingProfile{}
        property bool destinationSet: false

        onDestinationChanged: {
            destinationSet = destination != null && destination.type != "none";
            if (!destinationSet){
                navigationModel.route = null;
                if (!routingModel.ready){
                    routingModel.cancel();
                }
            }
            console.log("Navigation destination changed: " + Utils.locationStr(navigationModel.destination));
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

        function setup(vehicle, profile, route, destination){
            navigationModel.profile = profile;
            navigationModel.vehicle = vehicle;
            navigationModel.route = route;
            navigationModel.destination = destination;
            routingModel.rerouteRequested = false;
        }

        function reroute(){
            if (!navigationModel.destinationSet){
                return;
            }

            var startLoc = routingModel.locationEntryFromPosition(positionSource.lat, positionSource.lon);
            console.log("Navigation rerouting \"" + Utils.locationStr(startLoc) + "\" -> \"" + Utils.locationStr(navigationModel.destination) + "\" by " + navigationModel.vehicle);
            routingModel.rerouteRequested = true;
            routingModel.setStartAndTarget(startLoc, navigationModel.destination, navigationModel.profile);
        }

        function stop(){
            //: remorse dialog
            remorse.execute(qsTr("Stopping navigation"),
                            function() {
                                clear();
                            },
                            5 * 1000);
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
            console.log("Rerouting destination: " + Utils.locationStr(navigationModel.destination));
            if (routingModel.ready && routingModel.rerouteRequested && navigationModel.destinationSet){
                routingModel.rerouteRequested = false;
                navigationModel.route = routingModel.route;
            }
        }
    }

    Tracker {
        id: tracker
    }

    PositionSimulator {
        id: positionSimulator
        track: PositionSimulationTrack
        Component.onCompleted: {
            if (PositionSimulationTrack != ""){
                console.log("PositionSimulationTrack: " + PositionSimulationTrack)
            }
        }

        onPositionChanged: {
            console.log("simulate position: " + latitude + " " + longitude);

            positionSource.lat = latitude;
            positionSource.lon = longitude;
            positionSource.horizontalAccuracyValid = horizontalAccuracyValid;
            positionSource.horizontalAccuracy = horizontalAccuracy;

            positionSource.positionValid = !isNaN(latitude) && !isNaN(longitude);

            positionSource.lastUpdate = new Date();
            positionSource.altitudeValid = altitudeValid;
            if (altitudeValid){
                positionSource.altitude = altitude;
                positionSource.lastAltitudeUpdate = positionSource.lastUpdate;
                positionSource.verticalAccuracy = verticalAccuracy;
                positionSource.verticalAccuracyValid = verticalAccuracyValid;
            }

            positionSource.updateInternal();
        }
    }


    PositionSource {
        id: positionSource

        active: PositionSimulationTrack == ""

        updateInterval: 1000 // ms

        property bool positionValid: false
        property double lat: 0.0
        property double lon: 0.0

        property date lastUpdate: new Date(0)

        property bool horizontalAccuracyValid: false
        property double horizontalAccuracy: 0

        property double altitude: 0
        property bool altitudeValid: false
        property date lastAltitudeUpdate: new Date(0)

        property double verticalAccuracy: 0
        property bool verticalAccuracyValid: false

        signal update(bool positionValid, double lat, double lon, bool horizontalAccuracyValid, double horizontalAccuracy, date lastUpdate)

        // trigger update signal with recent data
        function updateRequest(){
            update(positionValid, lat, lon, horizontalAccuracyValid, horizontalAccuracy, lastUpdate);
        }

        function updateInternal() {
            // console.log("update internal: " + lat + " " + lon + " (active: " + positionSource.active + ")");
            update(positionValid, lat, lon, horizontalAccuracyValid, horizontalAccuracy, lastUpdate);

            // update location for navigation
            if (navigationModel.destinationSet){
                navigationModel.locationChanged(valid, // valid
                                                lat, lon,
                                                horizontalAccuracyValid,
                                                horizontalAccuracy);
            }
            // console.log("position: " + latitude + " " + longitude);
            if (tracker.tracking){
                tracker.locationChanged(lastUpdate,
                                        valid,
                                        lat, lon,
                                        horizontalAccuracyValid, horizontalAccuracy,
                                        altitudeValid, altitude,
                                        verticalAccuracyValid, verticalAccuracy);

            }
        }

        onPositionValidChanged: {
            console.log("Positioning is " + (positionValid ? "valid":"invalid"))
            console.log("Last error " + sourceError)
            console.log("Last update " + lastUpdate)

            for (var m in supportedPositioningMethods) {
                console.log("Method " + m)
            }
            update(positionValid, lat, lon, horizontalAccuracyValid, horizontalAccuracy, lastUpdate);
        }

        onPositionChanged: {
            positionSource.lat = position.coordinate.latitude;
            positionSource.lon = position.coordinate.longitude;
            positionSource.horizontalAccuracyValid = position.horizontalAccuracyValid;
            positionSource.horizontalAccuracy = position.horizontalAccuracy;

            positionSource.positionValid = position.latitudeValid &&
                                           position.longitudeValid &&
                                           !isNaN(lat) && !isNaN(lon);

            lastUpdate = position.timestamp;
            if (!lastUpdate ||
                // it seems that some GPS modules produces invalid time (year 2000)
                // https://together.jolla.com/question/225700/wrong-date-from-gps-on-jolla-1/?sort=votes&page=1
                // this is simple workaround, when timestamp is before 2020-04-21, we are using device time
                lastUpdate.getTime() < 1587448500800){
                console.log("GPS event timestamp is not valid!");
                lastUpdate = new Date();
            }
            if (position.altitudeValid){
                altitude = position.coordinate.altitude;
                altitudeValid = true;
                lastAltitudeUpdate = lastUpdate;
                verticalAccuracy = position.verticalAccuracy;
                verticalAccuracyValid = position.verticalAccuracyValid;
            }

            updateInternal();
        }
    }

}
