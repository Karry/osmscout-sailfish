/*
 OSM Scout for Sailfish OS
 Copyright (C) 2020  Lukas Karas

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
    id: trackerPage

    Component.onCompleted: {
        if (!Global.tracker.tracking){

        }
    }

    SilicaFlickable{
        id: flickable
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingMedium

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                text: qsTr("Stop tracking")
                enabled: Global.tracker.tracking
                onClicked: {
                    Global.tracker.stopTracking();
                    pageStack.pop();
                }
            }
        }

        Column {
            id: content
            x: Theme.paddingMedium
            width: parent.width - 2*Theme.paddingMedium

            function round10(d){
                return Math.round(d * 10)/10;
            }

            PageHeader {
                title: trackModel.name
            }

            Label {
                text: Global.tracker.description
                visible: text != ""
                color: Theme.secondaryColor
                width: parent.width
                wrapMode: Text.WordWrap
            }
            DetailItem {
                id: distanceItem
                label: qsTr("Distance")
                value: Global.tracker.distance < 0 ? "?" :Utils.humanDistance(Global.tracker.distance)
            }
            DetailItem {
                id: rawDistanceItem
                label: qsTr("Raw distance")
                value: Global.tracker.rawDistance < 0 ? "?" :Utils.humanDistance(Global.tracker.rawDistance)
            }
            DetailItem {
                id: fromTime
                visible: Global.tracker.from.getTime() > 0
                label: qsTr("From")
                value: Qt.formatDateTime(Global.tracker.from)
            }
            DetailItem {
                id: toTime
                visible: Global.tracker.to.getTime() > 0
                label: qsTr("To")
                value: Qt.formatDateTime(Global.tracker.to)
            }
            DetailItem {
                id: duration
                visible: Global.tracker.duration > 0
                label: qsTr("Time")
                value: Utils.humanDurationLong(Global.tracker.duration / 1000)
            }
            DetailItem {
                id: movingDuration
                visible: Global.tracker.movingDuration > 0
                label: qsTr("Moving Time")
                value: Utils.humanDurationLong(Global.tracker.movingDuration / 1000)
            }
            DetailItem {
                id: speed
                visible: Global.tracker.averageSpeed >= 0
                label: qsTr("Speed ⌀/max")
                value: Utils.distanceUnits == "imperial" ?
                            (qsTr("%1 / %2 mi/h")
                                .arg(content.round10((Global.tracker.averageSpeed*3.6 * 1000) / 1609.344))
                                .arg(content.round10((Global.tracker.maxSpeed*3.6 * 1000) / 1609.344))) :
                            (qsTr("%1 / %2 km/h")
                                .arg(content.round10(Global.tracker.averageSpeed*3.6))
                                .arg(content.round10(Global.tracker.maxSpeed*3.6)))
            }
            DetailItem {
                id: movingSpeed
                visible: Global.tracker.movingAverageSpeed >= 0
                label: qsTr("Moving Speed ⌀")
                value: Utils.distanceUnits == "imperial" ?
                            (qsTr("%1 mi/h")
                                .arg(content.round10((Global.tracker.movingAverageSpeed*3.6 * 1000) / 1609.344))) :
                            (qsTr("%1 km/h")
                                .arg(content.round10(Global.tracker.movingAverageSpeed*3.6)))
            }
            DetailItem {
                id: elevation
                visible: Global.tracker.minElevation > -100000 && Global.tracker.maxElevation > -100000
                label: qsTr("Elevation min/max")
                value: Utils.distanceUnits == "imperial" ?
                           (qsTr("%1 / %2 ft a.s.l.")
                                .arg(Math.round(Global.tracker.minElevation * 3.2808))
                                .arg(Math.round(Global.tracker.maxElevation * 3.2808))):
                           (qsTr("%1 / %2 m a.s.l.")
                                .arg(Math.round(Global.tracker.minElevation))
                                .arg(Math.round(Global.tracker.maxElevation)))
            }
            DetailItem {
                id: ascent
                label: qsTr("Ascent")
                value: Utils.distanceUnits == "imperial" ?
                            qsTr("%1 ft")
                                .arg(Math.round(Global.tracker.ascent * 3.2808)) :
                            qsTr("%1 m")
                                .arg(Math.round(Global.tracker.ascent))
            }
            DetailItem {
                id: descent
                label: qsTr("Descent")
                value: Utils.distanceUnits == "imperial" ?
                            qsTr("%1 ft")
                                .arg(Math.round(Global.tracker.descent * 3.2808)) :
                            qsTr("%1 m")
                                .arg(Math.round(Global.tracker.descent))
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
