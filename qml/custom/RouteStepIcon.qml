/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2018 Lukas Karas

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

Image{
    id: routeStepIcon

    property string stepType: 'unknown'
    property string unknownTypeIcon: 'information'

    /*
     * This is mapping libosmscout route step types and step icons.
     */
    property variant iconMapping: {
        'information': 'information',

        'start': 'start',
        'drive-along': 'drive-along',
        'target': 'target',

        'turn': 'turn',
        'turn-sharp-left': 'turn-sharp-left',
        'turn-left': 'turn-left',
        'turn-slightly-left': 'turn-slightly-left',
        'continue-straight-on': 'continue-straight-on',
        'turn-slightly-right': 'turn-slightly-right',
        'turn-right': 'turn-right',
        'turn-sharp-right': 'turn-sharp-right',

        'enter-roundabout': 'enter-roundabout',
        'leave-roundabout': 'leave-roundabout',

        'enter-motorway': 'change-motorway',
        'change-motorway': 'change-motorway',
        'leave-motorway': 'leave-motorway',

        'name-change': 'information'
    }

    function iconUrl(icon){
        return 'image://harbour-osmscout/routestep/' + icon + '.svg?' + Theme.primaryColor;
    }

    function typeIcon(type){
      if (typeof iconMapping[type] === 'undefined'){
          console.log("Can't find icon for type " + type);
          return iconUrl(unknownTypeIcon);
      }
      return iconUrl(iconMapping[type]);
    }

    source: typeIcon(stepType)

    fillMode: Image.PreserveAspectFit
    horizontalAlignment: Image.AlignHCenter
    verticalAlignment: Image.AlignVCenter

    sourceSize.width: width
    sourceSize.height: height
}
