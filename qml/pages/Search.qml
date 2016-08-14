/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
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

import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: searchPage
    property string searchString

    onSearchStringChanged: {
        console.log("searchString: " + searchString);
        //suggestionModel.setPattern(searchString);
    }

    SilicaListView {
        id: listView
        anchors.fill: parent

        VerticalScrollDecorator {}

        header: SearchField{
            id: searchField
            placeholderText: qsTr("Search places...")
            width: searchPage.width

            Binding {
               target: searchPage
               property: "searchString"
               value: searchField.text.toLowerCase().trim()
            }
            Component.onCompleted: {
                searchField.forceActiveFocus()
            }
        }

        LocationListModel {
            id: suggestionModel
        }

        model: suggestionModel
        delegate: ListItem {
            Label {
                text: label
                font.pixelSize: Theme.fontSizeLarge
                anchors.verticalCenter: parent.verticalCenter
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                x: Theme.paddingLarge
            }
        }
    }
}
