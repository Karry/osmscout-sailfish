import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle{
  id: wrapper
  property string label
  property string link
  property string linkText
  
  height: labelComponent.height + linkComponent.height + Theme.paddingSmall
  color: "transparent"

  Label{
      id: labelComponent
      text: label

      font.pixelSize: Theme.fontSizeMedium
      font.bold: true

      width: parent.width
      color: Theme.primaryColor
  }
  Label{
      id: linkComponent
      text: linkText

      font.pixelSize: Theme.fontSizeSmall
      font.underline: true
      color: Theme.highlightColor
      truncationMode: TruncationMode.Fade

      width: parent.width
      x: Theme.paddingMedium
      anchors.top: labelComponent.bottom

      MouseArea{
          onClicked: Qt.openUrlExternally(link)
          anchors.fill: parent
      }
  }

}
