/*
 OSM Scout for Sailfish OS
 Copyright (C) 2022  Lukas Karas

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

Item {
    property var types: [
        QT_TRANSLATE_NOOP("trackType", 'Walk'),
        QT_TRANSLATE_NOOP("trackType", 'Hike'),
        QT_TRANSLATE_NOOP("trackType", 'Run'),
        QT_TRANSLATE_NOOP("trackType", 'Road bike'),
        QT_TRANSLATE_NOOP("trackType", 'Mountain bike'),
        QT_TRANSLATE_NOOP("trackType", 'Car'),
        QT_TRANSLATE_NOOP("trackType", 'Walking the dog'),
        QT_TRANSLATE_NOOP("trackType", 'Swimming'),
        QT_TRANSLATE_NOOP("trackType", 'Inline skating'),
        QT_TRANSLATE_NOOP("trackType", 'Skiing'),
        QT_TRANSLATE_NOOP("trackType", 'Nordic skiing'),
        QT_TRANSLATE_NOOP("trackType", 'Horseback riding')
        ]

    property string unknownTypeIcon: 'pics/runner'

    /*
     * This is mapping track type to icon
     */
    property variant iconMapping: {
        'Walk': 'pics/foot',
        'Hike': 'pics/hike',
        'Run': 'pics/runner',
        'Road bike': 'pics/road-bike',
        'Mountain bike': 'pics/mountain-bike',
        'Car': 'pics/car',
        'Walking the dog': 'poi-icons/dog-park',
        'Swimming': 'poi-icons/swimming',
        'Inline skating': 'pics/inline-skating',
        'Skiing': 'poi-icons/skiing',
        'Nordic skiing': 'pics/nordic-skiing',
        'Horseback riding': 'pics/horseback-riding'
    }

    function iconUrl(icon){
        return 'image://harbour-osmscout/' + icon + '.svg?' + Theme.primaryColor;
    }

    function typeIcon(type){
        if (type=="") {
            return iconUrl(unknownTypeIcon);
        }

        if (typeof iconMapping[type] === 'undefined'){
            console.log("Can't find icon for track type " + type);
            return iconUrl(unknownTypeIcon);
        }
        return iconUrl(iconMapping[type]);
    }

}
