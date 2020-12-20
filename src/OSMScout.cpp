/*
  OSMScout for SFOS
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

#include <osmscout/util/Logger.h>
#include <osmscout/OSMScoutQt.h>

#include "AppSettings.h" // Application settings
#include "IconProvider.h" // IconProvider
#include "Arguments.h"
#include "MemoryManager.h"
#include "LocFile.h"

// collections
#include "Storage.h"
#include "CollectionModel.h"
#include "CollectionListModel.h"
#include "CollectionTrackModel.h"
#include "CollectionMapBridge.h"
#include "Tracker.h"

#include "SearchHistoryModel.h"

#include <harbour-osmscout/private/Config.h>
#include <harbour-osmscout/private/Version.h>

// SFOS
#include <sailfishapp/sailfishapp.h>

// Qt includes
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QStandardPaths>
#include <QQmlContext>
#include <QFileInfo>
#include <QtCore/QtGlobal>

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QStorageInfo>

// std
#include <iostream>

#ifndef OSMSCOUT_SAILFISH_VERSION_STRING
static_assert(false, "OSMSCOUT_SAILFISH_VERSION_STRING should be defined by build system");
#endif

#ifndef LIBOSMSCOUT_GIT_HASH
static_assert(false, "LIBOSMSCOUT_GIT_HASH should be defined by build system");
#endif

using namespace osmscout;

static QObject *appSettingsSingletontypeProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  return new AppSettings();
}

std::string osPrettyName(){
  QSettings osRelease("/etc/os-release", QSettings::IniFormat);
  QVariant prettyName=osRelease.value("PRETTY_NAME");
  if (prettyName.isValid()){
    return prettyName.toString().toStdString();
  } else {
    return "Unknown OS";
  }
}

std::string versionStrings(){
  std::stringstream ss;
  ss << "harbour-osmscout"
     << " " << OSMSCOUT_SAILFISH_VERSION_STRING
     << " (libosmscout " << LIBOSMSCOUT_GIT_HASH << ", Qt " << qVersion() << ", " << osPrettyName() << ")";

  return ss.str();
}

Q_DECL_EXPORT int main(int argc, char* argv[])
{
#ifdef Q_WS_X11
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

  QGuiApplication *app = SailfishApp::application(argc, argv);

  app->setOrganizationDomain("libosmscout.sf.net");
  app->setApplicationName("harbour-osmscout"); // Harbour name have to be used - for correct cache dir
  app->setApplicationVersion(OSMSCOUT_SAILFISH_VERSION_STRING);

  ArgParser argParser(app, argc, argv);

  osmscout::CmdLineParseResult argResult=argParser.Parse();
  if (argResult.HasError()) {
    std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  Arguments args=argParser.GetArguments();
  if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }
  if (args.version) {
    std::cout << versionStrings() << std::endl;
    return 0;
  }

  osmscout::log.Debug(args.logLevel >= Arguments::LogLevel::Debug);
  osmscout::log.Info(args.logLevel >= Arguments::LogLevel::Info);
  osmscout::log.Warn(args.logLevel >= Arguments::LogLevel::Warn);
  osmscout::log.Error(args.logLevel >= Arguments::LogLevel::Error);

  std::cout << "Starting " << versionStrings() << std::endl;

#if defined(HAVE_MMAP)
  qDebug() << "Usage of memory mapped files is supported.";
#else
  qWarning() << "Usage of memory mapped files is NOT supported.";
#endif

  QByteArray envVar = qgetenv("NEMO_RESOURCE_CLASS_OVERRIDE");
  if (envVar.isEmpty()) {
    // setup audio class to navigator, music player is paused on navigation message and resumed then
    qputenv("NEMO_RESOURCE_CLASS_OVERRIDE", "navigator");
  }

#ifdef QT_QML_DEBUG
  qWarning() << "Starting QML debugger on port 1234.";
  qQmlEnableDebuggingHelper.startTcpDebugServer(1234);
#endif

  OSMScoutQt::RegisterQmlTypes("harbour.osmscout.map", 1, 0);

  qRegisterMetaType<MapView*>("MapView*");
  qRegisterMetaType<std::vector<Collection>>("std::vector<Collection>");
  qRegisterMetaType<std::vector<SearchItem>>("std::vector<SearchItem>");
  qRegisterMetaType<std::shared_ptr<std::vector<osmscout::gpx::TrackPoint>>>("std::shared_ptr<std::vector<osmscout::gpx::TrackPoint> >");
  qRegisterMetaType<std::optional<double>>("std::optional<double>");
  qRegisterMetaType<TrackStatistics>("TrackStatistics");
  qRegisterMetaType<Collection>("Collection");
  qRegisterMetaType<Track>("Track");
  qRegisterMetaType<Waypoint>("Waypoint");

  qmlRegisterType<CollectionListModel>("harbour.osmscout.map", 1, 0, "CollectionListModel");
  qmlRegisterType<CollectionModel>("harbour.osmscout.map", 1, 0, "CollectionModel");
  qmlRegisterType<CollectionTrackModel>("harbour.osmscout.map", 1, 0, "CollectionTrackModel");
  qmlRegisterType<CollectionMapBridge>("harbour.osmscout.map", 1, 0, "CollectionMapBridge");
  qmlRegisterType<Tracker>("harbour.osmscout.map", 1, 0, "Tracker");
  qmlRegisterType<SearchHistoryModel>("harbour.osmscout.map", 1, 0, "SearchHistoryModel");
  qmlRegisterType<LocFile>("harbour.osmscout.map", 1, 0, "LocFile");

  qmlRegisterSingletonType<AppSettings>("harbour.osmscout.map", 1, 0, "AppSettings", appSettingsSingletontypeProvider);

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

  // setup c++ locale
  try {
    std::locale::global(std::locale(""));
  } catch (const std::runtime_error& e) {
    std::cerr << "Cannot set locale: \"" << e.what() << "\"" << std::endl;
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
    .WithVoiceProviders(SailfishApp::pathTo("resources/voice-providers.json").toLocalFile())
    .WithBasemapLookupDirectory(SailfishApp::pathTo("resources/world").toLocalFile())
    .WithMapLookupDirectories(databaseLookupDirectories)
    .AddCustomPoiType("_highlighted")
    .AddCustomPoiType("_waypoint")
    .AddCustomPoiType("_track")
    .WithCacheLocation(cache + QDir::separator() + "OsmTileCache")
    .WithIconDirectory(SailfishApp::pathTo("map-icons").toLocalFile())
    .WithStyleSheetDirectory(SailfishApp::pathTo("map-styles").toLocalFile())
    .WithTileCacheSizes(/* online */ args.desktop ?  60 : 50, /* offline */ args.desktop ? 200 : 60)
    .WithUserAgent("OSMScoutForSFOS", OSMSCOUT_SAILFISH_VERSION_STRING)
    .Init();

  if (!initSuccess) {
    std::cerr << "Cannot initialize OSMScoutQt" << std::endl;
    return 1;
  }

  Storage::initInstance(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
  MemoryManager memoryManager; // lives in UI thread

  int result;
  if (!args.desktop) {
    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty("OSMScoutVersionString", OSMSCOUT_SAILFISH_VERSION_STRING);
    view->engine()->addImageProvider(QLatin1String("harbour-osmscout"), new IconProvider());
    view->engine()->addImportPath(SailfishApp::pathTo("qmlplugins").toLocalFile());
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
