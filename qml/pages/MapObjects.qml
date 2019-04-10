/*
 OSM Scout for Sailfish OS
 Copyright (C) 2017 Lukas Karas

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
    id: mapObjects

    property var view;
    property point screenPosition;
    property int mapWidth;
    property int mapHeight;

    onStatusChanged: {
        if (status == PageStatus.Activating){
            // show objects on view center
            // TODO: show objects on place mark
            mapObjectInfo.setPosition(view,
                                      mapWidth, mapHeight,
                                      screenPosition.x, screenPosition.y);
        }
    }


    MapObjectInfoModel{
        id: mapObjectInfo
    }

    SilicaListView {
        id: mapObjectInfoView
        model: mapObjectInfo
        anchors.fill: parent

        VerticalScrollDecorator {}
        clip: true

        delegate: Column{
            spacing: Theme.paddingSmall

            // model roles:
            //  label: object type (node / way / area)
            //  type:  database type (amenity_hospital, parking...)
            //  id:    file offset osed for unique identification (with object type and database instance)
            //  name
            //  object: overlay object
            //  phone
            //  website
            //  lat
            //  lon
            Row {
                POIIcon{
                    id: poiIcon
                    poiType: (typeof type=="undefined")?"":type
                    width: Theme.iconSizeMedium
                    height: Theme.iconSizeMedium
                }
                Column{
                    Row{
                        spacing: Theme.paddingSmall
                        Label {
                            id: typeLabel
                            text: (typeof type=="undefined")?"":qsTranslate("databaseType", type)
                        }
                        Label {
                            id: labelLabel
                            color: Theme.highlightColor
                            text: (typeof label=="undefined")?"":qsTranslate("objectType", label)
                        }
                    }
                    Label {
                        id: nameLabel
                        text: {
                            if (typeof name=="undefined"){
                                return "";
                            }else{
                                return "\""+name+"\"";
                            }
                        }
                    }
                    Label {
                        id: idLabel
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                        text: ""+id+""
                    }
              }
            }
        }
    }

    BusyIndicator {
        id: busyIndicator
        running: !mapObjectInfo.ready
        size: BusyIndicatorSize.Large
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
}
