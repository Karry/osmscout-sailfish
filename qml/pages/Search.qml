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

/**
 * Inspired by SearchPage.qml in ComponentGallery - showcase of Silica components
 *
 * The main page components are searchField (SearchField type) with binded string
 * property searchString and suggestionView (SilicaListView type).
 *
 * Page can have three states:
 *
 * - empty - suggestionView model is setup to poiTypesModel;
 *           list of POI types is displayed.
 * - poi - suggestionView model is setup to poiModel;
 *         list of near POIs of specific types is displayed
 * - search - suggestionView model is setup to searchModel;
 *            list of search result is displayed (based on free text lookup and address lookup)
 *
 * State is changed in onSearchStringChanged slot - when searchString is empty,
 * empty state is used; when seach string starts with poi: prefix, given string is parsed
 * and poi state is used. Search state is used otherwise. Poi string is using
 * simple syntax:
 *
 *      poi:<distance in meters>:<spase separated POI types>.
 *
 * Using this, you can search everything that exists in the database, you are not limited
 * to predefined POI types in poiTypesModel. For example string
 *
 *      poi:1000:amenity_post_box amenity_post_office amenity_post_office_building
 *
 * will show you post boxes and post offices up to distance 1km around you.
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

    states: [
        State { name: "empty";  },
        State { name: "poi";    },
        State { name: "search"; }
    ]


    onStateChanged: {
        console.log("Search state changed: "+ state);
    }

    onSelectLocation: {
        console.log("selectLocation: " + location);
    }

    Timer {
        id: postponeTimer
        interval: 1500
        running: false
        repeat: false
        onTriggered: {
            if (postponedSearchString==searchString && state!="poi"){
                console.log("Search postponed short expression: \"" + searchString + "\"");
                searchModel.pattern=searchString;
            }
        }
    }

    onSearchStringChanged: {
        if (searchString.length == 0){
            highlighRegexp = new RegExp("", 'i')
        }else{
            highlighRegexp = new RegExp("("+searchString.replace(' ','|')+")", 'i')
        }

        if (searchString.length==0){
            state="empty";
            suggestionView.model = poiTypesModel;
            suggestionView.delegate = poiItem;
        } else if (searchString.length>3 && searchString.substring(0,4)=="poi:") {
            if (state!="poi"){
                state="poi";
                searchModel.pattern="";
                suggestionView.model=poiModel;
                suggestionView.delegate = searchItem;
            }
        } else {
            if (state!="search"){
                state="search";
                suggestionView.model=searchModel;
                suggestionView.delegate = searchItem;
            }
        }

        if (searchPage.state==="poi"){
            console.log("Search "+ state + " expression: " + searchString);
            var parts=searchString.split(":");
            if (parts.length>=3){
                poiModel.maxDistance = parts[1] / 1;
                poiModel.types = parts[2].split(" ");
            }else{
                poiModel.maxDistance = 1000;
                poiModel.types = parts[1].split(" ");
            }
        } else {
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
                var selectedLocation = suggestionView.model.get(0)
                if (selectedLocation !== null) {
                    selectLocation(selectedLocation);
                    pageStack.pop(acceptDestination); // accept without preview
                }
            }
        }
    }

    property var highlighRegexp: new RegExp("", 'i')

    onHighlighRegexpChanged: {
        console.log("highlight regexp: " + highlighRegexp);
    }

    Component {
        id: poiItem

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
                poiType: iconType
                width: Theme.iconSizeMedium
                height: width
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
                    text: qsTr(label)
                }
                Label {
                    id: descriptionLabel

                    width: searchPage.width - searchField.textLeftMargin - (2*Theme.paddingSmall)
                    wrapMode: Text.WordWrap

                    text: qsTr("Up to distance %1").arg(Utils.humanDistance(distance))

                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeSmall
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    searchField.text = "poi:" + distance + ":" + types;
                    console.log("search expression: " + searchField.text);
                }
            }
        }
    }

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
                width: Theme.iconSizeMedium
                height: width
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
                              (label== "" ? qsTr("Unnamed") :(searchString=="" ? label : Theme.highlightText(label, highlighRegexp, Theme.highlightColor)))
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
                    var selectedLocation = suggestionView.model.get(index)

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

        model: poiTypesModel // searchModel
        delegate: poiItem // searchItem

        VerticalScrollDecorator {}

        BusyIndicator {
            id: busyIndicator
            running: searchPage.state !== "poi" && (searchModel.searching || poiModel.searching)
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

    ListModel {
        id: poiTypesModel

        // amenities
        ListElement { label: QT_TR_NOOP("Restaurant");    iconType: "amenity_restaurant"; distance: 1500; types: "amenity_restaurant amenity_restaurant_building"; }
        ListElement { label: QT_TR_NOOP("Fast Food");     iconType: "amenity_fast_food";  distance: 1500; types: "amenity_fast_food amenity_fast_food_building"; }
        ListElement { label: QT_TR_NOOP("Cafe");          iconType: "amenity_cafe";       distance: 1500; types: "amenity_cafe amenity_cafe_building"; }
        ListElement { label: QT_TR_NOOP("ATM");           iconType: "amenity_atm";        distance: 1500; types: "amenity_atm"; }
        ListElement { label: QT_TR_NOOP("Drinking water"); iconType: "amenity_drinking_water"; distance: 1500; types: "amenity_drinking_water"; }
        ListElement { label: QT_TR_NOOP("Toilets");       iconType: "amenity_toilets";    distance: 1500; types: "amenity_toilets"; }

        // public transport
        ListElement { label: QT_TR_NOOP("Public transport stop"); iconType: "railway_tram_stop"; distance: 1500;
            types: "railway_station railway_subway_entrance railway_tram_stop highway_bus_stop railway_halt amenity_ferry_terminal"; }

        ListElement { label: QT_TR_NOOP("Fuel");          iconType: "amenity_fuel";       distance: 10000; types: "amenity_fuel amenity_fuel_building"; }
        ListElement { label: QT_TR_NOOP("Accomodation");  iconType: "tourism_hotel";      distance: 10000;
            types: "tourism_hotel tourism_hotel_building tourism_hostel tourism_hostel_building tourism_motel tourism_motel_building tourism_alpine_hut tourism_alpine_hut_building"; }
        ListElement { label: QT_TR_NOOP("Camp");          iconType: "tourism_camp_site";  distance: 10000; types: "tourism_camp_site tourism_caravan_site"; }

        // and somethig for fun
        ListElement { label: QT_TR_NOOP("Via ferrata route"); iconType: "natural_peak";   distance: 20000;
            types: "highway_via_ferrata_easy highway_via_ferrata_moderate highway_via_ferrata_difficult highway_via_ferrata_extreme"; }
    }

    NearPOIModel {
        id: poiModel
        lat: searchCenterLat
        lon: searchCenterLon
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

            header.title = (selectedLocation.type=="coordinate") ?
                        Utils.formatCoord(selectedLocation.lat, selectedLocation.lon, AppSettings.gpsFormat) :
                        selectedLocation.label;
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
