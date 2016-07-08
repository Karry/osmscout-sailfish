
import QtQuick 2.0
import Sailfish.Silica 1.0


import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: placeDetailPage

    property double placeLat: 0.1;
    property double placeLon: 0.2;

    onStatusChanged: {
        if (status == PageStatus.Activating){
            map.showCoordinatesInstantly(placeLat, placeLon);
            map.addPositionMark(0, placeLat, placeLon);
            locationInfoModel.setLocation(placeLat, placeLon);
        }
    }

    function changePosition(lat, lon, moveMap){
        placeLat = lat;
        placeLon = lon;
        map.addPositionMark(0, placeLat, placeLon);
        locationInfoModel.setLocation(placeLat, placeLon);
        if (moveMap){
            map.showCoordinates(placeLat, placeLon);
        }
    }

    function formatCoord(lat, lon){
        // use consistent GeoCoord toString method
        return  (Math.round(lat * 100000)/100000) + " " + (Math.round(lon * 100000)/100000);
    }

    LocationInfoModel{
        id: locationInfoModel
    }

    Drawer {
        anchors.fill: parent

        dock: placeDetailPage.isPortrait ? Dock.Top : Dock.Left
        open: true

        background:  SilicaFlickable{

            anchors.fill: parent

            Column {
                id: placePanel

                width: parent.width

                spacing: Theme.paddingMedium


                Row{
                    spacing: Theme.paddingMedium
                    anchors.right: parent.right
                    Label {
                        id: placeLocationLabel
                        text: formatCoord(placeLat, placeLon)
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    IconButton{
                        icon.source: "image://theme/icon-m-clipboard"
                        onClicked: {
                            Clipboard.text = placeLocationLabel.text
                        }
                    }
                }


            }
            BusyIndicator {
                id: busyIndicator
                running: !locationInfoModel.ready
                size: BusyIndicatorSize.Large
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MapComponent {
          id: map

          anchors.fill: parent
          showCurrentPosition: true

          onTap: {
              console.log("tap: " + sceenX + "x" + screenY + " @ " + lat + " " + lon);
              changePosition(lat, lon, true);
          }
          onLongTap: {
              console.log("long tap: " + sceenX + "x" + screenY + " @ " + lat + " " + lon);
              changePosition(lat, lon, false)
          }
        }
    }
}
