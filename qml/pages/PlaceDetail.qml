
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
            return Math.round(distance) + " "+ qsTr("meters");
        }
        if (distance < 20000){
            return (Math.round((distance/1000) * 10)/10) + " "+ qsTr("km");
        }
        return Math.round(distance/1000) + " "+ qsTr("km");
    }
    function humanBearing(bearing){
        if (bearing == "W")
            return qsTr("west");
        if (bearing == "E")
            return qsTr("east");
        if (bearing == "S")
            return qsTr("south");
        if (bearing == "N")
            return qsTr("north");

        return bearing;
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

            /*
            OpacityRampEffect {
                //enabled: !onlineTileProviderComboBox._menuOpen //true
                offset: 1 - 1 / slope
                slope: locationInfoView.height / (Theme.paddingLarge * 4)
                direction: 2
                sourceItem: locationInfoView
            }
            */

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
                width: parent.width - (2 * Theme.paddingMedium)
                x: Theme.paddingMedium

                anchors {
                    top: placeLocationRow.bottom
                }

                Label {
                    id: placeDistanceLabel
                    text: locationInfoModel.distance(currentLocLat, currentLocLon, placeLat, placeLon) < 2 ?
                              qsTr("You are here") :
                              qsTr("%1 %2 from you")
                                .arg(humanDistance(locationInfoModel.distance(currentLocLat, currentLocLon, placeLat, placeLon)))
                                .arg(humanBearing(locationInfoModel.bearing(currentLocLat, currentLocLon, placeLat, placeLon)))

                    color: Theme.highlightColor
                    width: parent.width
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            SilicaListView {
                id: locationInfoView
                width: parent.width - (2 * Theme.paddingMedium)
                //contentHeight: content.height + 2*Theme.paddingLarge
                spacing: Theme.paddingMedium
                x: Theme.paddingMedium
                model: locationInfoModel
                //model: 100

                opacity: locationInfoModel.ready ? 1.0 : 0.0
                Behavior on opacity { FadeAnimation {} }

                VerticalScrollDecorator {}
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

                        text: qsTr("%1 %2 from")
                            .arg(humanDistance(distance))
                            .arg(humanBearing(bearing))

                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                        visible: !inPlace
                    }
                    Label {
                        id: entryPoi

                        width: locationInfoView.width

                        text: poi // label
                        font.pixelSize: Theme.fontSizeLarge
                        visible: poi != ""
                    }
                    Label {
                        id: entryAddress

                        width: locationInfoView.width

                        text: address
                        font.pixelSize: Theme.fontSizeLarge
                        visible: address != ""
                    }
                    Label {
                        id: entryRegion

                        width: locationInfoView.width

                        text: {
                            if (region.length > 0){
                                var str = region[0];
                                if (postalCode != ""){
                                    str += ", "+ postalCode;
                                }
                                if (region.length > 1){
                                    for (var i=1; i<region.length; i++){
                                        str += ", "+ region[i];
                                    }
                                }
                            }else if (postalCode!=""){
                                return postalCode;
                            }
                        }
                        font.pixelSize: Theme.fontSizeMedium
                        visible: region.length > 0 || postalCode != ""
                    }
                    Row {
                        id: phoneRow
                        visible: phone != ""
                        width: locationInfoView.width
                        height: phoneIcon.height

                        IconButton{
                            id: phoneIcon
                            visible: phone != ""
                            icon.source: "image://theme/icon-m-dialpad"

                            MouseArea{
                                onClicked: Qt.openUrlExternally("tel:%1".arg(phone))
                                anchors.fill: parent
                            }
                        }
                        Label {
                            id: phoneLabel
                            anchors.left: phoneIcon.right
                            anchors.verticalCenter: phoneIcon.verticalCenter
                            visible: phone != ""
                            text: phone

                            color: Theme.highlightColor
                            truncationMode: TruncationMode.Fade

                            MouseArea{
                                onClicked: Qt.openUrlExternally("tel:%1".arg(phone))
                                anchors.fill: parent
                            }
                        }
                    }
                    Row {
                        id: websiteRow
                        visible: website != ""
                        width: locationInfoView.width
                        height: websiteIcon.height

                        IconButton{
                            id: websiteIcon
                            visible: website != ""
                            icon.source: "image://theme/icon-m-region"

                            MouseArea{
                                onClicked: Qt.openUrlExternally(website)
                                anchors.fill: parent
                            }
                        }
                        Label {
                            id: websiteLabel
                            anchors.left: websiteIcon.right
                            anchors.verticalCenter: websiteIcon.verticalCenter
                            visible: website != ""
                            text: website

                            font.underline: true
                            color: Theme.highlightColor
                            truncationMode: TruncationMode.Fade

                            MouseArea{
                                onClicked: Qt.openUrlExternally(website)
                                anchors.fill: parent
                            }
                        }
                    }

                }
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
