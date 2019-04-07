/*
 OSM Scout for Sailfish OS
 Copyright (C) 2019  Lukas Karas

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

Row {
    id: websiteRow

    property string website: ""

    visible: website != ""
    width: parent.width
    height: websiteIcon.height

    IconButton{
        id: websiteIcon
        visible: website != ""
        icon.source: "image://theme/icon-m-region"

        MouseArea{
            onClicked: Qt.openUrlExternally(website)
            anchors.fill: parent
        }
    }
    Label {
        id: websiteLabel
        anchors.left: websiteIcon.right
        anchors.verticalCenter: websiteIcon.verticalCenter
        visible: website != ""
        text: {
            // simple url normalisation for view
            var str=website.toString();

            // remove standard protocols
            if (str.indexOf("http://")===0){
                str=str.substring(7);
            }else if (str.indexOf("https://")===0){
                str=str.substring(8);
            }

            // remove last slash
            if (str.lastIndexOf("/")===(str.length-1)){
                str=str.substring(0,str.length-1);
            }
            return str;
        }

        font.underline: true
        color: Theme.highlightColor
        truncationMode: TruncationMode.Fade

        MouseArea{
            onClicked: {
                var url=website;
                if (url.indexOf("://")===-1){
                    // if there is no protocol defined, add default one to make Qt.openUrlExternally function happy
                    // https should be standard these days
                    url = "https://" + url;
                }
                console.log("opening url: " + url);
                Qt.openUrlExternally(url);
            }
            anchors.fill: parent
        }
    }
}
