import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle{
    id: scaleIndicator

    property double pixelSize: 1
    property double indicatorWidth: computeWidth()
    property string label: "?"

    function computeWidth(){
        var wrapperWidthInM = pixelSize * scaleIndicator.width;
        var scale = 1;
        var displayDiv = 1;
        var suffix = "m"
        while (wrapperWidthInM / scale > 10){
            scale *= 10;
        }
        var displayed = Math.floor(wrapperWidthInM / scale) * scale;
        if (displayed >= 1000){
            suffix = "km";
            displayDiv = 1000;
        }
        label = (displayed / displayDiv) + " " + suffix;
        indicatorWidth = displayed / pixelSize;

        //console.log("pixelSize: " + pixelSize + " indicatorWidth: " + indicatorWidth + " => " + label)
        return indicatorWidth;
    }

    height: labelComponent.height + 10
    width: 300
    color: "transparent"

    onPixelSizeChanged: {
        computeWidth()
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
