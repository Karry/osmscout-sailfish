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

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils
import ".." // Global singleton

Page {
    id: voiceSelectorPage

    BusyIndicator {
        id: busyIndicator
        running: availableVoices.loading
        size: BusyIndicatorSize.Large
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }

    AvailableVoicesModel {
        id: availableVoices
    }

    SilicaFlickable{
        anchors.fill: parent
        contentHeight: contentColumn.childrenRect.height

        VerticalScrollDecorator {}
        Column {
            id: contentColumn
            anchors.fill: parent

            PageHeader{
                id: downloadMapHeader
                title: qsTr("Installed voices")
            }

            Label{
                id: aboutText
                textFormat: Text.RichText;
                text: "<style>a:link { color: " + Theme.highlightColor + "; }</style>" +
                      qsTr(
                          "Voice samples were created as part of <a href=\"https://community.kde.org/Marble/VoiceOfMarble\">VoiceOfMarble</a> project. " +
                          "Licensed under terms of <a href=\"https://creativecommons.org/licenses/by-sa/3.0/\">CC BY-SA 3.0</a> license."
                          )
                onLinkActivated: Qt.openUrlExternally(link)
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall

                width: parent.width - 2* Theme.paddingLarge
                x: Theme.paddingLarge
            }

            SilicaListView {
                id: listView

                width: parent.width
                height: contentHeight + Theme.paddingMedium // binding loop, but how to define?
                spacing: Theme.paddingMedium

                model: availableVoices

                delegate: TextSwitch{
                    checked: model.state == AvailableVoicesModel.Downloaded
                    busy: model.state == AvailableVoicesModel.Downloading
                    text: "%1 - %2".arg(qsTranslate("resource", lang))
                                   .arg(qsTranslate("resource", name))

                    description: qsTr("Author: %1").arg(author)

                    property bool initialized: false
                    onCheckedChanged: {
                        if (initialized){
                            console.log("state: " + model.state);
                            var indexObj = availableVoices.index(index, 0);
                            if (model.state == AvailableVoicesModel.Downloaded){
                                console.log("remove: " + index);
                                availableVoices.remove(indexObj);
                            } else {
                                console.log("download: " + index);
                                availableVoices.download(indexObj);
                            }
                        }
                    }
                    Component.onCompleted: {
                        initialized=true;
                    }
                }
            }
        }
    }

    Column{
        visible: availableVoices.fetchError != ""
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: Theme.horizontalPageMargin
        spacing: Theme.paddingLarge
        width: parent.width

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTranslate("message", availableVoices.fetchError)
            font.pixelSize: Theme.fontSizeMedium
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            preferredWidth: Theme.buttonWidthMedium
            //: button visible when fetching of available voices from server fails
            text: qsTr("Refresh")
            onClicked: {
                console.log("Reloading...")
                availableVoices.reload();
            }
        }
    }
}
