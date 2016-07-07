
import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle{
    id : osmCopyright
    anchors{
        left: parent.left
        bottom: parent.bottom
    }
    width: label.width + 20
    height: label.height + 12

    color: "white"
    opacity: 0.7

    Text {
        id: label
        text: qsTr("© OpenStreetMap contributors")
        anchors.centerIn: parent

        font.pointSize: Theme.fontSizeExtraSmall *0.6
    }

}
