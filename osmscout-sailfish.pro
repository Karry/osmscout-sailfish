# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
TARGET = osmscout-sailfish

CONFIG += sailfishapp


SAILFISHAPP_ICONS = 86x86 108x108 128x128 256x256

# to disable building translations every time, comment out the
# following CONFIG line
#CONFIG += sailfishapp_i18n


# German translation is enabled as an example. If you aren't
# planning to localize your app, remember to comment out the
# following TRANSLATIONS line. And also do not forget to
# modify the localized app name in the the .desktop file.
TRANSLATIONS += translations/en.ts \
                translations/hu.ts

lupdate_only {
SOURCES =   qml/*.qml \
            qml/custom/*.qml \
            qml/pages/*.qml
}


DISTFILES += \
    qml/AboutDialog.qml \
    qml/desktop.qml \
    qml/main.qml \
    qml/SearchDialog.qml \
    qml/custom/DialogActionButton.qml \
    qml/custom/LineEdit.qml \
    qml/custom/Link.qml \
    qml/custom/LocationSearch.qml \
    qml/custom/MapButton.qml \
    qml/custom/MapComponent.qml \
    qml/custom/MapDialog.qml \
    qml/custom/MapRenderingIndicator.qml \
    qml/custom/OSMCopyright.qml \
    qml/custom/ScaleIndicator.qml \
    qml/custom/ScrollIndicator.qml \
    qml/pages/About.qml \
    qml/pages/Cover.qml \
    qml/pages/Map.qml \
    qml/pages/PlaceDetail.qml

HEADERS += \
    src/DBThread.h \
    src/InputHandler.h \
    src/LocationInfoModel.h \
    src/MapWidget.h \
    src/OSMTile.h \
    src/OsmTileDownloader.h \
    src/RoutingModel.h \
    src/SearchLocationModel.h \
    src/Settings.h \
    src/Theme.h \
    src/TileCache.h

SOURCES += \
    src/DBThread.cpp \
    src/InputHandler.cpp \
    src/LocationInfoModel.cpp \
    src/MapWidget.cpp \
    src/OSMScout.cpp \
    src/OSMTile.cpp \
    src/OsmTileDownloader.cpp \
    src/PerformanceTest.cpp \
    src/RoutingModel.cpp \
    src/SearchLocationModel.cpp \
    src/Settings.cpp \
    src/Theme.cpp \
    src/TileCache.cpp

