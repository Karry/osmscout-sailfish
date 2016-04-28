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

#ifndef OSMTILE_H
#define	OSMTILE_H

/**
 * Util class with function useful for work with OSM tiles (mercator projection)
 * as defined here: http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
 */
static const double GRAD_TO_RAD = 2 * M_PI / 360;

class OSMTile{
public:
    static inline double minLat(){
        return -85.0511;
    }
    static inline double maxLat(){
        return +85.0511;
    }
    static inline double minLon(){
        return -180.0;
    }
    static inline double maxLon(){
        return 180.0;
    }
    static inline int osmTileOriginalWidth(){
        return 256;
    }
    static inline double tileDPI(){
        return 96.0;
    }
};

#endif	/* OSMTILE_H */

