/*
 OSM Scout for Sailfish OS
 Copyright (C) 2019  Lukas Karas

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
    id: phoneRow
    property string phone: ""

    visible: phone != ""
    width: parent.width
    height: phoneIcon.height

    IconButton{
        id: phoneIcon
        visible: phone != ""
        icon.source: "image://theme/icon-m-dialpad"

        MouseArea{
            onClicked: Qt.openUrlExternally("tel:%1".arg(phone))
            anchors.fill: parent
        }
    }
    Label {
        id: phoneLabel
        anchors.left: phoneIcon.right
        anchors.verticalCenter: phoneIcon.verticalCenter
        visible: phone != ""
        text: phone

        color: Theme.highlightColor
        truncationMode: TruncationMode.Fade

        MouseArea{
            onClicked: Qt.openUrlExternally("tel:%1".arg(phone))
            anchors.fill: parent
        }
    }
}
