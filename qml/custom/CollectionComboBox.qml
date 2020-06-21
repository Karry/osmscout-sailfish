/*
 OSM Scout for Sailfish OS
 Copyright (C) 2020  Lukas Karas

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

import "."
import "../custom/Utils.js" as Utils

ComboBox {
    id: collectionComboBox

    property bool initialised: false

    signal collectionRequest

    label: qsTr("Collection")

    CollectionListModel {
        id: collectionListModel
        ordering: AppSettings.collectionsOrdering
        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading);
            if (!loading){
                if (collectionListModel.rowCount()==0){
                    console.log("there is no collection, openning newCollectionDialog...");
                    collectionRequest();
                }else{
                    var rowIndex = collectionListModel.index(collectionComboBox.currentIndex, 0);
                    var collectionId = collectionListModel.data(rowIndex, CollectionListModel.IdRole);
                    var collectionName = collectionListModel.data(rowIndex, CollectionListModel.NameRole);

                    for (var row=0; row < collectionListModel.rowCount(); row++){
                        rowIndex = collectionListModel.index(row, 0);
                        var id = collectionListModel.data(rowIndex, CollectionListModel.IdRole);
                        console.log("test row " + row + " id " + id + " == " + AppSettings.lastCollection);
                        if (id == AppSettings.lastCollection){
                            collectionId = id;
                            collectionName = collectionListModel.data(rowIndex, CollectionListModel.NameRole);
                            collectionComboBox.currentIndex = row;
                            break;
                        }
                    }

                    console.log("Selected collection " + collectionId + ": " + collectionName);
                    waypointDialog.collectionId = collectionId;
                    collectionComboBox.value = collectionName;
                    collectionComboBox.initialised = true;
                }
            }
        }
    }
    menu: ContextMenu {
        Repeater {
            model: collectionListModel
            MenuItem {
                text: name
            }
        }
    }
    onCurrentItemChanged: {
        if (!initialised){
            return;
        }
        var rowIndex = collectionListModel.index(currentIndex, 0);
        var collectionId = collectionListModel.data(rowIndex, CollectionListModel.IdRole);
        var collectionName = collectionListModel.data(rowIndex, CollectionListModel.NameRole);
        console.log("Selected collection: " + collectionName + " (" + collectionId + ")");
        waypointDialog.collectionId = collectionId;
        collectionComboBox.value = collectionName;
    }

    BusyIndicator {
        id: busyIndicator
        running: collectionListModel.loading
        size: BusyIndicatorSize.Small
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
}
