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

Dialog{
    id: waypointDialog

    property double latitude
    property double longitude
    property string collectionId
    property alias name: nameTextField.text
    property alias description: descriptionTextArea.text
    property bool rejectRequested: false;
    property bool newCollectionRequested: false;

    acceptDestinationAction: PageStackAction.Pop
    canAccept: nameTextField.text.length > 0 && collectionId.length > 0
    onAccepted: {
        console.log("Adding waypoint " + nameTextField.text + " to collection " + collectionId);
        collectionModel.createWaypoint(latitude, longitude, name, description);
    }
    Component.onCompleted: {
        nameTextField.focus = true;
    }

    DialogHeader {
        id: waypointDialogheader
        title: qsTr("New waypoint")
        //acceptText : qsTr("Show")
        //cancelText : qsTr("Cancel")
    }
    onStatusChanged: {
        console.log("dialog status: "+ waypointDialog.status);
        if (waypointDialog.status == DialogStatus.Opened) {
            if (newCollectionRequested){
                newCollectionDialog.open();
                newCollectionRequested = false;
            }
            if (rejectRequested){
                waypointDialog.reject();
            }
        }
    }

    CollectionModel {
        id: collectionModel
        collectionId: waypointDialog.collectionId
        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading);
        }
    }

    CollectionEditDialog {
        id: newCollectionDialog
        title: qsTr("New collection")
        name: qsTr("Default")
        onRejected: {
            rejectRequested = true;
            waypointDialog.reject();
        }
        onAccepted: {
            console.log("Create new collection: " + name + " / " + description);
            collectionListModel.createCollection(name, description);
        }
    }
    SilicaFlickable {
        id: flickable
        anchors{
            top: waypointDialogheader.bottom
            right: parent.right
            bottom: parent.bottom
            left: parent.left
        }

        VerticalScrollDecorator {}

        Column{
            width: parent.width

            ComboBox {
                id: collectionComboBox
                width: parent.width
                label: qsTr("Collection")

                property bool initialised: false

                CollectionListModel {
                    id: collectionListModel
                    onLoadingChanged: {
                        console.log("onLoadingChanged: " + loading);
                        if (!loading){
                            if (collectionListModel.rowCount()==0){
                                console.log("there is no collection, openning newCollectionDialog...");
                                newCollectionRequested = true;
                                newCollectionDialog.open();
                            }else{
                                var rowIndex = collectionListModel.index(collectionComboBox.currentIndex, 0);
                                var collectionId = collectionListModel.data(rowIndex, CollectionListModel.IdRole);
                                var collectionName = collectionListModel.data(rowIndex, CollectionListModel.NameRole);
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

            TextField {
                id: nameTextField
                width: parent.width
                label: qsTr("Name")
                placeholderText: qsTr("Name")
                //horizontalAlignment: textAlignment
                EnterKey.onClicked: {
                    //text = "Return key pressed";
                    descriptionTextArea.focus = true;
                }
            }

            TextArea {
                id: descriptionTextArea
                width: parent.width
                height: 300
                label: qsTr("Description")
                placeholderText: qsTr("Description")
            }
        }
    }
}
