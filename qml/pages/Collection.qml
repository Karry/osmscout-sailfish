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
import Sailfish.Share 1.0
import QtPositioning 5.2
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils

Page {
    id: collectionPage
    property string collectionId: "-1"
    signal selectWaypoint(double lat, double lon, var waypointId)
    signal selectTrack(LocationEntry bbox, var trackId)
    property var acceptDestination

    RemorsePopup { id: remorse }

    function makeVisible(){
        collectionListModel.editCollection(
                    collectionModel.collectionId,
                    true,
                    collectionModel.name,
                    collectionModel.description);
        AppSettings.showCollections = true;
    }

    onSelectTrack: {
        makeVisible();
        collectionModel.setTrackVisibility(trackId, true);
    }
    onSelectWaypoint: {
        makeVisible();
        collectionModel.setWaypointVisibility(waypointId, true);
    }

    CollectionListModel {
        id: collectionListModel
    }

    CollectionModel {
        id: collectionModel
        collectionId: collectionPage.collectionId

        waypointFirst: AppSettings.waypointFirst
        ordering: AppSettings.collectionOrdering

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

            Rectangle {
                x: Theme.paddingMedium * 0.2
                y: 0
                height: entryIcon.height
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
            Image {
                id: visibleIcon
                source: "image://theme/icon-m-favorite-selected"
                visible: model.visible

                width: Theme.iconSizeSmall
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter
                height: width

                x: Theme.paddingMedium + entryIcon.width - width*1.0
                y: entryIcon.height - height*0.75

                sourceSize.width: width
                sourceSize.height: height
            }
            MouseArea{
                anchors.left: entryIcon.left
                anchors.top: entryIcon.top
                anchors.right: visibleIcon.right
                anchors.bottom: visibleIcon.bottom
                onClicked: {
                    console.log("Changing entry (" + model.id + ") visibility to: " + !model.visible);
                    if (model.type == "waypoint"){
                        collectionModel.setWaypointVisibility(model.id, !model.visible);
                    } else {
                        collectionModel.setTrackVisibility(model.id, !model.visible);
                    }
                }
            }
            Column{
                id: entryDescription
                x: Theme.paddingMedium
                anchors.left: entryIcon.right
                anchors.right: detailColumn.left
                anchors.verticalCenter: entryIcon.verticalCenter

                Label {
                    id: nameLabel

                    width: parent.width
                    truncationMode: TruncationMode.Fade
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
            onClicked: {
                if (model.type == "waypoint"){
                    waypointDialog.id = model.id;
                    waypointDialog.name = model.name;
                    waypointDialog.description = model.description;
                    waypointDialog.latitude = model.latitude;
                    waypointDialog.longitude = model.longitude;
                    waypointDialog.time = model.time;
                    waypointDialog.elevation = model.elevation;

                    waypointDialog.previewMap.removeAllOverlayObjects();
                    waypointDialog.previewMap.showCoordinatesInstantly(model.latitude, model.longitude);

                    console.log("Creating overlay node type: " + model.waypointType);
                    var wpt = waypointDialog.previewMap.createOverlayNode(model.waypointType);
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
                    //: track/waypoint context menu
                    text: qsTr("Edit")
                    onClicked: {
                        console.log("Edit " + model.type + " " + model.id + " " + model.name + ", " + model.description + "...");
                        if (model.type == "waypoint"){
                            editDialog.itemId = model.id;
                            editDialog.itemType = model.type;
                            editDialog.name = model.name;
                            editDialog.description = model.description;
                            editDialog.symbolSelectorVisible = true;
                            editDialog.symbol = model.symbol;
                            editDialog.open();
                        } else {
                            editTrackDialog.itemId = model.id;
                            editTrackDialog.itemType = model.type;
                            editTrackDialog.name = model.name;
                            editTrackDialog.description = model.description;
                            editDialog.symbolSelectorVisible = false;
                            pageStack.push(editTrackDialog)
                        }
                    }
                }
                MenuItem {
                    //: track/waypoint context menu
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
                    //: waypoint context menu
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
                    //: track context menu
                    text: qsTr("Export")
                    visible: model.type == "track"


                    property ListModel directories: ListModel {}
                    signal exportTrack(string directory, string name, bool includeWaypoints, int accuracyFilter)

                    onExportTrack: {
                        console.log("Exporting to file " + name + " to " + directory);
                        collectionModel.exportTrackToFile(model.id, name, directory, includeWaypoints, accuracyFilter);
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
                                            name: model.filesystemName,
                                            directories: directories,
                                            includeWaypoints: false
                                       })

                        exportPage.selected.connect(exportTrack);
                    }
                }
                MenuItem {
                    //: track context menu
                    text: qsTr("Share")

                    ShareAction {
                        id: shareAction
                        mimeType: "text/xml"
                    }

                    signal exported(int id, string filePath)
                    property ListModel directories: ListModel {}
                    signal exportTrack(string directory, string name, bool includeWaypoints, int accuracyFilter)

                    onExported: {
                        if (id != model.id && model.type == "track") {
                            return;
                        }
                        console.log("Track " + id + " exported to " + filePath);

                        shareAction.mimeType = "text/xml"
                        // TODO: who will delete temporary file?
                        shareAction.resources = [filePath]
                        shareAction.trigger()
                    }
                    onExportTrack: {
                        console.log("Exporting to file " + name + " to " + directory);
                        collectionModel.trackExported.connect(exported);
                        collectionModel.exportTrackToFile(model.id, name, directory, includeWaypoints, accuracyFilter);
                    }

                    onClicked: {
                        if (model.type == "track") {
                            console.log("Opening export dialog...")
                            directories.append({"dir": "/tmp"});

                            var exportPage = pageStack.push(Qt.resolvedUrl("CollectionExport.qml"),
                                           {
                                                name: model.filesystemName,
                                                directories: directories,
                                                includeWaypoints: false,
                                                selectDirectory: false
                                           })

                            exportPage.selected.connect(exportTrack);
                        } else { // waypoint
                            var mimeType = "text/x-url";
                            var placeLink = Utils.shareLink(model.latitude, model.longitude);

                            var status = model.name + (model.description === "" ? "": "\n" + model.description) + "\n" + placeLink;
                            var linkTitle = model.name;
                            var content = {
                                "data": placeLink,
                                "type": mimeType
                            }

                            // also some non-standard fields for Twitter/Facebook status sharing:
                            content["status"] = status;
                            content["linkTitle"] = linkTitle;

                            shareAction.resources = [content]
                            shareAction.mimeType = mimeType
                            shareAction.trigger()
                        }
                    }
                }
                MenuItem {
                    //: track/waypoint context menu
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
                //: collection pull down menu
                text: qsTr("Export")

                property ListModel directories: ListModel {}
                signal exportCollection(string directory, string name, bool includeWaypoints, int accuracyFilter)

                onExportCollection: {
                    console.log("Exporting to file " + name + " to " + directory);
                    collectionModel.exportToFile(name, directory, includeWaypoints, accuracyFilter);
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
                                        directories: directories,
                                        includeWaypoints: true
                                   })

                    exportPage.selected.connect(exportCollection);
                }
            }
            MenuItem {
                //: collection pull down menu
                text: qsTr("Share")

                ShareAction {
                    id: collectionShareAction
                    mimeType: "text/xml"
                }

                signal exported(int id, string filePath)
                property ListModel directories: ListModel {}
                signal exportCollection(string directory, string name, bool includeWaypoints, int accuracyFilter)

                onExported: {
                    console.log("Collection " + id + " exported to " + filePath);
                    if (id != collectionModel.collectionId) {
                        return;
                    }

                    collectionShareAction.mimeType = "text/xml"
                    // TODO: who will delete temporary file?
                    collectionShareAction.resources = [filePath]
                    collectionShareAction.trigger()
                }
                onExportCollection: {
                    console.log("Exporting to file " + name + " to " + directory);
                    collectionModel.exported.connect(exported);
                    collectionModel.exportToFile(name, directory, includeWaypoints, accuracyFilter);
                }

                onClicked: {
                    console.log("Opening export dialog...")
                    directories.append({"dir": "/tmp"});

                    var exportPage = pageStack.push(Qt.resolvedUrl("CollectionExport.qml"),
                                   {
                                        name: collectionModel.filesystemName,
                                        directories: directories,
                                        includeWaypoints: true,
                                        selectDirectory: false
                                   })

                    exportPage.selected.connect(exportCollection);
                }
            }
            MenuItem {
                //: collection pull down menu
                text: qsTr("Order by...")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("CollectionOrdering.qml"));
                }
            }
            MenuItem {
                //: collection pull down menu
                text: qsTr("Edit")
                onClicked: {
                    editDialog.itemType = "collection";
                    editDialog.itemId = collectionModel.collectionId;
                    editDialog.name = collectionModel.name;
                    editDialog.description = collectionModel.description;
                    editDialog.open();
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

    Page{
        id: editTrackDialog

        property string itemType
        property string itemId: ""
        property string name
        property string description

        SilicaListView {
            interactive: true
            anchors.fill: parent
            height: childrenRect.height
            id: menu

            VerticalScrollDecorator {}

            model: ListModel {
                //: track edit menu
                ListElement { itemtext: QT_TR_NOOP("Name and description");  itemicon: "image://theme/icon-m-edit"; action: "rename";}
                //: track edit menu
                ListElement { itemtext: QT_TR_NOOP("Color");                 itemicon: "image://theme/icon-m-ambience"; action: "color";}
                //: track edit menu
                ListElement { itemtext: QT_TR_NOOP("Crop start");            itemicon: "image://theme/icon-m-crop"; action: "crop-start";}
                //: track edit menu
                ListElement { itemtext: QT_TR_NOOP("Crop end");              itemicon: "image://theme/icon-m-crop"; action: "crop-end";}
                //: track edit menu
                ListElement { itemtext: QT_TR_NOOP("Split");                 itemicon: "image://theme/icon-m-flip"; action: "split";}
                //: track edit menu
                ListElement { itemtext: QT_TR_NOOP("Drop inaccurate nodes"); itemicon: "image://theme/icon-m-reset"; action: "filter";}
            }

            delegate: ListItem{
                id: searchRow

                function onAction(action){
                    if (action == "rename"){
                        editDialog.itemId = editTrackDialog.itemId;
                        editDialog.itemType = editTrackDialog.itemType;
                        editDialog.name = editTrackDialog.name;
                        editDialog.description = editTrackDialog.description;
                        editDialog.acceptDestination = collectionPage;
                        editDialog.acceptDestinationAction = PageStackAction.Pop;
                        editDialog.open();
                    } else if (action == "color"){
                        pageStack.push(Qt.resolvedUrl("TrackColor.qml"),
                                       {
                                            trackId: editTrackDialog.itemId,
                                            acceptPage: collectionPage
                                       });
                    } else if (action == "filter"){
                        pageStack.push(Qt.resolvedUrl("TrackFilter.qml"),
                                       {
                                            trackId: editTrackDialog.itemId,
                                            acceptPage: collectionPage
                                       });
                    } else {
                        pageStack.push(Qt.resolvedUrl("TrackEdit.qml"),
                                       {
                                            trackId: editTrackDialog.itemId,
                                            acceptPage: collectionPage,
                                            action: action
                                       });
                    }
                }

                //spacing: Theme.paddingMedium
                anchors.right: parent.right
                anchors.left: parent.left
                IconButton {
                    id: menuIcon
                    icon.source: itemicon

                    icon.fillMode: Image.PreserveAspectFit
                    icon.sourceSize.width: Theme.iconSizeMedium
                    icon.sourceSize.height: Theme.iconSizeMedium

                    enabled: isEnabled(action)
                    onClicked: onAction(action)
                }

                Label {
                    id: menuLabel
                    anchors.left: menuIcon.right
                    text: qsTr(itemtext)
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.primaryColor
                }

                onClicked: onAction(action)
            }

            header: PageHeader {
                title: qsTr("Edit track \"%1\"").arg(editTrackDialog.name);
            }
        }
    }

    CollectionEditDialog {
        id: editDialog

        property string itemType
        property string itemId: ""
        title: itemType == "waypoint" ? qsTr("Edit waypoint"): (itemType === "track" ? qsTr("Edit track") : qsTr("Edit collection"))

        onAccepted: {
            if (itemType == "waypoint"){
                console.log("Edit waypoint " + itemId + ": " + name + " / " + description);
                collectionModel.editWaypoint(itemId, name, description, symbol);
            }else if (itemType === "track"){
                console.log("Edit track " + itemId + ": " + name + " / " + description);
                collectionModel.editTrack(itemId, name, description);
            } else { // collection
                console.log("Edit collection " + itemId + ": " + name + " / " + description);
                collectionListModel.editCollection(itemId, collectionModel.collectionVisible, name, description);
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
