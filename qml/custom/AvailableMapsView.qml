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

    signal click(int row, variant item)
    
    model: DelegateModel {
        id: visualModel
        model: originModel
        rootIndex : listView.rootIndex
        delegate:  ListItem{
            property variant myData: model

            width: listView.width
            height: entryIcon.height

            Row{
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.paddingMedium
                x: Theme.paddingMedium

                Image {
                    id: entryIcon

                    width:  Theme.fontSizeMedium * 2
                    height: Theme.fontSizeMedium * 2

                    source: dir? "image://theme/icon-m-folder" : "image://theme/icon-m-dot"
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    sourceSize.width: width
                    sourceSize.height: height
                }

                Label{
                    id: nameLabel
                    height: entryIcon.height
                    font.pixelSize: Theme.fontSizeMedium
                    verticalAlignment: Text.AlignVCenter
                    text: name
                }
            }

            Column{
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: Theme.paddingMedium
                //width: Math.max(sizeLabel.width, dateLabel.width) + Theme.paddingMedium

                Label{
                    id: sizeLabel
                    anchors.right: parent.right
                    visible: !dir
                    text: size
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
                Label{
                    id: dateLabel
                    anchors.right: parent.right
                    visible: !dir
                    text: Qt.formatDate(time)
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
            }

            onClicked: {
                //console.log("index: " + index );
                listView.click(index, myData)
            }
        }
    }
}
