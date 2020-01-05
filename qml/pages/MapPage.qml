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
import Nemo.KeepAlive 1.2

import QtPositioning 5.2
import QtGraphicalEffects 1.0

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils
import ".." // Global singleton

Page {
    id: mapPage

    signal selectLocation(LocationEntry location)
    signal showWaypoint(double lat, double lon)
    signal showTrack(LocationEntry bbox, var trackId)
    signal showRoute(OverlayWay route, var routeId)

    // TODO: resume tracking when some track is still open

    onSelectLocation: {
        console.log("selectLocation: " + location);
        map.showLocation(location);
        drawer.open = false;
    }
    onShowWaypoint: {
        console.log("selectWaypoint: " + lat + ", " + lon);
        map.showCoordinates(lat, lon);
        drawer.open = false;
    }
    onShowTrack: {
        console.log("show Track "+ trackId);
        map.showLocation(bbox);
        drawer.open = false;
    }
    onShowRoute: {
        map.addOverlayObject(routeId, route);
        drawer.open = false;
    }

    RemorsePopup {
        id: remorse
        Component.onCompleted: {
            Global.remorse = remorse;
        }
    }
    KeepAlive {
        id: keepAlive
        enabled: Global.navigationModel.destinationSet &&
                 ((Global.navigationModel.vehicle === "car" && Global.navigationModel.nextRouteStep < 700) ||
                  (Global.navigationModel.vehicle === "bicycle" && Global.navigationModel.nextRouteStep < 200) ||
                  (Global.navigationModel.nextRouteStep < 30))

        onEnabledChanged: {
            console.log((enabled ? "Enabling" : "Disabling") +
                        " keepAlive, " +
                        (Global.navigationModel.destinationSet ? ("next navigation step in " + Utils.humanDistance(Global.navigationModel.nextRouteStep)) : "no navigation in progress"));
        }
    }

    MapDownloadsModel{
        id:mapDownloadsModel

        onMapDownloadFails: {
            remorse.execute(qsTranslate("message", message), function() { }, 10 * 1000);
        }
    }

    Component.onDestruction: {
        console.log("store map position");
        AppSettings.mapView = map.view;
    }
    Component.onCompleted: {
        console.log("restore map position");
        map.view = AppSettings.mapView;

        Global.navigationModel.onRouteChanged.connect(function(){
            var way = Global.navigationModel.routeWay;
            if (way==null){
                map.removeOverlayObject(0);
            }else{
                map.addOverlayObject(0,way);
            }
        });

        Global.mapPage = mapPage;
        Global.mainMap = map;
    }

    Settings {
        id: settings
    }

    Drawer {
        id: drawer
        anchors.fill: parent

        dock: mapPage.isPortrait ? Dock.Top : Dock.Left
        open: false

        background: SilicaListView {
            interactive: true
            anchors.fill: parent
            height: childrenRect.height
            id: menu
            model: ListModel {
                ListElement { itemtext: QT_TR_NOOP("Search");       itemicon: "image://theme/icon-m-search";         action: "search";   }
                ListElement { itemtext: QT_TR_NOOP("Where am I?");  itemicon: "image://theme/icon-m-whereami";       action: "whereami"; }
                ListElement { itemtext: QT_TR_NOOP("Navigation");   itemicon: "image://theme/icon-m-shortcut";       action: "routing";  }
                ListElement { itemtext: QT_TR_NOOP("Collections");  itemicon: "image://theme/icon-m-favorite";       action: "collections";}
                ListElement { itemtext: QT_TR_NOOP("Tracker");      itemicon: 'image://harbour-osmscout/pics/tracker.svg'; action: "tracker";}
                ListElement { itemtext: QT_TR_NOOP("Offline maps"); itemicon: "image://theme/icon-m-cloud-download"; action: "downloads";}
                ListElement { itemtext: QT_TR_NOOP("Map settings"); itemicon: "image://theme/icon-m-levels";         action: "layers";   }
                ListElement { itemtext: QT_TR_NOOP("Settings");     itemicon: "image://theme/icon-m-developer-mode"; action: "settings"; }
                ListElement { itemtext: QT_TR_NOOP("About");        itemicon: "image://theme/icon-m-about";          action: "about";    }
            }

            delegate: ListItem{
                id: searchRow

                function isEnabled(action){
                    return ((action == "whereami" && Global.positionSource.positionValid) ||
                            action == "about" || action == "layers" || action == "search" ||
                            action == "settings" || action == "downloads" || action == "routing" ||
                            action == "collections" || action == "tracker");
                }

                function onAction(action){
                    if (!isEnabled(action))
                        return;

                    // store view
                    AppSettings.mapView = map.view;

                    if (action == "whereami"){
                        if (Global.positionSource.positionValid){
                            pageStack.push(Qt.resolvedUrl("PlaceDetail.qml"),
                                           {
                                               placeLat: Global.positionSource.lat,
                                               placeLon: Global.positionSource.lon
                                           })
                        }else{
                            console.log("I can't say where you are. Position is not valid!")
                        }
                    }else if (action == "about"){
                        pageStack.push(Qt.resolvedUrl("About.qml"))
                    }else if (action == "search"){
                        var searchPage=pageStack.push(Qt.resolvedUrl("Search.qml"),
                                                      {
                                                          searchCenterLat: Global.positionSource.lat,
                                                          searchCenterLon: Global.positionSource.lon,
                                                          acceptDestination: mapPage
                                                      });
                        searchPage.selectLocation.connect(selectLocation);
                    }else if (action == "routing"){
                        pageStack.push(Qt.resolvedUrl("Routing.qml"))
                    }else if (action == "layers"){
                        pageStack.push(Qt.resolvedUrl("Layers.qml"))
                    }else if (action == "settings"){
                        pageStack.push(Qt.resolvedUrl("Settings.qml"))
                    }else if (action == "downloads"){
                        pageStack.push(Qt.resolvedUrl("Downloads.qml"))
                    }else if (action == "collections") {
                        var collectionsPage = pageStack.push(Qt.resolvedUrl("Collections.qml"),{
                                                                acceptDestination: mapPage
                                                            });
                        collectionsPage.selectWaypoint.connect(showWaypoint);
                        collectionsPage.selectTrack.connect(showTrack);
                    }else if (action == "tracker"){
                        pageStack.push(Qt.resolvedUrl("Tracker.qml"))
                    }else{
                        console.log("TODO: "+ action)
                    }
                }

                //spacing: Theme.paddingMedium
                anchors.right: parent.right
                anchors.left: parent.left
                IconButton {
                    id: menuIcon
                    icon.source: itemicon

                    icon.fillMode: Image.PreserveAspectFit
                    icon.sourceSize.width: Theme.iconSizeMedium
                    icon.sourceSize.height: Theme.iconSizeMedium

                    enabled: isEnabled(action)
                    onClicked: onAction(action)
                }

                Label {
                    id: menuLabel
                    anchors.left: menuIcon.right
                    text: qsTr(itemtext)
                    anchors.verticalCenter: parent.verticalCenter
                    color: isEnabled(action)? Theme.primaryColor: Theme.secondaryColor
                }

                onClicked: onAction(action)
            }
        }

        MapComponent {
            id: map

            focus: true
            anchors.fill: parent

            topMargin: nextStepBox.height + speedIndicator.height

            showCurrentPosition: true
            vehiclePosition: Global.navigationModel.vehiclePosition
            followVehicle: Global.navigationModel.destinationSet
            renderingType: Global.navigationModel.destinationSet ? "plane" : "tiled"

            Connections {
                target: Global.navigationModel
                onDestinationSetChanged: {
                    if (!Global.navigationModel.destinationSet){
                        console.log("Rotate back to 0");
                        map.rotateTo(0);
                    }
                }
            }

            CollectionMapBridge{
                map: map
            }

            onTap: {

                console.log("tap: " + screenX + "x" + screenY + " @ " + lat + " " + lon + " (map center "+ map.view.lat + " " + map.view.lon + ")");
                if (drawer.open){
                    drawer.open = false;
                }
            }
            onLongTap: {

                console.log("long tap: " + screenX + "x" + screenY + " @ " + lat + " " + lon);

                pageStack.push(Qt.resolvedUrl("PlaceDetail.qml"),
                               {
                                   placeLat: lat,
                                   placeLon: lon,
                                   mapPage: mapPage,
                                   mainMap: map
                               })
            }
            /*
            onViewChanged: {
                //console.log("map center "+ map.view.lat + " " + map.view.lon + "");
            }
            */

            /*
            void doubleTap(const QPoint p, const osmscout::GeoCoord c);
            void longTap(const QPoint p, const osmscout::GeoCoord c);
            void tapLongTap(const QPoint p, const osmscout::GeoCoord c);
            */

            Keys.onPressed: {
                if (event.key === Qt.Key_Plus) {
                    map.zoomIn(2.0)
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Minus) {
                    map.zoomOut(2.0)
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Up) {
                    map.up()
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Down) {
                    map.down()
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Left) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        map.rotateLeft();
                    }
                    else {
                        map.left();
                    }
                    event.accepted = true
                }
                else if (event.key === Qt.Key_Right) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        map.rotateRight();
                    }
                    else {
                        map.right();
                    }
                    event.accepted = true
                }
                else if (event.modifiers===Qt.ControlModifier &&
                         event.key === Qt.Key_F) {
                    searchDialog.focus = true
                    event.accepted = true
                }
                else if (event.modifiers===Qt.ControlModifier &&
                         event.key === Qt.Key_D) {
                    map.toggleDaylight();
                }
                else if (event.modifiers===Qt.ControlModifier &&
                         event.key === Qt.Key_R) {
                    map.reloadStyle();
                }
            }

            Rectangle {
                id : currentPositionBtn

                anchors{
                    right: parent.right
                    bottom: parent.bottom
                    rightMargin: Theme.paddingMedium
                    bottomMargin: Theme.paddingMedium
                }
                width: Theme.iconSizeLarge
                height: width

                color: Theme.rgba(Theme.highlightDimmerColor, 0.2)

                radius: width*0.5

                Rectangle {
                    id: positionIndicator
                    anchors{
                        horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    width: 20
                    height: 20

                    color: Global.positionSource.positionValid &&
                           ((new Date()).getTime() - Global.positionSource.lastUpdate.getTime() < 60000) ?
                               "#6000FF00" : "#60738d73"

                    border.color: Theme.rgba(Theme.primaryColor, 0.8)
                    border.width: 1
                    radius: width*0.5
                }

                IconButton{
                    id: possitionLockIcon
                    icon.source: "image://theme/icon-s-secure"
                    x: parent.width - (width * 0.75)
                    y: parent.height - (height * 0.75)

                    opacity: map.lockToPosition ? 1 : 0
                    Behavior on opacity { PropertyAnimation {} }
                }

                MouseArea {
                  id: currentPositionBtnMouseArea
                  anchors.fill: parent

                  hoverEnabled: true

                  Timer {
                      id: showCurrentPositionTimer
                      interval: 120 // double click tolerance
                      running: false
                      repeat: false
                      onTriggered: {
                          if (Global.positionSource.positionValid){
                              map.showCoordinates(Global.positionSource.lat, Global.positionSource.lon);
                          }
                      }
                  }
                  onClicked: {
                      // postpone jump a little bit, double click can apears...
                      showCurrentPositionTimer.running = true;
                  }
                  onDoubleClicked: {
                      showCurrentPositionTimer.running = false;
                      if (Global.positionSource.positionValid){
                          map.lockToPosition = !map.lockToPosition;
                          console.log(map.lockToPosition ? "bound map with current possition" : "unbound map from current possition");
                      }
                  }
                }

            }

            Rectangle {
                id : compassBtn

                anchors{
                    right: parent.right
                    bottom: currentPositionBtn.top
                    rightMargin: Theme.paddingMedium
                    bottomMargin: Theme.paddingMedium
                }
                width: Theme.iconSizeLarge
                height: width

                color: Theme.rgba(Theme.highlightDimmerColor, 0.2)

                radius: width*0.5
                rotation: Utils.rad2Deg(map.view.angle)
                opacity: map.view.angle === 0 ? (Global.navigationModel.destinationSet ? 0.4 : 0.0) : 1.0
                Behavior on opacity { PropertyAnimation {} }

                Image {
                    id: compassIcon
                    anchors.fill: parent
                    source: "image://harbour-osmscout/pics/compass.svg?" + Theme.primaryColor
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    sourceSize.width: width
                    sourceSize.height: height
                }

                MouseArea {
                  id: compasBtnMouseArea
                  anchors.fill: parent

                  hoverEnabled: true

                  onClicked: {
                      console.log("Rotate back to 0");
                      map.rotateTo(0);
                  }
                }
            }
        }

        ShaderEffectSource {
            id: blurSource

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: nextStepBox.height
            visible: nextStepBox.visible

            //color: "transparent"

            sourceItem: map
            recursive: true
            live: true
            sourceRect: Qt.rect(nextStepBox.x, nextStepBox.y, nextStepBox.width, nextStepBox.height)
        }

        Rectangle {
            id: nextStepBox

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: Global.navigationModel.destinationSet ? (Theme.iconSizeLarge + 3*Theme.paddingMedium) + navigationContextMenu.height : 0
            visible: Global.navigationModel.destinationSet
            color: "transparent"

            FastBlur {
                id: blur
                anchors.fill: parent
                source: blurSource
                radius: 32
            }

            Rectangle {
                id: nextStepBackground
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: Global.navigationModel.destinationSet ? (Theme.iconSizeLarge + 3*Theme.paddingMedium) : 0

                //color: "transparent"
                color: nextStepMouseArea.pressed ? Theme.rgba(Theme.highlightDimmerColor, 0.5) : Theme.rgba(Theme.highlightDimmerColor, 0.7)

                RouteStepIcon{
                    id: nextStepIcon
                    stepType: Global.navigationModel.nextRouteStep.type
                    roundaboutExit: Global.navigationModel.nextRouteStep.roundaboutExit
                    roundaboutClockwise: Global.navigationModel.nextRouteStep.roundaboutClockwise
                    height: Theme.iconSizeLarge
                    width: height
                    anchors{
                        left: parent.left
                        margins: Theme.paddingMedium
                        verticalCenter: parent.verticalCenter
                    }

                    BusyIndicator{
                        id: reroutingIndicator
                        running: Global.routingModel.rerouteRequested
                        size: BusyIndicatorSize.Medium
                        anchors.centerIn: parent
                    }
                    Text{
                        id: roundaboutExit
                        text: Global.navigationModel.nextRouteStep.type == "leave-roundabout" ? Global.navigationModel.nextRouteStep.roundaboutExit : ""
                        anchors.centerIn: parent
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.primaryColor
                    }
                }
                Text{
                    id: distanceToNextStep
                    text: Utils.humanDistanceVerbose(Global.navigationModel.nextRouteStep.distanceTo)
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeLarge
                    anchors{
                        top: parent.top
                        left: nextStepIcon.right
                        topMargin: Theme.paddingMedium
                        leftMargin: Theme.paddingMedium
                    }
                }
                Text{
                    id: arrivalTime
                    text: qsTr("ETA %1").arg(Qt.formatTime(Global.navigationModel.arrivalEstimate))
                    visible: !isNaN(Global.navigationModel.arrivalEstimate.getTime())
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeLarge
                    anchors{
                        top: parent.top
                        right: parent.right
                        topMargin: Theme.paddingMedium
                        rightMargin: Theme.paddingMedium
                    }
                }
                Text{
                    id: nextStepDescription
                    text: Global.navigationModel.nextRouteStep.shortDescription
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.secondaryColor
                    wrapMode: Text.Wrap
                    anchors{
                        top: distanceToNextStep.bottom
                        left: distanceToNextStep.left
                        right: parent.right
                        margins: Theme.paddingSmall
                    }
                }

                MouseArea{
                    id: nextStepMouseArea
                    anchors.fill: parent
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("NavigationInstructions.qml"),{})
                    }
                    onPressAndHold: {
                        navigationContextMenu.open(nextStepBox);
                    }
                }
            }
            ContextMenu {
                id: navigationContextMenu
                MenuItem {
                    text: qsTr("Stop navigation")
                    onClicked: Global.navigationModel.stop();
                }
                MenuItem {
                    //: menu item: open routing page with current navigation destination
                    text: qsTr("Change vehicle");
                    enabled: Global.navigationModel.destinationSet
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("Routing.qml"),
                                       {
                                           toLat: Global.navigationModel.destination.lat,
                                           toLon: Global.navigationModel.destination.lon,
                                           toName: Global.navigationModel.destination.label
                                       });
                    }
                }
            }
        }

        SpeedIndicator {
            id: speedIndicator
            currentSpeed: Global.navigationModel.currentSpeed
            maximumSpeed: Global.navigationModel.maxAllowedSpeed
            visible: Global.navigationModel.destinationSet

            anchors.left: parent.left
            anchors.top: nextStepBox.bottom
            anchors.margins: Theme.paddingMedium
            height: visible ? Theme.iconSizeLarge : 0
            width: height
        }

        Rectangle {
            id : menuBtn
            anchors{
                right: parent.right
                top: nextStepBox.bottom

                topMargin: Theme.paddingMedium
                rightMargin: Theme.paddingMedium
                bottomMargin: Theme.paddingMedium
                leftMargin: Theme.paddingMedium
            }
            width: Theme.iconSizeLarge
            height: width
            radius: Theme.paddingMedium

            rotation: drawer.open ? 180 : 0
            Behavior on rotation { PropertyAnimation {} }

            color: Theme.rgba(Theme.highlightDimmerColor, 0.2)

            IconButton{
                icon.source: "image://theme/icon-m-menu"
                anchors{
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
            }
            MouseArea {
                id: menuBtnMouseArea
                anchors.fill: parent
                onClicked: {
                    drawer.open = !drawer.open
                }
            }
        }

    }
}
