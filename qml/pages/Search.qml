/*
 OSM Scout for Sailfish OS
 Copyright (C) 2017  Lukas Karas

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

/*
 * Inspired by SearchPage.qml in ComponentGallery - showcase of Silica components
 */
Page {
    id: searchPage
    property string searchString
    property bool keepSearchFieldFocus
    signal selectLocation(LocationEntry location)
    property var acceptDestination;
    property double searchCenterLat
    property double searchCenterLon

    property string postponedSearchString

    onSelectLocation: {
        console.log("selectLocation: " + location);
    }

    Timer {
        id: postponeTimer
        interval: 1500
        running: false
        repeat: false
        onTriggered: {
            if (postponedSearchString==searchString){
                console.log("Search postponed short expression: \"" + searchString + "\"");
                searchModel.pattern=searchString;
            }
        }
    }

    onSearchStringChanged: {
        // postpone search of short expressions
        if (searchString.length>3){
            console.log("Search: \"" + searchString + "\"");
            searchModel.pattern=searchString;
        }else{
            postponedSearchString=searchString;
            console.log("Postpone search of short expression: \"" + searchString + "\"");
            if (postponeTimer.running){
                postponeTimer.restart();
            }else{
                postponeTimer.start();
            }
        }
    }

    Column {
        id: headerContainer

        width: searchPage.width

        SearchField {
            id: searchField
            width: parent.width

            Binding {
                target: searchPage
                property: "searchString"
                value: searchField.text.trim()
            }
            Component.onCompleted: {
                searchField.forceActiveFocus()
            }
            EnterKey.onClicked: {
                var selectedLocation = searchModel.get(0)
                if (selectedLocation !== null) {
                    selectLocation(selectedLocation);
                    pageStack.pop(acceptDestination); // accept without preview
                }
            }
        }
    }

    property var highlighRegexp: new RegExp("("+searchString.replace(' ','|')+")", 'i')

    Component {
        id: searchItem

        BackgroundItem {
            id: backgroundItem
            height: Math.max(entryIcon.height,entryDescription.height)

            ListView.onAdd: AddAnimation {
                target: backgroundItem
            }
            ListView.onRemove: RemoveAnimation {
                target: backgroundItem
            }

            POIIcon{
                id: entryIcon
                poiType: type
                width: 64
                height: 64
                anchors{
                    right: entryDescription.left
                }
            }
            Column{
                id: entryDescription
                x: searchField.textLeftMargin

                Label {
                    id: labelLabel
                    width: parent.width
                    color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    textFormat: Text.StyledText
                    text: (type=="coordinate") ?
                              Utils.formatCoord(lat, lon, AppSettings.gpsFormat) :
                              (searchString=="" ? label : Theme.highlightText(label, highlighRegexp, Theme.highlightColor))
                }
                Label {
                    id: entryRegion

                    width: searchPage.width - searchField.textLeftMargin - (2*Theme.paddingSmall)
                    wrapMode: Text.WordWrap

                    text: {
                        var str = "";
                        if (region.length > 0){
                            var start = 0;
                            while (start < region.length && region[start] == label){
                                start++;
                            }
                            if (start < region.length){
                                str = region[start];
                                for (var i=start+1; i<region.length; i++){
                                    str += ", "+ region[i];
                                }
                            }else{
                                str = region[0];
                            }
                        }
                        return str;
                    }
                    color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeMedium
                    //visible: region.length > 0
                    height: region.length > 0 ? contentHeight : 1
                }
                Label{
                    id: distanceLabel
                    width: parent.width
                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    text: Utils.humanDistance(distance) + " " + Utils.humanBearing(bearing)
                }
            }
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    var selectedLocation = searchModel.get(index)

                    // the else case should never happen
                    if (selectedLocation !== null) {
                        previewDialog.selectedLocation = selectedLocation;
                        previewDialog.acceptDestination = acceptDestination;

                        previewDialog.open();
                    }
                }
            }
        }
    }

    SilicaListView {
        id: suggestionView
        model: searchModel
        anchors.fill: parent
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium

        currentIndex: -1 // otherwise currentItem will steal focus

        header:  Item {
            id: header
            width: headerContainer.width
            height: headerContainer.height
            Component.onCompleted: headerContainer.parent = header
        }

        delegate: searchItem

        VerticalScrollDecorator {}

        BusyIndicator {
            id: busyIndicator
            running: searchModel.searching
            size: BusyIndicatorSize.Large
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
        Component.onCompleted: {
            if (keepSearchFieldFocus) {
                searchField.forceActiveFocus()
            }
            keepSearchFieldFocus = false
        }
    }

    LocationListModel {
        id: searchModel
        lat: searchCenterLat
        lon: searchCenterLon

        // compute rank for location, it should be in range 0~1
        function locationRank(loc){

            if (loc.type=="coordinate"){
                return 1;
            } else if (loc.type=="object"){
                var rank=1;

                if (loc.objectType=="boundary_country"){
                    rank*=1;
                }else if (loc.objectType=="boundary_state"){
                    rank*=0.93;
                } else if (loc.objectType=="boundary_administrative" ||
                           loc.objectType=="place_town"){
                    rank*=0.9;
                } else if (loc.objectType=="highway_residential" ||
                           loc.objectType=="address"){
                    rank*=0.8;
                } else if (loc.objectType=="railway_station" ||
                           loc.objectType=="railway_tram_stop" ||
                           loc.objectType=="railway_subway_entrance" ||
                           loc.objectType=="highway_bus_stop"
                          ){
                    rank*=0.7;
                }else{
                    rank*=0.5;
                }
                var distance=loc.distanceTo(searchCenterLat, searchCenterLon);
                rank*= 1 / Math.log( (distance/1000) + Math.E);

                //console.log("rank " + loc.label + ": " + rank + "");
                return rank;
            }

            return 0;
        }

        compare: function(a, b){
            // console.log("compare " + a.label + " ("+locationRank(a)+") <> " + b.label + " ("+locationRank(b)+")");
            return locationRank(b) - locationRank(a);
        }

        equals: function(a, b){
            if (a.objectType == b.objectType &&
                a.distanceTo(b.lat, b.lon) < 300 &&
                a.distanceTo(searchCenterLat, searchCenterLon) > 3000
                ){
                // console.log("equals " + a.label + " <> " + b.label + ", " + a.objectType + " <> " + b.objectType + " distance: " + a.distanceTo(b.lat, b.lon));
                return true;
            }
            return false;
        }
    }

    Dialog{
        id: previewDialog

        property var selectedLocation;

        onSelectedLocationChanged: {
            previewMap.showLocation(selectedLocation);
            previewMap.addPositionMark(0, selectedLocation.lat, selectedLocation.lon);
            previewMap.removeAllOverlayObjects();

            mapObjectInfo.setLocationEntry(selectedLocation);

            header.title = selectedLocation.label;
        }

        acceptDestinationAction: PageStackAction.Pop

        onAccepted: {
            // the else case should never happen
            if (selectedLocation !== null) {
                selectLocation(selectedLocation);
            }
        }

        MapObjectInfoModel{
            id: mapObjectInfo
            onReadyChanged: {
                console.log("ready changed: " + ready +  " rows: "+rowCount());
                if (ready){
                    var cnt=rowCount();
                    for (var row=0; row<cnt; row++){
                        var obj=mapObjectInfo.createOverlayObject(row);
                        obj.type="_highlighted";
                        //console.log("object "+row+": "+obj);
                        previewMap.addOverlayObject(row, obj);
                    }
                }
            }
        }

        DialogHeader {
            id: header
            //title: qsTr("Search result")
            //acceptText : qsTr("Accept")
            //cancelText : qsTr("Cancel")
        }

        MapComponent{
            id: previewMap
            anchors{
                top: header.bottom
                right: parent.right
                left: parent.left
                bottom: parent.bottom
            }
            showCurrentPosition: true
        }
    }
}
