import QtQuick 2.2

import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Window 2.0
import QtQml.Models 2.2

import QtPositioning 5.2

import harbour.osmscout.map 1.0
//import net.sf.libosmscout.map 1.0

import "custom"
import "." // Global singleton

Window {
    id: mainWindow
    objectName: "main"
    title: "OSMScout"
    visible: true
    width: 480
    height: 800

    function openAboutDialog() {
        var component = Qt.createComponent("AboutDialog.qml")
        var dialog = component.createObject(mainWindow, {})

        dialog.opened.connect(onDialogOpened)
        dialog.closed.connect(onDialogClosed)
        dialog.open()
    }

    function showLocation(location) {
        map.showLocation(location)
    }

    function onDialogOpened() {
        info.visible = false
        navigation.visible = false
    }

    function onDialogClosed() {
        info.visible = true
        navigation.visible = true

        map.focus = true
    }

    Settings {
        id: settings
        //mapDPI: 50
        //fontName: "Comic Sans MS"
        fontName: "Sans"
    }

    Component.onCompleted: {
        Global.mapPage = mainWindow;
        Global.mainMap = map;
        console.log("completed: " + map + " / " + Global.mainMap);
    }


    CollectionListModel {
        id: collectionModel
        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading);
        }
        Component.onCompleted: {
            collectionModel.importCollection("test.gpx");
        }
    }

    /*
    CollectionTrackModel {
        id: trackModel
        trackId: "19"
        property var cropped: false

        onLoadingChanged: {
            if (!loading && !cropped){
                trackModel.cropEnd(10);
                cropped = true;
            }
        }
    }
    */

    InstalledMapsModel{
        id: installedMapsModel

        function getData(row, role){
            return installedMapsModel.data(installedMapsModel.index(row, 0), role);
        }

        function listMaps(){
            console.log("list maps " + installedMapsModel.rowCount());
            for (var row = 0; row < installedMapsModel.rowCount(); row++){
                console.log(" map " + row +": "+ getData(row, InstalledMapsModel.NameRole) + " (" + getData(row, InstalledMapsModel.PathRole) + ")");
            }
            console.log("done");
        }

        Component.onCompleted: listMaps()
        onDatabaseListChanged: listMaps()
    }

    LocationInfoModel {
        id: locationInfoModel
        Component.onCompleted: {
            locationInfoModel.setLocation(50.08923, 14.49837);
        }

        function getData(row, role){
            return locationInfoModel.data(locationInfoModel.index(row, 0), role);
        }

        onReadyChanged: {
            if (!ready){
                console.log("Loading objects around...");
                return;
            }
            console.log("objects around: " + locationInfoModel.rowCount());
            for (var row = 0; row < locationInfoModel.rowCount(); row++) {
                console.log("  " + row + ":");
                console.log("    label: " + getData(row, LocationInfoModel.LabelRole));
                console.log("    region: " + getData(row, LocationInfoModel.RegionRole));
                console.log("    address: " + getData(row, LocationInfoModel.AddressRole));
                console.log("    inPlace: " + getData(row, LocationInfoModel.InPlaceRole));
                console.log("    distance: " + getData(row, LocationInfoModel.DistanceRole));
                console.log("    bearing: " + getData(row, LocationInfoModel.BearingRole));
                console.log("    poi: " + getData(row, LocationInfoModel.PoiRole));
                console.log("    type: " + getData(row, LocationInfoModel.TypeRole));
                console.log("    postalCode: " + getData(row, LocationInfoModel.PostalCodeRole));
                console.log("    website: " + getData(row, LocationInfoModel.WebsiteRole));
                console.log("    phone: " + getData(row, LocationInfoModel.PhoneRole));
                console.log("    addressLocation: " + getData(row, LocationInfoModel.AddressLocationRole));
                console.log("    addressNumber: " + getData(row, LocationInfoModel.AddressNumberRole));
            }
        }
    }

    PositionSource {
        id: positionSource

        active: true

        onValidChanged: {
            console.log("Positioning is " + valid)
            console.log("Last error " + sourceError)

            for (var m in supportedPositioningMethods) {
                console.log("Method " + m)
            }
        }

        onPositionChanged: {
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
        }
    }

    GridLayout {
        id: content
        anchors.fill: parent

        Map {
            id: map
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true

            CollectionMapBridge{
                map: map
            }

            renderingType: "tiled" // plane or tiled

            //property var overlayWay: map.createOverlayArea("_route");
            property var overlayWay: map.createOverlayArea();
            onTap: {
                overlayWay.addPoint(lat, lon);
                map.addOverlayObject(0, overlayWay);

                var wpt = map.createOverlayNode("_waypoint");
                wpt.addPoint(lat, lon);
                wpt.name = "Pos: " + lat + " " +lon;
                map.addOverlayObject(1, wpt);

                console.log("tap: " + screenX + "x" + screenY + " @ " + lat + " " + lon+ " (map center "+ map.view.lat + " " + map.view.lon + ")");
            }
            onLongTap: {

                console.log("long tap: " + screenX + "x" + screenY + " @ " + lat + " " + lon);
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
                    //searchDialog.focus = true
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

            TiledMapOverlay {
                anchors.fill: parent
                view: map.view
                enabled: true
                opacity: 0.7
                // If you intend to use tiles from OpenMapSurfer services in your own applications please contact us.
                // https://korona.geog.uni-heidelberg.de/contact.html
                provider: {
                      "id": "ASTER_GDEM",
                      "name": "Hillshade",
                      "servers": [
                          //"https://korona.geog.uni-heidelberg.de/tiles/asterh/x=%2&y=%3&z=%1"
                          "https://osmscout.karry.cz/hillshade/tile.php?z=%1&x=%2&y=%3"
                      ],
                      "maximumZoomLevel": 18,
                      "copyright": "© IAT, METI, NASA, NOAA",
                    }
            }

            // Use PinchArea for multipoint zoom in/out?
            /*
            SearchDialog {
                id: searchDialog

                y: Theme.vertSpace

                anchors.horizontalCenter: parent.horizontalCenter

                desktop: map

                onShowLocation: {
                    map.showLocation(location)
                }

                onStateChanged: {
                    if (state==="NORMAL") {
                        onDialogClosed()
                    }
                    else {
                        onDialogOpened()
                    }
                }
            }
            */

            // Bottom left column
            ColumnLayout {
                id: info

                x: 4
                y: parent.height-height-4

                spacing: 4

                MapButton {
                    id: about
                    label: "?"

                    onClicked: {
                        openAboutDialog()
                    }
                }
            }

            // Bottom right column
            ColumnLayout {
                id: navigation

                x: parent.width-width-4
                y: parent.height-height-4

                spacing: 4

                MapButton {
                    id: zoomIn
                    label: "+"

                    onClicked: {
                        map.zoomIn(2.0)
                    }
                }

                MapButton {
                    id: zoomOut
                    label: "-"

                    onClicked: {
                        map.zoomOut(2.0)
                    }
                }
            }
        }
    }
}
