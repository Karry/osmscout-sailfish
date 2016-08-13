
import QtQuick 2.0
import Sailfish.Silica 1.0

import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"

CoverBackground {
    id: cover
    /*
    CoverPlaceholder {
        //% "OSM Scout"
        text: qsTr("OSM Scout")
        icon.source: "image://theme/harbour-osmscout"
    }
    */
    Settings {
        id: settings
    }
    property bool initialized: false;
    onStatusChanged: {
        if (status == PageStatus.Activating){
            if (!initialized){
                map.view = settings.mapView;
                initialized = true;
            }
            map.lockToPosition = true;
        }
    }
    PositionSource {
        id: positionSource

        active: true

        property bool valid: false;
        property double lat: 0.0;
        property double lon: 0.0;

        onPositionChanged: {
            positionSource.valid = position.latitudeValid && position.longitudeValid;
            positionSource.lat = position.coordinate.latitude;
            positionSource.lon = position.coordinate.longitude;

            map.locationChanged(
               position.latitudeValid && position.longitudeValid,
               position.coordinate.latitude, position.coordinate.longitude,
               position.horizontalAccuracyValid, position.horizontalAccuracy);
        }
    }
    OpacityRampEffect {
        enabled: true
        offset: 1. - (header.height + Theme.paddingLarge) / map.height
        slope: map.height / Theme.paddingLarge / 3.
        direction: 3
        sourceItem: map
    }
    Rectangle{
        id: header

        height: icon.height + 2* Theme.paddingMedium
        x: Theme.paddingMedium

        Image{
            id: icon
            source: "image://theme/harbour-osmscout"
            x: 0
            y: Theme.paddingMedium
            height: Theme.fontSizeMedium * 1.5
            width: height
        }
        Label{
            id: headerText
            anchors{
                verticalCenter: parent.verticalCenter
                left: icon.right
                leftMargin: Theme.paddingSmall
            }
            text: qsTr("OSM Scout")
            font.pixelSize: Theme.fontSizeMedium
        }
    }
    Map {
        id: map

        focus: true
        anchors.fill: parent

        showCurrentPosition: true
        lockToPosition: true
    }
    CoverActionList {
        Timer {
            id: bindToCurrentPositionTimer
            interval: 600 // zoom duration
            running: false
            repeat: false
            onTriggered: {
                map.lockToPosition = true;
            }
        }

        enabled: true
        iconBackground: true
        CoverAction {
            iconSource: "file:///usr/share/harbour-osmscout/pics/icon-cover-remove.png"
            onTriggered: {
                map.zoomOut(2.0)
                bindToCurrentPositionTimer.restart();
            }
        }
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                map.zoomIn(2.0)
                bindToCurrentPositionTimer.restart();
            }
        }
    }
}
