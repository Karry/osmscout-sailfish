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

SilicaListView {
    id: listView

    property AvailableMapsModel originModel
    property var rootIndex

    signal click(int row, bool dir, string name)
    
    model: DelegateModel {
        id: visualModel
        model: originModel
        rootIndex : listView.rootIndex
        delegate:  Item{
            width: listView.width
            height: entryIcon.height

            Row{
                spacing: Theme.paddingMedium

                Image {
                    id: entryIcon

                    //anchors.horizontalCenter: parent.horizontalCenter
                    //anchors.verticalCenter: parent.verticalCenter
                    width:  Theme.fontSizeMedium * 2
                    height: Theme.fontSizeMedium * 2

                    source: dir? "image://theme/icon-m-folder" : "image://theme/icon-m-dot"
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    sourceSize.width: width
                    sourceSize.height: height

                    //Component.onCompleted: {
                    //    console.log("height: "+height+", "+nameLabel.height)
                    //}
                }

                Label{
                    id: nameLabel
                    height: entryIcon.height
                    font.pixelSize: Theme.fontSizeMedium
                    verticalAlignment: Text.AlignVCenter
                    text: name
                }
            }

            Label{
                id: sizeLabel
                visible: !dir
                text: size
                anchors.top: parent.top
                anchors.right: parent.right
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
            Label{
                id: dateLabel
                visible: !dir
                text: Qt.formatDate(time)
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }

            MouseArea{
                anchors.fill: parent
                onClicked: {
                    //console.log("index: " + index );
                    listView.click(index, dir, name)
                }
            }
        }
    }
}
