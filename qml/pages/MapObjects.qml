
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

            Row {
                POIIcon{
                    id: poiIcon
                    poiType: (typeof label=="undefined")?"":label
                    width: 64
                    height: 64
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
