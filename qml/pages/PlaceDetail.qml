/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
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
import "../custom/Utils.js" as Utils

Page {
    id: placeDetailPage

    property double placeLat: 0.1;
    property double placeLon: 0.2;

    property bool currentLocValid: false;
    property double currentLocLat: 0;
    property double currentLocLon: 0;

    property var mapPage
    property var mainMap

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

    Settings {
        id: settings
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
        id: drawer
        anchors.fill: parent

        dock: placeDetailPage.isPortrait ? Dock.Top : Dock.Left
        open: true
        backgroundSize: placeDetailPage.isPortrait ? drawer.height * 0.6 : drawer.width * 0.6

        background:  Rectangle{

            anchors.fill: parent
            color: "transparent"

            OpacityRampEffect {
                //enabled: !placeLocationComboBox._menuOpen //true
                offset: 1 - 1 / slope
                slope: locationInfoView.height / (Theme.paddingLarge * 4)
                direction: 2
                sourceItem: locationInfoView
            }

            Rectangle{
                id: placeLocationRow

                color: "transparent"
                width: parent.width
                height: Math.max(placeLocationComboBox.height, clipboardBtn.height) + placeDistanceLabel.height
                anchors{
                    horizontalCenter: parent.horizontalCenter
                }

                ComboBox {
                    id: placeLocationComboBox
                    y: (clipboardBtn.height - contentItem.height) /2
                    contentItem.opacity: 0

                    property bool initialized: false

                    menu: ContextMenu {
                        MenuItem { text: Utils.formatCoord(placeLat, placeLon, "degrees") }
                        MenuItem { text: Utils.formatCoord(placeLat, placeLon, "geocaching") }
                        MenuItem { text: Utils.formatCoord(placeLat, placeLon, "numeric") }
                    }
                    onCurrentItemChanged: {
                        if (!initialized){
                            return;
                        }
                        var format = "degrees";
                        if (currentIndex == 0){
                            format = "degrees";
                        }else if (currentIndex == 1){
                            format = "geocaching";
                        }else if (currentIndex == 2){
                            format = "numeric";
                        }

                        AppSettings.gpsFormat = format
                    }
                    Component.onCompleted: {
                        currentIndex = 0;
                        if (AppSettings.gpsFormat === "degrees"){
                            currentIndex = 0;
                        } else if (AppSettings.gpsFormat === "geocaching"){
                            currentIndex = 1;
                        } else if (AppSettings.gpsFormat === "numeric"){
                            currentIndex = 2;
                        }

                        initialized = true;
                    }
                }
                IconButton{
                    id: clipboardBtn
                    anchors{
                        right: parent.right
                    }

                    icon.source: "image://theme/icon-m-clipboard"
                    onClicked: {
                        Clipboard.text = placeLocationLabel.text
                    }
                }
                /* it seems to be impossible to align ComboBox content to right, so we hide it (contentItem.opacity) and add this another Label :) */
                Label {
                    id: placeLocationLabel
                    text: placeLocationComboBox.value //formatCoord(placeLat, placeLon)
                    y: (clipboardBtn.height - height) /2
                    color: Theme.highlightColor
                    anchors{
                        right: clipboardBtn.left
                    }
                }
                Label {
                    id: placeDistanceLabel
                    text: locationInfoModel.distance(currentLocLat, currentLocLon, placeLat, placeLon) < 2 ?
                              qsTr("You are here") :
                              qsTr("%1 %2 from you")
                                .arg(Utils.humanDistance(locationInfoModel.distance(currentLocLat, currentLocLon, placeLat, placeLon)))
                                .arg(Utils.humanBearing(locationInfoModel.bearing(currentLocLat, currentLocLon, placeLat, placeLon)))

                    color: Theme.highlightColor
                    //width: parent.width
                    font.pixelSize: Theme.fontSizeSmall
                    anchors{
                        right: clipboardBtn.left
                        bottom: parent.bottom
                    }
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
                    top: placeLocationRow.bottom
                    bottom: parent.bottom
                }

                delegate: Column{
                    spacing: Theme.paddingSmall

                    Label {
                        id: entryDistanceLabel

                        width: locationInfoView.width

                        text: qsTr("%1 %2 from")
                            .arg(Utils.humanDistance(distance))
                            .arg(Utils.humanBearing(bearing))

                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                        visible: !inPlace
                    }
                    Row {
                      POIIcon{
                          id: poiIcon
                          poiType: type
                          width: 64
                          height: 64
                      }
                      Column{
                        Label {
                            id: entryPoi

                            width: locationInfoView.width - poiIcon.width - (2*Theme.paddingSmall)

                            text: poi // label
                            font.pixelSize: Theme.fontSizeLarge
                            visible: poi != ""
                        }
                        Label {
                            id: entryAddress

                            width: locationInfoView.width - poiIcon.width - (2*Theme.paddingSmall)

                            text: address
                            font.pixelSize: Theme.fontSizeLarge
                            visible: address != ""
                        }
                        Label {
                            id: entryRegion

                            width: locationInfoView.width - poiIcon.width - (2*Theme.paddingSmall)
                            wrapMode: Text.WordWrap

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
                                    return str;
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
                                text: {
                                    // simple url normalisation for view
                                    var str=website.toString();

                                    // remove standard protocols
                                    if (str.indexOf("http://")===0){
                                        str=str.substring(7);
                                    }else if (str.indexOf("https://")===0){
                                        str=str.substring(8);
                                    }

                                    // remove last slash
                                    if (str.lastIndexOf("/")===(str.length-1)){
                                        str=str.substring(0,str.length-1);
                                    }
                                    return str;
                                }

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
                }
                footer: Rectangle {
                    color: "transparent"
                    width: locationInfoView.width
                    height: 2*Theme.paddingLarge
                }
            }

            Row{
                id : placeTools
                width: osmNoteBtn.width+searchBtn.width+routeBtn.width+objectsBtn.width+Theme.paddingLarge
                height: objectsBtn.height
                anchors{
                    bottom: parent.bottom
                    right: parent.right
                }

                IconButton{
                    id: osmNoteBtn

                    icon.source: "image://theme/icon-m-edit"
                    onClicked: Qt.openUrlExternally("http://www.openstreetmap.org/note/new#map="+map.view.magLevel+"/"+placeLat.toString()+"/"+placeLon.toString()+"")
                }

                IconButton{
                    id: searchBtn

                    icon.source: "image://theme/icon-m-search"
                    onClicked: {

                        var searchPage=pageStack.push(Qt.resolvedUrl("Search.qml"),
                                      {
                                        searchCenterLat: placeLat,
                                        searchCenterLon: placeLon,
                                        acceptDestination: mapPage
                                      });
                        searchPage.selectLocation.connect(mapPage.selectLocation);
                    }
                }

                IconButton{
                    id: routeBtn

                    icon.source: "image://theme/icon-m-shortcut"
                    onClicked: {

                        pageStack.push(Qt.resolvedUrl("Routing.qml"),
                                       {
                                           toLat: placeLat,
                                           toLon: placeLon,
                                           mapPage: mapPage,
                                           mainMap: mainMap
                                       })
                    }
                }

                IconButton{
                    id: objectsBtn

                    icon.source: "image://theme/icon-m-question"
                    onClicked: {

                        pageStack.push(Qt.resolvedUrl("MapObjects.qml"),
                                       {
                                           view: map.view,
                                           screenPosition: map.screenPosition(placeLat, placeLon),
                                           mapWidth: map.width,
                                           mapHeight: map.height
                                       })
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
              console.log("tap: " + screenX + "x" + screenY + " @ " + lat + " " + lon);
              changePosition(lat, lon, true);
          }
          onLongTap: {
              console.log("long tap: " + screenX + "x" + screenY + " @ " + lat + " " + lon);
              changePosition(lat, lon, false)
          }
        }
    }
}
