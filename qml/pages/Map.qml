
import QtQuick 2.0
import Sailfish.Silica 1.0

import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: mapPage


    Settings {
        id: settings
        //mapDPI: 50
    }

    onStatusChanged: {
        if (status == PageStatus.Activating){
            map.view = settings.mapView;
        }
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
            interactive: false
            anchors.fill: parent
            height: childrenRect.height
            id: menu
            model: ListModel {
                ListElement { itemtext: "Search";       itemicon: "image://theme/icon-m-search";         action: "search";   }
                ListElement { itemtext: "Where am I?";  itemicon: "image://theme/icon-m-whereami";       action: "whereami"; }
                ListElement { itemtext: "Settings";     itemicon: "image://theme/icon-m-developer-mode"; action: "settings"; }
                ListElement { itemtext: "Layers";       itemicon: "image://theme/icon-m-levels";         action: "layers";   }
                ListElement { itemtext: "Bookmarks";    itemicon: "image://theme/icon-m-favorite";       action: "bookmarks";}
                ListElement { itemtext: "About";        itemicon: "image://theme/icon-m-about";          action: "about";    }
            }

            delegate: ListItem{
                id: searchRow

                function isEnabled(action){
                    return ((action == "whereami" && positionSource.valid) ||
                            action == "about")
                }

                function onAction(action){
                    if (action == "whereami"){
                        if (positionSource.valid){
                            pageStack.push(Qt.resolvedUrl("PlaceDetail.qml"),
                                           {placeLat: positionSource.lat, placeLon: positionSource.lon})
                        }else{
                            console.log("I can't say where you are. Position is not valid!")
                        }
                    }else if (action == "about"){
                        pageStack.push(Qt.resolvedUrl("About.qml"))
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

            showCurrentPosition: true

            onTap: {

                console.log("tap: " + sceenX + "x" + screenY + " @ " + lat + " " + lon + " (map center "+ map.view.lat + " " + map.view.lon + ")");
                if (drawer.open){
                    drawer.open = false;
                }
            }
            onLongTap: {

                console.log("long tap: " + sceenX + "x" + screenY + " @ " + lat + " " + lon);

                pageStack.push(Qt.resolvedUrl("PlaceDetail.qml"),
                               {placeLat: lat, placeLon: lon})
            }
            onViewChanged: {
                //console.log("map center "+ map.view.lat + " " + map.view.lon + "");
                settings.mapView = map.view;
            }

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
                id : menuBtn
                anchors{
                    right: parent.right
                    top: parent.top

                    topMargin: Theme.paddingMedium
                    rightMargin: Theme.paddingMedium
                    bottomMargin: Theme.paddingMedium
                    leftMargin: Theme.paddingMedium
                }
                width: 90
                height: 90
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
                width: 90
                height: 90

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


                MouseArea {
                  id: currentPositionBtnMouseArea
                  anchors.fill: parent

                  hoverEnabled: true
                  onClicked: {
                      if (positionSource.valid){
                          map.showCoordinates(positionSource.lat, positionSource.lon);
                      }
                  }
                }

            }
        }
    }
}
