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

import QtQuick 2.2
import Sailfish.Silica 1.0
import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: downloadsPage

    RemorsePopup { id: remorse }

    MapDownloadsModel{
        id:mapDownloadsModel
        onMapDownloadFails: {
            remorse.execute(qsTranslate("message", message), function() { }, 10 * 1000);
        }
    }
    AvailableMapsModel{
        id: availableMapsModel
    }

    BusyIndicator {
        id: busyIndicator
        running: availableMapsModel.loading
        size: BusyIndicatorSize.Large
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }

    SilicaFlickable{
        anchors.fill: parent
        contentHeight: contentColumn.childrenRect.height
        VerticalScrollDecorator {}

        Column {
            id: contentColumn
            anchors.fill: parent

            Column {
                id: downloadsColumn
                width: parent.width
                visible: mapDownloadsModel.rowCount() > 0
                SectionHeader{ text: qsTr("Download Progress") }
                ListView {
                    id: downloadsListView
                    //x: Theme.paddingMedium
                    width: parent.width
                    interactive: false
                    height: contentHeight // binding loop, but how to define?

                    model:mapDownloadsModel

                    delegate: ListItem{
                        Row{
                            spacing: Theme.paddingMedium
                            x: Theme.paddingMedium
                            Image{
                                width:  Theme.fontSizeMedium * 2
                                height: Theme.fontSizeMedium * 2
                                source:"image://theme/icon-m-cloud-download"
                                verticalAlignment: Image.AlignVCenter
                            }

                            Column{
                                Label {
                                    text: mapName
                                }
                                Row {
                                    spacing: Theme.paddingMedium

                                    Label{
                                        text: {
                                            if (model==null)
                                                return "?";
                                            var fraction = model.progressRole;
                                            var percent = (Math.round(fraction * 1000)/10);
                                            return (percent-Math.round(percent)==0) ? percent+".0 %": percent+" %";
                                        }
                                    }
                                    Label{
                                        text: progressDescription!="" ? "(" + progressDescription + ")": ""
                                    }
                                }
                            }
                        }
                    }
                }
                Component.onCompleted: {
                    mapDownloadsModel.modelReset.connect(onModelReset);
                }
                function onModelReset() {
                    console.log("mapDownloadsModel rows: "+mapDownloadsModel.rowCount());
                    downloadsColumn.visible = mapDownloadsModel.rowCount() > 0
                }
            }

            SectionHeader{
                id: downloadMapHeader
                text: qsTr("Download Map")
            }

            AvailableMapsView{
                id:availableMapListView

                width: parent.width
                height: contentHeight + Theme.paddingMedium // binding loop, but how to define?
                spacing: Theme.paddingMedium

                originModel:availableMapsModel

                onClick: {
                    var index=availableMapsModel.index(row, /*column*/ 0 /* parent */);
                    //console.log("clicked to: "+item.name+" / " + index);
                    if (item.dir){
                        pageStack.push(Qt.resolvedUrl("MapList.qml"),
                                       {availableMapsModel: availableMapsModel, rootDirectoryIndex: index, rootName: item.name, downloadsPage: downloadsPage})
                    }else{
                        pageStack.push(Qt.resolvedUrl("MapDetail.qml"),
                                       {availableMapsModel: availableMapsModel, mapIndex: index, mapName: item.name, mapItem: item, downloadsPage: downloadsPage})
                    }
                }
            }
        }
    }

    Column{
        visible: availableMapsModel.fetchError != ""
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: Theme.horizontalPageMargin
        spacing: Theme.paddingLarge
        width: parent.width

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTranslate("message", availableMapsModel.fetchError)
            font.pixelSize: Theme.fontSizeMedium
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            preferredWidth: Theme.buttonWidthMedium
            text: qsTr("Refresh")
            onClicked: {
                console.log("Reloading...")
                availableMapsModel.reload();
            }
        }
    }

}
