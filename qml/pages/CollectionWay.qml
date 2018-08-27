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
    id: wayDialog

    property string name;
    property string description;
    signal selectWay(LocationEntry selectWay);
    property var acceptPage;

    acceptDestination: wayDialog.acceptPage
    acceptDestinationAction: PageStackAction.Pop
    onAccepted: {
        // TODO
        selectWay(null);
    }

    DialogHeader {
        id: wayDialogheader
        //title: wayDialog.name
        acceptText : qsTr("Show")
        //cancelText : qsTr("Cancel")
    }

    Rectangle{
        anchors{
            top: wayDialogheader.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }
        color: "transparent"


        Drawer {
            id: drawer
            anchors.fill: parent

            dock: wayDialog.isPortrait ? Dock.Top : Dock.Left
            open: true
            backgroundSize: wayDialog.isPortrait ? drawer.height * 0.6 : drawer.width * 0.6

            background:  Rectangle{

                anchors.fill: parent
                color: "transparent"

                /*
                OpacityRampEffect {
                    //enabled: !placeLocationComboBox._menuOpen //true
                    offset: 1 - 1 / slope
                    slope: locationInfoView.height / (Theme.paddingLarge * 4)
                    direction: 2
                    sourceItem: locationInfoView
                }
                */
                SilicaFlickable{
                    id: flickable
                    anchors.fill: parent
                    //contentHeight: content.height + header.height + 2*Theme.paddingLarge

                    VerticalScrollDecorator {}

                    Column {
                        Label {
                            text: wayDialog.description
                            x: Theme.paddingMedium
                            width: parent.width - 2*Theme.paddingMedium
                            //visible: text != ""
                            color: Theme.secondaryColor
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            MapComponent{
                id: wayPreviewMap
                showCurrentPosition: true
                anchors.fill: parent
            }
        }
    }
}
