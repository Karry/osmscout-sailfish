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

import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: downloadsPage

    MapDownloadsModel{
        id:mapDownloadsModel
    }
    AvailableMapsModel{
        id: availableMapsModel
    }

    SilicaFlickable{
        anchors.fill: parent
        contentHeight: contentColumn.childrenRect.height

        VerticalScrollDecorator {}

        Column {
            id: contentColumn
            anchors.fill: parent

            Column {
                width: parent.width
                visible: mapDownloadsModel.rowCount() > 0
                SectionHeader{ text: qsTr("Download Progress") }
                ListView {
                    id: downloadsListView
                    x: Theme.paddingMedium
                    width: parent.width - 2*Theme.paddingMedium
                    interactive: false
                    height: contentHeight // binding loop, but how to define?

                    model:mapDownloadsModel

                    delegate: Column{
                        spacing: Theme.paddingSmall

                        Label {
                            text: mapName
                        }
                        Row {
                            Label{
                                text: (model!=null) ? Math.round(model.progressRole * 100)+" %" : "?"
                            }
                            Label{
                                text: progressDescription
                            }
                        }

                    }
                }
            }

            SectionHeader{
                id: downloadMapHeader
                text: qsTr("Download Map")
            }

            AvailableMapsView{
                id:availableMapListView

                //anchors.fill: parent
                x: Theme.paddingMedium
                width: parent.width - 2*Theme.paddingMedium
                height: contentHeight // binding loop, but how to define?
                spacing: Theme.paddingMedium
                interactive: false

                originModel:availableMapsModel

                /*
                Component.onCompleted: {
                    availableMapsModel.loaded.connect(onLoaded)
                }
                function onLoaded() {
                    console.log("loaded rows: "+availableMapsModel.rowCount());
                }
                */

                onClick: {
                    var index=availableMapsModel.index(row, /*column*/ 0 /* parent */);
                    //console.log("clicked to: "+name+" / " + index);
                    if (dir){
                        pageStack.push(Qt.resolvedUrl("MapList.qml"),
                                       {availableMapsModel: availableMapsModel, rootDirectoryIndex: index, rootName: name})
                    }
                }
            }
        }
    }
}
