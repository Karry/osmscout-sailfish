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
