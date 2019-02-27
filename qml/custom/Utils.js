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
    if (distance < 1500){
        return Math.round(distance) + " " + qsTr("meters");
    }
    if (distance < 20000){
        return (Math.round((distance/1000) * 10)/10) + " " + qsTr("km");
    }
    return Math.round(distance/1000) + " " + qsTr("km");
}
function humanBearing(bearing){
    if (bearing == "W")
        return qsTr("west");
    if (bearing == "E")
        return qsTr("east");
    if (bearing == "S")
        return qsTr("south");
    if (bearing == "N")
        return qsTr("north");

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
                Utils.formatCoord(location.lat, location.lon, AppSettings.gpsFormat) :
                location.label;
}
