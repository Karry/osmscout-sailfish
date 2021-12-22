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

import "."
import "../custom/Utils.js" as Utils

Dialog{
    id: waypointDialog

    property string collectionId: AppSettings.lastCollection

    property alias name: nameTextField.text
    property alias description: descriptionTextArea.text
    property alias title: waypointDialogheader.title
    property bool rejectRequested: false
    property bool newCollectionRequested: false
    property bool symbolSelectorVisible: false
    property alias symbol: symbolSelector.symbol

    canAccept: nameTextField.text.length > 0 && collectionId.length > 0

    DialogHeader {
        id: waypointDialogheader
        //title: qsTr("New waypoint")
        //acceptText : qsTr("Show")
        //cancelText : qsTr("Cancel")
    }

    function dialogStatus(status){
        // https://sailfishos.org/develop/docs/silica/qml-sailfishsilica-sailfish-silica-dialog.html/
        if (status == DialogStatus.Closed){
            return "Closed";
        }
        if (status == DialogStatus.Opening){
            return "Opening";
        }
        if (status == DialogStatus.Opened){
            return "Opened";
        }
        if (status == DialogStatus.Closing){
            return "Closing";
        }

        return "Unknown";
    }


    onStatusChanged: {
        console.log("dialog status: "+ dialogStatus(waypointDialog.status));
        if (waypointDialog.status == DialogStatus.Opened) {
            nameTextField.focus = true;
            if (newCollectionRequested){
                newCollectionDialog.open();
                newCollectionRequested = false;
            } else if (rejectRequested){
                waypointDialog.reject();
            } else {
                nameTextField.focus = true;
                nameTextField.forceActiveFocus();
                nameTextField.selectAll();
            }
        }
        if (waypointDialog.status == DialogStatus.Closed){
            // force close keyboard
            nameTextField.focus = false;
            descriptionTextArea.focus = false;
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
        CollectionListModel {
            id: collectionListModel
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

            CollectionComboBox {
                id: collectionComboBox
                width: parent.width

                onCollectionRequest: {
                    newCollectionRequested = true;
                    newCollectionDialog.open();
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

            SymbolSelector {
                id: symbolSelector
                visible: symbolSelectorVisible
            }
        }
    }
}
