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

Dialog{
    id: trackDialog

    signal selectTrack(LocationEntry bbox, var trackId);
    property var acceptPage;
    property var trackId;

    acceptDestination: trackDialog.acceptPage
    acceptDestinationAction: PageStackAction.Pop

    canAccept: !trackModel.loading && trackModel.pointCount > 0
    onAccepted: {
        selectTrack(trackModel.boundingBox, trackId);
    }

    CollectionTrackModel{
        id: trackModel
        trackId: trackDialog.trackId

        onBoundingBoxChanged: {
           wayPreviewMap.showLocation(trackModel.boundingBox);
        }

        onLoadingChanged: {
            console.log("loading chagned: " + loading+ " segments: "+trackModel.segmentCount);
            if (!loading){
                var cnt=trackModel.segmentCount;
                for (var segment=0; segment<cnt; segment++){
                    var obj=trackModel.createOverlayForSegment(segment);
                    obj.type="_track";
                    console.log("add overlay for segment "+segment+": "+obj);
                    //console.log("object "+row+": "+obj);
                    wayPreviewMap.addOverlayObject(segment, obj);
                }
            }
        }
    }

    DialogHeader {
        id: trackDialogheader
        //title: trackModel.name
        acceptText : qsTr("Show")
        //cancelText : qsTr("Cancel")
        spacing: 0
    }

    Rectangle{
        anchors{
            top: trackDialogheader.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }
        color: "transparent"

        Drawer {
            id: drawer
            anchors.fill: parent

            dock: trackDialog.isPortrait ? Dock.Top : Dock.Left
            open: true
            backgroundSize: trackDialog.isPortrait ? (trackDialog.height * 0.5) - trackDialogheader.height : drawer.width * 0.5

            background:  Rectangle{

                anchors.fill: parent
                color: "transparent"

                OpacityRampEffect {
                    offset: 1 - 1 / slope
                    slope: flickable.height / (Theme.paddingLarge * 4)
                    direction: 2
                    sourceItem: flickable
                }

                Label {
                    id: titleText
                    text: trackModel.name
                    font.pixelSize: flickable.contentY == 0 ? Theme.fontSizeExtraLarge : Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                    color: Theme.highlightColor

                    opacity: text.length > 0 ? 1.0 : 0.0

                    property real spacing: flickable.contentY == 0 ? Theme.paddingLarge: Theme.paddingMedium
                    x: spacing
                    y: spacing
                    width: parent.width - 2*spacing

                    Behavior on opacity { FadeAnimation { } }
                    Behavior on font.pixelSize { NumberAnimation { duration: 200 } }
                    Behavior on spacing { NumberAnimation { duration: 200 } }
                }

                SilicaFlickable{
                    id: flickable
                    anchors.fill: parent
                    contentHeight: content.height + Theme.paddingMedium

                    VerticalScrollDecorator {}

                    Column {
                        id: content
                        x: Theme.paddingMedium
                        width: parent.width - 2*Theme.paddingMedium

                        function round10(d){
                            return Math.round(d * 10)/10;
                        }

                        Rectangle {
                            id: titleFlickablePlaceholder
                            width: parent.width
                            height: titleText.height + titleText.spacing
                            color: "transparent"
                        }

                        Label {
                            text: trackModel.description
                            visible: text != ""
                            color: Theme.secondaryColor
                            width: parent.width
                            wrapMode: Text.WordWrap
                        }
                        DetailItem {
                            id: typeDetail
                            visible: trackModel.type != ""
                            //: track type
                            label: qsTr("Type")
                            value: qsTr(trackModel.type)
                        }
                        DetailItem {
                            id: distanceItem
                            label: qsTr("Distance")
                            value: trackModel.distance < 0 ? "?" :Utils.humanDistance(trackModel.distance)
                        }
                        DetailItem {
                            id: rawDistanceItem
                            label: qsTr("Raw distance")
                            value: trackModel.rawDistance < 0 ? "?" :Utils.humanDistance(trackModel.rawDistance)
                        }
                        DetailItem {
                            id: fromTime
                            visible: trackModel.from.getTime() > 0
                            //: From date time
                            label: qsTr("From")
                            value: Qt.formatDateTime(trackModel.from)
                        }
                        DetailItem {
                            id: toTime
                            visible: trackModel.to.getTime() > 0
                            //: To date time
                            label: qsTr("To")
                            value: Qt.formatDateTime(trackModel.to)
                        }
                        DetailItem {
                            id: duration
                            visible: trackModel.duration > 0
                            //: Track duration
                            label: qsTr("Time")
                            value: Utils.humanDurationLong(trackModel.duration / 1000)
                        }
                        DetailItem {
                            id: movingDuration
                            visible: trackModel.movingDuration > 0
                            label: qsTr("Moving Time")
                            value: Utils.humanDurationLong(trackModel.movingDuration / 1000)
                        }
                        DetailItem {
                            id: speed
                            visible: trackModel.averageSpeed >= 0
                            label: qsTr("Speed ⌀/max")
                            value: Utils.distanceUnits == "imperial" ?
                                        (qsTr("%1 / %2 mi/h")
                                            .arg(content.round10((trackModel.averageSpeed*3.6 * 1000) / 1609.344))
                                            .arg(content.round10((trackModel.maxSpeed*3.6 * 1000) / 1609.344))) :
                                        (qsTr("%1 / %2 km/h")
                                            .arg(content.round10(trackModel.averageSpeed*3.6))
                                            .arg(content.round10(trackModel.maxSpeed*3.6)))
                        }
                        DetailItem {
                            id: movingSpeed
                            visible: trackModel.movingAverageSpeed >= 0
                            label: qsTr("Moving Speed ⌀")
                            value: Utils.distanceUnits == "imperial" ?
                                        (qsTr("%1 mi/h")
                                            .arg(content.round10((trackModel.movingAverageSpeed*3.6 * 1000) / 1609.344))) :
                                        (qsTr("%1 km/h")
                                            .arg(content.round10(trackModel.movingAverageSpeed*3.6)))
                        }
                        DetailItem {
                            id: elevation
                            visible: trackModel.minElevation > -100000 && trackModel.maxElevation > -100000
                            label: qsTr("Elevation min/max")
                            value: Utils.distanceUnits == "imperial" ?
                                       (qsTr("%1 / %2 ft a.s.l.")
                                            .arg(Math.round(trackModel.minElevation * 3.2808))
                                            .arg(Math.round(trackModel.maxElevation * 3.2808))):
                                       (qsTr("%1 / %2 m a.s.l.")
                                            .arg(Math.round(trackModel.minElevation))
                                            .arg(Math.round(trackModel.maxElevation)))
                        }
                        DetailItem {
                            id: ascent
                            label: qsTr("Ascent")
                            value: Utils.humanSmallDistance(trackModel.ascent)
                        }
                        DetailItem {
                            id: descent
                            label: qsTr("Descent")
                            value: Utils.humanSmallDistance(trackModel.descent)
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

                            trackId: trackDialog.trackId

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

            MapComponent{
                id: wayPreviewMap
                showCurrentPosition: true
                anchors.fill: parent
            }
        }
    }
    BusyIndicator {
        id: busyIndicator
        running: trackModel.loading
        size: BusyIndicatorSize.Large
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
}
