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

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils

Page {
    property string description
    Column {
        anchors.fill: parent
        PageHeader {
            title: qsTr("No offline database!")
        }
        Label{
            id: aboutText
            text: description
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontSizeSmall

            width: parent.width - 2* Theme.paddingLarge
            x: Theme.paddingLarge
        }
    }
}
