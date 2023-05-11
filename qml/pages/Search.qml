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
 * Page can have four states:
 *
 * - empty - suggestionView model is setup to poiTypesModel;
 *           list of POI types is displayed.
 * - poi - suggestionView model is setup to poiModel;
 *         list of near POIs of specific types is displayed
 * - search - suggestionView model is setup to searchModel;
 *            list of search result is displayed (based on free text lookup and address lookup)
 * - history - suggestionView model is setup to historyModel;
 * - waypoint - suggestionView model is setup to nearWaypointModel;
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
    property bool enableContextMenu: true
    property alias searchFieldText: searchField.text

    property string postponedSearchString

    states: [
        State { name: "empty" },
        State { name: "poi" },
        State { name: "search" },
        State { name: "history" },
        State { name: "waypoint" }
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
            if (postponedSearchString==searchString && state!="poi" && state!="history"){
                console.log("Search postponed short expression: \"" + searchString + "\"");
                searchModel.pattern=searchString;
            }
        }
    }

    Timer {
        // workaround for the behavior when search field lost focus when we change model
        id: searchFieldFocusTimer
        interval: 10
        running: false
        repeat: false
        onTriggered: {
            searchField.forceActiveFocus();
        }
    }

    Settings {
        id: settings
    }

    onSearchStringChanged: {
        if (searchString.length == 0){
            highlighRegexp = new RegExp("", 'i')
        }else{
            highlighRegexp = new RegExp("("+searchString.replace(' ','|')+")", 'i')
        }

        if (searchString.length==0){
            state = "empty";
            suggestionView.model = poiTypesModel;
            suggestionView.delegate = poiItem;
        } else if (searchString.length > 4 && searchString.substring(0,4) == "poi:") {
            if (state != "poi"){
                state = "poi";
                searchModel.pattern = "";
                suggestionView.model = poiModel;
                suggestionView.delegate = searchItem;
            }
        } else if (searchString.length == 8 && searchString.substring(0,8) == ":history") {
            if (state != "history"){
                state = "history";
                searchModel.pattern = "";
                suggestionView.model = historyModel;
                suggestionView.delegate = historyItem;
            }
        } else if (searchString.length >= 9 && searchString.substring(0,9) == ":waypoint") {
            if (state != "waypoint"){
                state = "waypoint";
                searchModel.pattern = "";
                suggestionView.model = nearWaypointModel;
                suggestionView.delegate = waypointItem;
                nearWaypointModel.lat = searchCenterLat;
                nearWaypointModel.lon = searchCenterLon;
            }
        } else {
            if (state != "search"){
                state="search";
                suggestionView.model = searchModel;
                suggestionView.delegate = searchItem;
            }
        }

        if (searchPage.state === "poi"){
            console.log("Search "+ state + " expression: " + searchString);
            var parts=searchString.split(":");
            if (parts.length>=3){
                poiModel.maxDistance = parts[1] / 1;
                poiModel.types = parts[2].split(" ");
            }else{
                poiModel.maxDistance = 1000;
                poiModel.types = parts[1].split(" ");
            }
        } else if (searchPage.state === "waypoint") {
            console.log("Search "+ state + " expression: " + searchString);
            var parts=searchString.split(":"); // ":waypoint:1000" => [ '', 'waypoint', '1000' ]
            if (parts.length>=3){
                nearWaypointModel.maxDistance = parts[2] / 1;
            } else{
                nearWaypointModel.maxDistance = 1000;
            }
        } else if (searchPage.state === "search") {
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
        suggestionView.currentIndex = -1; // otherwise currentItem will steal focus
        if (searchString.length == 0){
            searchFieldFocusTimer.start(); // search field cleared, force focus to search field
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
                    historyModel.savePattern();
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
        id: waypointItem
        BackgroundItem {
            id: backgroundItem
            height: Math.max(entryIcon.height,entryDescription.height) + contextMenu.height
            highlighted: mouseArea.pressed

            Component.onCompleted: {
                console.log("icon height: " + entryIcon.height + " entryDescription height: " + entryDescription.height);
            }

            Image{
                id: entryIcon
                source: "image://harbour-osmscout/poi-icons/marker.svg"
                width: Theme.iconSizeMedium
                height: width

                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter

                sourceSize.width: width
                sourceSize.height: height

                anchors{
                    right: entryDescription.left
                }
            }
            Item { // why Column is not working here?
                id: entryDescription
                x: searchField.textLeftMargin
                //y: (entryIcon.height-(labelLabel.height+distanceLabel.height))/2
                height: labelLabel.height + distanceLabel.height

                Component.onCompleted: {
                    console.log("labelLabel height: " + labelLabel.height + " distanceLabel height: " + distanceLabel.height);
                }

                Label {
                    id: labelLabel
                    width: parent.width
                    color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    textFormat: Text.StyledText
                    text: model.name
                    height: contentHeight
                }
                Label {
                    id: distanceLabel
                    width: parent.width
                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    text: Utils.humanDistance(distance) + " " + Utils.humanBearing(bearing)
                    height: contentHeight
                    anchors.top: labelLabel.bottom
                }
            }
            MouseArea {
                id: mouseArea
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
                onPressAndHold: {
                    contextMenu.open(backgroundItem);
                }
            }
            ContextMenu {
                id: contextMenu
                MenuItem {
                    text: qsTr("Route to")
                    visible: enableContextMenu
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("Routing.qml"),
                                       {
                                           toLat: model.lat,
                                           toLon: model.lon,
                                           toName: labelLabel.text
                                       })
                    }
                }
            }
        }
    }

    Component {
        id: historyItem

        BackgroundItem {
            id: backgroundItem
            height: Math.max(entryIcon.height,entryDescription.height) + contextMenu.height
            highlighted: mouseArea.pressed

            ListView.onAdd: AddAnimation {
                target: backgroundItem
            }
            ListView.onRemove: RemoveAnimation {
                target: backgroundItem
            }

            Image{
                id: entryIcon
                source: "image://theme/icon-m-backup"
                width: Theme.iconSizeMedium
                height: width

                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter

                sourceSize.width: width
                sourceSize.height: height

                anchors{
                    right: entryDescription.left
                }
            }
            Column{
                id: entryDescription
                x: searchField.textLeftMargin
                y: (entryIcon.height-labelLabel.height)/2

                Label {
                    id: labelLabel
                    width: parent.width
                    color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    textFormat: Text.StyledText
                    text: pattern
                }
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: {
                    searchField.text = pattern;
                    console.log("search expression: " + searchField.text);
                }
                onPressAndHold: {
                    contextMenu.open(backgroundItem);
                }
            }
            ContextMenu {
                id: contextMenu
                MenuItem {
                    //: context menu for removing phrase from search history
                    text: qsTr("Remove")
                    onClicked: {
                        Remorse.itemAction(backgroundItem,
                                           //: label for remorse timer when removing item from search history
                                           qsTr("Removing"),
                                           function() { historyModel.removePattern(pattern); });
                    }
                }
            }
        }

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
                Component.onCompleted: {
                    if (iconType === ":history"){
                        entryIcon.source = "image://theme/icon-m-backup";
                    } else if (iconType === ":waypoint"){
                        entryIcon.source = "image://theme/icon-m-favorite-selected";
                    }
                }
            }
            Column{
                id: entryDescription
                x: searchField.textLeftMargin
                y: descriptionLabel.visible ? 0 : (entryIcon.height-labelLabel.height)/2

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
                    visible: distance > 0

                    text: qsTr("Up to distance %1").arg(Utils.humanDistance(distance))

                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeSmall
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (types == ":history") {
                        searchField.text = ":history";
                    } else if (types == ":waypoint") {
                        searchField.text = ":waypoint:" + distance;
                    } else {
                        searchField.text = "poi:" + distance + ":" + types;
                    }
                    console.log("search expression: " + searchField.text);
                }
            }
        }
    }

    Component {
        id: searchItem

        BackgroundItem {
            id: backgroundItem
            height: Math.max(entryIcon.height,entryDescription.height) + contextMenu.height
            highlighted: mouseArea.pressed

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

                    property string localizedLabel: settings.showAltLanguage && model.altLangName !== "" ?
                                                        (model.altLangName + (model.label!=="" ? " (" + model.label +")" : "")) :
                                                        model.label

                    text: (type==="coordinate") ?
                              Utils.formatCoord(lat, lon, AppSettings.gpsFormat) :
                              (localizedLabel === "" ? qsTr("Unnamed") :(searchString=="" ? localizedLabel : Theme.highlightText(localizedLabel, highlighRegexp, Theme.highlightColor)))
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
                    visible: !isNaN(distance)
                    text: Utils.humanDistance(distance) + " " + Utils.humanBearing(bearing)
                }
            }
            MouseArea{
                id: mouseArea
                anchors.fill: parent
                onClicked: {
                    var selectedLocation = suggestionView.model.get(index)

                    // the else case should never happen
                    if (selectedLocation !== null) {
                        previewDialog.selectedLocation = selectedLocation;
                        previewDialog.acceptDestination = acceptDestination;

                        previewDialog.open();
                        historyModel.savePattern();
                    }
                }
                onPressAndHold: {
                    if (enableContextMenu){
                        contextMenu.open(backgroundItem);
                    }
                }
            }
            ContextMenu {
                id: contextMenu
                MenuItem {
                    text: qsTr("Route to")
                    visible: enableContextMenu
                    onClicked: {
                        var selectedLocation = suggestionView.model.get(index)
                        historyModel.savePattern();
                        pageStack.push(Qt.resolvedUrl("Routing.qml"),
                                       {
                                           toLat: selectedLocation.lat,
                                           toLon: selectedLocation.lon,
                                           toName: labelLabel.text
                                       })
                    }
                }
                MenuItem {
                    //: "Add to collection" alternatively
                    text: qsTr("Add as waypoint")
                    visible: enableContextMenu
                    onClicked: {
                        var selectedLocation = suggestionView.model.get(index)
                        historyModel.savePattern();
                        pageStack.push(Qt.resolvedUrl("NewWaypoint.qml"),
                                      {
                                        latitude: selectedLocation.lat,
                                        longitude: selectedLocation.lon,
                                        acceptDestination: searchPage,
                                        name: (model.type==="coordinate") ?
                                                  Utils.formatCoord(lat, lon, AppSettings.gpsFormat) :
                                                  (settings.showAltLanguage && model.altLangName !== "" ?
                                                       (model.altLangName + (model.label!=="" ? " (" + model.label +")" : "")) :
                                                       model.label),
                                        description: entryRegion.text
                                      });
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
            running: (searchPage.state == "search" && searchModel.searching) ||
                     (searchPage.state == "poi" && poiModel.searching) ||
                     (searchPage.state == "waypoint" && nearWaypointModel.searching)
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

        // special entry for search history
        ListElement { label: QT_TR_NOOP("Search history");       iconType: ":history";           distance: 0;     types: ":history"; }

        // special entry for near waypoints
        //: search page, entry for list near waypoints from collections
        ListElement { label: QT_TR_NOOP("Waypoint");             iconType: ":waypoint";          distance: 10000; types: ":waypoint"; }

        // amenities
        ListElement { label: QT_TR_NOOP("Restaurant");    iconType: "amenity_restaurant"; distance: 1500; types: "amenity_restaurant amenity_restaurant_building"; }
        ListElement { label: QT_TR_NOOP("Fast Food");     iconType: "amenity_fast_food";  distance: 1500; types: "amenity_fast_food amenity_fast_food_building"; }
        ListElement { label: QT_TR_NOOP("Cafe");          iconType: "amenity_cafe";       distance: 1500; types: "amenity_cafe amenity_cafe_building"; }
        ListElement { label: QT_TR_NOOP("Pub");           iconType: "amenity_pub";        distance: 1500; types: "amenity_pub amenity_pub_building"; }
        ListElement { label: QT_TR_NOOP("Bar");           iconType: "amenity_bar";        distance: 1500; types: "amenity_bar amenity_bar_building"; }
        ListElement { label: QT_TR_NOOP("ATM, Bank");     iconType: "amenity_atm";        distance: 1500; types: "amenity_atm amenity_bank amenity_bank_building"; }
        ListElement { label: QT_TR_NOOP("Drinking water"); iconType: "amenity_drinking_water"; distance: 1500; types: "amenity_drinking_water"; }
        ListElement { label: QT_TR_NOOP("Toilets");       iconType: "amenity_toilets";    distance: 1500; types: "amenity_toilets"; }

        // public transport
        ListElement { label: QT_TR_NOOP("Public transport stop"); iconType: "railway_tram_stop"; distance: 1500;
            types: "railway_station railway_subway_entrance railway_tram_stop highway_bus_stop railway_halt amenity_ferry_terminal"; }

        ListElement { label: QT_TR_NOOP("Fuel");          iconType: "amenity_fuel";       distance: 10000; types: "amenity_fuel amenity_fuel_building"; }
        ListElement { label: QT_TR_NOOP("Charging station"); iconType: "amenity_charging_station"; distance: 10000; types: "amenity_charging_station"; }
        ListElement { label: QT_TR_NOOP("Pharmacy");      iconType: "amenity_pharmacy";   distance: 10000; types: "amenity_pharmacy"; }
        ListElement { label: QT_TR_NOOP("Accomodation");  iconType: "tourism_hotel";      distance: 10000;
            types: "tourism_hotel tourism_hotel_building tourism_hostel tourism_hostel_building tourism_motel tourism_motel_building tourism_alpine_hut tourism_alpine_hut_building"; }
        ListElement { label: QT_TR_NOOP("Camp");          iconType: "tourism_camp_site";  distance: 10000; types: "tourism_camp_site tourism_caravan_site"; }
        ListElement { label: QT_TR_NOOP("Castle, Manor"); iconType: "historic_castle";    distance: 10000;
            types: "historic_castle historic_castle_building historic_manor historic_manor_building historic_ruins historic_ruins_building"; }
        //: start of stream/river, drining water sometimes
        ListElement { label: QT_TR_NOOP("Spring");        iconType: "natural_spring";     distance: 2000; types: "natural_spring"; }

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
                // when location is GPS position, show it first
                return 1;
            } else if (loc.type=="object"){

                // set rank by object type
                var typeRank=0.5;
                if (loc.objectType=="boundary_country"){
                    typeRank=1;
                }else if (loc.objectType=="boundary_state"){
                    typeRank=0.93;
                } else if (loc.objectType=="boundary_administrative" ||
                           loc.objectType=="place_town"){
                    typeRank=0.9;
                } else if (loc.objectType=="highway_residential" ||
                           loc.objectType=="address"){
                    typeRank=0.8;
                } else if (loc.objectType=="railway_station" ||
                           loc.objectType=="railway_tram_stop" ||
                           loc.objectType=="railway_subway_entrance" ||
                           loc.objectType=="highway_bus_stop"
                          ){
                    typeRank=0.7;
                }

                // near objects with higher rank
                var distance=loc.distanceTo(searchCenterLat, searchCenterLon);
                var distanceRank = 1 / Math.log( (distance/1000) + Math.E);

                // better match with higher rank
                var matchRank=0.5;
                if (loc.label==searchModel.pattern){
                    matchRank=1;
                } else if (Utils.startsWith(loc.label, searchModel.pattern)){
                    matchRank=0.75;
                }

                var rank = typeRank * distanceRank * matchRank;
                //console.log("rank of " + loc.label + ": " + typeRank + " * " + distanceRank + " * " + matchRank + " = " + rank + "");
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

    SearchHistoryModel {
        id: historyModel

        function savePattern() {
            if (searchPage.state == "search" && searchPage.searchString.length > 0){
                console.log("Adding to search history: " + searchPage.searchString);
                historyModel.addPattern(searchPage.searchString);
            }
        }
    }

    NearWaypointModel {
        id: nearWaypointModel
    }

    Dialog{
        id: previewDialog

        property var selectedLocation;

        onSelectedLocationChanged: {
            previewMap.showLocation(selectedLocation);
            previewMap.addPositionMark(0, selectedLocation.lat, selectedLocation.lon);
            previewMap.removeAllOverlayObjects();
            //console.log("clear map: "+previewMap);

            previewDialogHeader.title = (selectedLocation.type=="coordinate") ?
                        Utils.formatCoord(selectedLocation.lat, selectedLocation.lon, AppSettings.gpsFormat) :
                        selectedLocation.label;

            mapObjectInfo.setLocationEntry(selectedLocation);
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
                        if (obj===null){
                            console.log("Cannot create overlay object for row " + row + "!");
                            continue;
                        }
                        console.log("object "+row+": "+obj+" map: "+previewMap);
                        obj.type="_highlighted";
                        previewMap.addOverlayObject(row, obj);
                    }
                    if (cnt>0){
                        previewDialogHeader.title = "";
                    }
                }
            }
        }

        DialogHeader {
            id: previewDialogHeader
            //title: qsTr("Search result")
            //acceptText : qsTr("Accept")
            //cancelText : qsTr("Cancel")
        }

        SilicaListView {
            id: locationInfoView

            anchors.top: previewDialogHeader.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: Math.min(Math.max(Theme.iconSizeMedium, contentHeight + 2*Theme.paddingMedium), parent.height/2)

            spacing: Theme.paddingMedium
            model: mapObjectInfo

            VerticalScrollDecorator {}
            clip: true

            BusyIndicator {
                id: locationInfoBusyIndicator
                running: !mapObjectInfo.ready
                size: BusyIndicatorSize.Medium
                anchors.horizontalCenter: locationInfoView.horizontalCenter
                anchors.verticalCenter: locationInfoView.verticalCenter
            }

            delegate: BackgroundItem{
                height: objectDetailRow.height
                highlighted: previewMouseArea.pressed

                MouseArea{
                    id: previewMouseArea
                    anchors.fill: parent
                    onClicked: {
                        console.log("Put position mark to "+model.lat+" "+model.lon+" map: "+previewMap);
                        previewMap.addPositionMark(0,model.lat,model.lon);
                    }
                }

                Row {
                    id: objectDetailRow
                    spacing: Theme.paddingMedium
                    x: Theme.paddingMedium
                    width: parent.width -2*Theme.paddingMedium
                    POIIcon{
                        id: poiIcon
                        poiType: type
                        y: Theme.paddingMedium
                        width: Theme.iconSizeMedium
                        height: Theme.iconSizeMedium
                    }
                    Column{
                        Label {
                            font.pixelSize: Theme.fontSizeExtraLarge
                            wrapMode: Text.Wrap
                            color: Theme.highlightColor

                            property string localizedLabel: settings.showAltLanguage && (typeof model.altLangName != "undefined") ?
                                                                (model.altLangName + (model.name!=="" ? " (" + model.name +")" : "")) :
                                                                model.name

                            text: (previewDialog.selectedLocation.type=="coordinate") ?
                                      Utils.formatCoord(previewDialog.selectedLocation.lat, previewDialog.selectedLocation.lon, AppSettings.gpsFormat) :
                                      (localizedLabel==""? qsTr("Unnamed"):localizedLabel);

                        }
                        Label {
                            id: entryAddress

                            width: locationInfoView.width - poiIcon.width - (2*Theme.paddingMedium)

                            text: addressLocation + (addressLocation!="" && addressNumber!="" ? " ":"") + addressNumber
                            font.pixelSize: Theme.fontSizeLarge
                            visible: addressLocation != "" || addressNumber != ""
                        }
                        Label {
                            id: entryRegion

                            width: locationInfoView.width - poiIcon.width - (2*Theme.paddingMedium)
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
        }
        MapComponent{
            id: previewMap
            showCurrentPosition: true

             anchors{
                 top: locationInfoView.bottom
                 right: parent.right
                 left: parent.left
                 bottom: parent.bottom
             }
        }

    }
}
