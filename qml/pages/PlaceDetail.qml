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
import QtPositioning 5.2
import QtQml.Models 2.2

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils
import ".." // Global singleton

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

    Settings {
        id: settings
    }

    Connections {
        target: Global.positionSource
        onUpdate: {
            currentLocValid = positionValid;
            currentLocLat = lat;
            currentLocLon = lon;
        }
    }
    Component.onCompleted: {
        Global.positionSource.updateRequest();
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
                    onPressAndHold: {
                        // improve default ComboBox UX :-)
                        clicked(mouse);
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
                    visible: currentLocValid
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
                          width: Theme.iconSizeMedium
                          height: Theme.iconSizeMedium
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
                        PhoneRow {
                            id: phoneRow
                            phone: model.phone
                        }
                        WebsiteRow {
                            id: websiteRow
                            website: model.website
                        }


                    }
                    }
                }
                footer: Rectangle {
                    color: "transparent"
                    width: locationInfoView.width
                    // Theme.itemSizeSmall is size of IconButton
                    height:  Theme.itemSizeSmall+Theme.paddingLarge
                }
            }

            Row{
                id : placeTools
                width: shareBtn.width+waypointBtn.width+osmNoteBtn.width+searchBtn.width+routeBtn.width+objectsBtn.width+Theme.paddingLarge
                height: objectsBtn.height
                anchors{
                    bottom: parent.bottom
                    right: parent.right
                }

                function placeInfo(){
                    var address = "";
                    var name = "";
                    if (locationInfoModel.ready){
                        delegateModel.model = locationInfoModel;
                        // if there is poi, use it as waypoint name
                        // if poi has address, use it as waypoint description
                        for (var row = 0; row < locationInfoModel.rowCount(); row++) {
                            var item = delegateModel.items.get(row).model;
                            if (item.poi != ""){
                                name = item.poi;
                                if (item.address != ""){
                                    address = item.address;
                                }
                                break;
                            }
                            if (item.address != ""){
                                address = item.address;
                                break;
                            }
                        }
                    }
                    return {"address": address, "name": name};
                }

                IconButton{
                    id: shareBtn
                    icon.source: "image://theme/icon-m-share"

                    function shortCoord(deg){
                        return (Math.round(deg * 100000)/100000).toString();
                    }

                    onClicked: {
                        var fileName = "place.txt";
                        var mimeType = "text/x-url";
                        var placeLink = "https://osm.org/?mlat=" + shortCoord(placeLat) + "&mlon=" + shortCoord(placeLon);
                        var info = placeTools.placeInfo();
                        var name = info.name;
                        var address = info.address;
                        var status = (address === "" ?  placeLink : address + ": " + placeLink);
                        var linkTitle = (name === "" ?  placeLocationLabel.text : name);
                        var content = {
                            "name": fileName,
                            "data": placeLink,
                            "type": mimeType
                        }

                        // also some non-standard fields for Twitter/Facebook status sharing:
                        content["status"] = status;
                        content["linkTitle"] = linkTitle;

                        pageStack.animatorPush("Sailfish.TransferEngine.SharePage",
                                               {
                                                   //: Page header for share method selection
                                                   "header": qsTr("Share place link"),
                                                   "serviceFilter": ["sharing", "e-mail", "IM"],
                                                   "mimeType": mimeType,
                                                   "content": content
                                               })
                    }
                }

                IconButton{
                    id: waypointBtn

                    DelegateModel {
                        id: delegateModel
                    }
                    icon.source: "image://theme/icon-m-favorite"
                    onClicked: {
                        var info = placeTools.placeInfo();
                        var name = info.name;
                        var address = info.address;
                        console.log("add waypoint \"" + name + "\" on address: " + address);

                        pageStack.push(Qt.resolvedUrl("NewWaypoint.qml"),
                                      {
                                        latitude: placeLat,
                                        longitude: placeLon,
                                        acceptDestination: Global.mapPage,
                                        description: address,
                                        name: name
                                      });
                    }
                }

                IconButton{
                    id: osmNoteBtn

                    icon.source: "image://theme/icon-m-edit"
                    onClicked: Qt.openUrlExternally("https://www.openstreetmap.org/note/new#map="+map.view.magLevel+"/"+placeLat.toString()+"/"+placeLon.toString()+"")
                }

                IconButton{
                    id: searchBtn

                    icon.source: "image://theme/icon-m-search"
                    onClicked: {

                        var searchPage=pageStack.push(Qt.resolvedUrl("Search.qml"),
                                      {
                                        searchCenterLat: placeLat,
                                        searchCenterLon: placeLon,
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
                                           toLat: placeLat,
                                           toLon: placeLon
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
