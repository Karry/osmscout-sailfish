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
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: collectionListPage

    CollectionListModel {
        id: collectionListModel
        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading);
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

                source: "image://theme/icon-m-favorite"

                width: Theme.iconSizeMedium
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter
                height: width
                x: Theme.paddingMedium
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
                console.log("selected collection: " + name);
            }
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Edit")
                    onClicked: {
                        console.log("Edit " + model.id + " " + model.name + ", " + model.description + "...");
                        editCollectionDialog.collectionId =  model.id;
                        nameTextField.text = model.name;
                        descriptionTextArea.text = model.description;
                        editCollectionDialog.open();
                        nameTextField.focus = true;
                    }
                }
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: {
                        var idx = model.id
                        Remorse.itemAction(collectionItem,
                                           qsTr("Deleting"),
                                           function() { collectionListModel.deleteCollection(idx) });
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
                text: qsTr("Create new")
                onClicked: {
                    console.log("Create new collection...")
                    editCollectionDialog.collectionId = -1;
                    nameTextField.text = "";
                    descriptionTextArea.text = "";
                    editCollectionDialog.open();
                    nameTextField.focus = true;
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

    Dialog{
        id: editCollectionDialog

        property int collectionId: -1

        onAccepted: {
            if (collectionId < 0){
                console.log("Create new collection: " + nameTextField.text + " / " + descriptionTextArea.text);
                collectionListModel.createCollection(nameTextField.text, descriptionTextArea.text);
            }else{
                console.log("Edit collection " + collectionId + ": " + nameTextField.text + " / " + descriptionTextArea.text);
                collectionListModel.editCollection(collectionId, nameTextField.text, descriptionTextArea.text);
            }
            parent.focus = true;
        }
        onRejected: {
            parent.focus = true;
        }

        canAccept: nameTextField.text.length > 0

        DialogHeader {
            id: newCollectionheader
            title: editCollectionDialog.collectionId < 0 ? qsTr("New collection"): qsTr("Edit collection")
            //acceptText : qsTr("Accept")
            //cancelText : qsTr("Cancel")
        }

        Column{
            width: parent.width
            anchors{
                top: newCollectionheader.bottom
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
