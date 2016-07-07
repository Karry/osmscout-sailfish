import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle{

    property int magLevel: 0;
    property bool finished: false;

    width: busyIndicator.width + Theme.paddingMedium
    height: busyIndicator.height + Theme.paddingMedium

    color: "transparent"
    opacity: map.finished ? 0 : 0.9
    Behavior on opacity { NumberAnimation {} }

    BusyIndicator{
        id: busyIndicator
        running: !finished
        size: BusyIndicatorSize.Medium
        anchors.centerIn: parent
    }
    Text {
        id: magLevelLabel
        text: magLevel
        anchors.centerIn: parent
        opacity: 0.4
        font.pointSize: Theme.fontSizeMedium
    }

}
