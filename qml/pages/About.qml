
import QtQuick 2.0
import Sailfish.Silica 1.0

import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"

Page {
    id: aboutPage

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: content.height + header.height + 2*Theme.paddingLarge

        VerticalScrollDecorator {}

        Rectangle{
            id: header

            height: icon.height + 2* Theme.paddingLarge
            Image{
                id: icon
                source: "image://theme/harbour-osmscout"
                x: 2* Theme.paddingLarge
                y: Theme.paddingLarge
            }
            PageHeader {
                id: headerText
                anchors.left: icon.right
                anchors.verticalCenter: parent.verticalCenter
                title: qsTr("OSM Scout %1").arg(OSMScoutVersionString)
            }
        }

        Column {
            id: content
            width: parent.width
            anchors.top: header.bottom

            SectionHeader{
                id: aboutHeader
                text: qsTr("About")
            }

            Label{
                id: aboutText
                text: qsTr(
                          "OSM Scout for Sailfish OS is developed as open-source by volunteers in their free time. " +
                          "You can help to improve this application by reporting bugs, creating translations or developing new features. " +
                          "Any help is welcome."
                          )
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall

                width: parent.width - 2* Theme.paddingLarge
                x: Theme.paddingLarge
            }

            Rectangle{
                id: links
                color: "transparent"
                height: githubLink.height + libLink.height + tutorialLink.height + 2*Theme.paddingLarge
                width: parent.width

                Link{
                    id:githubLink
                    width: parent.width - 2* Theme.paddingLarge
                    x: Theme.paddingLarge
                    y: Theme.paddingLarge

                    label: qsTr("GitHub page")
                    link: "https://github.com/Karry/osmscout-sailfish"
                    linkText: "github.com/Karry/osmscout-sailfish"
                }
                Link{
                    id:libLink
                    width: parent.width - 2* Theme.paddingLarge
                    x: Theme.paddingLarge

                    label: qsTr("OSMScout library GitHub page")
                    link: "https://github.com/Framstag/libosmscout"
                    linkText: "github.com/Framstag/libosmscout"

                    anchors.top: githubLink.bottom
                }
                Link{
                    id:tutorialLink
                    width: parent.width - 2* Theme.paddingLarge
                    x: Theme.paddingLarge

                    label: qsTr("Offline map import tutorial")
                    link: "http://libosmscout.sourceforge.net/tutorials/Importing"
                    linkText: "libosmscout.sourceforge.net/tutorials/Importing"

                    anchors.top: libLink.bottom
                }
            }

            SectionHeader{
                id: translatorsHeader
                text: qsTr("Translators")
            }

            Column{
                id: translatorsColumn

                spacing: Theme.paddingMedium
                width: parent.width - 2* Theme.paddingLarge
                x: Theme.paddingLarge

                Label{
                    text: qsTr("Czech: Lukáš Karas")
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeSmall
                }
                Label{
                    text: qsTr("Hungarian: Miklós Márton")
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeSmall
                }
                Label{
                    text: qsTr("Norwegian Bokmål: Håvard Moen")
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeSmall
                }
                Label{
                    text: qsTr("Polish: Atlochowski")
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeSmall
                }
                Label{
                    text: qsTr("Swedish: Åke Engelbrektson")
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

        }
    }
}
