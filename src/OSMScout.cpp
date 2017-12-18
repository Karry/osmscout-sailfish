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

// std
#include <iostream>

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

#include <sailfishapp/sailfishapp.h>

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
#include <osmscout/private/Config.h>

#ifndef OSMSCOUT_SAILFISH_VERSION_STRING
#warning "OSMSCOUT_SAILFISH_VERSION_STRING should be defined by build system"
#define OSMSCOUT_SAILFISH_VERSION_STRING "?.?.?"
#endif

// Library settings
#include <osmscout/Settings.h>
// Application settings
#include "AppSettings.h"

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
    
  int           result;  

#if defined(HAVE_MMAP)
  qDebug() << "Usage of memory mapped files is supported.";
#else
  qWarning() << "Usage of memory mapped files is NOT supported.";
#endif

  OSMScoutQt::RegisterQmlTypes("harbour.osmscout.map", 1, 0);
  qRegisterMetaType<MapView*>();

  qmlRegisterType<LocationInfoModel>("harbour.osmscout.map", 1, 0, "LocationInfoModel");
  qmlRegisterType<OnlineTileProviderModel>("harbour.osmscout.map", 1, 0, "OnlineTileProviderModel");
  qmlRegisterType<MapStyleModel>("harbour.osmscout.map", 1, 0, "MapStyleModel");
  qmlRegisterType<StyleFlagsModel>("harbour.osmscout.map", 1, 0, "StyleFlagsModel");

  qmlRegisterSingletonType<AppSettings>("harbour.osmscout.map", 1, 0, "AppSettings", appSettingsSingletontypeProvider);

  osmscout::log.Debug(true);

  bool desktop = false;
  for (QString arg: app->arguments()){
      desktop |= (arg == "--desktop");
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

  // we should use QStorageInfo for Qt >= 5.4
  QFile file("/etc/mtab"); // Linux specific
  if (file.open(QFile::ReadOnly)) {
    while(true) {
      QStringList parts = QString::fromUtf8(file.readLine()).trimmed().split(" ");
      if (parts.count() > 1) {
        QString mountPoint=parts[1].replace("\\040", " ");
        if (mountPoint.startsWith("/media") && QFileInfo(mountPoint).isDir()){ // Sailfish OS specific mount point base for SD cards!
          qDebug() << "Found storage:" << mountPoint;
          databaseLookupDirectories << mountPoint + QDir::separator() + "Maps";
        }
      } else {
        break;
      }
    }
  }

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

  OSMScoutQt::FreeInstance();

  return result;
}
