
import QtQuick 2.0
import Sailfish.Silica 1.0

import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: mapPage

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
            positionIndicator.color = (position.latitudeValid && position.longitudeValid) ? "#8000FF00": "#80FF0000";

            map.locationChanged(
                        position.latitudeValid && position.longitudeValid,
                        position.coordinate.latitude, position.coordinate.longitude,
                        position.horizontalAccuracyValid, position.horizontalAccuracy);
        }
    }

        Map {
            id: map

            focus: true
            anchors.fill: parent

            showCurrentPosition: true

            function getFreeRect() {
                return Qt.rect(Theme.horizSpace,
                               Theme.vertSpace+searchDialog.height+Theme.vertSpace,
                               map.width-2*Theme.horizSpace,
                               map.height-searchDialog.y-searchDialog.height-3*Theme.vertSpace)
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


            Rectangle{
                id : osmCopyright
                anchors{
                    left: parent.left
                    bottom: parent.bottom
                }
                width: label.width + 20
                height: label.height + 12

                color: "white"
                opacity: 0.7

                Text {
                    id: label
                    text: qsTr("© OpenStreetMap contributors")
                    anchors.centerIn: parent

                    font.pointSize: 12
                }

            }

            Rectangle {
                id : currentPositionBtn

                anchors{
                    right: parent.right
                    bottom: parent.bottom
                    rightMargin: 8
                    bottomMargin: 8
                }
                width: 90
                height: 90

                color: "#80FFFFFF"
                border.color: "white"
                border.width: 1
                radius: width*0.5

                Rectangle {
                    id: positionIndicator
                    anchors{
                        horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    width: 20
                    height: 20

                    color: "#80FF0000"
                    border.color: "black"
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
