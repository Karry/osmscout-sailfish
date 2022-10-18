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
import Sailfish.Pickers 1.0
import Sailfish.Share 1.0
import QtPositioning 5.2
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils

Page {
    id: collectionStatisticsPage
    property string collectionId: "-1"

    RemorsePopup { id: remorse }

    CollectionStatisticsModel {
        id: statisticsModel
        collectionId: collectionStatisticsPage.collectionId
        onError: {
            remorse.execute(message, function() { }, 10 * 1000);
        }
    }

    SilicaListView {
        anchors.fill: parent
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium
        clip: true
        model: statisticsModel

        header: Column{
            id: header
            width: parent.width

            PageHeader {
                title: statisticsModel.loading ? qsTr("Loading collection") : statisticsModel.name;
            }
            Label {
                text: statisticsModel.description
                x: Theme.paddingMedium
                width: parent.width - 2*Theme.paddingMedium
                //visible: text != ""
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
            }
            Item{
                width: parent.width
                height: Theme.paddingMedium
            }
        }

        delegate: ListItem {
            contentHeight: statColumn.height

            TrackTypeIcon {
                id: typeIcon
                type: model.type

                width: Theme.iconSizeExtraLarge
                height: width
                opacity: 0.2

                icon.fillMode: Image.PreserveAspectFit
                icon.sourceSize.width: width
                icon.sourceSize.height: height
            }

            Column {
                id: statColumn
                x: Theme.paddingMedium
                width: parent.width - 2*Theme.paddingMedium

                function round10(d){
                    return Math.round(d * 10)/10;
                }

                SectionHeader {
                    id: typeHeader
                    text: model.type != "" ? qsTr(model.type) : qsTr("Unknown type")
                    //width: parent.width
                }

                DetailItem {
                    id: trackCountItem
                    label: qsTr("Track count")
                    value: model.trackCount
                }
                DetailItem {
                    id: distanceItem
                    label: qsTr("Distance")
                    value: model.distance < 0 ? "?" :Utils.humanDistance(model.distance)
                }
                DetailItem {
                    id: longestTrackItem
                    label: qsTr("Longest track")
                    value: model.longestTrack < 0 ? "?" :Utils.humanDistance(model.longestTrack)
                }
                DetailItem {
                    id: duration
                    visible: model.duration > 0
                    //: Track duration
                    label: qsTr("Time")
                    value: Utils.humanDurationLong(model.duration / 1000)
                }
                DetailItem {
                    id: movingDuration
                    visible: model.movingDuration > 0
                    label: qsTr("Moving Time")
                    value: Utils.humanDurationLong(model.movingDuration / 1000)
                }
                DetailItem {
                    id: speed
                    visible: model.maxSpeed >= 0
                    label: qsTr("Max speed")
                    value: Utils.distanceUnits == "imperial" ?
                                (qsTr("%1 mi/h")
                                    .arg(statColumn.round10((model.maxSpeed*3.6 * 1000) / 1609.344))) :
                                (qsTr("%1 km/h")
                                    .arg(statColumn.round10(model.maxSpeed*3.6)))
                }
                DetailItem {
                    id: elevation
                    visible: model.minElevation > -100000 && model.maxElevation > -100000
                    label: qsTr("Elevation min/max")
                    value: Utils.distanceUnits == "imperial" ?
                               (qsTr("%1 / %2 ft a.s.l.")
                                    .arg(Math.round(model.minElevation * 3.2808))
                                    .arg(Math.round(model.maxElevation * 3.2808))):
                               (qsTr("%1 / %2 m a.s.l.")
                                    .arg(Math.round(model.minElevation))
                                    .arg(Math.round(model.maxElevation)))
                }
                DetailItem {
                    id: ascent
                    label: qsTr("Ascent")
                    value: Utils.humanSmallDistance(model.ascent)
                }
                DetailItem {
                    id: descent
                    label: qsTr("Descent")
                    value: Utils.humanSmallDistance(model.descent)
                }
            }
        }
    }
    BusyIndicator {
        id: busyIndicator
        running: statisticsModel.loading
        size: BusyIndicatorSize.Large
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
}
