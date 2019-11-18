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

.pragma library

// variables from setting are synchronised from Global singleton
var distanceUnits = "metrics";
var gpsFormat = "numeric"

function formatDegree(degree){
    var minutes = (degree - Math.floor(degree)) * 60;
    var seconds = (minutes - Math.floor(minutes )) * 60;
    return Math.floor(degree) + "°"
        + (minutes<10?"0":"") + Math.floor(minutes) + "'"
        + (seconds<10?"0":"") + seconds.toFixed(2) + "\"";
}
function formatDegreeLikeGeocaching(degree){
    var minutes = (degree - Math.floor(degree)) * 60;
    return Math.floor(degree) + "°"
          + (minutes<10?"0":"") + minutes.toFixed(3) + "'"
}

function formatCoord(lat, lon, format){
    if (format === "geocaching"){
        return (lat>0? "N":"S") +" "+ formatDegreeLikeGeocaching( Math.abs(lat) ) + " " +
                (lon>0? "E":"W") +" "+ formatDegreeLikeGeocaching( Math.abs(lon) );
    }
    if (format === "numeric"){
        return  (Math.round(lat * 100000)/100000) + " " + (Math.round(lon * 100000)/100000);
    }
    // format === "degrees"
    return formatDegree( Math.abs(lat) ) + (lat>0? "N":"S") + " " + formatDegree( Math.abs(lon) ) + (lon>0? "E":"W");
}

function humanDistance(distance){
    if (typeof distanceUnits != "undefined" && distanceUnits == "imperial"){
        var feet = distance * 3.2808;
        if (feet < 1500){
            return Math.round(feet) + " " + qsTr("feet");
        }
        var miles = distance / 1609.344;
        if (miles < 20){
            return (Math.round(miles * 10)/10) + " " + qsTr("miles");
        }
        return Math.round(miles) + " " + qsTr("miles");
    }else{
        if (distance < 1500){
            return Math.round(distance) + " " + qsTr("meters");
        }
        if (distance < 20000){
            return (Math.round((distance/1000) * 10)/10) + " " + qsTr("km");
        }
        return Math.round(distance/1000) + " " + qsTr("km");
    }
}
function humanDistanceVerbose(distance){
    if (typeof distanceUnits != "undefined" && distanceUnits == "imperial"){
        var feet = distance * 3.2808;
        if (feet < 150){
            return (Math.round(feet / 10) * 10) + " " + qsTr("feet");
        }
        var miles = distance / 1609.344;
        if (miles < 2){
            return (Math.round(feet / 100) * 100) + " " + qsTr("feet");
        }
        return Math.round(miles) + " " + qsTr("miles");
    }else{
        if (distance < 150){
            return Math.round(distance/10)*10 + " "+ qsTr("meters");
        }
        if (distance < 2000){
            return Math.round(distance/100)*100 + " "+ qsTr("meters");
        }
        return Math.round(distance/1000) + " "+ qsTr("km");
    }
}

function humanSpeed(kmph){
    if ((typeof distanceUnits) != "undefined" && distanceUnits == "imperial"){
        var mph = (kmph * 1000) / 1609.344;
        return Math.round(mph);
    }else{
        return Math.round(kmph);
    }
}

/**
 * Localise bearing in sense indicating location of some place
 * "this place is located XX meters northeast from you"
 */
function humanBearing(bearing){
    // N, NE, E, SE, S, SW, W, NW
    if (bearing == "W") {
        //: in sense indicating location of some place: "place is located WEST"
        return qsTr("west");
    }
    if (bearing == "E") {
        //: in sense indicating location of some place: "place is located EAST"
        return qsTr("east");
    }
    if (bearing == "S") {
        //: in sense indicating location of some place: "place is located SOUTH"
        return qsTr("south");
    }
    if (bearing == "N") {
        //: in sense indicating location of some place: "place is located NORTH"
        return qsTr("north");
    }

    if (bearing == "NE") {
        //: in sense indicating location of some place: "place is located NORTHEAST"
        return qsTr("northeast");
    }
    if (bearing == "SE") {
        //: in sense indicating location of some place: "place is located SOUTHEAST"
        return qsTr("southeast");
    }
    if (bearing == "SW") {
        //: in sense indicating location of some place: "place is located SOUTHWEST"
        return qsTr("southwest");
    }
    if (bearing == "NW") {
        //: in sense indicating location of some place: "place is located NORTHWEST"
        return qsTr("northwest");
    }

    return bearing;
}

function humanDuration(seconds){
    var hours   = Math.floor(seconds / 3600);
    var minutes = Math.floor((seconds - (hours * 3600)) / 60);

    if (hours   < 10) {hours   = "0"+hours;}
    if (minutes < 10) {minutes = "0"+minutes;}
    return hours+':'+minutes;
}

function humanDurationLong(seconds){
    var hours   = Math.floor(seconds / 3600);
    var rest    = seconds - (hours * 3600);
    var minutes = Math.floor(rest / 60);
    var sec     = Math.floor(rest - (minutes * 60));

    if (hours   < 10) {hours   = "0"+hours;}
    if (minutes < 10) {minutes = "0"+minutes;}
    if (sec     < 10) {sec = "0"+sec;}
    return hours+':'+minutes+':'+sec;
}

function humanDirectory(directory){
    return directory
        .replace(/^\/home\/nemo$/i, qsTr("Home"))
        .replace(/^\/home\/nemo\/Documents$/i, qsTr("Documents"))
        .replace(/^\/media\/sdcard\/[^/]*$/i, qsTr("SD card"))
        .replace(/^\/run\/media\/nemo\/[^/]*$/i, qsTr("SD card"))
        .replace(/^\/home\/nemo\//i, "[" + qsTr("Home") + "] ")
        .replace(/^\/media\/sdcard\/[^/]*\//i, "[" + qsTr("SD card") + "] ")
        .replace(/^\/run\/media\/nemo\/[^/]*\//i, "[" + qsTr("SD card") + "] ");
}

function locationStr(location){
    if (location==null){
        return "";
    }
    return (location.label=="" || location.type=="coordinate") ?
                formatCoord(location.lat, location.lon, gpsFormat) :
                location.label;
}

function rad2Deg(radians){
    return radians * 180.0 / Math.PI;
}

function deg2Rad(degrees){
    return degrees * Math.PI / 180.0;
}
