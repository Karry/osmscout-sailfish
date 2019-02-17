/*
 OSM Scout for Sailfish OS
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
import QtPositioning 5.2

import Sailfish.Silica 1.0

import "pages"
import harbour.osmscout.map 1.0

ApplicationWindow {
    id: mainWindow

    initialPage: Component {
        MapPage{
        }
    }

    cover: Component {
        Cover{
        }
    }

    allowedOrientations: Orientation.All
    _defaultPageOrientations: Orientation.All
}
