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

        onRouteFailed: {
            remorse.execute(qsTranslate("message", reason), function() { }, 10 * 1000);
        }
    }
    function computeRoute() {
        if ((fromSelector.location !== null) && (toSelector.location!== null)) {
            console.log("Routing \"" + Utils.locationStr(fromSelector.location) + "\" -> \"" + Utils.locationStr(toSelector.location) + "\" by " + vehicleComboBox.selected);
            route.vehicle = vehicleComboBox.selected;
            route.setStartAndTarget(fromSelector.location,
                                    toSelector.location,
                                    vehicleComboBox.selected);
            AppSettings.lastVehicle = vehicleComboBox.selected;
        } else {
            route.clear();
        }
    }

    canAccept: (fromSelector.location !== null) && (toSelector.location!== null)
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
                text: qsTr("Exchage route")
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
            ComboBox {
                id: vehicleComboBox
                label: qsTr("By")

                property bool initialized: false
                property string selected: ""
                property ListModel vehiclesModel: ListModel {}

                menu: ContextMenu {
                    Repeater {
                        id: vehicleRepeater
                        model: vehicleComboBox.vehiclesModel
                        MenuItem { text: qsTranslate("routerVehicle", vehicle) }
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
                    var vehicles=route.availableVehicles();
                    selected = vehicles[currentIndex];
                    console.log("Selected vehicle: "+selected);
                }
                Component.onCompleted: {
                    var vehicles=route.availableVehicles()
                    for (var i in vehicles){
                        var vehicle = vehicles[i];
                        console.log("Vehicle: "+vehicle);
                        vehiclesModel.append({"vehicle": vehicle});
                        if (vehicle==AppSettings.lastVehicle){
                            currentIndex = i;
                        }
                    }
                    initialized = true;
                }
            }
        }
    }

}
