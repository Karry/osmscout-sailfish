/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2018 Lukas Karas

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

import harbour.osmscout.map 1.0
import "Utils.js" as Utils

Item {
    id: item

    anchors.right: parent.right;
    anchors.left: parent.left;
    height: stepDelta.height + Math.max(entryDescription.implicitHeight +
                                            (destinationsText.visible ? destinationsText.implicitHeight : 0) +
                                            Theme.paddingMedium,
                                        icon.height)

    Column{
        id: stepDelta
        width: parent.width - 2*Theme.paddingMedium
        x: Theme.paddingMedium

        DetailItem {
            id: distanceItem
            visible: model.distanceDelta > 0
            label: qsTr("Distance")
            value: routeReady ? Utils.humanDistance(model.distanceDelta) : "?"
        }
    }

    RouteStepIcon{
        id: icon
        anchors.top: stepDelta.bottom
        stepType: model.type
        roundaboutExit: model.roundaboutExit
        roundaboutClockwise: model.roundaboutClockwise
        width: Theme.iconSizeLarge
        height: width

        Component.onCompleted: {
            console.log("width: "+width);
        }
    }

    Label {
        id: entryDescription

        x: Theme.paddingMedium

        anchors.left: icon.right
        anchors.top: stepDelta.bottom
        width: parent.width - (2*Theme.paddingMedium) - icon.width
        text: model.description
        wrapMode: Text.Wrap
    }

    Label {
        id: destinationsText

        y: Theme.paddingMedium
        x: Theme.paddingMedium
        visible: destinations.length > 0
        anchors.left: icon.right
        anchors.top: entryDescription.bottom
        width: parent.width - (2*Theme.paddingMedium) - icon.width

        text: qsTr("Destinations: %1").arg(destinations.join(", "))
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.secondaryColor

        wrapMode: Text.Wrap
    }

}
