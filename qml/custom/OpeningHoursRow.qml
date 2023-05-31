/*
 OSM Scout for Sailfish OS
 Copyright (C) 2023  Lukas Karas

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

Row {
    id: openingHoursRow

    property alias openingHours: model.openingHours

    OpeningHoursModel {
        id: model
        property string todayStr: model.today.join(", ")
        onParseError: {
            console.log("openingHours parse error: " + openingHours);
        }
    }

    width: parent.width
    height: openingHoursIcon.height
    visible: openingHours!=""

    IconButton {
        id: openingHoursIcon

        icon.source: "image://theme/icon-m-clock"
    }

    Label {
        id: openingHoursLabel

        anchors.left: openingHoursIcon.right
        anchors.verticalCenter: openingHoursIcon.verticalCenter
        text: model.todayStr!="" ? model.todayStr : openingHours
        color: Theme.highlightColor
        truncationMode: TruncationMode.Fade
    }
}
