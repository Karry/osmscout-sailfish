/*
 OSM Scout for Sailfish OS
 Copyright (C) 2017  Lukas Karas

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

import QtQuick 2.2
import Sailfish.Silica 1.0

import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils
import ".." // Global singleton

Dialog {
    id: routeDescription

    property RoutingListModel route
    property bool routeReady: route != null && route.ready
    property bool failed: false
    property bool fromCurrentLocation: false
    property LocationEntry destination
    property var mapPage
    property var mainMap

    Component.onCompleted: {
        console.log("RouteDescription vehicle initialised: " + route.vehicle);
    }

    canAccept: routeReady

    acceptDestination: mapPage
    acceptDestinationAction: PageStackAction.Pop

    RemorsePopup { id: remorse }

    Connections {
        target: route
        onComputingChanged: {
            progressBar.opacity = routeReady ? 0:1;
        }

        onRouteFailed: {
            if (status==PageStatus.Active){
                pageStack.pop();
            }
            // to avoid warning "cannot pop while transition is in progress"
            // we setup failed flag and pop page later
            failed=true;
        }
        onRoutingProgress: {
            //console.log("progress: "+percent);
            progressBar.indeterminate=false;
            progressBar.value=percent;
            progressBar.valueText=percent+" %";
            progressBar.label=qsTr("Calculating the route")
        }
    }
    onStatusChanged: {
        if (failed && status==PageStatus.Active){
            pageStack.pop();
        }
    }

    onAccepted: {
        var routeWay=route.routeWay;
        //mainMap.addOverlayObject(0,routeWay);
        mapPage.showRoute(routeWay, 0);
        console.log("add overlay way \"" + routeWay.type + "\" ("+routeWay.size+" nodes)");
        if (fromCurrentLocation && destination && destination.type != "none"){
            console.log("Navigation destination: \"" + Utils.locationStr(destination) + "\" by " + route.vehicle + " profile: " + route.profile);
            Global.navigationModel.setup(route.vehicle, route.profile, route.route, destination)
        }
    }

    onRejected: {
        route.cancel();
    }

    DialogHeader {
        id: header
        //title: "Route"
        acceptText : fromCurrentLocation ? qsTr("Navigate") : qsTr("Accept")
        cancelText : routeReady ? "" : qsTr("Cancel")
    }


    SilicaListView {
        id: stepsView
        model: route

        VerticalScrollDecorator {}
        clip: true

        anchors{
            top: header.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium

        header: Column{
            visible: routeReady && route.count>0
            width: parent.width - 2*Theme.paddingMedium
            x: Theme.paddingMedium

            MapComponent {
                id: routePreviewMap
                showCurrentPosition: true
                width: parent.width
                height: Math.min(width, stepsView.height)

                function showRoutePreview() {
                    if (routeDescription.routeReady){
                        var routeWay=route.routeWay;
                        routePreviewMap.addOverlayObject(0,routeWay);
                        routePreviewMap.showLocation(routeWay.boundingBox);
                    }
                }

                Connections {
                    target: routeDescription
                    onRouteReadyChanged: {
                        routePreviewMap.showRoutePreview();
                    }
                }

                Component.onCompleted: {
                    showRoutePreview();
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingMedium
            }

            DetailItem {
                id: distanceItem
                label: qsTr("Distance")
                value: routeReady ? Utils.humanDistance(route.length) : "?"
            }
            DetailItem {
                id: durationItem
                label: qsTr("Duration")
                value: routeReady ? Utils.humanDuration(route.duration) : "?"
            }
            DetailItem {
                id: ascentItem
                label: qsTr("Ascent")
                visible: route.vehicle != "car" && elevationChart.pointCount >= 2
                value: Utils.humanDistance(elevationChart.ascent)
            }
            DetailItem {
                id: descentItem
                label: qsTr("Descent")
                visible: route.vehicle != "car" && elevationChart.pointCount >= 2
                value: Utils.humanDistance(elevationChart.descent)
            }
            SectionHeader{
                id: routeElevationProfileHeader
                visible: route.vehicle != "car"
                text: qsTr("Elevation profile")
            }
            RouteElevationChart {
                id: elevationChart
                width: parent.width
                height: route.vehicle == "car" ? 0 : Math.min((width / 1920) * 1080, 512)
                way: route.vehicle == "car" ? null : route.routeWay
            }
            SectionHeader{
                id: routeStepsHeader
                text: qsTr("Route steps")
            }
        }

        delegate: RoutingStep{}

        ProgressBar {
            id: progressBar
            width: parent.width
            maximumValue: 100
            value: 50
            indeterminate: true
            valueText: ""
            label: qsTr("Preparing")
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}

