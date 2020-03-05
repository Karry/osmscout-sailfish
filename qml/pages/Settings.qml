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

import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils
import ".." // Global singleton

Page {
    id: settingsPage

    Settings {
        id: settings
    }

    property double placeLat: Global.positionSource.lat == 0 ? 50.0886581 : Global.positionSource.lat
    property double placeLon: Global.positionSource.lon == 0 ? 14.4111289 : Global.positionSource.lon

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: content.height + 2*Theme.paddingLarge

        VerticalScrollDecorator {}

        Column {
            id: content
            width: parent.width

            PageHeader { title: qsTr("Settings") }

            ComboBox {
                id: fontSizeComboBox
                width: parent.width

                property bool initialized: false

                label: qsTr("Units")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Metrics") }
                    MenuItem { text: qsTr("Imperial") }
                }

                onCurrentItemChanged: {
                    if (!initialized){
                        return;
                    }
                    if (currentIndex==0)
                        settings.units="metrics";
                    if (currentIndex==1)
                        settings.units="imperial";
                    console.log("Set units to " + settings.units);
                }
                Component.onCompleted: {
                    if (settings.units=="imperial"){
                        currentIndex = 1;
                    }else{
                        currentIndex = 0;
                    }
                    initialized = true;
                }
                onPressAndHold: {
                    // improve default ComboBox UX :-)
                    clicked(mouse);
                }
            }
            ComboBox {
                id: placeLocationComboBox
                label: qsTr("Coordinates")

                property bool initialized: false

                menu: ContextMenu {
                    MenuItem { text: Utils.formatCoord(placeLat, placeLon, "degrees") }
                    MenuItem { text: Utils.formatCoord(placeLat, placeLon, "geocaching") }
                    MenuItem { text: Utils.formatCoord(placeLat, placeLon, "numeric") }
                }
                onCurrentItemChanged: {
                    if (!initialized){
                        return;
                    }
                    var format = "degrees";
                    if (currentIndex == 0){
                        format = "degrees";
                    }else if (currentIndex == 1){
                        format = "geocaching";
                    }else if (currentIndex == 2){
                        format = "numeric";
                    }

                    AppSettings.gpsFormat = format
                }
                Component.onCompleted: {
                    currentIndex = 0;
                    if (AppSettings.gpsFormat === "degrees"){
                        currentIndex = 0;
                    } else if (AppSettings.gpsFormat === "geocaching"){
                        currentIndex = 1;
                    } else if (AppSettings.gpsFormat === "numeric"){
                        currentIndex = 2;
                    }

                    initialized = true;
                }
                onPressAndHold: {
                    // improve default ComboBox UX :-)
                    clicked(mouse);
                }
            }
        }
    }
}
