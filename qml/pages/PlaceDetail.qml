
import QtQuick 2.0
import Sailfish.Silica 1.0
import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: placeDetailPage

    property double placeLat: 0.1;
    property double placeLon: 0.2;

    property bool currentLocValid: false;
    property double currentLocLat: 0;
    property double currentLocLon: 0;

    onStatusChanged: {
        if (status == PageStatus.Activating){
            map.showCoordinatesInstantly(placeLat, placeLon);
            map.addPositionMark(0, placeLat, placeLon);
            locationInfoModel.setLocation(placeLat, placeLon);
        }
    }

    function changePosition(lat, lon, moveMap){
        placeLat = lat;
        placeLon = lon;
        map.addPositionMark(0, placeLat, placeLon);
        locationInfoModel.setLocation(placeLat, placeLon);
        if (moveMap){
            map.showCoordinates(placeLat, placeLon);
        }
    }

    function formatCoord(lat, lon){
        // use consistent GeoCoord toString method
        return  (Math.round(lat * 100000)/100000) + " " + (Math.round(lon * 100000)/100000);
    }

    function humanDistance(distance){
        if (distance < 1500){
            return Math.round(distance) + " meters";
        }
        if (distance < 20000){
            return (Math.round(distance * 100)/100) + " km";
        }
        return Math.round(distance/1000) + " km";
    }

    PositionSource {
        id: positionSource
        active: true

        onPositionChanged: {
            currentLocValid = position.latitudeValid && position.longitudeValid;
            currentLocLat = position.coordinate.latitude;
            currentLocLon = position.coordinate.longitude;
        }
    }

    LocationInfoModel{
        id: locationInfoModel
    }

    Drawer {
        anchors.fill: parent

        dock: placeDetailPage.isPortrait ? Dock.Top : Dock.Left
        open: true


        background:  Rectangle{

            anchors.fill: parent
            color: "transparent"

            Row{
                id: placeLocationRow

                spacing: Theme.paddingMedium
                anchors.right: parent.right
                Label {
                    id: placeLocationLabel
                    text: formatCoord(placeLat, placeLon)
                    anchors.verticalCenter: parent.verticalCenter
                }
                IconButton{
                    icon.source: "image://theme/icon-m-clipboard"
                    onClicked: {
                        Clipboard.text = placeLocationLabel.text
                    }
                }
            }
            Row{
                id: placeDistanceRow
                spacing: Theme.paddingMedium
                visible: currentLocValid
                height: placeDistanceLabel.height

                anchors {
                    top: placeLocationRow.bottom
                }

                Label {
                    id: placeDistanceLabel
                    text: qsTrId("%1 %2 from you")
                        .arg(humanDistance(locationInfoModel.distance(currentLocLat, currentLocLon, placeLat, placeLon)))
                        .arg(locationInfoModel.bearing(currentLocLat, currentLocLon, placeLat, placeLon))

                    color: Theme.highlightColor
                    width: parent.width
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            SilicaListView {
                id: locationInfoView
                width: parent.width
                model: locationInfoModel
                //model: 100

                opacity: locationInfoModel.ready ? 1.0 : 0.0
                Behavior on opacity { FadeAnimation {} }

                clip: true

                anchors {
                    top: placeDistanceRow.bottom
                    bottom: parent.bottom
                }


                delegate: Column{
                    spacing: Theme.paddingSmall

                    Label {
                        id: entryDistanceLabel

                        width: locationInfoView.width

                        text: qsTrId("%1 %2 from")
                            .arg(humanDistance(distance))
                            .arg(bearing)

                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                        visible: !inPlace
                    }
                    Label {
                        id: entryTitle

                        width: locationInfoView.width

                        text: title // label
                        font.pixelSize: Theme.fontSizeLarge
                    }
                    Label {
                        id: entryAddress

                        width: locationInfoView.width

                        text: address
                        font.pixelSize: Theme.fontSizeMedium
                    }
                }
                VerticalScrollDecorator {}

            }
            BusyIndicator {
                id: busyIndicator
                running: !locationInfoModel.ready
                size: BusyIndicatorSize.Large
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MapComponent {
          id: map

          anchors.fill: parent
          showCurrentPosition: true

          onTap: {
              console.log("tap: " + sceenX + "x" + screenY + " @ " + lat + " " + lon);
              changePosition(lat, lon, true);
          }
          onLongTap: {
              console.log("long tap: " + sceenX + "x" + screenY + " @ " + lat + " " + lon);
              changePosition(lat, lon, false)
          }
        }
    }
}
