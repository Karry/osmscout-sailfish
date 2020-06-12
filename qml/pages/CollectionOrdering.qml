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
import Sailfish.Pickers 1.0
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils

Page {
    id: collectionOrderingPage

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: content.height + 2*Theme.paddingLarge

        VerticalScrollDecorator {}

        Column {
            id: content
            width: parent.width

            PageHeader { title: qsTr("Collection entries ordering") }

            ComboBox {
                id: fontSizeComboBox
                width: parent.width

                property bool initialized: false

                label: qsTr("Order by")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Date") }
                    MenuItem { text: qsTr("Date, descent") }
                    MenuItem { text: qsTr("Name") }
                    MenuItem { text: qsTr("Name, descent") }
                }

                onCurrentItemChanged: {
                    if (!initialized){
                        return;
                    }
                    AppSettings.collectionOrdering = currentIndex;

                }
                Component.onCompleted: {
                    currentIndex = AppSettings.collectionOrdering;
                    initialized = true;
                }
                onPressAndHold: {
                    // improve default ComboBox UX :-)
                    clicked(mouse);
                }
            }

            TextSwitch{
                id: waypointFirstSwitch
                width: parent.width

                checked: AppSettings.waypointFirst
                //: switch for diplaying waypoints before tracks in collection
                text: qsTr("Waypoints first")

                onCheckedChanged: {
                    AppSettings.waypointFirst = checked;
                }
            }
        }
    }
}
