/*
 OSM Scout for Sailfish OS
 Copyright (C) 2022  Lukas Karas

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

MenuItem {
    id: menuItem
    property string type: 'unknown'

    text: qsTr(type)

    horizontalAlignment: Text.AlignLeft
    leftPadding: Theme.horizontalPageMargin + icon.width

    TrackTypeIcon {
        id: icon
        type: menuItem.type

        icon.fillMode: Image.PreserveAspectFit
        icon.sourceSize.width: Theme.iconSizeMedium
        icon.sourceSize.height: Theme.iconSizeMedium
    }
}
