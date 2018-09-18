/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2016  Lukáš Karas

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

// Custom QML objects
#include <osmscout/MapWidget.h>
#include <osmscout/SearchLocationModel.h>
#include <osmscout/LocationInfoModel.h>
#include <osmscout/OnlineTileProviderModel.h>
#include <osmscout/RoutingModel.h>
#include <osmscout/AvailableMapsModel.h>
#include <osmscout/MapDownloadsModel.h>
#include <osmscout/MapStyleModel.h>
#include <osmscout/StyleFlagsModel.h>
#include <osmscout/MapObjectInfoModel.h>

#include <osmscout/util/Logger.h>
#include <osmscout/OSMScoutQt.h>


#include <osmscout/Settings.h> // Library settings
#include "AppSettings.h" // Application settings

// collections
#include "Storage.h"
#include "CollectionModel.h"
#include "CollectionListModel.h"
#include "CollectionTrackModel.h"
#include "CollectionMapBridge.h"

#include <harbour-osmscout/private/Config.h>

// SFOS
#include <sailfishapp/sailfishapp.h>

// Qt includes
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QStandardPaths>
#include <QQmlContext>
#include <QFileInfo>

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#if QT_VERSION >= 0x050400
#define HAS_QSTORAGE
#include <QStorageInfo>
#endif

// std
#include <iostream>

#ifndef OSMSCOUT_SAILFISH_VERSION_STRING
#warning "OSMSCOUT_SAILFISH_VERSION_STRING should be defined by build system"
#define OSMSCOUT_SAILFISH_VERSION_STRING "?.?.?"
#endif

using namespace osmscout;

static QObject *appSettingsSingletontypeProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  return new AppSettings();
}

int main(int argc, char* argv[])
{
#ifdef Q_WS_X11
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

  QGuiApplication *app = SailfishApp::application(argc, argv);

  app->setOrganizationDomain("libosmscout.sf.net");
  app->setApplicationName("harbour-osmscout"); // Harbour name have to be used - for correct cache dir
  app->setApplicationVersion(OSMSCOUT_SAILFISH_VERSION_STRING);
    
  int           result;  

#if defined(HAVE_MMAP)
  qDebug() << "Usage of memory mapped files is supported.";
#else
  qWarning() << "Usage of memory mapped files is NOT supported.";
#endif

#ifdef QT_QML_DEBUG
  qWarning() << "Starting QML debugger on port 1234.";
  qQmlEnableDebuggingHelper.startTcpDebugServer(1234);
#endif

  OSMScoutQt::RegisterQmlTypes("harbour.osmscout.map", 1, 0);

  qRegisterMetaType<MapView*>("MapView*");
  qRegisterMetaType<std::vector<Collection>>("std::vector<Collection>");
  qRegisterMetaType<Collection>("Collection");
  qRegisterMetaType<Track>("Track");
  qRegisterMetaType<Waypoint>("Waypoint");

  qmlRegisterType<CollectionListModel>("harbour.osmscout.map", 1, 0, "CollectionListModel");
  qmlRegisterType<CollectionModel>("harbour.osmscout.map", 1, 0, "CollectionModel");
  qmlRegisterType<CollectionTrackModel>("harbour.osmscout.map", 1, 0, "CollectionTrackModel");
  qmlRegisterType<CollectionMapBridge>("harbour.osmscout.map", 1, 0, "CollectionMapBridge");

  qmlRegisterSingletonType<AppSettings>("harbour.osmscout.map", 1, 0, "AppSettings", appSettingsSingletontypeProvider);

  osmscout::log.Debug(true);

  bool desktop = false;
  for (QString arg: app->arguments()){
      desktop = (arg == "--desktop");
  }

  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);  
  QString cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  
  QStringList databaseLookupDirectories;

  // if user has Maps directory in "Documents" already, we will use it
  // for compatibility reasons
  if (QFile::exists(docs + QDir::separator() + "Maps")) {
    databaseLookupDirectories << docs + QDir::separator() + "Maps";
  }else{
    // we will use Maps in "Home" directory otherwise
    databaseLookupDirectories << home + QDir::separator() + "Maps";
  }

  // QStorageInfo available from Qt >= 5.4
#ifdef HAS_QSTORAGE
  for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {

    QString mountPoint = storage.rootPath();

    // Sailfish OS specific mount point base for SD cards!
    if (storage.isValid() &&
        storage.isReady() &&
        (mountPoint.startsWith("/media") ||
         mountPoint.startsWith("/run/media/") /* SFOS >= 2.2 */ )
         ) {

      qDebug() << "Found storage:" << mountPoint;
      databaseLookupDirectories << mountPoint + QDir::separator() + "Maps";
    }
  }
#endif

  // install translator
  QTranslator translator;
  QLocale locale;
  if (translator.load(locale.name(), SailfishApp::pathTo("translations").toLocalFile())) {
    qDebug() << "Install translator for locale " << locale << "/" << locale.name();
    app->installTranslator(&translator);
  }else{
    qWarning() << "Can't load translator for locale" << locale << "/" << locale.name() <<
            "(" << SailfishApp::pathTo("translations").toLocalFile() << ")";
  }

  bool initSuccess=OSMScoutQt::NewInstance()
    .WithOnlineTileProviders(SailfishApp::pathTo("resources/online-tile-providers.json").toLocalFile())
    .WithMapProviders(SailfishApp::pathTo("resources/map-providers.json").toLocalFile())
    .WithBasemapLookupDirectory(SailfishApp::pathTo("resources/world").toLocalFile())
    .WithMapLookupDirectories(databaseLookupDirectories)
    .AddCustomPoiType("_highlighted")
    .AddCustomPoiType("_waypoint")
    .WithCacheLocation(cache + QDir::separator() + "OsmTileCache")
    .WithIconDirectory(SailfishApp::pathTo("map-icons").toLocalFile())
    .WithStyleSheetDirectory(SailfishApp::pathTo("map-styles").toLocalFile())
    .WithTileCacheSizes(/* online */ desktop ?  40 : 40, /* offline */ desktop ? 200 : 50)
    .WithUserAgent("OSMScoutForSFOS", OSMSCOUT_SAILFISH_VERSION_STRING)
    .Init();

  if (!initSuccess) {
    std::cerr << "Cannot initialize OSMScoutQt" << std::endl;
    return 1;
  }

  Storage::initInstance(QStandardPaths::writableLocation(QStandardPaths::DataLocation));

  if (!desktop) {
    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty("OSMScoutVersionString", OSMSCOUT_SAILFISH_VERSION_STRING);
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->showFullScreen();
    result=app->exec();
  } else {
    QQmlApplicationEngine window(SailfishApp::pathTo("qml/desktop.qml"));
    result=app->exec();
  }

  Storage::clearInstance();
  OSMScoutQt::FreeInstance();

  return result;
}
