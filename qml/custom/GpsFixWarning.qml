/*
 OSM Scout for Sailfish OS
 Copyright (C) 2025  Lukas Karas

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

Row {
    spacing: Theme.paddingMedium

    Image{
        id: warningIcon

        source: "image://theme/icon-m-about"

        width: Theme.iconSizeMedium
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignVCenter
        height: width
    }
    Label {
        id: positionWarningLabel
        text: qsTr("Waiting for GPS fix")
        anchors.verticalCenter: parent.verticalCenter
    }
}
