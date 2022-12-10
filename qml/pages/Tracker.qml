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
    objectName: "Tracker"

    property bool rejectRequested: false;
    property bool newTrackRequested: false;

    Component.onCompleted: {
        if (!Global.tracker.tracking){
            newTrackRequested=true;
            newTrackDialog.open();
        }
    }

    function pageStatus(status){
        // https://sailfishos.org/develop/docs/silica/qml-sailfishsilica-sailfish-silica-page.html/
        if (status == PageStatus.Inactive){
            return "Inactive";
        }
        if (status == PageStatus.Activating){
            return "Activating";
        }
        if (status == PageStatus.Active){
            return "Active";
        }
        if (status == PageStatus.Deactivating){
            return "Deactivating";
        }

        return "Unknown";
    }

    onStatusChanged: {
        console.log("Tracker page status: " + pageStatus(trackerPage.status));
        if (trackerPage.status == PageStatus.Active) {
            //console.log("newTrackRequested: " + newTrackRequested + ", rejectRequested: " + rejectRequested);
            if (newTrackRequested){
                newTrackDialog.trackType = AppSettings.lastTrackType;
                newTrackDialog.open();
                newTrackRequested = false;
            }
            if (rejectRequested){
                pageStack.pop();
            }
        }
    }

    CollectionEditDialog {
        id: editDialog

        property string itemId: ""
        title: qsTr("Edit track")
        symbolSelectorVisible: false
        trackTypeSelectorVisible: true

        onAccepted: {
            console.log("Edit track " + itemId + ": " + name + " / " + description + " type: " + trackType);
            Global.tracker.editTrack(itemId, name, description, trackType);
            parent.focus = true;
        }
        onRejected: {
            parent.focus = true;
        }
    }

    CollectionEntryDialog{
        id: newTrackDialog
        symbolSelectorVisible: false
        trackTypeSelectorVisible: true

        title: qsTr("New track")

        name: Qt.formatDateTime(new Date())
        //acceptDestinationAction: PageStackAction.Pop

        onAccepted: {
            console.log("Start tracking, track " + name + " in collection " + collectionId + ", type " + trackType);
            AppSettings.lastCollection = collectionId;
            AppSettings.lastTrackType = trackType;
            Global.tracker.startTracking(collectionId, name, description, trackType);
        }
        onRejected: {
            trackerPage.rejectRequested = true;
            //console.log("New track: rejectRequested!");
            pageStack.pop();
        }
    }

    SilicaFlickable{
        id: flickable
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingMedium

        VerticalScrollDecorator {}

        RemorsePopup { id: remorse }

        PullDownMenu {
            MenuItem {
                text: qsTr("Change color")
                enabled: Global.tracker.tracking
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("TrackColor.qml"),
                                   {
                                        trackId: Global.tracker.trackId,
                                        acceptPage: trackerPage
                                   });
                }
            }
            MenuItem {
                text: qsTr("Rename track")
                enabled: Global.tracker.tracking
                onClicked: {
                    editDialog.itemId = Global.tracker.trackId;
                    editDialog.name = Global.tracker.name;
                    editDialog.description = Global.tracker.description;
                    editDialog.trackType = Global.tracker.type;
                    console.log("edit dialog track type: " + Global.tracker.type);
                    editDialog.open();
                }
            }
            MenuItem {
                text: qsTr("Stop tracking")
                enabled: Global.tracker.tracking
                onClicked: {
                    //: remorse dialog
                    remorse.execute(qsTr("Stopping tracker"),
                                    function() {
                                        Global.tracker.stopTracking();
                                        pageStack.pop();
                                    },
                                    5 * 1000);
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
                title: Global.tracker.name
            }


            Label {
                text: Global.tracker.description
                visible: text != ""
                color: Theme.secondaryColor
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Row {
                id: errorRow
                visible: Global.tracker.errors > 0
                width: parent.width
                spacing: Theme.paddingMedium

                Image{
                    id: errorIcon
                    source: "image://theme/icon-lock-warning"
                    width: Theme.iconSizeSmall
                    height: Math.max(width, errorLabel.height)
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    //x: Theme.paddingMedium
                }
                Label {
                    id: errorLabel
                    text: qsTr("There was %n error(s) during tracking. Recent: %2", "", Global.tracker.errors).arg(Global.tracker.lastError)
                    color: Theme.highlightColor
                    wrapMode: Text.WordWrap
                    width: parent.width - (errorIcon.width +2*errorRow.spacing)
                }
            }

            DetailItem {
                id: typeDetail
                visible: Global.tracker.type != ""
                //: track type
                label: qsTr("Type")
                value: qsTranslate("trackType", Global.tracker.type)
            }

            SectionHeader{ text: qsTr("Current data") }

            DetailItem {
                id: lastUpdateTime
                visible: Global.positionSource.lastUpdate.getTime() > 0
                //: Last GPS update time
                label: qsTr("Last update")
                value: Qt.formatTime(Global.positionSource.lastUpdate, Qt.DefaultLocaleLongDate)
            }
            DetailItem {
                id: lastHorizontalAccuracy
                visible: lastUpdateTime.visible
                label: qsTr("Horizontal accuracy")
                value: Global.positionSource.horizontalAccuracyValid ? Utils.humanSmallDistance(Global.positionSource.horizontalAccuracy) : "-"
            }
            DetailItem {
                id: lastAltitude
                // altitude update may be less often than horizontal position
                // we make it visible when it is not older than 5 minutes than horizontal data
                visible: lastUpdateTime.visible && (Global.positionSource.lastUpdate.getTime() - Global.positionSource.lastAltitudeUpdate.getTime()) < (5 * 60 * 1000)
                label: qsTr("Altitude")
                value: Utils.distanceUnits == "imperial" ?
                           (qsTr("%1 ft a.s.l.")
                                .arg(Math.round(Global.positionSource.altitude * 3.2808))):
                           (qsTr("%1 m a.s.l.")
                                .arg(Math.round(Global.positionSource.altitude)))
            }
            DetailItem {
                id: lastVerticalAccuracy
                visible: lastAltitude.visible
                label: qsTr("Vertical accuracy")
                value: Global.positionSource.verticalAccuracyValid ? Utils.humanSmallDistance(Global.positionSource.verticalAccuracy) : "-"
            }

            SectionHeader{ text: qsTr("Statistics") }

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
                //: From date time
                label: qsTr("From")
                value: Qt.formatDateTime(Global.tracker.from)
            }
            DetailItem {
                id: toTime
                visible: Global.tracker.to.getTime() > 0
                //: To date time
                label: qsTr("To")
                value: Qt.formatDateTime(Global.tracker.to)
            }
            DetailItem {
                id: duration
                visible: Global.tracker.duration > 0
                //: Track duration
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
                value: Utils.humanSmallDistance(Global.tracker.ascent)
            }
            DetailItem {
                id: descent
                label: qsTr("Descent")
                value: Utils.humanSmallDistance(Global.tracker.descent)
            }
            TrackElevationChart {
                id: elevationChart
                width: parent.width
                height: Math.min((width / 1920) * 1080, 512)

                lineColor: Theme.highlightColor
                lineWidth: 5
                gradientTopColor: Theme.rgba(Theme.secondaryHighlightColor, 0.6)
                //gradientBottomColor: Theme.rgba(Theme.highlightColor, 0.6)
                textColor: Theme.secondaryHighlightColor
                textPixelSize: Theme.fontSizeTiny
                textPadding: Theme.paddingSmall

                trackId: Global.tracker.trackId

                Timer {
                    id: updateEleChartTimer
                    interval: 5*60*1000 // every 5 minutes
                    repeat: true
                    running: Global.tracker.tracking

                    onTriggered: {
                        console.log("Update tracker elevation chart: " + Global.tracker.trackId)
                        elevationChart.trackId = Global.tracker.trackId
                    }
                }

                BusyIndicator {
                    id: elevationChartBusyIndicator
                    running: elevationChart.loading
                    size: BusyIndicatorSize.Medium
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
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
