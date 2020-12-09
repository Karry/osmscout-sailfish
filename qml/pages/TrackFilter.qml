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

    property var acceptPage;
    property var trackId;
    property int accuracyFilter: -1

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

    canAccept: !trackModel.loading && accuracyFilter > 0
    onAccepted: {
        // TODO: implement filtering in c++
        // trackModel.filterNodes(accuracyFilter);
    }

    CollectionTrackModel{
        id: trackModel
        trackId: trackEditDialog.trackId

        accuracyFilter: trackEditDialog.accuracyFilter

        onBoundingBoxChanged: {
            console.log("bounding box changed");
            wayPreviewMap.showLocation(trackModel.boundingBox);
        }

        onLoadingChanged: {
            console.log("loading changed: " + loading+ " segments: " + trackModel.segmentCount);
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
        id: trackEditDialogheader
        title: trackModel.name
        acceptText: qsTr("Drop nodes")
        //cancelText : qsTr("Cancel")
        spacing: trackEditDialog.isPortrait ? Theme.paddingLarge : 0
    }

    ComboBox {
        id: accuracyComboBox
        width: parent.width

        anchors{
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }

        enabled: !trackModel.loading

        property bool initialized: false

        //: track edit
        label: qsTr("Drop inaccurate nodes")
        menu: ContextMenu {
            //: option dropping inaccurate nodes (track edit)
            MenuItem { text: qsTr("> 100 m") }
            //: option dropping inaccurate nodes (track edit)
            MenuItem { text: qsTr("> 30 m") }
            //: option dropping inaccurate nodes (track edit)
            MenuItem { text: qsTr("> 20 m") }
            //: option dropping inaccurate nodes (track edit)
            MenuItem { text: qsTr("> 15 m") }
            //: option dropping inaccurate nodes (track edit)
            MenuItem { text: qsTr("> 10 m") }
        }
        onCurrentItemChanged: {
            if (!initialized){
                return;
            }

            trackEditDialog.accuracyFilter = 100;
            if (currentIndex == 1){
                trackEditDialog.accuracyFilter = 30;
            } else if (currentIndex == 2){
                trackEditDialog.accuracyFilter = 20;
            } else if (currentIndex == 3){
                trackEditDialog.accuracyFilter = 15;
            } else if (currentIndex == 4){
                trackEditDialog.accuracyFilter = 10;
            }
            console.log("set accuracyFilter to " + trackEditDialog.accuracyFilter);
        }
        Component.onCompleted: {
            initialized = true;
        }
        onPressAndHold: {
            // improve default ComboBox UX :-)
            clicked(mouse);
        }
    }
    MapComponent{
        id: wayPreviewMap
        showCurrentPosition: true
        anchors{
            top: trackEditDialogheader.bottom
            right: parent.right
            left: parent.left
            bottom: accuracyComboBox.top
            bottomMargin: trackEditDialog.isPortrait ? Theme.paddingLarge : 0
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
