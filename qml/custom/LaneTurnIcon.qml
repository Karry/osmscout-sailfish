/*
 OSM Scout for Sailfish OS
 Copyright (C) 2020 Lukas Karas

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

Image{
    id: laneTurnIcon

    property string turnType: 'unknown'
    property string unknownTypeIcon: 'empty'
    property int roundaboutExit: -1
    property bool outline: false

    /*
     * This is mapping libosmscout route step types and step icons.
     */
    property variant iconMapping: {
        '': 'empty',
        'left': 'left',
        'slight_left': 'left',
        'merge_to_left': 'left',
        'through;left': 'through_left',
        'through;slight_left': 'through_left',
        'through;sharp_left': 'through_left',
        'through': 'through',
        'through;right': 'through_right',
        'through;slight_right': 'through_right',
        'through;sharp_right': 'through_right',
        'right': 'right',
        'slight_right': 'right',
        'merge_to_right': 'right'
    }

    function iconUrl(icon){
        return 'image://harbour-osmscout/laneturn/' + icon + (outline ? '_outline' : '') + '.svg?' + Theme.primaryColor;
    }

    function typeIcon(type){
        console.log("icon type: " + type);
        if (typeof iconMapping[type] === 'undefined'){
            console.log("Can't find icon for type " + type);
            return iconUrl(unknownTypeIcon);
        }
        return iconUrl(iconMapping[type]);
    }

    source: typeIcon(turnType)

    fillMode: Image.PreserveAspectFit
    horizontalAlignment: Image.AlignHCenter
    verticalAlignment: Image.AlignVCenter

    sourceSize.width: width
    sourceSize.height: height

}
