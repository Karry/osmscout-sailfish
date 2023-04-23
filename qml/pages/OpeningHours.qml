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
import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils

Page {
    id: openingHoursPage

    property string name: ""
    property string type: ""
    property alias openingHours: objectTypeModel.openingHours

    OpeningHoursModel {
        id: objectTypeModel

        property string todayStr: objectTypeModel.today.join(", ")

        onParseError: {
            console.log("openingHours parse error: " + openingHours);
        }
    }
    Settings {
        id: settings
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        spacing: Theme.paddingMedium
        x: Theme.paddingMedium
        clip: true
        currentIndex: -1 // otherwise currentItem will steal focus
        model: objectTypeModel

        VerticalScrollDecorator {
        }

        header: Column {
            PageHeader {
                id: header

                width: listView.width
                title: qsTr("Opening hours")
            }

            Row {
                POIIcon {
                    id: poiIcon

                    poiType: openingHoursPage.type
                    width: Theme.iconSizeMedium
                    height: Theme.iconSizeMedium
                }
                Label {
                    width: listView.width - poiIcon.width - (2 * Theme.paddingSmall)
                    text: openingHoursPage.name
                    font.pixelSize: Theme.fontSizeExtraLarge
                }
            }
            Text {
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
                text: objectTypeModel.openingHours
                visible: objectTypeModel.rowCount()==0 // parsing failed
                color: Theme.secondaryHighlightColor
                textFormat: Text.PlainText
                anchors {
                    left: parent.left
                    right: parent.right
                    topMargin: Theme.paddingSmall
                    leftMargin: Theme.paddingSmall
                    rightMargin: Theme.paddingSmall
                }
            }
        }

        delegate: BackgroundItem {
            id: item

            highlighted: isToday

            Text {
                id: labelText

                y: Theme.paddingSmall
                width: Math.max(parent.width*0.25, 300)
                anchors {
                    left: parent.left
                    rightMargin: Theme.paddingSmall
                }
                horizontalAlignment: Text.AlignRight
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeMedium
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                text: model.day
            }

            Text {
                id: valueText

                y: Theme.paddingSmall
                x: labelText.width + Theme.paddingSmall
                width: parent.width-(x+Theme.paddingSmall)
                horizontalAlignment: Text.AlignLeft
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeMedium
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                text: model.timeIntervals.join(", ")
            }
        }
    }
}
