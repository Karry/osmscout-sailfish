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

ComboBox {
    PositionSource {
        id: positionSource

        active: true

        property bool valid: true; // TODO: false on device
        property double lat: 50.07;//0.0;
        property double lon: 14.43;//0.0;

        onPositionChanged: {
            positionSource.valid = position.latitudeValid && position.longitudeValid;
            positionSource.lat = position.coordinate.latitude;
            positionSource.lon = position.coordinate.longitude;
        }
    }
    RoutingListModel{
        id: routingModel
    }

    id: selector

    property LocationEntry location: null
    property bool initialized: false
    property bool initWithCurrentLocation: false

    signal selectLocation(LocationEntry loc)

    onSelectLocation: {
        console.log("selectLocation: " + loc);
        location=loc;
        value=location.label;
        pageStack.pop();
    }

    function activated(activeIndex){
        console.log("Activated, index: "+activeIndex);
        if (activeIndex==0){
            value=currentItem.text;
            location=routingModel.locationEntryFromPosition(positionSource.lat, positionSource.lon);
        }
        if (activeIndex==1){
            var searchPage=pageStack.push(Qt.resolvedUrl("../pages/Search.qml"));
            searchPage.selectLocation.connect(selectLocation);
        }
    }

    value: qsTr("Select location...")
    menu: ContextMenu {
        MenuItem { text: qsTr("Current location") }
        MenuItem { text: qsTr("Search") }
    }

    Connections {
        target: selector.menu
        onActivated: {
            activated(index);
        }
    }

    onCurrentItemChanged: {
        console.log("CurrentItemChanged, initialised: "+initialized+", index: "+currentIndex);
    }
    Component.onCompleted: {
        initialized = true;
        currentIndex = -1;
        console.log("onCompleted, initialised: "+initialized+", index: "+currentIndex);
        if (initWithCurrentLocation && positionSource.valid){
            currentIndex=0;
            activated(0);
        }
    }
}
