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
CONFIG += sailfishapp_i18n


TRANSLATIONS += translations/en.ts \
                translations/hu.ts \
                translations/cs.ts \
                translations/pl.ts


lupdate_only {
SOURCES =   qml/*.qml \
            qml/custom/*.qml \
            qml/pages/*.qml
}

# find qml -type f | sort | sed 's/$/ \\/'
DISTFILES += \
    qml/AboutDialog.qml \
    qml/custom/AvailableMapsView.qml \
    qml/custom/DialogActionButton.qml \
    qml/custom/LineEdit.qml \
    qml/custom/Link.qml \
    qml/custom/LocationSearch.qml \
    qml/custom/MapButton.qml \
    qml/custom/MapComponent.qml \
    qml/custom/MapDialog.qml \
    qml/custom/MapRenderingIndicator.qml \
    qml/custom/OSMCopyright.qml \
    qml/custom/POIIcon.qml \
    qml/custom/ScaleIndicator.qml \
    qml/custom/ScrollIndicator.qml \
    qml/desktop.qml \
    qml/main.qml \
    qml/pages/About.qml \
    qml/pages/Cover.qml \
    qml/pages/Downloads.qml \
    qml/pages/Layers.qml \
    qml/pages/MapDetail.qml \
    qml/pages/MapList.qml \
    qml/pages/Map.qml \
    qml/pages/PlaceDetail.qml \
    qml/pages/Search.qml \
    qml/SearchDialog.qml

HEADERS += \
    src/MapStyleHelper.h

SOURCES += \
    src/MapStyleHelper.cpp \
    src/OSMScout.cpp \
    src/PerformanceTest.cpp

