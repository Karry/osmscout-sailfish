/*
 OSM Scout for Sailfish OS
 Copyright (C) 2023  Lukas Karas

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
    id: collectionListPage

    signal selectWaypoint(LocationEntry waypoint)
    property var acceptDestination;

    RemorsePopup { id: remorse }

    CollectionListModel {
        id: collectionListModel
        ordering: AppSettings.collectionsOrdering
        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading);
        }
        onError: {
            remorse.execute(message, function() { }, 10 * 1000);
        }
    }

    SilicaListView {
        id: collectionListView
        anchors.fill: parent
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium
        clip: true

        currentIndex: -1 // otherwise currentItem will steal focus

        model: collectionListModel
        delegate: ListItem {
            id: collectionItem2

            ListView.onAdd: AddAnimation {
                target: collectionItem2
            }
            ListView.onRemove: RemoveAnimation {
                target: collectionItem2
            }

            Image{
                id: entryIcon

                source: model.visible ?
                            (model.visibleAll ? "image://theme/icon-m-favorite-selected" : ("image://harbour-osmscout/pics/icon-m-favorite-halfselected.png?" + Theme.primaryColor)) :
                            "image://theme/icon-m-favorite"

                width: Theme.iconSizeMedium
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter
                height: width
                x: Theme.paddingMedium
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
                console.log("selected collection: " + model.name + " (" + model.id + ")");
                collectionModel.collectionId = model.id;
                //waypointSelectorDialog.open();
                pageStack.push(waypointSelectorDialog)
            }
        }

        header: PageHeader {
            id: header
            title: qsTr("Collections")
        }

        VerticalScrollDecorator {}

        BusyIndicator {
            id: busyIndicator
            running: collectionListModel.loading
            size: BusyIndicatorSize.Large
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    CollectionModel {
        id: collectionModel

        waypointFirst: true
        showTracks: false
        ordering: AppSettings.collectionOrdering

        onLoadingChanged: {
            console.log("onLoadingChanged: " + loading + ", collection " + collectionModel.name + " (" + collectionModel.collectionId + ")");
        }
        onError: {
            remorse.execute(message, function() { }, 10 * 1000);
        }
    }

    Page{
        id: waypointSelectorDialog

        SilicaListView {
            id: collectionView
            anchors.fill: parent
            spacing: Theme.paddingMedium
            x: Theme.paddingMedium
            clip: true

            currentIndex: -1 // otherwise currentItem will steal focus

            TrackTypes {
                id: trackTypes
            }

            model: collectionModel
            delegate: ListItem {
                id: collectionItem

                ListView.onAdd: AddAnimation {
                    target: collectionItem
                }
                ListView.onRemove: RemoveAnimation {
                    target: collectionItem
                }

                Rectangle {
                    x: Theme.paddingMedium * 0.2
                    y: 0
                    height: entryIcon2.height
                    width: Theme.paddingMedium * 0.6
                    radius: width / 2

                    function waypointColor() {
                        if (model.color !== "") {
                            return model.color;
                        }
                        return "#ff0000"; // default symbol is red cyrcle (marker)
                    }

                    function routeColor() {
                        if (model.color !== "") {
                            return model.color;
                        }
                        return "#10a000"; // default route color in standard stylesheet
                    }

                    color: model.type === "waypoint" ? waypointColor() : routeColor()
                    opacity: 0.6
                }


                Image{
                    id: entryIcon2

                    source: (model.type == "waypoint" ? 'image://harbour-osmscout/poi-icons/marker.svg?' + Theme.primaryColor : trackTypes.typeIcon(model.trackType))

                    width: Theme.iconSizeMedium
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    height: width
                    x: Theme.paddingMedium

                    sourceSize.width: width
                    sourceSize.height: height

                }
                Image {
                    id: visibleIcon
                    source: "image://theme/icon-m-favorite-selected"
                    visible: model.visible

                    width: Theme.iconSizeSmall
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    height: width

                    x: Theme.paddingMedium + entryIcon2.width - width*1.0
                    y: entryIcon2.height - height*0.75

                    sourceSize.width: width
                    sourceSize.height: height
                }
                Column{

                    anchors.leftMargin: Theme.paddingMedium
                    anchors.left: entryIcon2.right
                    anchors.right: detailColumn.left
                    anchors.verticalCenter: entryIcon2.verticalCenter

                    Label {

                        width: parent.width
                        truncationMode: TruncationMode.Fade
                        textFormat: Text.StyledText
                        text: name
                    }
                    Label {

                        visible: description != ""
                        text: description.replace(/\n/g, " ")
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                        width: parent.width
                        truncationMode: TruncationMode.Fade
                    }
                }
                Column{
                    id: detailColumn
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: Theme.paddingMedium
                    //width: Math.max(sizeLabel.width, dateLabel.width) + Theme.paddingMedium

                    Label{
                        id: dateLabel
                        anchors.right: parent.right
                        text: Qt.formatDate(time)
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                        visible: Qt.formatDate(time) != ""
                    }
                    Label{
                        id: lengthLabel
                        anchors.right: parent.right
                        visible: model.distance > 0
                        text: model.distance < 0 ? "?" :Utils.humanDistance(model.distance)
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                    }
                }

                property LocationEntry waypoint;

                onClicked: {
                    selectWaypoint(model.locationObject)
                    pageStack.pop(acceptDestination);
                }
            }
            header: Column{
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

            BusyIndicator {
                running: collectionModel.loading || collectionModel.exporting
                size: BusyIndicatorSize.Large
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }

    }
}


