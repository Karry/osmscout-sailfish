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

Dialog{
    id: collectionSelectorDialog

    property string collectionId: "-1"
    property alias title: moveDialogheader.title
    property bool acceptOnSelect: true
    property alias acceptText: moveDialogheader.acceptText
    property alias cancelText: moveDialogheader.cancelText

    DialogHeader {
        id: moveDialogheader
    }

    CollectionListModel {
        id: collectionListModel
    }

    SilicaListView {
        id: collectionListView
        anchors{
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            top: moveDialogheader.bottom
        }
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium
        clip: true

        currentIndex: -1 // otherwise currentItem will steal focus

        model: collectionListModel
        delegate: ListItem {
            id: collectionItem

            highlighted: model.id === collectionSelectorDialog.collectionId

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
                console.log("selected collection: " + model.name);
                collectionSelectorDialog.collectionId = model.id;
                if (acceptOnSelect){
                    collectionSelectorDialog.accept();
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

}
