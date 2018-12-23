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

MapBase {
  id: map

  property double topMargin:0

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
          topMargin: map.topMargin
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
