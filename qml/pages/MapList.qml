/*
 OSM Scout for Sailfish OS
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
    id: mapListPage

    property AvailableMapsModel availableMapsModel
    property var rootDirectoryIndex
    property string rootName
    property var downloadsPage

    SilicaFlickable{
        anchors.fill: parent
        contentHeight: contentColumn.childrenRect.height

        VerticalScrollDecorator {}
        Column {
            id: contentColumn
            anchors.fill: parent

            PageHeader{
                id: downloadMapHeader
                title: rootName
            }

            AvailableMapsView {
                id: availableMapListView

                originModel: availableMapsModel
                rootIndex: rootDirectoryIndex

                width: parent.width
                height: contentHeight + Theme.paddingMedium // binding loop, but how to define?
                spacing: Theme.paddingMedium

                onClick: {
                    var index=availableMapsModel.index(row, /*column*/ 0, /* parent */ rootDirectoryIndex);
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
}
