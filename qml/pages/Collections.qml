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
import Sailfish.Pickers 1.0
import QtPositioning 5.2
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: collectionListPage

    signal selectWaypoint(double lat, double lon)
    signal selectTrack(LocationEntry bbox, var trackId);
    property var acceptDestination;

    RemorsePopup { id: remorse }

    CollectionListModel {
        id: collectionListModel
        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading);
        }
        onError: {
            remorse.execute(message, function() { }, 10 * 1000);
        }
    }

    SilicaListView {
        id: collectionListView
        anchors.fill: parent
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium
        clip: true

        currentIndex: -1 // otherwise currentItem will steal focus

        model: collectionListModel
        delegate: ListItem {
            id: collectionItem

            ListView.onAdd: AddAnimation {
                target: collectionItem
            }
            ListView.onRemove: RemoveAnimation {
                target: collectionItem
            }

            Image{
                id: entryIcon

                source: model.visible ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"

                width: Theme.iconSizeMedium
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter
                height: width
                x: Theme.paddingMedium

                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        console.log("Changing collection (" + model.id + ") visibility to: " + !model.visible);
                        collectionListModel.editCollection(model.id, !model.visible, model.name, model.description);
                    }
                }
            }
            Column{
                id: entryDescription
                x: Theme.paddingMedium
                anchors.left: entryIcon.right
                anchors.right: parent.right
                anchors.verticalCenter: entryIcon.verticalCenter

                Label {
                    id: nameLabel

                    width: parent.width
                    textFormat: Text.StyledText
                    text: name
                }
                Label {
                    id: descriptionLabel

                    visible: description != ""
                    text: description.replace(/\n/g, " ")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                }
            }
            onClicked: {
                console.log("selected collection: " + model.name + " (" + model.id + ")");
                var collectionPage = pageStack.push(Qt.resolvedUrl("Collection.qml"),
                               {
                                    collectionId: model.id,
                                    acceptDestination: acceptDestination
                               })
                collectionPage.selectWaypoint.connect(selectWaypoint);
                collectionPage.selectTrack.connect(selectTrack);
            }
            menu: ContextMenu {
                MenuItem {
                    text: model.visible ? qsTr("Hide on map") : qsTr("Show on map")
                    onClicked: {
                        console.log("Changing collection (" + model.id + ") visibility to: " + !model.visible);
                        collectionListModel.editCollection(model.id, !model.visible, model.name, model.description);
                    }
                }
                MenuItem {
                    text: qsTr("Edit")
                    onClicked: {
                        console.log("Edit " + model.id + " " + model.name + ", " + model.description + "...");
                        editCollectionDialog.collectionId = model.id;
                        editCollectionDialog.name = model.name;
                        editCollectionDialog.description = model.description;
                        editCollectionDialog.collectionVisible = model.visible;
                        editCollectionDialog.open();
                        //editCollectionDialog.nameTextField.focus = true;
                    }
                }
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: {
                        Remorse.itemAction(collectionItem,
                                           qsTr("Deleting"),
                                           function() { collectionListModel.deleteCollection(model.id) });
                    }
                }
            }
        }

        header: PageHeader {
            id: header
            title: qsTr("Collections")
        }

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                text: qsTr("Import")
                onClicked: {
                    pageStack.push(filePickerPage)
                }
            }
            MenuItem {
                text: qsTr("Create new")
                onClicked: {
                    console.log("Create new collection...")
                    editCollectionDialog.collectionId = "";
                    editCollectionDialog.name = "";
                    editCollectionDialog.description = "";
                    editCollectionDialog.collectionVisible = false;
                    editCollectionDialog.open();
                    //editCollectionDialog.nameTextField.focus = true;
                }
            }
        }

        BusyIndicator {
            id: busyIndicator
            running: collectionListModel.loading
            size: BusyIndicatorSize.Large
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Component {
        id: filePickerPage
        FilePickerPage {
            nameFilters: [ '*.gpx' ]
            onSelectedContentPropertiesChanged: {
                console.log("Importing " + selectedContentProperties.filePath);
                collectionListModel.importCollection(selectedContentProperties.filePath);
            }
        }
    }

    CollectionEditDialog {
        id: editCollectionDialog

        property string collectionId: ""
        property bool collectionVisible: false
        title: collectionId.length == 0 ? qsTr("New collection"): qsTr("Edit collection")

        onAccepted: {
            if (collectionId.length == 0){
                console.log("Create new collection: " + name + " / " + description);
                collectionListModel.createCollection(name, description);
            }else{
                console.log("Edit collection " + collectionId + ": " + name + " / " + description);
                collectionListModel.editCollection(collectionId, collectionVisible, name, description);
            }
            parent.focus = true;
        }
        onRejected: {
            parent.focus = true;
        }
    }
}
