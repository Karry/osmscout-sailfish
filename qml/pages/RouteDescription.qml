/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2017  Lukas Karas

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
    id: routingPage

    property RoutingListModel route

    SilicaListView {
        id: stepsView
        model: route
        anchors.fill: parent
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium

        delegate: BackgroundItem {
            id: backgroundItem
            //height: entryDescription.height
            height: entryDescription.implicitHeight+Theme.paddingMedium

            ListView.onAdd: AddAnimation {
                target: backgroundItem
            }
            ListView.onRemove: RemoveAnimation {
                target: backgroundItem
            }
            Label {
                id: entryDescription
                x: Theme.paddingMedium

                width: parent.width-2*Theme.paddingMedium
                //color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                //textFormat: Text.StyledText
                text: label
            }
        }

        BusyIndicator {
            id: busyIndicator
            running: !route.ready
            size: BusyIndicatorSize.Large
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}

