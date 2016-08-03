
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
        contentHeight: content.height + 2*Theme.paddingLarge

        VerticalScrollDecorator {}

        Rectangle {
            id: content
            //spacing: Theme.paddingLarge
            width: parent.width
            height: header.height + aboutHeader.height + aboutText.height + links.height + 2*Theme.paddingLarge
            color: "transparent"

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
                    title: qsTrId("OSM Scout %1").arg(OSMScoutVersionString)
                }
            }

            SectionHeader{
                id: aboutHeader
                text: "About"
                anchors.top: header.bottom
            }

            Label{
                id: aboutText
                text: qsTrId(
                          "OSM Scout for Sailfish OS is map application developed as open-source by volunteers in their free time. " +
                          "You can help to improve this application by reporting bugs, creating translations or developing new features. " +
                          "Any help is welcome."
                          )
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall

                width: parent.width - 2* Theme.paddingLarge
                x: Theme.paddingLarge

                anchors {
                    top: aboutHeader.bottom
                    topMargin: Theme.paddingSmall
                }
            }

            Rectangle{
                id: links
                color: "transparent"
                height: githubLink.height + libLink.height + tutorialLink.height
                width: parent.width

                anchors {
                    top: aboutText.bottom
                    topMargin: Theme.paddingLarge
                }

                Link{
                    id:githubLink
                    width: parent.width - 2* Theme.paddingLarge
                    x: Theme.paddingLarge

                    label: "GitHub page"
                    link: "https://github.com/Karry/osmscout-sailfish"
                    linkText: "github.com/Karry/osmscout-sailfish"
                }
                Link{
                    id:libLink
                    width: parent.width - 2* Theme.paddingLarge
                    x: Theme.paddingLarge

                    label: "OSMScout library GitHub page"
                    link: "https://github.com/Framstag/libosmscout"
                    linkText: "github.com/Framstag/libosmscout"

                    anchors.top: githubLink.bottom
                }
                Link{
                    id:tutorialLink
                    width: parent.width - 2* Theme.paddingLarge
                    x: Theme.paddingLarge

                    label: "Offline map import tutorial"
                    link: "http://libosmscout.sourceforge.net/tutorials/Importing/"
                    linkText: "libosmscout.sourceforge.net/tutorials/Importing"

                    anchors.top: libLink.bottom
                }
            }
        }
    }
}
