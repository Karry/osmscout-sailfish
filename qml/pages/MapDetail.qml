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
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"

Page {
    property AvailableMapsModel availableMapsModel

    property string mapName
    property variant mapItem
    property var downloadsPage

    MapDownloadsModel{
        id:mapDownloadsModel
    }

    SilicaFlickable{
        anchors.fill: parent
        contentHeight: contentColumn.childrenRect.height

        VerticalScrollDecorator {}
        Column{
            id: contentColumn
            anchors.fill: parent

            spacing: Theme.paddingMedium

            PageHeader{
                title: mapName
            }

            Label{
                id: descriptionText

                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium

                text: mapItem.description
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
            }

            Column{
                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium
                Label{
                    text: qsTr("Size")
                    color: Theme.primaryColor
                }
                Label{
                    text: mapItem.size
                    color: Theme.highlightColor
                }
            }

            Column{
                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium
                Label{
                    text: qsTr("Last Update")
                    color: Theme.primaryColor
                }
                Label{
                    text: Qt.formatDate(mapItem.time)
                    color: Theme.highlightColor
                }
            }

            Column{
                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium
                Label{
                    text: qsTr("Data Version")
                    color: Theme.primaryColor
                }
                Label{
                    text: mapItem.version
                    color: Theme.highlightColor
                }
            }

            SectionHeader{
                id: downloadMapHeader
                text: qsTr("Download")
            }
            ComboBox {
                id: destinationDirectoryComboBox

                property bool initialized: false
                property string selected: ""
                property ListModel directories: ListModel {}

                label: qsTr("Directory")
                menu: ContextMenu {
                    id: contextMenu
                    Repeater {
                        model: destinationDirectoryComboBox.directories
                        MenuItem { text: dir }
                    }
                }
                onCurrentItemChanged: {
                    if (!initialized){
                        return;
                    }
                    var dirs=mapDownloadsModel.getLookupDirectories();
                    selected = dirs[currentIndex];
                    //selected = directories[currentIndex].dir
                }
                Component.onCompleted: {
                    var dirs=mapDownloadsModel.getLookupDirectories();
                    for (var i in dirs){
                        var dir = dirs[i];
                        if (selected==""){
                            selected=dir;
                        }
                        console.log("Dir: "+dir);
                        directories.append({"dir": dir});
                    }
                    initialized = true;
                }
            }
            Button{
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Download")
                onClicked: {
                    var dir=mapDownloadsModel.suggestedDirectory(mapItem.map, destinationDirectoryComboBox.selected);
                    mapDownloadsModel.downloadMap(mapItem.map, dir);
                    console.log("downloading to " + dir);
                    pageStack.pop(downloadsPage);
                }
            }

        }

    }
}
