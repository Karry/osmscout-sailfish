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
import QtPositioning 5.2
import QtQml.Models 2.2

import harbour.osmscout.map 1.0

import "."
import "../custom/Utils.js" as Utils

ComboBox {
    id: trackTypeComboBox
    label: qsTr("Type")

    property string selected: ""

    TrackTypes {
        id: trackTypes
    }

    menu: ContextMenu {
        Repeater {
            model: trackTypes.types
            TrackTypeMenuItem {
                type: modelData
            }
        }
    }
    onPressAndHold: {
        // improve default ComboBox UX :-)
        trackTypeComboBox.clicked(mouse);
    }
    onCurrentItemChanged: {
        selected = trackTypes.types[currentIndex];
        console.log("Selected type: "+selected);
    }
    onSelectedChanged: {
        console.log("Selected type: " + selected);
        for (var i in trackTypes.types){
            var type = trackTypes.types[i];
            if (type==selected && currentIndex != i){
                currentIndex = i;
                console.log("Type index: " + i + " (" + type + ")");
                break;
            }
        }
    }
}
