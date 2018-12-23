/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2018  Lukas Karas

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

import harbour.osmscout.map 1.0

Map{
  id: map
  renderingType: "tiled" // plane or tiled

  TiledMapOverlay {
      anchors.fill: parent
      view: map.view
      enabled: AppSettings.hillShades
      opacity: AppSettings.hillShadesOpacity
      // If you intend to use tiles from OpenMapSurfer services in your own applications please contact us.
      // https://korona.geog.uni-heidelberg.de/contact.html
      provider: {
            "id": "ASTER_GDEM",
            "name": "Hillshade",
            "servers": [
              //"https://korona.geog.uni-heidelberg.de/tiles/asterh/x=%2&y=%3&z=%1"
              "https://osmscout.karry.cz/hillshade/tile.php?z=%1&x=%2&y=%3"
            ],
            "maximumZoomLevel": 19,
            "copyright": "© IAT, METI, NASA, NOAA",
          }
  }
}
