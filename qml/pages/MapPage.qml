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
import Nemo.Notifications 1.0
import Nemo.DBus 2.0

import QtPositioning 5.2
import QtGraphicalEffects 1.0

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils
import ".." // Global singleton

Page {
    id: mapPage

    signal selectLocation(LocationEntry location)
    signal showWaypoint(double lat, double lon, var waypointId)
    signal showTrack(LocationEntry bbox, var trackId)
    signal showRoute(OverlayWay route, var routeId)


    onSelectLocation: {
        console.log("selectLocation: " + location);
        map.showLocation(location);
        drawer.open = false;
    }
    onShowWaypoint: {
        console.log("selectWaypoint: " + lat + ", " + lon + " (" + waypointId + ")");
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
    DisplayBlanking {
        id: keepAlive
        property bool active: Global.navigationModel.destinationSet && AppSettings.navigationKeepAlive
        // seems that preventBlanking is write only, so we are using custom "active" property
        preventBlanking: active

        function printStatus(){
            var statusStr = "Unknown";
            if (status === DisplayBlanking.On){
                statusStr = "On"
            } else if (status === DisplayBlanking.Off){
                statusStr = "Off"
            } else if (status === DisplayBlanking.Dimmed){
                statusStr = "Dimmed"
            }

            console.log("Display status: " + statusStr + ", prevent blanking: " + active + ", " +
                        (Global.navigationModel.destinationSet ? ("next navigation step in " + Utils.humanDistance(Global.navigationModel.nextRouteStep)) : "no navigation in progress"));
        }

        onActiveChanged: {
            printStatus();
        }

        onStatusChanged: {
            printStatus();
        }
    }

    Notification {
        id: downloaderErrorNotification

        category: "x-osmscout.error"
        //: notification summary
        previewSummary: qsTr("Map download error")
        urgency: Notification.Critical
        isTransient: false
        expireTimeout: 0 // do not expire

        property var aliveIds: []

        appIcon: "image://theme/icon-lock-warning"

        remoteActions: [ {
                        name: "default",
                        service: "cz.karry.osmscout.OSMScout",
                        path: "/cz/karry/osmscout/OSMScout",
                        iface: "cz.karry.osmscout.OSMScout",
                        method: "openPage",
                        arguments: [ "Downloads", {} ]
                    } ]

        Component.onDestruction: {
            while (downloaderErrorNotification.replacesId != 0){
                console.log("Closing notification " + downloaderErrorNotification.replacesId);
                downloaderErrorNotification.close();
                if (downloaderErrorNotification.aliveIds.length > 0){
                    downloaderErrorNotification.replacesId = downloaderErrorNotification.aliveIds.pop();
                }
            }
        }
    }

    MapDownloadsModel{
        id: mapDownloadsModel

        onMapDownloadFails: {
            //remorse.execute(qsTranslate("message", message), function() { }, 10 * 1000);
            console.log("Map downloader error: " + message);
            var trMsg = qsTranslate("message", message);
            if (downloaderErrorNotification.aliveIds.length < 100 && downloaderErrorNotification.replacesId != 0){
                downloaderErrorNotification.aliveIds.push(downloaderErrorNotification.replacesId);
            }

            downloaderErrorNotification.body = trMsg; // have to be there for displaying in notification area
            downloaderErrorNotification.previewBody = trMsg;
            downloaderErrorNotification.replacesId = 0; // when zero, it creates new notification for every instance
            downloaderErrorNotification.publish();
            deviceErrorNotification.publish();
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

    Notification {
        id: networkErrorNotification
        category: "network.error"
    }

    DBusAdaptor {
        // test:
        // dbus-send --session --type=method_call --print-reply --dest=cz.karry.osmscout.OSMScout /cz/karry/osmscout/OSMScout cz.karry.osmscout.OSMScout.openUrl string:geo:37.786,-122.399
        // dbus-send --session --type=method_call --print-reply --dest=cz.karry.osmscout.OSMScout /cz/karry/osmscout/OSMScout cz.karry.osmscout.OSMScout.openPage string:Downloads string:nothing

        service: "cz.karry.osmscout.OSMScout"
        iface: "cz.karry.osmscout.OSMScout"
        path: "/cz/karry/osmscout/OSMScout"
        xml: '\
     <interface name="cz.karry.osmscout.OSMScout">
       <method name="openPage">
         <arg name="page" type="s" direction="in">
           <doc:doc>
             <doc:summary>
               Name of the page to open
               (https://github.com/mentaljam/harbour-osmscout/tree/master/qml/pages)
             </doc:summary>
           </doc:doc>
         </arg>
         <arg name="arguments" type="a{sv}" direction="in">
           <doc:doc>
             <doc:summary>
               Arguments to pass to the page
             </doc:summary>
           </doc:doc>
         </arg>
       </method>
       <method name="openUrl">
         <arg name="url" type="s" direction="in">
           <doc:doc>
             <doc:summary>
               url of map service
             </doc:summary>
           </doc:doc>
         </arg>
       </method>
     </interface>'

       function openUrl(url) {
           __silica_applicationwindow_instance.activate()

           var urlStr = url + "";
           console.log("open url: " + url);
           if (!Utils.startsWith(urlStr, "geo:")) {
               console.log("unsupported url: " + url);
               networkErrorNotification.previewBody = qsTr("Unsupported url %1").arg(url);
               networkErrorNotification.publish();
               return;
           }
           var arr = urlStr.substring(4).split('?', 2);
           var search = "";
           if (arr.length >= 2){
               var params = arr[1].split('&');
               for (var i=0; i < params.length; i++){
                   if (Utils.startsWith(params[i], "q=")) {
                       search=decodeURIComponent(params[i].substring(2));
                   }
               }
           }

           var coords = arr[0].split(';')[0].split(',');
           if (coords.length < 2) {
               console.log("cannot parse url: " + url);
               networkErrorNotification.previewBody = qsTr("Cannot parse url %1").arg(url);
               networkErrorNotification.publish();
               return;
           }
           var lat = Number(coords[0]);
           var lon = Number(coords[1]);
           if (isNaN(lat) || isNaN(lon) || lat < -90 || lat > 90 || lon < -180 || lon > 180){
               console.log("cannot parse url: " + url);
               networkErrorNotification.previewBody = qsTr("Cannot parse url %1").arg(url);
               networkErrorNotification.publish();
               return;
           }

           if (search !== "") {
               if (lat == 0 && lon == 0) {
                   lat = Global.positionSource.lat;
                   lon = Global.positionSource.lon;
               } else {
                   map.showCoordinates(lat, lon);
               }

               var searchPage=pageStack.push(Qt.resolvedUrl("Search.qml"),
                                             {
                                                 searchCenterLat: lat,
                                                 searchCenterLon: lon,
                                                 searchFieldText: search,
                                                 acceptDestination: mapPage
                                             });
               searchPage.selectLocation.connect(selectLocation);
           } else {
               // go to location and even open its details...
               map.showCoordinates(lat, lon);
               pageStack.push(Qt.resolvedUrl("PlaceDetail.qml"),
                              {
                                  placeLat: lat,
                                  placeLon: lon
                              })
           }
       }

       function openPage(page, arguments) {
           __silica_applicationwindow_instance.activate()
           console.log("D-Bus: activate page " + page + " (current: " + pageStack.currentPage.objectName + ")");
           if ((page === "Tracker" || page === "Downloads") && page !== pageStack.currentPage.objectName) {
               pageStack.push(Qt.resolvedUrl("%1.qml".arg(page)), arguments)
           }
       }
    }

    // resume tracking when some track is still open
    Dialog {
        id: trackerResumeDialog

        Component.onCompleted: {
            if (Global.tracker.canBeResumed && !Global.tracker.tracking){
                trackerResumeDialog.open();
            }
        }

        Notification {
            // device.error generates sound notification, but overrides expiration timeout
            // for that reason this instance is used just for sound
            id: deviceErrorNotification
            category: "device.error"
        }

        Notification {
            id: trackerErrorNotification

            category: "x-osmscout.error"
            //: notification summary
            previewSummary: qsTr("Tracker error")
            urgency: Notification.Critical
            isTransient: false
            expireTimeout: 0 // do not expire

            appIcon: "image://theme/icon-lock-warning"

            remoteActions: [ {
                            name: "default",
                            service: "cz.karry.osmscout.OSMScout",
                            path: "/cz/karry/osmscout/OSMScout",
                            iface: "cz.karry.osmscout.OSMScout",
                            method: "openPage",
                            arguments: [ "Tracker", {} ]
                        } ]

            onClicked: {
                console.log("clicked: trackerErrorNotification");
                pageStack.push(Qt.resolvedUrl("Tracker.qml"));
            }
            // Component.onCompleted: {
            //     trackerErrorNotification.body = "Startup test body";
            //     trackerErrorNotification.previewBody = "Startup test";
            //     trackerErrorNotification.publish();
            //     deviceErrorNotification.publish();
            // }
            Component.onDestruction: {
                trackerErrorNotification.close();
            }
        }

        Connections {
            target: Global.tracker
            onOpenTrackLoaded: {
                if (Global.tracker.canBeResumed && !Global.tracker.tracking){
                    trackerResumeDialog.open();
                }
            }
            onError: {
                console.log("Tracker error: " + message);
                trackerErrorNotification.body = message; // have to be there for displaying in notification area
                trackerErrorNotification.previewBody = message;
                trackerErrorNotification.itemCount = Global.tracker.errors; // how many errors so far
                //trackerErrorNotification.replacesId = 0; // when zero, it creates new notification for every instance
                trackerErrorNotification.publish();
                deviceErrorNotification.publish();
            }
            onTrackingChanged: {
                if (!Global.tracker.tracking){
                    trackerErrorNotification.close();
                }
            }
        }

        onAccepted: {
            Global.tracker.resumeTrack(Global.tracker.openTrackId);
        }
        onRejected: {
            Global.tracker.closeOpen(Global.tracker.openTrackId);
        }

        DialogHeader {
            id: resumeDialogheader
            title: qsTr("Resume tracking?")
            //acceptText : qsTr("Show")
            //cancelText : qsTr("Cancel")
        }

        Label {
            anchors{
                top: resumeDialogheader.bottom
                bottom: parent.bottom
            }
            x: Theme.paddingMedium
            width: parent.width - 2*Theme.paddingMedium

            text: Global.tracker.openTrackName
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

            VerticalScrollDecorator {}

            TouchInteractionHint {
                id: hint
                loops: 5

                interactionMode: TouchInteraction.Swipe
                direction: TouchInteraction.Up

                Connections {
                    target: drawer
                    onOpenChanged: {
                        if (counter.active && !menu.atYEnd) {
                            hint.start()
                            counter.increase();
                        }
                    }
                }
                Connections {
                    target: menu
                    onAtYEndChanged: {
                        if (menu.atYEnd) {
                            hint.stop();
                        }
                    }
                }
            }
            InteractionHintLabel {
                anchors.bottom: parent.bottom
                text: qsTr("Scroll down for more entries")
                opacity: hint.running ? 1.0 : 0.0
                Behavior on opacity { FadeAnimation { duration: 1000 } }
            }
            FirstTimeUseCounter {
                id: counter
                limit: 2
                key: "/apps/harbour-osmscout/menu_hint_count"
            }


            model: ListModel {
                //: menu item for Search on map
                ListElement { itemtext: QT_TR_NOOP("Search");       itemicon: "image://theme/icon-m-search";         action: "search";   }

                //: menu item for path with place details
                ListElement { itemtext: QT_TR_NOOP("Where am I?");  itemicon: "image://theme/icon-m-whereami";       action: "whereami"; }

                //: menu item for navigation and routing
                ListElement { itemtext: QT_TR_NOOP("Navigation");   itemicon: "image://theme/icon-m-shortcut";       action: "routing";  }

                //: menu item for collections of tracks and waypoints
                ListElement { itemtext: QT_TR_NOOP("Collections");  itemicon: "image://theme/icon-m-favorite";       action: "collections";}

                //: menu item for GPS tracker
                ListElement { itemtext: QT_TR_NOOP("Tracker");      itemicon: 'image://harbour-osmscout/pics/tracker.svg'; action: "tracker";}

                //: menu item for offline map downloader
                ListElement { itemtext: QT_TR_NOOP("Offline maps"); itemicon: "image://theme/icon-m-cloud-download"; action: "downloads";}

                //: menu item for map settings
                ListElement { itemtext: QT_TR_NOOP("Map");          itemicon: "image://theme/icon-m-levels";         action: "layers";   }

                //: menu item for application settings
                ListElement { itemtext: QT_TR_NOOP("Settings");     itemicon: "image://theme/icon-m-developer-mode"; action: "settings"; }

                //: menu item for about page
                ListElement { itemtext: QT_TR_NOOP("About");        itemicon: "image://theme/icon-m-about";          action: "about";    }
            }

            InstalledMapsModel{
                id: installedMapsModel
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
                        console.log("installedMapsModel.rowCount(): " + installedMapsModel.rowCount());
                        if (installedMapsModel.rowCount() <= 0) {
                            pageStack.push(Qt.resolvedUrl("DatabaseWarning.qml"),
                                           {
                                              description: qsTr("Offline map database is required for search functionality.")
                                           });
                            return;
                        }

                        var searchPage=pageStack.push(Qt.resolvedUrl("Search.qml"),
                                                      {
                                                          searchCenterLat: Global.positionSource.lat,
                                                          searchCenterLon: Global.positionSource.lon,
                                                          acceptDestination: mapPage
                                                      });
                        searchPage.selectLocation.connect(selectLocation);
                    }else if (action == "routing"){
                        if (installedMapsModel.rowCount() <= 0) {
                            pageStack.push(Qt.resolvedUrl("DatabaseWarning.qml"),
                                           {
                                              description: qsTr("Offline map database is required for navigation functionality.")
                                           });
                            return;
                        }
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

            topMargin: nextStepBox.height +
                       speedIndicator.height + (speedIndicator.height > 0 ? Theme.paddingMedium: 0) +
                       infoPanel.height + (infoPanel.height > 0 ? Theme.paddingMedium: 0);
            bottomMargin: poiBox.height

            showCurrentPosition: true
            interactiveIcons: true
            vehiclePosition: Global.navigationModel.vehiclePosition
            followVehicle: Global.navigationModel.destinationSet
            renderingType: Global.navigationModel.destinationSet ? "plane" : "tiled"
            vehicleAutoRotateMap: AppSettings.vehicleAutoRotateMap

            // automatic day/night switch
            property bool automaticNightModeEnabled: AppSettings.automaticNightMode && Global.navigationModel.destinationSet && Global.sunriseSunset.ready
            property bool daylight: Global.sunriseSunset.day && !Global.navigationModel.positionEstimateInTunnel
            property string recentStylesheet: ""

            StyleFlagsModel {
                id: styleFlagsModel
            }

            function dayNightSwitch() {
                if (automaticNightModeEnabled) {
                    styleFlagsModel.setFlag("daylight", daylight);
                }
            }

            Component.onCompleted: dayNightSwitch()
            onStylesheetFilenameChanged: {
                if (recentStylesheet !== map.stylesheetFilename) {
                    recentStylesheet = map.stylesheetFilename;
                    dayNightSwitch();
                }
            }
            onAutomaticNightModeEnabledChanged: dayNightSwitch()
            onDaylightChanged: dayNightSwitch();

            // automatic map rotation
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
                id: collectionMapBridge
                map: map
                enabled: AppSettings.showCollections
            }

            onIconTapped: {
                //(QPoint screenCoord, double lat, double lon, QString databasePath,
                //            QString objectType, quint64 objectId, int poiId, QString type,
                //            QString name, QString altName, QString ref, QString operatorName, QString phone, QString website);
                poiBox.objectType = objectType;
                poiBox.objectId = objectId;
                poiBox.poiId = poiId;
                poiBox.type = type;
                poiBox.name = name;
                poiBox.altName = altName;
                poiBox.ref = ref;
                poiBox.operatorName = operatorName;
                poiBox.phone = phone;
                poiBox.website = website;
                poiBox.lat = lat;
                poiBox.lon = lon;
                poiBox.show();
            }
            onTap: {
                console.log("tap: " + screenX + "x" + screenY + " @ " + lat + " " + lon + " (map center "+ map.view.lat + " " + map.view.lon + ")");
                if (drawer.open){
                    drawer.open = false;
                }
                poiBox.hide();
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

                property bool active: AppSettings.showCurrentPosition
                visible: active

                anchors{
                    right: parent.right
                    bottom: parent.bottom
                    rightMargin: Theme.paddingMedium
                    bottomMargin: (active ? Theme.paddingMedium : 0) + poiBox.height
                }
                width: Theme.iconSizeLarge
                height: active ? width : 0

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
                id : newPlaceBtn

                anchors{
                    right: parent.right
                    bottom: currentPositionBtn.top
                    rightMargin: Theme.paddingMedium
                    bottomMargin: active ? Theme.paddingMedium : 0
                }
                width: Theme.iconSizeLarge

                property bool active: AppSettings.showNewPlace && Global.positionSource.positionValid
                visible: active
                height: active ? width : 0

                color: Theme.rgba(Theme.highlightDimmerColor, 0.2)

                radius: width*0.5

                Image {
                    id: newPlaceIcon
                    anchors.fill: parent
                    source: "image://harbour-osmscout/pics/new-place.svg?" + Theme.primaryColor
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    sourceSize.width: width
                    sourceSize.height: height
                }

                MouseArea {
                  id: newPlaceBtnMouseArea
                  anchors.fill: parent

                  hoverEnabled: true

                  onClicked: {
                      var waypointDescription = "";
                      if (Global.positionSource.horizontalAccuracyValid){
                          waypointDescription = "±" + Utils.humanDistanceCompact(Global.positionSource.horizontalAccuracy/2);
                      }

                      pageStack.push(Qt.resolvedUrl("NewWaypoint.qml"),
                                    {
                                      latitude: Global.positionSource.lat,
                                      longitude: Global.positionSource.lon,
                                      acceptDestination: Global.mapPage,
                                      description: waypointDescription,
                                      name: ""
                                    });
                  }
                }
            }

            Rectangle {
                id : compassBtn

                anchors{
                    right: parent.right
                    bottom: newPlaceBtn.top
                    rightMargin: Theme.paddingMedium
                    bottomMargin: active ? Theme.paddingMedium : 0
                }
                width: Theme.iconSizeLarge
                property bool active: Global.navigationModel.destinationSet && AppSettings.showMapOrientation
                visible: active
                height: active ? width : 0

                color: Theme.rgba(Theme.highlightDimmerColor, 0.2)

                radius: width*0.5
                rotation: Utils.rad2Deg(map.angle)
                opacity: map.angle === 0 ? (Global.navigationModel.destinationSet ? 0.4 : 0.0) : 1.0
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

                IconButton{
                    id: compassLockIcon
                    icon.source: "image://theme/icon-s-secure"
                    x: parent.width - (width * 0.75)
                    y: parent.height - (height * 0.75)

                    opacity: map.vehicleAutoRotateMap ? 0 : 1
                    Behavior on opacity { PropertyAnimation {} }
                }

                MouseArea {
                  id: compasBtnMouseArea
                  anchors.fill: parent

                  hoverEnabled: true

                  onClicked: {
                      console.log("Rotate back to 0");
                      map.rotateTo(0);
                  }
                  onDoubleClicked: {
                      AppSettings.vehicleAutoRotateMap = !AppSettings.vehicleAutoRotateMap;
                  }
                }
            }
        }

        ShaderEffectSource {
            id: poiBlurSource

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: poiBox.height
            visible: poiBox.visible

            //color: "transparent"

            sourceItem: map
            recursive: true
            live: true
            sourceRect: Qt.rect(poiBlurSource.x, poiBlurSource.y, poiBlurSource.width, poiBlurSource.height)
        }

        Rectangle {
            id: poiBox

            property string objectType: ""
            property int objectId: 0
            property int poiId: 0
            property double lat
            property double lon
            property string type: ""
            property bool isWaypoint: Utils.startsWith(poiBox.type, "_waypoint")
            property string name: ""
            property string altName: ""
            property string ref: ""
            property string operatorName: ""
            property string phone: ""
            property string website: ""

            function hide() {
                state = "HIDDEN";
            }
            function show() {
                state = "MINIMAL";
            }

            state: "HIDDEN"

            states: [
                State {
                    name: "HIDDEN"
                    PropertyChanges { target: poiBox; height: 0; }
                },
                State {
                    name: "MINIMAL"
                    PropertyChanges { target: poiBox; height: poiDetails.height; }
                },
                State {
                    name: "INFORMATIVE"
                    PropertyChanges { target: poiBox; height: poiDetails.height; }
                }
            ]

            onObjectTypeChanged: {
                if (objectType=="") {
                    state = "HIDDEN";
                } else {
                    state = "MINIMAL";
                }
            }

            Behavior on height {
              NumberAnimation {
                duration: 200
              }
            }

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 0
            visible: height > 0
            color: "transparent"

            FastBlur {
                id: poiBlur
                anchors.fill: parent
                source: poiBlurSource
                radius: 32
            }

            Rectangle {
                id: poiBackground
                anchors.fill: parent
                color: nextStepMouseArea.pressed ? Theme.rgba(Theme.highlightDimmerColor, 0.5) : Theme.rgba(Theme.highlightDimmerColor, 0.7)

                IconButton{
                    id: poiMoreIcon
                    icon.source: "image://theme/icon-m-change-type"
                    anchors{
                        right: parent.right
                        top: parent.top
                    }
                    rotation: poiBox.state == "INFORMATIVE" ? 180 : 0
                    Behavior on rotation {
                        PropertyAnimation {
                            duration: 200
                        }
                    }
                }
                MouseArea {
                    anchors{
                        top: parent.top
                        right: parent.right
                        left: parent.left
                    }
                    height: Theme.iconSizeMedium

                    onClicked: {
                        if (poiBox.state == "INFORMATIVE") {
                            poiBox.state = "MINIMAL";
                        } else {
                            poiBox.state = "INFORMATIVE";
                        }
                    }
                }

                Row {
                    id: poiDetails
                    POIIcon{
                        id: poiIcon
                        poiType: poiBox.type
                        width: Theme.iconSizeMedium
                        height: Theme.iconSizeMedium
                    }
                    Column{
                        Label {
                            width: poiBox.width - poiIcon.width - (2*Theme.paddingSmall)
                            text: qsTranslate("objectType", poiBox.isWaypoint ? "waypoint" : poiBox.type)
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                        }
                        Label {
                            width: poiBox.width - poiIcon.width - (2*Theme.paddingSmall)
                            property string nameFormated: settings.showAltLanguage && poiBox.altName != "" ?
                                                              (poiBox.altName + (poiBox.name != ""? " (" +poiBox.name+ ")" : "")) :
                                                              poiBox.name
                            text: nameFormated != "" ? nameFormated : (poiBox.operatorName!=""? poiBox.operatorName : poiBox.ref)
                            font.pixelSize: Theme.fontSizeMedium
                        }
                        PhoneRow {
                            id: phoneRow
                            phone: poiBox.phone
                            visible: poiBox.phone!="" && poiBox.state == "INFORMATIVE"
                        }
                        WebsiteRow {
                            id: websiteRow
                            website: poiBox.website
                            visible: poiBox.website!="" && poiBox.state == "INFORMATIVE"
                        }
                        Row{
                            id : placeTools
                            visible: poiBox.state == "INFORMATIVE"
                            width: waypointBtn.width+osmNoteBtn.width+searchBtn.width+routeBtn.width+Theme.paddingLarge
                            height: waypointBtn.height

                            CollectionModel {
                                id: collectionModel
                                property int waypointId;
                                property string operation
                                onLoadingChanged: {
                                    if (operation!="edit"){
                                        return;
                                    }

                                    for (var row=0; row<collectionModel.rowCount(); row++){
                                        var index=collectionModel.index(row, 0);
                                        var id=collectionModel.data(index, CollectionModel.IdRole);
                                        var type=collectionModel.data(index, CollectionModel.TypeRole);
                                        if (id==waypointId && type=="waypoint") {
                                            editDialog.itemId = id;
                                            editDialog.name = collectionModel.data(index, CollectionModel.NameRole);
                                            editDialog.description = collectionModel.data(index, CollectionModel.DescriptionRole);
                                            editDialog.symbolSelectorVisible = true;
                                            editDialog.symbol = collectionModel.data(index, CollectionModel.SymbolRole);
                                            editDialog.open();
                                            collectionModel.operation="";
                                            return;
                                        }
                                    }
                                    console.log("Waypoint " + collectionModel.waypointId + " (collection " + collectionModel.collectionId + ") was not found.");
                                    collectionModel.operation="";
                                }
                            }

                            CollectionEditDialog {
                                id: editDialog

                                property string itemId: ""
                                title: qsTr("Edit waypoint")

                                onAccepted: {
                                    console.log("Edit waypoint " + itemId + ": " + name + " / " + description);
                                    collectionModel.editWaypoint(itemId, name, description, symbol);
                                    poiBox.hide();
                                }
                                onRejected: {
                                }
                            }

                            IconButton{
                                id: waypointBtn
                                icon.source: poiBox.isWaypoint ? "image://harbour-osmscout/pics/edit-place.svg?" + Theme.primaryColor :
                                                                 "image://harbour-osmscout/pics/new-place.svg?" + Theme.primaryColor
                                icon.sourceSize.width: Theme.iconSizeMedium
                                icon.sourceSize.height: Theme.iconSizeMedium

                                onClicked: {
                                    if (poiBox.isWaypoint) {
                                        // edit poi editor when it is poi
                                        var ids = collectionMapBridge.getWaypointIds(poiBox.poiId);
                                        if (ids.length<2) {
                                            console.log("Cannot find waypoint with poi id \"" + poiBox.poiId + "\"");
                                        } else {
                                            collectionModel.operation = "edit"
                                            collectionModel.waypointId = ids[1];
                                            collectionModel.collectionId = ids[0];
                                            console.log("Request for edit waypoint " + collectionModel.waypointId + " (collection " + collectionModel.collectionId + ")");
                                            return;
                                        }
                                    }

                                    var name = poiBox.name;
                                    console.log("Add waypoint \"" + name + "\"");
                                    pageStack.push(Qt.resolvedUrl("NewWaypoint.qml"),
                                                  {
                                                    latitude: poiBox.lat,
                                                    longitude: poiBox.lon,
                                                    acceptDestination: Global.mapPage,
                                                    description: "",
                                                    name: name
                                                  });
                                }
                            }

                            IconButton{
                                id: waypointDeteleBtn
                                icon.source: "image://theme/icon-m-delete"
                                visible: poiBox.isWaypoint
                                onClicked: {
                                    var ids = collectionMapBridge.getWaypointIds(poiBox.poiId);
                                    if (ids.length<2) {
                                        console.log("Cannot find waypoint with poi id \"" + poiBox.poiId + "\"");
                                    } else {
                                        collectionModel.operation = "delete"
                                        collectionModel.waypointId = ids[1];
                                        collectionModel.collectionId = ids[0];
                                        console.log("Request for delete waypoint " + collectionModel.waypointId + " (collection " + collectionModel.collectionId + ")");
                                        //: remorse dialog
                                        remorse.execute(qsTr("Deleting"),
                                                        function() {
                                                            if (collectionModel.operation == "delete") {
                                                                collectionModel.deleteWaypoint(collectionModel.waypointId);
                                                                collectionModel.operation = "";
                                                                poiBox.hide();
                                                                map.deactivateIcons();
                                                            }
                                                        },
                                                        5 * 1000);
                                    }
                                }
                            }

                            IconButton{
                                id: osmNoteBtn

                                icon.source: "image://theme/icon-m-edit"
                                onClicked: Qt.openUrlExternally("https://www.openstreetmap.org/note/new#map="+map.view.magLevel+"/"+poiBox.lat.toString()+"/"+poiBox.lon.toString()+"")
                            }

                            IconButton{
                                id: searchBtn

                                icon.source: "image://theme/icon-m-search"
                                onClicked: {

                                    var searchPage=pageStack.push(Qt.resolvedUrl("Search.qml"),
                                                  {
                                                    searchCenterLat: poiBox.lat,
                                                    searchCenterLon: poiBox.lon,
                                                    acceptDestination: Global.mapPage
                                                  });
                                    searchPage.selectLocation.connect(Global.mapPage.selectLocation);
                                }
                            }

                            IconButton{
                                id: routeBtn

                                icon.source: "image://theme/icon-m-shortcut"
                                onClicked: {

                                    pageStack.push(Qt.resolvedUrl("Routing.qml"),
                                                   {
                                                       toLat: poiBox.lat,
                                                       toLon: poiBox.lon
                                                   })
                                }
                            }
                        }
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
            height: Global.navigationModel.destinationSet ? nextStepBackground.height + navigationContextMenu.height : 0
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
                height: Global.navigationModel.destinationSet ?
                            Math.max(Theme.iconSizeLarge + 3*Theme.paddingMedium,
                                     Theme.paddingMedium + distanceToNextStep.height + Theme.paddingMedium + nextStepDescription.height +
                                     (destinationsText.visible ? Theme.paddingSmall + destinationsText.height : 0)) : 0

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
                Label {
                    id: destinationsText

                    visible: Global.navigationModel.nextRouteStep.destinations.length > 0
                    anchors{
                        left: distanceToNextStep.left
                        top: nextStepDescription.bottom
                        margins: Theme.paddingSmall
                    }

                    text: qsTr("Destinations: %1").arg(Global.navigationModel.nextRouteStep.destinations.join(", "))
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor

                    wrapMode: Text.NoWrap
                    truncationMode: TruncationMode.Fade
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

        LaneTurns {
            id: laneTurnsComponent

            laneTurns: Global.navigationModel.laneTurns
            visible: Global.navigationModel.laneSuggested
            suggestedLaneFrom: Global.navigationModel.suggestedLaneFrom
            suggestedLaneTo: Global.navigationModel.suggestedLaneTo

            radius: Theme.paddingMedium
            color: Theme.rgba(Theme.highlightDimmerColor, 0.4)
            height: Theme.iconSizeLarge

            anchors{
                top: nextStepBox.bottom
                margins: Theme.paddingMedium
                horizontalCenter: parent.horizontalCenter
            }
            maxWidth: parent.width - (speedIndicator.width + menuBtn.width + 4*Theme.paddingMedium)
        }

        SpeedIndicator {
            id: speedIndicator
            currentSpeed: Global.navigationModel.currentSpeed
            maximumSpeed: Global.navigationModel.maxAllowedSpeed
            visible: Global.navigationModel.destinationSet
            z: 1

            anchors.left: parent.left
            anchors.top: nextStepBox.bottom
            anchors.margins: Theme.paddingMedium
            height: visible ? Theme.iconSizeLarge : 0
            width: height
        }

        Rectangle {
            id: infoPanel
            visible: trackerIndicator.active || elevationIndicator.active || accuracyIndicator.active

            anchors.left: parent.left
            anchors.top: speedIndicator.bottom
            anchors.margins: Theme.paddingMedium

            radius: Theme.paddingMedium

            color: Theme.rgba(Theme.highlightDimmerColor, 0.4)

            width: Math.max(trackerIndicator.width, Math.max(elevationIndicator.width, accuracyIndicator.width))
            height: trackerIndicator.height + elevationIndicator.height + accuracyIndicator.height


            Rectangle {
                id: trackerIndicator

                property bool active: Global.tracker.tracking && AppSettings.showTrackerDistance
                visible: active
                color: "transparent"

                height: visible ? Theme.iconSizeMedium : 0
                width: trackerDistanceLabel.width + runnerIcon.width + Theme.paddingMedium

                anchors.left: parent.left
                anchors.top: parent.top

                Image {
                    id: runnerIcon
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: height                    
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignLeft
                    sourceSize.width: width
                    sourceSize.height: height

                    TrackTypes {
                        id: trackTypes
                    }

                    source: trackTypes.typeIcon(Global.tracker.type)

                    property double originalOpacity: 0.6
                    opacity: originalOpacity
                    Behavior on opacity { PropertyAnimation {} }
                    Timer{
                        id: opacityRevertTimer
                        running: false
                        repeat: false
                        interval: 200
                        onTriggered: {
                            runnerIcon.opacity = runnerIcon.originalOpacity;
                        }
                    }
                    Connections {
                        target: Global.tracker
                        onStatisticsUpdated: {
                            runnerIcon.opacity = 1.0;
                            opacityRevertTimer.running = true;
                        }
                    }

                    onWidthChanged: {
                        console.log("runner dimensions: " + width + " x " + height);
                    }
                }
                Text {
                    id: trackerDistanceLabel
                    text: Utils.humanDistanceCompact(Global.tracker.distance)
                    anchors.left: runnerIcon.right
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.rgba(Theme.primaryColor, 1.0)

                    font.pointSize: Theme.fontSizeExtraSmall
                }
                MouseArea {
                    id: runnerBtnMouseArea
                    anchors.fill: parent
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("Tracker.qml"))
                    }
                }
            }

            Rectangle {
                id: elevationIndicator

                property bool active: Global.positionSource.altitudeValid && AppSettings.showElevation
                visible: active
                color: "transparent"

                height: visible ? Theme.iconSizeMedium : 0
                width: elevationLabel.width + elevationIcon.width + Theme.paddingMedium

                anchors.left: parent.left
                anchors.top: trackerIndicator.bottom

                Image {
                    id: elevationIcon
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: height
                    source: "image://harbour-osmscout/poi-icons/mountain.svg?" + Theme.primaryColor
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignLeft
                    sourceSize.width: width
                    sourceSize.height: height
                    opacity: 0.6
                }
                Text {
                    id: elevationLabel
                    text: Utils.elevationShort(Global.positionSource.altitude)
                    anchors.left: elevationIcon.right
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.rgba(Theme.primaryColor, 1.0)

                    font.pointSize: Theme.fontSizeExtraSmall
                }
                MouseArea {
                    id: elevationIndicatorMouseArea
                    anchors.fill: parent
                    onClicked: {
                        var searchPage=pageStack.push(Qt.resolvedUrl("Search.qml"),
                                                      {
                                                          searchCenterLat: Global.positionSource.lat,
                                                          searchCenterLon: Global.positionSource.lon,
                                                          searchFieldText: "poi:10000:natural_peak",
                                                          acceptDestination: mapPage
                                                      });
                    }
                }
            }

            Rectangle {
                id: accuracyIndicator

                property bool active: Global.positionSource.horizontalAccuracyValid && AppSettings.showAccuracy
                visible: active
                color: "transparent"

                height: visible ? Theme.iconSizeMedium : 0
                width: accuracyLabel.width + accuracyIcon.width + Theme.paddingMedium

                anchors.left: parent.left
                anchors.top: elevationIndicator.bottom

                Image {
                    id: accuracyIcon
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: height
                    source: "image://harbour-osmscout/poi-icons/circle-stroked.svg?" + Theme.primaryColor
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignLeft
                    sourceSize.width: width
                    sourceSize.height: height
                    opacity: 0.6
                }
                Text {
                    id: accuracyLabel
                    text: Utils.humanDistanceCompact(Global.positionSource.horizontalAccuracy)
                    anchors.left: accuracyIcon.right
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.rgba(Theme.primaryColor, 1.0)

                    font.pointSize: Theme.fontSizeExtraSmall
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
        Rectangle {
            id : showCollectionsBtn
            anchors{
                right: parent.right
                top: menuBtn.bottom

                topMargin: Theme.paddingMedium
                rightMargin: Theme.paddingMedium
                bottomMargin: Theme.paddingMedium
                leftMargin: Theme.paddingMedium
            }

            property bool active: AppSettings.showCollectionToggle
            visible: active
            width: Theme.iconSizeLarge
            height: active ? width : 0

            radius: Theme.paddingMedium

            color: Theme.rgba(Theme.highlightDimmerColor, 0.2)

            Image{
                source: AppSettings.showCollections ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter
                sourceSize.width: width
                sourceSize.height: height
            }
            MouseArea {
                id: showCollectionsBtnMouseArea
                anchors.fill: parent
                onClicked: {
                    AppSettings.showCollections = !AppSettings.showCollections
                }
            }
        }
        Rectangle {
            id : nightModeBtn
            anchors{
                right: parent.right
                top: showCollectionsBtn.bottom

                topMargin: Theme.paddingMedium
                rightMargin: Theme.paddingMedium
                bottomMargin: Theme.paddingMedium
                leftMargin: Theme.paddingMedium
            }

            property bool active: AppSettings.showNightModeToggle
            property bool daylight: false
            visible: active
            width: Theme.iconSizeLarge
            height: active ? width : 0

            radius: Theme.paddingMedium

            color: Theme.rgba(Theme.highlightDimmerColor, 0.2)

            function update() {
                for (var row = 0; row < styleFlagsModel.rowCount(); row++) {
                    var index = styleFlagsModel.index(row, 0);
                    var key = styleFlagsModel.data(index, StyleFlagsModel.KeyRole);
                    if (key === "daylight") {
                        nightModeBtn.daylight = styleFlagsModel.data(index, StyleFlagsModel.ValueRole);
                        // console.log("update daylight to " + nightModeBtn.daylight);
                        return;
                    }
                }
                console.log("stylesheet has no daylight flag (" + styleFlagsModel.rowCount() + " flags)");
            }

            Connections {
                target: map
                onStylesheetFilenameChanged: nightModeBtn.update()
            }
            Connections {
                target: styleFlagsModel
                onModelReset: nightModeBtn.update()
            }

            Component.onCompleted: update()

            Image{
                source: nightModeBtn.daylight ? "image://theme/icon-m-day" : "image://theme/icon-m-night"
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter
                sourceSize.width: width
                sourceSize.height: height
            }
            MouseArea {
                id: nightModeBtnMouseArea
                anchors.fill: parent
                onClicked: {
                    //AppSettings.showCollections = !AppSettings.showCollections
                    styleFlagsModel.setFlag("daylight", !nightModeBtn.daylight);
                }
            }
        }
    }
}
