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

import harbour.osmscout.map 1.0

import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle{
    id : osmCopyright
    anchors{
        left: parent.left
        bottom: parent.bottom
    }

    Settings {
        id: settings
        property string copyrightText: ""
        property double defaultOpacity: 1

        function update(){
            osmCopyright.opacity = (onlineTiles && copyrightText != "") ? defaultOpacity: 0;
        }

        onOnlineTileProviderIdChanged: {
            copyrightText = qsTranslate("resource", settings.onlineProviderCopyright());
            update();
        }
        onOnlineTilesChanged: {
            update();
        }
        Component.onCompleted: {
            defaultOpacity = osmCopyright.opacity
            copyrightText = qsTranslate("resource", settings.onlineProviderCopyright());
            update();
        }
    }

    width: label.width + 20
    height: label.height + 12
    radius: Theme.paddingSmall

    color: Theme.rgba(Theme.highlightDimmerColor, 0.2)

    Text {
        id: label
        text: qsTr(settings.copyrightText)
        anchors.centerIn: parent
        color: Theme.rgba(Theme.primaryColor, 0.8)

        font.pointSize: Theme.fontSizeExtraSmall *0.6
    }

}
