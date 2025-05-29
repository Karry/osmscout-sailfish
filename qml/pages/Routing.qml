/*
 OSM Scout for Sailfish OS
 Copyright (C) 2017  Lukas Karas

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

import "../custom"
import "../custom/Utils.js" as Utils
import ".." // Global singleton

Dialog {
    id: routingPage

    property double toLat: -1000
    property double toLon: -1000
    property string toName: ""

    RemorsePopup { id: remorse }

    RoutingListModel{
        id: route

        property string vehicle: "car";
        property RoutingProfile profile: null

        onRouteFailed: {
            remorse.execute(qsTranslate("message", reason), function() { }, 10 * 1000);
        }
    }
    function computeRoute() {
        if ((fromSelector.location !== null) && (toSelector.location!== null)) {
            console.log("Routing \"" + Utils.locationStr(fromSelector.location) + "\" -> \"" + Utils.locationStr(toSelector.location) + "\" with vehicle " + vehicleComboBox.selected);
            route.vehicle = vehicleComboBox.selected;
            route.profile = vehicleComboBox.prepareProfile();
            route.setStartAndTarget(fromSelector.location,
                                    toSelector.location,
                                    route.profile);
            AppSettings.lastVehicle = vehicleComboBox.selected;
        } else {
            route.clear();
        }
    }

    canAccept: (fromSelector.location !== null) && (toSelector.location !== null)
    acceptDestination: Qt.resolvedUrl("RouteDescription.qml")
    acceptDestinationAction: PageStackAction.Push
    acceptDestinationProperties: {
        "route": route,
        "mapPage": Global.mapPage,
        "mainMap": Global.mainMap,
        "destination": toSelector.location,
        "fromCurrentLocation": fromSelector.useCurrentLocation
    }

    onAccepted: {
        computeRoute();
    }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                text: qsTr("Reverse direction")
                onClicked: {
                    // swap selector state
                    var fromLocation=fromSelector.location;
                    var fromLabel=fromSelector.value;
                    var fromCurrent=fromSelector.useCurrentLocation;
                    var fromIndex=fromSelector.currentIndex;

                    fromSelector.location=toSelector.location;
                    fromSelector.value=toSelector.value;
                    fromSelector.useCurrentLocation=toSelector.useCurrentLocation;
                    fromSelector.currentIndex=toSelector.currentIndex;

                    toSelector.location=fromLocation;
                    toSelector.value=fromLabel;
                    toSelector.useCurrentLocation=fromCurrent;
                    toSelector.currentIndex=fromIndex;
                }
            }
        }

        Column{
            id: content
            anchors.fill: parent

            DialogHeader {
                id: header
                title: qsTr("Search route")
                acceptText : qsTr("Route!")
                cancelText : ""
            }

            LocationSelector {
                id: fromSelector
                width: parent.width
                //: Routing FROM place
                label: qsTr("From")
                initWithCurrentLocation: true
            }
            GpsFixWarning {
                visible: fromSelector.useCurrentLocation && fromSelector.location == null
                x: Theme.horizontalPageMargin
            }
            LocationSelector {
                id: toSelector
                width: parent.width
                //: Routing TO place
                label: qsTr("To")

                Component.onCompleted: {
                    if (toLat!=-1000 && toLon!=-1000){
                        toSelector.location=route.locationEntryFromPosition(toLat, toLon);
                        if (toName!=""){
                            toSelector.value=toName;
                        }else{
                            toSelector.value=Utils.formatCoord(toLat, toLon, AppSettings.gpsFormat);
                        }
                    }
                }
            }
            GpsFixWarning {
                visible: toSelector.useCurrentLocation && toSelector.location == null
                x: Theme.horizontalPageMargin
            }

            ComboBox {
                id: vehicleComboBox
                label: qsTr("By")

                property bool initialized: false
                property string selected: ""
                property var vehicleProfiles: ["car", "road-bike", "mountain-bike", "foot"]

                function prepareProfile() {
                    var routingProfile = Qt.createQmlObject("import harbour.osmscout.map 1.0\n" +
                                                            "RoutingProfile{}",
                                                            Global.navigationModel,
                                                            "dynamicRoutingProfile");
                    if (selected=="car"){
                        routingProfile.vehicle=RoutingProfile.CarVehicle;
                    } else if (selected == "road-bike" || selected == "mountain-bike") {
                        routingProfile.vehicle=RoutingProfile.BicycleVehicle;
                    } else if (selected == "foot") {
                        routingProfile.vehicle=RoutingProfile.FootVehicle;
                    }

                    var speedTable = routingProfile.speedTable;
                    if (selected == "road-bike") {
                        routingProfile.maxSpeed = 30;

                        speedTable['highway_cycleway']=20;
                        speedTable['highway_living_street']=20;
                        speedTable['highway_path:1']=12;
                        speedTable['highway_path:2']=10;
                        speedTable['highway_path:3']=9;
                        speedTable['highway_path:4']=8;
                        speedTable['highway_path:5']=7;
                        speedTable['highway_primary']=30;
                        speedTable['highway_primary_link']=30;
                        speedTable['highway_residential']=10;
                        speedTable['highway_road']=22;
                        speedTable['highway_roundabout']=10;
                        speedTable['highway_secondary']=30;
                        speedTable['highway_secondary_link']=30;
                        speedTable['highway_service']=16;
                        speedTable['highway_tertiary']=20;
                        speedTable['highway_tertiary_link']=20;
                        speedTable['highway_track:1']=22;
                        speedTable['highway_track:2']=20;
                        speedTable['highway_track:3']=15;
                        speedTable['highway_track:4']=10;
                        speedTable['highway_track:5']=8;
                        speedTable['highway_trunk']=20;
                        speedTable['highway_trunk_link']=20;
                        speedTable['highway_unclassified']=20;
                        speedTable['highway_footway']=8;

                        if (!mainRoadSwitch.checked){
                            speedTable['highway_primary']=2;
                            speedTable['highway_primary_link']=2;
                            speedTable['highway_secondary']=4;
                            speedTable['highway_secondary_link']=4;
                        }
                        if (!footwaySwitch.checked){
                            speedTable['highway_footway']=0;
                        }
                    } else if (selected == "mountain-bike") {
                        routingProfile.maxSpeed = 25;

                        speedTable['highway_cycleway']=20;
                        speedTable['highway_living_street']=20;
                        speedTable['highway_path:1']=20;
                        speedTable['highway_path:2']=18;
                        speedTable['highway_path:3']=14;
                        speedTable['highway_path:4']=10;
                        speedTable['highway_path:5']=8;
                        speedTable['highway_primary']=25;
                        speedTable['highway_primary_link']=25;
                        speedTable['highway_residential']=10;
                        speedTable['highway_road']=22;
                        speedTable['highway_roundabout']=10;
                        speedTable['highway_secondary']=25;
                        speedTable['highway_secondary_link']=25;
                        speedTable['highway_service']=16;
                        speedTable['highway_tertiary']=20;
                        speedTable['highway_tertiary_link']=20;
                        speedTable['highway_track:1']=20;
                        speedTable['highway_track:2']=20;
                        speedTable['highway_track:3']=18;
                        speedTable['highway_track:4']=16;
                        speedTable['highway_track:5']=12;
                        speedTable['highway_trunk']=20;
                        speedTable['highway_trunk_link']=20;
                        speedTable['highway_unclassified']=20;

                        if (!mainRoadSwitch.checked){
                            speedTable['highway_primary']=2;
                            speedTable['highway_primary_link']=2;
                            speedTable['highway_secondary']=4;
                            speedTable['highway_secondary_link']=4;
                        }
                        if (!footwaySwitch.checked){
                            speedTable['highway_footway']=0;
                        }
                    } else if (selected == "foot") {
                        if (!mainRoadSwitch.checked){
                            speedTable['highway_primary']=1;
                            speedTable['highway_primary_link']=1;
                            speedTable['highway_secondary']=1;
                            speedTable['highway_secondary_link']=1;
                        }
                    }
                    routingProfile.speedTable = speedTable;
                    return routingProfile;
                }

                menu: ContextMenu {
                    RouteProfileMenuItem {
                        icon: "car"
                        text: qsTr("Car")
                    }
                    RouteProfileMenuItem {
                        icon: "road-bike"
                        text: qsTr("Road bike")
                    }
                    RouteProfileMenuItem {
                        icon: "mountain-bike"
                        text: qsTr("Mountain bike")
                    }
                    RouteProfileMenuItem {
                        icon: "foot"
                        text: qsTr("Foot")
                    }
                }
                onPressAndHold: {
                    // improve default ComboBox UX :-)
                    vehicleComboBox.clicked(mouse);
                }
                onCurrentItemChanged: {
                    if (!initialized){
                        return;
                    }
                    selected = vehicleProfiles[currentIndex];
                    console.log("Selected vehicle: "+selected);
                }
                Component.onCompleted: {                    
                    for (var i in vehicleProfiles){
                        var profile = vehicleProfiles[i];
                        console.log("Profile: "+profile);
                        if (profile==AppSettings.lastVehicle){
                            currentIndex = i;
                        }
                    }
                    initialized = true;
                }
            }

            TextSwitch{
                id: mainRoadSwitch
                width: parent.width

                checked: false
                //: witch to allow main roads (highway=primary|secondary) while routing for foot or bike
                text: qsTr("Allow main roads")
                visible: vehicleComboBox.selected == "road-bike" || vehicleComboBox.selected == "mountain-bike" || vehicleComboBox.selected == "foot"

                property bool initialized: false
                onCheckedChanged: {
                    if (!initialized){
                        return;
                    }

                    if (vehicleComboBox.selected == "road-bike"){
                        AppSettings.roadBikeAllowMainRoads = checked;
                    } else if (vehicleComboBox.selected == "mountain-bike"){
                        AppSettings.mountainBikeAllowMainRoads = checked;
                    } else if (vehicleComboBox.selected == "foot"){
                        AppSettings.footAllowMainRoads = checked;
                    }
                }
                function update() {
                    initialized = false;
                    if (vehicleComboBox.selected == "road-bike"){
                        checked = AppSettings.roadBikeAllowMainRoads;
                    } else if (vehicleComboBox.selected == "mountain-bike"){
                        checked = AppSettings.mountainBikeAllowMainRoads;
                    } else if (vehicleComboBox.selected == "foot"){
                        checked = AppSettings.footAllowMainRoads;
                    }
                    console.log("allow main roads for " + vehicleComboBox.selected + ": " + checked);
                    initialized = true;
                }
                Component.onCompleted: {
                    update();
                }
                Connections {
                    target: vehicleComboBox
                    onSelectedChanged: {
                        mainRoadSwitch.update();
                    }
                }
            }

            TextSwitch{
                id: footwaySwitch
                width: parent.width

                checked: false
                //: switch to allow footways while routing for bike
                text: qsTr("Allow footways")
                visible: vehicleComboBox.selected == "road-bike" || vehicleComboBox.selected == "mountain-bike"

                property bool initialized: false
                onCheckedChanged: {
                    if (!initialized){
                        return;
                    }

                    if (vehicleComboBox.selected == "road-bike"){
                        AppSettings.roadBikeAllowFootways = checked;
                    } else if (vehicleComboBox.selected == "mountain-bike"){
                        AppSettings.mountainBikeAllowFootways = checked;
                    }
                }
                function update() {
                    initialized = false;
                    if (vehicleComboBox.selected == "road-bike"){
                        checked = AppSettings.roadBikeAllowFootways;
                    } else if (vehicleComboBox.selected == "mountain-bike"){
                        checked = AppSettings.mountainBikeAllowFootways;
                    }
                    console.log("allow footways for " + vehicleComboBox.selected + ": " + checked);
                    initialized = true;
                }
                Component.onCompleted: {
                    update();
                }
                Connections {
                    target: vehicleComboBox
                    onSelectedChanged: {
                        footwaySwitch.update();
                    }
                }
            }
        }
    }

}
