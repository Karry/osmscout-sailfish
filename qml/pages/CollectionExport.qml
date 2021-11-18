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
import "../custom/Utils.js" as Utils

Dialog {
    id: exportPage

    property alias name: nameTextField.text
    property alias directory: destinationDirectoryComboBox.selected
    property alias includeWaypoints: waypointSwitch.checked
    property int accuracyFilter: -1
    property ListModel directories

    signal selected(string directory, string name, bool includeWaypoints, int accuracyFilter)

    canAccept: name.length > 0

    onAccepted: {
        console.log("selected: " + name + ", directory: " + directory);
        selected(directory, name, includeWaypoints, accuracyFilter);
    }

    DialogHeader {
        id: header
        acceptText : qsTr("Export")
        //cancelText : qsTr("Cancel")
    }

    Column{
        width: parent.width
        anchors{
            top: header.bottom
        }

        TextField {
            id: nameTextField
            width: parent.width
            label: qsTr("File name")
            placeholderText: qsTr("Name")
        }

        ComboBox {
            id: destinationDirectoryComboBox

            property string selected

            label: qsTr("Directory")
            menu: ContextMenu {
                id: contextMenu
                Repeater {
                    model: directories
                    MenuItem {
                        text: Utils.humanDirectory(dir)
                    }
                }
            }
            DelegateModel {
                id: delegateModel
            }
            onCurrentItemChanged: {
                //var dirs=mapDownloadsModel.getLookupDirectories();
                //selected = dirs[currentIndex];
                delegateModel.model = directories;
                var item = delegateModel.items.get(currentIndex).model;
                selected = item.dir;
            }
        }

        TextSwitch{
            id: waypointSwitch
            width: parent.width

            //: switch for exporting waypoints to gpx
            text: qsTr("Include waypoints")
        }

        ComboBox {
            id: accuracyComboBox

            property bool initialized: false

            //: gpx track export option
            label: qsTr("Drop inaccurate nodes")
            menu: ContextMenu {
                //: option dropping inaccurate nodes (gpx track export)
                MenuItem { text: qsTr("Keep everything") }
                //: option dropping inaccurate nodes (gpx track export)
                MenuItem { text: qsTr("> 100 m") }
                //: option dropping inaccurate nodes (gpx track export)
                MenuItem { text: qsTr("> 30 m") }
                //: option dropping inaccurate nodes (gpx track export)
                MenuItem { text: qsTr("> 10 m") }
            }
            onCurrentItemChanged: {
                if (!initialized){
                    return;
                }
                AppSettings.exportAccuracy = currentIndex;
                if (currentIndex == 0){
                    accuracyFilter = -1;
                } else if (currentIndex == 1){
                    accuracyFilter = 100;
                } else if (currentIndex == 2){
                    accuracyFilter = 30;
                } else if (currentIndex == 3){
                    accuracyFilter = 10;
                }
            }
            Component.onCompleted: {
                currentIndex = AppSettings.exportAccuracy;
                initialized = true;
            }
            onPressAndHold: {
                // improve default ComboBox UX :-)
                clicked(mouse);
            }
        }

    }
}
