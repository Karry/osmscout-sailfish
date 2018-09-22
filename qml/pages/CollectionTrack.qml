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

    canAccept: !trackModel.loading
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
                    obj.type="_highlighted";
                    console.log("add overlay for segment "+segment+": "+obj);
                    //console.log("object "+row+": "+obj);
                    wayPreviewMap.addOverlayObject(segment, obj);
                }
            }
        }
    }

    DialogHeader {
        id: trackDialogheader
        title: trackModel.name
        acceptText : qsTr("Show")
        //cancelText : qsTr("Cancel")
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

                SilicaFlickable{
                    id: flickable
                    anchors.fill: parent
                    contentHeight: content.height + Theme.paddingMedium

                    VerticalScrollDecorator {}

                    Column {
                        id: content
                        x: Theme.paddingMedium
                        width: parent.width - 2*Theme.paddingMedium

                        Label {
                            text: trackModel.description
                            visible: text != ""
                            color: Theme.secondaryColor
                            width: parent.width
                            wrapMode: Text.WordWrap
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
                            id: duration
                            label: qsTr("Duration")
                            value: Utils.humanDurationLong(trackModel.duration / 1000)
                        }
                        DetailItem {
                            id: movingDuration
                            label: qsTr("Moving duration")
                            value: Utils.humanDurationLong(trackModel.movingDuration / 1000)
                        }
                        DetailItem {
                            id: speed
                            label: qsTr("Speed max / ⌀")
                            value: trackModel.maxSpeed + " / " + trackModel.averageSpeed +" km"
                        }
                        DetailItem {
                            id: movingSpeed
                            label: qsTr("Moving ⌀ speed")
                            value: trackModel.movingAverageSpeed +" km"
                        }
                        DetailItem {
                            id: ascent
                            label: qsTr("Ascent")
                            value: trackModel.ascent + " m"
                        }
                        DetailItem {
                            id: descent
                            label: qsTr("Descent")
                            value: trackModel.descent + " m"
                        }
                        DetailItem {
                            id: elevation
                            visible: trackModel.minElevation > -100000 && trackModel.maxElevation > -100000
                            label: qsTr("Elevation min/max")
                            value: trackModel.minElevation + " / " + trackModel.maxElevation + " m"
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
