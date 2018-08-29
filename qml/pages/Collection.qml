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

Page {
    id: collectionPage
    property string collectionId: "-1"
    signal selectWaypoint(double lat, double lon)
    signal selectTrack(LocationEntry bbox, var trackId);
    property var acceptDestination;

    CollectionModel {
        id: collectionModel
        collectionId: collectionPage.collectionId
        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading);
        }
    }

    SilicaListView {
        id: collectionListView
        anchors.fill: parent
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium
        clip: true

        currentIndex: -1 // otherwise currentItem will steal focus

        model: collectionModel
        delegate: ListItem {
            id: collectionItem

            ListView.onAdd: AddAnimation {
                target: collectionItem
            }
            ListView.onRemove: RemoveAnimation {
                target: collectionItem
            }

            Image{
                id: entryIcon

                source: model.type == "waypoint" ?
                            '../../poi-icons/marker.svg' :
                            "../../pics/route.svg"

                width: Theme.iconSizeMedium
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter
                height: width
                x: Theme.paddingMedium

                sourceSize.width: width
                sourceSize.height: height
            }
            Column{
                id: entryDescription
                x: Theme.paddingMedium
                anchors.left: entryIcon.right
                anchors.right: parent.right
                anchors.verticalCenter: entryIcon.verticalCenter

                Label {
                    id: nameLabel

                    width: parent.width
                    textFormat: Text.StyledText
                    text: name
                }
                Label {
                    id: descriptionLabel

                    visible: description != ""
                    text: description.replace(/\n/g, " ")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                }
            }
            onClicked: {
                if (model.type == "waypoint"){
                    waypointDialog.name = model.name;
                    waypointDialog.description = model.description;
                    waypointDialog.latitude = model.latitude;
                    waypointDialog.longitude = model.longitude;

                    waypointDialog.previewMap.removeAllOverlayObjects();
                    waypointDialog.previewMap.showCoordinatesInstantly(model.latitude, model.longitude);

                    var wpt = waypointDialog.previewMap.createOverlayNode("_waypoint");
                    wpt.addPoint(model.latitude, model.longitude);
                    wpt.name = model.name;
                    waypointDialog.previewMap.addOverlayObject(0, wpt);
                    waypointDialog.open();
                }else{
                    var wayPage = pageStack.push(Qt.resolvedUrl("CollectionTrack.qml"),
                                   {
                                        trackId: model.id,
                                        acceptPage: acceptDestination
                                   })

                    wayPage.selectTrack.connect(selectTrack);
                }
            }
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Edit")
                    onClicked: {
                        console.log("TODO: Edit " + model.type + " " + model.id + " " + model.name + ", " + model.description + "...");
                    }
                }
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: {
                        var idx = model.id
                        Remorse.itemAction(collectionItem,
                                           qsTr("Deleting"),
                                           function() {
                                               if (model.type == "waypoint"){
                                                   collectionModel.deleteWaypoint(model.id);
                                               }else{
                                                   collectionModel.deleteTrack(model.id);
                                               }
                                           });
                    }
                }
            }
        }

        header: Column{
            id: header
            width: parent.width

            PageHeader {
                title: collectionModel.loading ? qsTr("Loading collection") : collectionModel.name;
            }
            Label {
                text: collectionModel.description
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

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                text: qsTr("Export")
                onClicked: {
                    console.log("TODO: Export")
                }
            }
        }

        BusyIndicator {
            id: busyIndicator
            running: collectionModel.loading
            size: BusyIndicatorSize.Large
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    CollectionWaypoint{
        id: waypointDialog
    }
}
