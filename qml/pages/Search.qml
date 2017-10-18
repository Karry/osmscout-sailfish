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

    AppSettings{
        id:appSettings
    }

    Timer {
        id: postponeTimer
        interval: 1500
        running: false
        repeat: false
        onTriggered: {
            if (postponedSearchString==searchString){
                console.log("Search postponed short expression: \"" + searchString + "\"");
                suggestionModel.pattern=searchString;
            }
        }
    }

    onSearchStringChanged: {
        // postpone search of short expressions
        if (searchString.length>3){
            console.log("Search: \"" + searchString + "\"");
            suggestionModel.pattern=searchString;
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
                var selectedLocation = suggestionModel.get(0)
                if (selectedLocation !== null) {
                    selectLocation(selectedLocation);
                    pageStack.pop(acceptDestination); // accept without preview
                }
            }
        }
    }


    SilicaListView {
        id: suggestionView
        model: suggestionModel
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

        delegate: BackgroundItem {
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
                              Utils.formatCoord(lat, lon, appSettings.gpsFormat) :
                              Theme.highlightText(label, searchString, Theme.highlightColor)
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
                    visible: region.length > 0
                }
            }
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    var selectedLocation = suggestionModel.get(index)

                    // the else case should never happen
                    if (selectedLocation !== null) {
                        previewMap.showLocation(selectedLocation);
                        previewMap.addPositionMark(0, lat, lon);
                        previewDialog.selectedLocation = selectedLocation;
                        previewDialog.acceptDestination = acceptDestination;

                        previewDialog.open();
                    }
                }
            }
        }

        VerticalScrollDecorator {}

        BusyIndicator {
            id: busyIndicator
            running: suggestionModel.searching
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
        id: suggestionModel
        lat: searchCenterLat
        lon: searchCenterLon
    }

    Dialog{
        id: previewDialog

        property var selectedLocation;

        acceptDestinationAction: PageStackAction.Pop

        onAccepted: {
            // the else case should never happen
            if (selectedLocation !== null) {
                selectLocation(selectedLocation);
            }
        }

        DialogHeader {
            id: header
            //title: "Search result"
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
        }
    }
}
