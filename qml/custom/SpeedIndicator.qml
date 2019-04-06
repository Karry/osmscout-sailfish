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

import QtQuick 2.2
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

Item{
    id: speedIndicator
    property double currentSpeed: -1
    property double maximumSpeed: -1
    property double roundedCurrentSpeed: Math.round(speedIndicator.currentSpeed)

    property double dimension: Math.min(width, height)

    visible: roundedCurrentSpeed >= 0 || maximumSpeed > 0;

    Rectangle {
        id: currentSpeedBackground
        anchors.left: parent.left
        anchors.top: parent.top
        width: parent.dimension
        height: parent.dimension
        radius: Theme.paddingMedium

        color: Theme.rgba(Theme.highlightDimmerColor, 0.7)
        Rectangle{
            id: alert
            anchors.fill: parent
            radius: Theme.paddingMedium
            color: speedIndicator.currentSpeed > speedIndicator.maximumSpeed &&
                   speedIndicator.maximumSpeed > 0 ?
                       Theme.rgba("#ff6060", 0.6) :
                       "transparent"
        }

        visible: speedIndicator.roundedCurrentSpeed > 0

        //color: Theme.rgba(Theme.highlightDimmerColor, 0.7)
        Column{
            anchors.centerIn: parent
            Text{
                text: speedIndicator.roundedCurrentSpeed
                color: Theme.primaryColor
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: currentSpeedBackground.height * 0.5
            }
            Text{
                text: qsTr("km/h")
                color: Theme.primaryColor
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: currentSpeedBackground.height * 0.2
            }
        }
    }
    Rectangle {
        id: maxSpeedBackground
        color: "#c00000"
        anchors.horizontalCenter: currentSpeedBackground.visible ? parent.right : parent.horizontalCenter
        anchors.verticalCenter: currentSpeedBackground.visible ? parent.bottom : parent.verticalCenter

        width: parent.dimension * (!maxSpeedBackground.visible ? 0 : currentSpeedBackground.visible ? 0.5 : 1.0)
        height: parent.dimension * (!maxSpeedBackground.visible ? 0 : currentSpeedBackground.visible ? 0.5 : 1.0)

        Behavior on x { PropertyAnimation {} }
        Behavior on y { PropertyAnimation {} }
        Behavior on width { PropertyAnimation {} }
        Behavior on height { PropertyAnimation {} }
        radius: width*0.5

        visible: speedIndicator.maximumSpeed > 0

        Rectangle{
            anchors.centerIn: parent
            width: parent.width * 0.7
            height: parent.height * 0.7
            radius: width*0.5
            color: "white"
            Text{
                text: Math.round(speedIndicator.maximumSpeed)
                color: "black"
                font.pixelSize: parent.height /2
                font.bold: true
                anchors.centerIn: parent
            }
        }
    }
}
