
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
    radius: Theme.paddingSmall

    color: Theme.rgba(Theme.highlightDimmerColor, 0.2)

    Text {
        id: label
        text: qsTr("© OpenStreetMap contributors")
        anchors.centerIn: parent
        color: Theme.rgba(Theme.primaryColor, 0.8)

        font.pointSize: Theme.fontSizeExtraSmall *0.6
    }

}
