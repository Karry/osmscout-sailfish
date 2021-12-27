/*
 OSM Scout for Sailfish OS
 Copyright (C) 2018  Lukas Karas

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

CollectionEntryDialog{
    id: waypointDialog

    symbolSelectorVisible: true

    property double latitude
    property double longitude

    title: qsTr("New waypoint")

    acceptDestinationAction: PageStackAction.Pop

    CollectionModel {
        id: collectionModel
        collectionId: waypointDialog.collectionId
        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading);
        }
    }

    onAccepted: {
        console.log("Adding waypoint " + name + " to collection " + collectionId + " symbol " + symbol);
        AppSettings.lastCollection = collectionId;
        collectionModel.createWaypoint(latitude, longitude, name, description, symbol);
    }
}
