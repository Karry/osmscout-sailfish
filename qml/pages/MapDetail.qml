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
    property variant installedDirectory
    property variant installedSize
    property variant installedVersion

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

    function updateDownloadDir(){
        console.log("Download directory: " + updateDirectory);

        var dirFound = false;
        if (destinationDirectoryComboBox.initialized){
            var directories=mapDownloadsModel.getLookupDirectories();
            console.log("directories: " + directories);
            for (var dirRow=0; dirRow < directories.length; dirRow++){
                if (updateDirectory==directories[dirRow]){
                    destinationDirectoryComboBox.currentIndex=dirRow;
                    dirFound = true;
                    break;
                }
            }
        }

        if (!dirFound){
            console.log("Not present: " + updateDirectory);
            updateDirectory = "";
        }
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
            // if this map is not installed yet, use last directory
            updateDirectory = AppSettings.lastMapDirectory
        } else {
            // try to evaluate update directory
            var latestReleaseTime=availableMapsModel.timeOfMap(path);
            console.log("map time: " + time + " latestReleaseTime: " + latestReleaseTime +" (" + typeof(latestReleaseTime) + ")");
            if (latestReleaseTime == null){
                console.log("This should not happen, map (" + path + ") is not available");
                return;
            }

            upToDate = latestReleaseTime.getTime() == time.getTime();
            updateAvailable = latestReleaseTime > time;

            //console.log("Installed count: "+installedMapsModel.rowCount());
            for (var row=0; row < installedMapsModel.rowCount(); row++){
                var index = installedMapsModel.index(row, 0);
                var p=installedMapsModel.data(index, InstalledMapsModel.PathRole);

                //console.log("Installed "+row+": "+p+" == "+path+" = "+equalPath(p,path));
                if (equalPath(p,path)){
                    var directory = installedMapsModel.data(index, InstalledMapsModel.DirectoryRole);
                    installedDirectory = directory;
                    installedSize = installedMapsModel.data(index, InstalledMapsModel.SizeRole);
                    installedVersion = installedMapsModel.data(index, InstalledMapsModel.VersionRole);
                    updateDirectory = directory.substring(0, directory.lastIndexOf("/"));
                    break;
                }
            }
        }

        // uff, seems that there is some race condition and I cannot call
        // updateDownloadDir(); function here, but have to postpone it
        var timer = Qt.createQmlObject("import QtQuick 2.0; Timer {}", root);
        timer.interval = 1;
        timer.repeat = false;
        timer.triggered.connect(updateDownloadDir)
        timer.start();
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
                visible: mapItem.description !== ""
            }


            SectionHeader {
                visible: updateAvailable || upToDate
                text: qsTr("Downloaded version")
            }
            Label {
                visible: updateAvailable || upToDate
                width: parent.width - 2*Theme.paddingMedium
                x: Theme.paddingMedium
                text: Utils.humanDirectory(installedDirectory)
                color: Theme.highlightColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
            }
            DetailItem {
                visible: updateAvailable || upToDate
                label: qsTr("Date")
                value: Qt.formatDate(installedTime)
            }
            DetailItem {
                visible: updateAvailable || upToDate
                label: qsTr("Size")
                value: installedSize
            }
            DetailItem {
                visible: updateAvailable || upToDate
                label: qsTr("Data version")
                value: installedVersion
            }

            SectionHeader{
                text: qsTr("Available version")
            }
            DetailItem {
                label: qsTr("Date")
                value: Qt.formatDate(mapItem.time)
            }
            DetailItem {
                label: qsTr("Size")
                value: mapItem.size
            }
            DetailItem {
                label: qsTr("Data Version")
                value: mapItem.version
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
                        console.log("NOT initialized");
                        return;
                    }
                    var dirs=mapDownloadsModel.getLookupDirectories();
                    selected = dirs[currentIndex];
                    //selected = directories[currentIndex].dir
                    console.log("changed, currentIndex=" + destinationDirectoryComboBox.currentIndex + " selected: " + selected);
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
                    console.log("initialized, currentIndex=" + destinationDirectoryComboBox.currentIndex);
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
                    AppSettings.lastMapDirectory = destinationDirectoryComboBox.selected;
                    console.log("AppSettings.lastMapDirectory = " + AppSettings.lastMapDirectory);
                    pageStack.pop(downloadsPage);
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
