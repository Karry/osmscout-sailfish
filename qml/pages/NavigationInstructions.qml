/*
 OSM Scout for Sailfish OS
 Copyright (C) 2019 Lukas Karas

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
    id: navigationInstructions

    SilicaListView {
        id: stepsView
        model: Global.navigationModel

        PullDownMenu {
            MenuItem {
                text: qsTr("Stop navigation")
                onClicked: {
                    Global.navigationModel.stop();
                    pageStack.pop();
                }
            }
        }

        VerticalScrollDecorator {}
        clip: true

        anchors.fill: parent
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium

        header: Column{
            width: parent.width - 2*Theme.paddingMedium

            Rectangle {
                id: headerPadding
                color: "transparent"
                height: Theme.paddingMedium
                width: parent.width
            }

            DetailItem {
                id: arrivalItem

                function formatArrivalTime(arrivalTime){
                    return Qt.formatDate(arrivalTime) == Qt.formatDate(new Date()) ? Qt.formatTime(arrivalTime) : Qt.formatDateTime(arrivalTime);
                }

                label: qsTr("Arrival")
                visible: !isNaN(Global.navigationModel.arrivalEstimate.getTime())
                value: formatArrivalTime(Global.navigationModel.arrivalEstimate)
            }
            DetailItem {
                id: remainingDistanceItem
                //: Distance to target, itinerary page
                label: qsTr("Distance")
                visible: Global.navigationModel.remainingDistance > 0
                value: Utils.humanDistance(Global.navigationModel.remainingDistance)
            }

            SectionHeader{
                id: itineraryHeader
                //: header of section with navigation instructions
                text: qsTr("Itinerary")
            }
        }

        delegate: RoutingStep{}
    }
}
