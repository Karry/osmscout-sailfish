import QtQuick 2.0
import Sailfish.Silica 1.0

import QtPositioning 5.2

import harbour.osmscout.map 1.0

Map {
  id: map

  PositionSource {
      id: positionSource
      active: true

      onPositionChanged: {
          map.locationChanged(
                      position.latitudeValid && position.longitudeValid,
                      position.coordinate.latitude, position.coordinate.longitude,
                      position.horizontalAccuracyValid, position.horizontalAccuracy);
      }
  }

  MapRenderingIndicator{
      id : renderProgress
      anchors{
          left: parent.left
          top: parent.top
      }

      zoomLevel: map.zoomLevel
      finished: map.finished
  }

  ScaleIndicator{
    pixelSize: map.pixelSize
    width: Math.min(parent.width / 2, 270) - Theme.paddingMedium
    x: Theme.paddingMedium
    opacity: 0.6
    anchors{
        bottom: osmCopyright.top
        bottomMargin: Theme.paddingSmall
    }
  }

  OSMCopyright{
      id : osmCopyright
      anchors{
          left: parent.left
          bottom: parent.bottom
      }
  }
}
