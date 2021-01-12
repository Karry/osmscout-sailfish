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
            console.log("Navigation destination: \"" + Utils.locationStr(destination) + "\" by " + route.vehicle);
            Global.navigationModel.setup(route.vehicle, route.route, destination)
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
            SectionHeader{
                id: routeElevationProfileHeader
                visible: route.vehicle != "car"
                text: qsTr("Elevation profile")
            }
            ElevationChart {
                id: elevationChart
                width: parent.width
                height: route.vehicle == "car" ? 0 : Math.min((width / 1920) * 1080, 512)

                lineColor: Theme.highlightColor
                lineWidth: 5
                gradientTopColor: Theme.rgba(Theme.secondaryHighlightColor, 0.6)
                //gradientBottomColor: Theme.rgba(Theme.highlightColor, 0.6)
                textColor: Theme.secondaryHighlightColor
                textPixelSize: Theme.fontSizeTiny
                textPadding: Theme.paddingSmall

                way: route.vehicle == "car" ? null : route.routeWay

                BusyIndicator {
                    id: busyIndicator
                    running: elevationChart.loading
                    size: BusyIndicatorSize.Medium
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
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

