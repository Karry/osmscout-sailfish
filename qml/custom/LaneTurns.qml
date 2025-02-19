/*
 OSM Scout for Sailfish OS
 Copyright (C) 2020 Lukas Karas

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

Rectangle{
    id: laneTurnsComponent

    color: "transparent"
    height: Theme.iconSizeLarge

    property var laneTurns: ["", "through", "through;right", "right"]
    property var laneTurn: "through"
    property var suggestedLaneFrom: 1
    property var suggestedLaneTo: 2
    property var maxWidth: height * 4

    property var iconWidth: Math.min(height * 0.60, maxWidth / laneTurns.length)
    property var iconOjects: []
    property var separatorObjects: []

    Component.onCompleted: {
        console.log("onCompleted");
        updateLane();
    }

    Component {
        id: iconComponent
        LaneTurnIcon{}
    }
    Component {
        id: separatorComponent
        Rectangle{
            height: parent.height;
            width: 2
            color: Theme.primaryColor
            opacity: 0.3
        }
    }

    onLaneTurnsChanged: {
        updateLane();
    }
    onSuggestedLaneFromChanged: {
        updateLane();
    }
    onSuggestedLaneToChanged: {
        updateLane();
    }

    function updateLane() {
        if (iconComponent.status == Component.Error) {
            // Error Handling
            console.log("Error loading component:", iconComponent.errorString());
            return
        }
        if (iconComponent.status != Component.Ready) {
            console.log("Component is not ready:", iconComponent.errorString());
            return
        }

        console.log("update");
        for (var i in iconOjects){
            iconOjects[i].destroy();
        }
        for (var i in separatorObjects){
            separatorObjects[i].destroy();
        }
        iconOjects = [];
        separatorObjects = [];

        var parentObj = laneTurnsComponent;
        parentObj.width = 0;
        for (var i in laneTurns){
            var turn = laneTurns[i];
            console.log("turn " + i + ": " + turn + " <" + suggestedLaneFrom + ", " + suggestedLaneTo + ">: " + laneTurn);
            var icon = iconComponent.createObject(parentObj,
                                              {
                                                  id: "turnIcon" + i,
                                                  turnType: turn,
                                                  suggestedTurn: laneTurn,
                                                  height: parentObj.height,
                                                  width: iconWidth,
                                                  suggested: ( i >= suggestedLaneFrom && i <= suggestedLaneTo)
                                              });
            if (icon == null) {
                // Error Handling
                console.log("Error creating icon object");
                return;
            }


            if (i==0){
                icon.anchors.left = parentObj.left;
            }else{
                console.log("iconOjects.length: " + iconOjects.length + " i: "+i);
                var prev = iconOjects[i-1];
                icon.anchors.left = prev.right;

                var separator = separatorComponent.createObject(parentObj);
                separator.anchors.left = prev.right;
                separatorObjects.push(separator);
            }

            iconOjects.push(icon);
            parentObj.width += icon.width;
        }
    }

}
