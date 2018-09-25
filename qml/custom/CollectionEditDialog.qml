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

Dialog{
    id: editCollectionDialog

    property alias name: nameTextField.text
    property alias description: descriptionTextArea.text
    property alias title: newCollectionheader.title

    canAccept: nameTextField.text.length > 0

    DialogHeader {
        id: newCollectionheader
        // title and accept/cancel texts should be setup by "user" of this component,
        // this dialog is used for everything - collections, tracks and waypoints
    }

    onStatusChanged: {
        console.log("dialog status: "+ editCollectionDialog.status);
        if (editCollectionDialog.status == DialogStatus.Opening) {
            nameTextField.focus = true;
        }
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
