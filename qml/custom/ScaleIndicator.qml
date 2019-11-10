import QtQuick 2.0
import Sailfish.Silica 1.0

import "Utils.js" as Utils

Rectangle{
    id: scaleIndicator

    property double pixelSize: 1 // in meters
    property double indicatorWidth: computeWidth()
    property string label: "?"

    function computeWidth(){
        if (Utils.distanceUnits == "imperial"){
            if (pixelSize * 3.2808 * scaleIndicator.width > 6000){
                return computeWidth2(pixelSize / 1609.344, "mi");
            }else{
                return computeWidth2(pixelSize * 3.2808, "ft");
            }
        }else{
            if (pixelSize * scaleIndicator.width > 1000){
                return computeWidth2(pixelSize / 1000, "km");
            }else{
                return computeWidth2(pixelSize, "m");
            }
        }
    }

    function computeWidth2(pixelSize, suffix){
        var wrapperWidthInUnits = pixelSize * scaleIndicator.width;
        var scale = 1;
        var displayDiv = 1;
        while (wrapperWidthInUnits / scale > 10){
            scale *= 10;
        }
        var displayed = Math.floor(wrapperWidthInUnits / scale) * scale;

        label = (displayed / displayDiv) + " " + suffix;
        var indicatorWidth = displayed / pixelSize;

        //console.log("pixelSize: " + pixelSize + " indicatorWidth: " + indicatorWidth + " => " + label)
        return indicatorWidth;
    }

    height: labelComponent.height + 10
    width: 300
    color: "transparent"

    onPixelSizeChanged: {
        indicatorWidth = computeWidth();
    }

    Rectangle{
        id: bar
        width: scaleIndicator.indicatorWidth
        height: 6
        color: "#000000"

        anchors.bottom: parent.bottom
    }
    Label{
        id: labelComponent
        text: scaleIndicator.label
        font.pixelSize: Theme.fontSizeSmall
        color: "#000000"

        x: Theme.paddingLarge
        y: 0
    }
}
