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

      magLevel: map.magLevel
      finished: map.finished
  }
  OSMCopyright{
      id : osmCopyright
      anchors{
          left: parent.left
          bottom: parent.bottom
      }
  }
}
