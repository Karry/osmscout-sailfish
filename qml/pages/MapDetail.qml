/*
 OSM Scout for Sailfish OS
 Copyright (C) 2016  Lukas Karas

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
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils

Page {
    property AvailableMapsModel availableMapsModel

    property string mapName
    property variant mapItem
    property var downloadsPage

    property bool upToDate: false
    property bool updateAvailable: false
    property string updateDirectory: ""
    property variant installedTime

    MapDownloadsModel{
        id:mapDownloadsModel
    }
    InstalledMapsModel{
        id: installedMapsModel
    }

    function equalPath(a,b){
        if (typeof a===typeof b && a.length===b.length){
            for (var i=0; i<a.length; i++){
                if (a[i]!=b[i]){
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    Component.onCompleted: {
        var path=mapItem.path;
        var time=installedMapsModel.timeOfMap(path);
        installedTime=time;
        updateAvailable=false;
        upToDate = false;

        console.log("checking updates for map " + mapName + " (" + path + ")");
        if ((typeof time === "undefined") || path.length==0){
            console.log("Not installed (" + path + ")");
            return;
        }
        var latestReleaseTime=availableMapsModel.timeOfMap(path);
        console.log("map time: " + time + " latestReleaseTime: " + latestReleaseTime +" (" + typeof(latestReleaseTime) + ")");
        if (latestReleaseTime == null){
            console.log("This should not happen, map (" + path + ") is not available");
            return;
        }

        upToDate = latestReleaseTime.getTime() == time.getTime();
        updateAvailable = latestReleaseTime > time;
        if (updateAvailable){
            //console.log("Installed count: "+installedMapsModel.rowCount());
            for (var row=0; row < installedMapsModel.rowCount(); row++){
                var p=installedMapsModel.data(installedMapsModel.index(row, 0), /*path*/ 0x0101);
                var directory=installedMapsModel.data(installedMapsModel.index(row, 0), /*directory*/ 0x0102);
                //console.log("Installed "+row+": "+p+" == "+path+" = "+equalPath(p,path));
                if (equalPath(p,path)){
                    updateDirectory = directory.substring(0, directory.lastIndexOf("/"));
                    console.log("Update directory: "+updateDirectory);
                    if (destinationDirectoryComboBox.initialized){
                        var directories=mapDownloadsModel.getLookupDirectories();
                        //console.log("directories: "+directories);
                        for (var dirRow=0; dirRow < directories.length; dirRow++){
                            if (updateDirectory==directories[dirRow]){
                                destinationDirectoryComboBox.currentIndex=dirRow;
                            }
                        }
                    }
                    return;
                }
            }
        }
    }

    SilicaFlickable{
        anchors.fill: parent
        contentHeight: contentColumn.childrenRect.height

        VerticalScrollDecorator {}
        Column{
            id: contentColumn
            anchors.fill: parent

            spacing: Theme.paddingMedium

            PageHeader{
                title: mapName
            }

            Label{
                id: descriptionText

                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium

                text: mapItem.description
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
            }

            Column{
                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium
                Label{
                    text: qsTr("Size")
                    color: Theme.primaryColor
                }
                Label{
                    text: mapItem.size
                    color: Theme.highlightColor
                }
            }

            Column{
                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium
                visible: updateAvailable || upToDate
                Label{
                    text: qsTr("Downloaded")
                    color: Theme.primaryColor
                }
                Label{
                    text: Qt.formatDate(installedTime)
                    color: Theme.highlightColor
                }
            }

            Column{
                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium
                Label{
                    text: qsTr("Last Update")
                    color: Theme.primaryColor
                }
                Label{
                    text: Qt.formatDate(mapItem.time)
                    color: Theme.highlightColor
                }
            }

            Column{
                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium
                Label{
                    text: qsTr("Data Version")
                    color: Theme.primaryColor
                }
                Label{
                    text: mapItem.version
                    color: Theme.highlightColor
                }
            }

            SectionHeader{
                id: downloadMapHeader
                text: qsTr("Download")
            }
            ComboBox {
                id: destinationDirectoryComboBox

                property bool initialized: false
                property string selected: updateDirectory
                property ListModel directories: ListModel {}

                label: qsTr("Directory")
                menu: ContextMenu {
                    id: contextMenu
                    Repeater {
                        model: destinationDirectoryComboBox.directories
                        MenuItem {
                            text: Utils.humanDirectory(dir)
                        }
                    }
                }
                onCurrentItemChanged: {
                    if (!initialized){
                        return;
                    }
                    var dirs=mapDownloadsModel.getLookupDirectories();
                    selected = dirs[currentIndex];
                    //selected = directories[currentIndex].dir
                }
                Component.onCompleted: {
                    var dirs=mapDownloadsModel.getLookupDirectories();
                    for (var i in dirs){
                        var dir = dirs[i];
                        if (selected==""){
                            selected=dir;
                        }
                        console.log("Dir: "+dir);
                        directories.append({"dir": dir});
                    }
                    initialized = true;
                }
            }
            Button{
                anchors.horizontalCenter: parent.horizontalCenter
                text: upToDate ? qsTr("Up-to-date") : (updateAvailable ? qsTr("Update") : qsTr("Download"))
                enabled: !upToDate
                onClicked: {
                    var dir=mapDownloadsModel.suggestedDirectory(mapItem.map, destinationDirectoryComboBox.selected);
                    mapDownloadsModel.downloadMap(mapItem.map, dir);
                    console.log("downloading to " + dir);
                    pageStack.pop(downloadsPage);
                }
            }

        }

    }
}
