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
import QtQml.Models 2.1

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils

Page {
    id: collectionsOrderingPage

    PageHeader {
        id: pageHeader
        title: qsTr("Collection ordering")
    }

    SilicaListView {
        id: collectionListView

        property bool initialized: false

        anchors{
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            top: pageHeader.bottom
        }
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium
        clip: true

        VerticalScrollDecorator {}

        Component.onCompleted: {
            currentIndex = AppSettings.collectionsOrdering;
            initialized = true;
        }

        model: ListModel {
           ListElement { index: CollectionListModel.DateAscent; itemtext: QT_TR_NOOP("Date, ascent") }
           ListElement { index: CollectionListModel.DateDescent; itemtext: QT_TR_NOOP("Date, descent") }
           ListElement { index: CollectionListModel.NameAscent; itemtext: QT_TR_NOOP("Name, ascent") }
           ListElement { index: CollectionListModel.NameDescent; itemtext: QT_TR_NOOP("Name, descent") }
        }

        delegate: ListItem{
            id: orderingRow

            //spacing: Theme.paddingMedium
            anchors.right: parent.right
            anchors.left: parent.left

            highlighted: model.index == AppSettings.collectionsOrdering

            Label {
                id: menuLabel
                x: Theme.paddingMedium
                text: qsTr(itemtext)
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.primaryColor
            }

            onClicked: {
                console.log("clicked " + index + ": " + itemtext)
                AppSettings.collectionsOrdering = index;
                pageStack.pop();
            }
        }

    }
}
