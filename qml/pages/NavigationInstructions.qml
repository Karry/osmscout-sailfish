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
                    Global.navigationModel.clear();
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
            //visible: routeReady && route.count>0
            width: parent.width - 2*Theme.paddingMedium
            x: Theme.paddingMedium

            /*
            DetailItem {
                id: distanceItem
                label: qsTr("Distance")
                value: routeReady ? Utils.humanDistance(route.length) : "?"
            }
            DetailItem {
                id: durationItem
                label: qsTr("Duration")
                value: routeReady ? Utils.humanDuration(route.duration) : "?"
            }
            */
            SectionHeader{
                id: itineraryHeader
                //: header of section with navigation instructions
                text: qsTr("Itinerary")
            }
        }

        delegate: RoutingStep{}
    }
}
