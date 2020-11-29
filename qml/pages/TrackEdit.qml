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
import Sailfish.Pickers 1.0
import QtPositioning 5.2
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils

Dialog {
    id: trackEditDialog

    property string action: "crop-start" // crop-start, crop-end, split
    property var acceptPage;
    property var trackId;
    property var position: 0;

    acceptDestination: trackEditDialog.acceptPage
    acceptDestinationAction: PageStackAction.Pop

    Timer {
        id: showOverviewTimer
        interval: 5000
        repeat: false

        onTriggered: {
            wayPreviewMap.showLocation(trackModel.boundingBox);
        }
    }

    function showPoint(){
        if (!trackModel.loading){
            var point=trackModel.getPoint(Math.min(position, trackModel.pointCount-1));
            var lat=point.x;
            var lon=point.y;
            wayPreviewMap.addPositionMark(0, lat, lon);
            wayPreviewMap.showCoordinates(lat, lon);
            showOverviewTimer.restart();
        }
    }

    onPositionChanged: {
        showPoint();
    }

    canAccept: !trackModel.loading && position>0 && position<trackModel.pointCount
    onAccepted: {
        if (trackEditDialog.action == "crop-start"){
            trackModel.cropStart(position);
        } else if (trackEditDialog.action == "crop-end"){
            trackModel.cropEnd(position);
        } else if (trackEditDialog.action == "split"){
            trackModel.split(position);
        }
    }

    CollectionTrackModel{
        id: trackModel
        trackId: trackEditDialog.trackId

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

                if (trackEditDialog.action == "crop-start"){
                    trackEditDialog.position = 0;
                } else if (trackEditDialog.action == "crop-end"){
                    trackEditDialog.position = trackModel.pointCount;
                } else if (trackEditDialog.action == "split"){
                    trackEditDialog.position = trackModel.pointCount /2;
                }
                showPoint();
            }
        }
    }

    DialogHeader {
        id: trackEditDialogheader
        title: trackModel.name
        acceptText: (trackEditDialog.action == "crop-start") ? qsTr("Crop start") : (trackEditDialog.action == "crop-end" ? qsTr("Crop end") : qsTr("Split"))
        //cancelText : qsTr("Cancel")
    }

    Rectangle{
        anchors{
            top: trackEditDialogheader.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }
        color: "transparent"

        Drawer {
            id: drawer
            anchors.fill: parent

            dock: trackEditDialog.isPortrait ? Dock.Top : Dock.Left
            open: true
            backgroundSize: trackEditDialog.isPortrait ? (trackEditDialog.height * 0.5) - trackEditDialogheader.height : drawer.width * 0.5

            background:  Rectangle {

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

                        Slider{
                            id: positionSlider
                            width: parent.width

                            value: trackEditDialog.position
                            enabled: !trackModel.loading
                            stepSize: 1.0
                            valueText: qsTr("%1 / %2").arg(trackEditDialog.position).arg(maximumValue)
                            minimumValue: 0
                            maximumValue: trackModel.pointCount
                            label: (trackEditDialog.action == "crop-start" || trackEditDialog.action == "crop-end") ? qsTr("Crop position") : qsTr("Split position")

                            onValueChanged: {
                                trackEditDialog.position = Math.round(value);
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
