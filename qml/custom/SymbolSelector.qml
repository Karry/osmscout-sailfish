/*
 OSM Scout for Sailfish OS
 Copyright (C) 2021  Lukas Karas

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

Column {
    id: symbolSelector
    width: parent.width

    property string symbol: "Red Circle"
    property var notSelectedScale: 0.75

    SectionHeader{ text: qsTr("Symbol") }

    Rectangle {
        width: parent.width
        color: "transparent"
        height: Theme.iconSizeLarge + Theme.paddingMedium

        SymbolIcon {
            centerX: (parent.width / 4) * 0.5
            symbolName: "Red Circle"
            symbolImage: "circle.svg?#b32020"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
        SymbolIcon {
            centerX: (parent.width / 4) * 1.5
            symbolName: "Green Circle"
            symbolImage: "circle.svg?#00b200"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
        SymbolIcon {
            centerX: (parent.width / 4) * 2.5
            symbolName: "Blue Circle"
            symbolImage: "circle.svg?#203bb3"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
        SymbolIcon {
            centerX: (parent.width / 4) * 3.5
            symbolName: "Yellow Circle"
            symbolImage: "circle.svg?#dfed00"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
    }
    Rectangle {
        width: parent.width
        color: "transparent"
        height: Theme.iconSizeLarge + Theme.paddingMedium

        SymbolIcon {
            centerX: (parent.width / 4) * 0.5
            symbolName: "Red Square"
            symbolImage: "square.svg?#b32020"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
        SymbolIcon {
            centerX: (parent.width / 4) * 1.5
            symbolName: "Green Square"
            symbolImage: "square.svg?#00b200"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
        SymbolIcon {
            centerX: (parent.width / 4) * 2.5
            symbolName: "Blue Square"
            symbolImage: "square.svg?#203bb3"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
        SymbolIcon {
            centerX: (parent.width / 4) * 3.5
            symbolName: "Yellow Square"
            symbolImage: "square.svg?#dfed00"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
    }
    Rectangle {
        width: parent.width
        color: "transparent"
        height: Theme.iconSizeLarge + Theme.paddingMedium

        SymbolIcon {
            centerX: (parent.width / 4) * 0.5
            symbolName: "Red Triangle"
            symbolImage: "triangle.svg?#b32020"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
        SymbolIcon {
            centerX: (parent.width / 4) * 1.5
            symbolName: "Green Triangle"
            symbolImage: "triangle.svg?#00b200"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
        SymbolIcon {
            centerX: (parent.width / 4) * 2.5
            symbolName: "Blue Triangle"
            symbolImage: "triangle.svg?#203bb3"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
        SymbolIcon {
            centerX: (parent.width / 4) * 3.5
            symbolName: "Yellow Triangle"
            symbolImage: "triangle.svg?#dfed00"
            selectedSymbol: symbolSelector.symbol
            onSelected: {
                symbolSelector.symbol = symbolName
            }
        }
    }
}
