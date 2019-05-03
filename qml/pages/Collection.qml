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

    RemorsePopup { id: remorse }

    function makeVisible(){
        collectionListModel.editCollection(
                    collectionModel.collectionId,
                    true,
                    collectionModel.name,
                    collectionModel.description);
    }

    onSelectTrack: {
        makeVisible();
    }
    onSelectWaypoint: {
        makeVisible();
    }

    CollectionListModel {
        id: collectionListModel
    }

    CollectionModel {
        id: collectionModel
        collectionId: collectionPage.collectionId
        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading + ", collection " + collectionModel.name + " (" + collectionModel.collectionId + ")");
        }
        onError: {
            remorse.execute(message, function() { }, 10 * 1000);
        }
    }

    SilicaListView {
        id: collectionView
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

                source: 'image://harbour-osmscout/' + (model.type == "waypoint" ? 'poi-icons/marker.svg' :'pics/route.svg') + '?' + Theme.primaryColor

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
                        console.log("Edit " + model.type + " " + model.id + " " + model.name + ", " + model.description + "...");
                        editDialog.itemId = model.id;
                        editDialog.itemType = model.type;
                        editDialog.name = model.name;
                        editDialog.description = model.description;
                        editDialog.open();
                    }
                }
                MenuItem {
                    text: qsTr("Move to")
                    onClicked: {
                        moveDialog.itemId = model.id;
                        moveDialog.itemType = model.type;
                        moveDialog.name = model.name;
                        moveDialog.description = model.description;
                        moveDialog.collectionId = collectionPage.collectionId;
                        moveDialog.open();
                    }
                }
                MenuItem {
                    text: qsTr("Route to")
                    visible: model.type == "waypoint"
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("Routing.qml"),
                                       {
                                           toLat: model.latitude,
                                           toLon: model.longitude,
                                           toName: model.name
                                       })
                    }
                }
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: {
                        Remorse.itemAction(collectionItem,
                                           qsTr("Deleting"),
                                           function() {
                                               if (model.type === "waypoint"){
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

                property ListModel directories: ListModel {}
                signal exportCollection(string directory, string name)

                onExportCollection: {
                    console.log("Exporting to file " + name + " to " + directory);
                    collectionModel.exportToFile(name, directory);
                }

                onClicked: {
                    console.log("Opening export dialog...")
                    if (directories.count == 0){
                        var dirs = collectionModel.getExportSuggestedDirectories();
                        for (var i in dirs){
                            var dir = dirs[i];
                            console.log("Suggested dir: " + dir);
                            directories.append({"dir": dir});
                        }
                    }

                    var exportPage = pageStack.push(Qt.resolvedUrl("CollectionExport.qml"),
                                   {
                                        name: collectionModel.filesystemName,
                                        directories: directories
                                   })

                    exportPage.selected.connect(exportCollection);
                }
            }
        }

        BusyIndicator {
            id: busyIndicator
            running: collectionModel.loading || collectionModel.exporting
            size: BusyIndicatorSize.Large
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    CollectionEditDialog {
        id: editDialog

        property string itemType
        property string itemId: ""
        title: itemType == "waypoint" ? qsTr("Edit waypoint"): qsTr("Edit track")

        onAccepted: {
            if (itemType == "waypoint"){
                console.log("Edit waypoint " + itemId + ": " + name + " / " + description);
                collectionModel.editWaypoint(itemId, name, description);
            }else{
                console.log("Edit track " + itemId + ": " + name + " / " + description);
                collectionModel.editTrack(itemId, name, description);
            }
            parent.focus = true;
        }
        onRejected: {
            parent.focus = true;
        }
    }

    CollectionWaypoint{
        id: waypointDialog
    }

    CollectionSelector{
        id: moveDialog

        property string itemType
        property string itemId: ""
        property string name
        property string description

        acceptText : qsTr("Move")

        canAccept : collectionId != collectionPage.collectionId

        title: qsTr("Move \"%1\" to").arg(name)

        onAccepted: {
            console.log("Move " + itemType + " id " + itemId + " to collection id " + moveDialog.collectionId);
            if (itemType === "waypoint"){
                collectionModel.moveWaypoint(itemId, moveDialog.collectionId);
            }else{
                collectionModel.moveTrack(itemId, moveDialog.collectionId);
            }
        }
    }


}
