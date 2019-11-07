/*
 OSM Scout for Sailfish OS
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
import Sailfish.Silica 1.0

import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"
import ".." // Global singleton

CoverBackground {
    id: cover

    property bool isLightTheme: Theme.colorScheme == Theme.DarkOnLight

    property bool initialized: false;
    onStatusChanged: {
        if (status == PageStatus.Activating){
            if (!initialized){
                map.view = AppSettings.mapView;
                initialized = true;
            }
            map.lockToPosition = true;
            //console.log("cover activating... " + positionSource.active)
            //positionSource.active = true;
            map.locationChanged(Global.positionSource.positionValid,
                                Global.positionSource.lat, Global.positionSource.lon,
                                Global.positionSource.horizontalAccuracyValid, Global.positionSource.horizontalAccuracy,
                                Global.positionSource.lastUpdate);
        }
    }

    Connections {
        target: Global.positionSource
        onUpdate: {
            if (cover.status == PageStatus.Active){
                map.locationChanged(positionValid,
                                    lat, lon,
                                    horizontalAccuracyValid, horizontalAccuracy,
                                    lastUpdate);
            }
        }
    }
    Connections {
        target: Global.navigationModel
        onRouteChanged: {
            var way = Global.navigationModel.routeWay;
            if (way==null){
                map.removeOverlayObject(0);
            }else{
                map.addOverlayObject(0,way);
            }
        }
    }

    OpacityRampEffect {
        enabled: true
        offset: 1. - (header.height + Theme.paddingLarge) / map.height
        slope: map.height / Theme.paddingLarge / 3.
        direction: 3
        sourceItem: map
    }
    Rectangle{
        id: header

        height: icon.height + 2* Theme.paddingMedium
        visible: !Global.navigationModel.destinationSet
        x: Theme.paddingMedium

        Image{
            id: icon
            source: "image://theme/harbour-osmscout"
            x: 0
            y: Theme.paddingMedium
            height: Theme.fontSizeMedium * 1.5
            width: height
        }
        Label{
            id: headerText
            anchors{
                verticalCenter: parent.verticalCenter
                left: icon.right
                leftMargin: Theme.paddingSmall
            }
            text: qsTr("OSM Scout")
            font.pixelSize: Theme.fontSizeMedium
        }
    }

    Rectangle {
        id: nextStepBox

        x: Theme.paddingMedium
        height: nextStepIcon.height + 2* Theme.paddingMedium
        visible: Global.navigationModel.destinationSet
        color: "transparent"

        RouteStepIcon{
            id: nextStepIcon
            stepType: Global.navigationModel.nextRouteStep.type
            roundaboutExit: Global.navigationModel.nextRouteStep.roundaboutExit
            x: 0
            y: Theme.paddingMedium
            height: Theme.fontSizeMedium * 1.5
            width: height
        }
        Text{
            id: distanceToNextStep

            function humanDistance(distance){
                if (distance < 150){
                    return Math.round(distance/10)*10 + " "+ qsTr("meters");
                }
                if (distance < 2000){
                    return Math.round(distance/100)*100 + " "+ qsTr("meters");
                }
                return Math.round(distance/1000) + " "+ qsTr("km");
            }
            text: humanDistance(Global.navigationModel.nextRouteStep.distanceTo)
            color: Theme.primaryColor
            font.pixelSize: Theme.fontSizeMedium
            anchors{
                verticalCenter: parent.verticalCenter
                left: nextStepIcon.right
                leftMargin: Theme.paddingSmall
            }
        }
    }

    MapBase {
        id: map

        focus: true
        anchors.fill: parent

        CollectionMapBridge{
            map: map
        }

        followVehicle: Global.navigationModel.destinationSet
        renderingType: Global.navigationModel.destinationSet ? "plane" : "tiled"

        Connections {
            target: Global.navigationModel
            onDestinationSetChanged: {
                if (!Global.navigationModel.destinationSet){
                    console.log("Rotate back to 0");
                    map.rotateTo(0);
                    bindToCurrentPositionTimer.restart();
                }
            }
        }

        showCurrentPosition: Global.navigationModel.vehiclePosition == null
        lockToPosition: true
        vehiclePosition: Global.navigationModel.vehiclePosition
        vehicleIconSize: 10
    }

    Timer {
        id: bindToCurrentPositionTimer
        interval: 600 // zoom duration
        running: false
        repeat: false
        onTriggered: {
            if (!Global.navigationModel.destinationSet){
                map.lockToPosition = true;
            }
        }
    }
    CoverActionList {

        enabled: true
        iconBackground: true
        CoverAction {
            // installed custom image provider is not available in cover page
            iconSource: isLightTheme ? "../../pics/icon-cover-remove-dark.png" : "../../pics/icon-cover-remove.png"
            onTriggered: {
                map.zoomOut(2.0);
                bindToCurrentPositionTimer.restart();
            }
        }
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                map.zoomIn(2.0);
                bindToCurrentPositionTimer.restart();
            }
        }
    }
}
