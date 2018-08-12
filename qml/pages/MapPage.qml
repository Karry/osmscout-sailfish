/*
 OSM Scout - a Qt backend for libosmscout and libosmscout-map
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

Page {
    id: mapPage

    property NavigationModel navigationModel
    property RoutingListModel routingModel

    signal selectLocation(LocationEntry location)

    onSelectLocation: {
        console.log("selectLocation: " + location);
        map.showLocation(location);
        drawer.open = false;
    }

    RemorsePopup { id: remorse }

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

        routingModel.computingChanged.connect(function(){
            if (routingModel.ready){
                map.addOverlayObject(0,routingModel.routeWay);
            }
        });
    }

    Settings {
        id: settings
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
            /*
            console.log("Position changed:")

            if (position.latitudeValid) {
                console.log("  latitude: " + position.coordinate.latitude)
            }

            if (position.longitudeValid) {
                console.log("  longitude: " + position.coordinate.longitude)
            }

            if (position.altitudeValid) {
                console.log("  altitude: " + position.coordinate.altitude)
            }

            if (position.speedValid) {
                console.log("  speed: " + position.speed)
            }

            if (position.horizontalAccuracyValid) {
                console.log("  horizontal accuracy: " + position.horizontalAccuracy)
            }

            if (position.verticalAccuracyValid) {
                console.log("  vertical accuracy: " + position.verticalAccuracy)
            }
            */

            positionSource.valid = position.latitudeValid && position.longitudeValid;
            positionSource.lat = position.coordinate.latitude;
            positionSource.lon = position.coordinate.longitude;
        }
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
                ListElement { itemtext: QT_TR_NOOP("Routing");      itemicon: "image://theme/icon-m-shortcut";       action: "routing";  }
                ListElement { itemtext: QT_TR_NOOP("Map downloads");itemicon: "image://theme/icon-m-cloud-download"; action: "downloads";}
                ListElement { itemtext: QT_TR_NOOP("Map settings"); itemicon: "image://theme/icon-m-levels";         action: "layers";   }
                ListElement { itemtext: QT_TR_NOOP("Collections");  itemicon: "image://theme/icon-m-favorite";       action: "collections";}
                ListElement { itemtext: QT_TR_NOOP("About");        itemicon: "image://theme/icon-m-about";          action: "about";    }
            }

            delegate: ListItem{
                id: searchRow

                function isEnabled(action){
                    return ((action == "whereami" && positionSource.valid) ||
                            action == "about" || action == "layers" || action == "search" || 
                            action == "downloads" || action == "routing" || action == "collections");
                }

                function onAction(action){
                    if (!isEnabled(action))
                        return;

                    // store view
                    AppSettings.mapView = map.view;

                    if (action == "whereami"){
                        if (positionSource.valid){
                            pageStack.push(Qt.resolvedUrl("PlaceDetail.qml"),
                                           {
                                               placeLat: positionSource.lat,
                                               placeLon: positionSource.lon,
                                               mapPage: mapPage,
                                               mainMap: map
                                           })
                        }else{
                            console.log("I can't say where you are. Position is not valid!")
                        }
                    }else if (action == "about"){
                        pageStack.push(Qt.resolvedUrl("About.qml"))
                    }else if (action == "search"){
                        var searchPage=pageStack.push(Qt.resolvedUrl("Search.qml"),
                                                      {
                                                          searchCenterLat: positionSource.lat,
                                                          searchCenterLon: positionSource.lon,
                                                          acceptDestination: mapPage
                                                      });
                        searchPage.selectLocation.connect(selectLocation);
                    }else if (action == "routing"){
                        pageStack.push(Qt.resolvedUrl("Routing.qml"),
                                       {
                                           mapPage: mapPage,
                                           mainMap: map
                                       })
                    }else if (action == "layers"){
                        pageStack.push(Qt.resolvedUrl("Layers.qml"))
                    }else if (action == "downloads"){
                        pageStack.push(Qt.resolvedUrl("Downloads.qml"))
                    }else if (action == "collections") {
                        pageStack.push(Qt.resolvedUrl("Collections.qml"))
                    }else{
                        console.log("TODO: "+ action)
                    }
                }

                //spacing: Theme.paddingMedium
                anchors.right: parent.right
                anchors.left: parent.left
                IconButton{
                    id: searchIcon
                    icon.source: itemicon
                    enabled: isEnabled(action)
                    onClicked: onAction(action)
                }

                Label {
                    id: searchLabel
                    anchors.left: searchIcon.right
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

            topMargin: nextStepBox.height

            showCurrentPosition: true

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
                id: nextStepBox

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: navigationModel.destinationSet ? (Theme.iconSizeLarge + 3*Theme.paddingMedium) : 0
                visible: navigationModel.destinationSet

                color: Theme.rgba(Theme.highlightDimmerColor, 0.8)

                RouteStepIcon{
                    id: nextStepIcon
                    stepType: navigationModel.nextRouteStep.type
                    height: Theme.iconSizeLarge
                    width: height
                    anchors{
                        left: parent.left
                        margins: Theme.paddingMedium
                        verticalCenter: parent.verticalCenter
                    }
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
                    text: humanDistance(navigationModel.nextRouteStep.distanceTo)
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
                    id: nextStepDescription
                    text: navigationModel.nextRouteStep.shortDescription
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

                    color: positionSource.valid ? "#6000FF00" : "#60FF0000"
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
                          if (positionSource.valid){
                              map.showCoordinates(positionSource.lat, positionSource.lon);
                          }
                      }
                  }
                  onClicked: {
                      // postpone jump a little bit, double click can apears...
                      showCurrentPositionTimer.running = true;
                  }
                  onDoubleClicked: {
                      showCurrentPositionTimer.running = false;
                      if (positionSource.valid){
                          map.lockToPosition = !map.lockToPosition;
                          console.log(map.lockToPosition ? "bound map with current possition" : "unbound map from current possition");
                      }
                  }
                }

            }
        }
    }
}
