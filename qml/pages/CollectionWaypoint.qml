/*
 OSM Scout for Sailfish OS
 Copyright (C) 2018  Lukas Karas

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
import Sailfish.Pickers 1.0
import QtPositioning 5.2
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"

Dialog{
    id: waypointDialog

    property string name;
    property string description;
    property double latitude;
    property double longitude;
    property alias previewMap: wptPreviewMap

    acceptDestination: collectionPage.acceptDestination
    acceptDestinationAction: PageStackAction.Pop
    onAccepted: {
        selectWaypoint(latitude, longitude);
    }

    DialogHeader {
        id: waypointDialogheader
        title: waypointDialog.name
        acceptText : qsTr("Show")
        //cancelText : qsTr("Cancel")
    }
    Rectangle{
        anchors{
            top: waypointDialogheader.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }
        color: "transparent"

        Drawer {
            id: drawer
            anchors.fill: parent

            dock: waypointDialog.isPortrait ? Dock.Top : Dock.Left
            open: true
            backgroundSize: waypointDialog.isPortrait ? (waypointDialog.height * 0.5) - waypointDialogheader.height : drawer.width * 0.5

            background:  Rectangle{

                anchors.fill: parent
                color: "transparent"

                OpacityRampEffect {
                    offset: 1 - 1 / slope
                    slope: flickable.height / (Theme.paddingLarge * 4)
                    direction: 2
                    sourceItem: flickable
                }

                SilicaFlickable{
                    id: flickable
                    anchors.fill: parent
                    contentHeight: content.height + Theme.paddingMedium

                    VerticalScrollDecorator {}

                    Column {
                        id: content
                        x: Theme.paddingMedium
                        width: parent.width - 2*Theme.paddingMedium

                        Label {
                            text: waypointDialog.description
                            x: Theme.paddingMedium
                            width: parent.width - 2*Theme.paddingMedium
                            //visible: text != ""
                            color: Theme.secondaryColor
                            wrapMode: Text.WordWrap
                        }
                        Rectangle {
                            id: footer
                            color: "transparent"
                            width: parent.width
                            height: 2*Theme.paddingLarge
                        }
                    }
                }
            }
            MapComponent{
                id: wptPreviewMap
                anchors.fill: parent
                showCurrentPosition: true
            }
        }
    }
}
