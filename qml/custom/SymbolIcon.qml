/*
 OSM Scout for Sailfish OS
 Copyright (C) 2021  Lukas Karas

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

Image {
    property string symbolName
    property string symbolImage
    property string selectedSymbol
    property double notSelectedScale: 0.6
    property double centerX: Theme.iconSizeLarge / 2
    property double centerY: Theme.iconSizeLarge / 2
    property int animationDuration: 100

    signal selected()

    x: centerX - width / 2
    y: centerY - height / 2

    width: Theme.iconSizeLarge * (selectedSymbol === symbolName ? 1: notSelectedScale)
    height: width
    opacity: (selectedSymbol === symbolName ? 1: 0.5)
    source: "image://harbour-osmscout/pics/" + symbolImage
    fillMode: Image.PreserveAspectFit
    horizontalAlignment: Image.AlignHCenter
    verticalAlignment: Image.AlignVCenter
    sourceSize.width: Theme.iconSizeLarge
    sourceSize.height: Theme.iconSizeLarge

    MouseArea {
        anchors.fill: parent
        onClicked: {
            selected()
        }
    }
    Behavior on opacity {
        NumberAnimation {
            duration: animationDuration
        }
    }
    Behavior on width {
        NumberAnimation {
            duration: animationDuration
        }
    }
    Behavior on height {
        NumberAnimation {
            duration: animationDuration
        }
    }
}
