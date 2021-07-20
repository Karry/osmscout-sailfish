/*
 OSM Scout for Sailfish OS
 Copyright (C) 2021  Lukas Karas

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

    property var acceptPage
    property var trackId
    property var color: ""

    acceptDestination: trackEditDialog.acceptPage
    acceptDestinationAction: PageStackAction.Pop

    canAccept: !trackModel.loading && color !== ""
    onAccepted: {
        console.log("setup track color: " + color);
        trackModel.setupColor(color);
    }

    function updatePreview() {
        var cnt=trackModel.segmentCount;
        for (var segment=0; segment<cnt; segment++){
            var obj=trackModel.createOverlayForSegment(segment);
            obj.type="_track";
            if (trackEditDialog.color !== ""){
                obj.color = trackEditDialog.color;
            }
            console.log("add overlay for segment "+segment+": "+obj);
            //console.log("object "+row+": "+obj);
            wayPreviewMap.addOverlayObject(segment, obj);
        }
    }

    CollectionTrackModel{
        id: trackModel
        trackId: trackEditDialog.trackId

        onBoundingBoxChanged: {
            console.log("bounding box changed");
            wayPreviewMap.showLocation(trackModel.boundingBox);
        }

        onLoadingChanged: {
            console.log("loading changed: " + loading+ " segments: " + trackModel.segmentCount);
            if (!loading){
                updatePreview();
            }
        }
    }

    DialogHeader {
        id: trackEditDialogheader
        title: trackModel.name
        acceptText: qsTr("Set color")
        spacing: trackEditDialog.isPortrait ? Theme.paddingLarge : 0
    }

    Drawer {
        id: drawer
        anchors {
            top: trackEditDialogheader.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        dock: trackEditDialog.isPortrait ? Dock.Top : Dock.Left
        backgroundSize: trackEditDialog.isPortrait ? drawer.width : drawer.height
        open: true

        background: ColorPicker {
            id: colorPicker
            anchors.fill: parent

            colors: [
                "#290076", "#5a1a22", "#3d3106", "#113f53", "#0b4328",
                "#4100b3", "#8a2834", "#6a570c", "#1e6d8f", "#12683f",
                "#5e00ff", "#c6394a", "#ae8f13", "#2c9dce", "#1ea966",
                "#844dff", "#d77580", "#e7c127", "#68bbdf", "#38dc8c",
                "#be99ff", "#e8b0b6", "#efd56c", "#a7d7ec", "#79e7b2",
            ]

            columns : 5

            onColorChanged: {
                console.log("You selected:", color);
                trackEditDialog.color = color;
                updatePreview();
            }
        }
        MapComponent{
            id: wayPreviewMap
            showCurrentPosition: true
            anchors.fill: parent
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
