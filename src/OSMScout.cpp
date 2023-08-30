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

#include <osmscout/log/Logger.h>
#include <osmscoutclientqt/OSMScoutQt.h>

#include "AppSettings.h" // Application settings
#include "IconProvider.h" // IconProvider
#include "Arguments.h"
#include "MemoryManager.h"
#include "LocFile.h"

// collections
#include "Storage.h"
#include "CollectionModel.h"
#include "CollectionStatisticsModel.h"
#include "CollectionListModel.h"
#include "CollectionTrackModel.h"
#include "CollectionMapBridge.h"
#include "Tracker.h"

#include "SearchHistoryModel.h"
#include "NearWaypointModel.h"
#include "TrackElevationChartWidget.h"
#include "PositionSimulator.h"

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
#include <QTranslator>
#include <QtCore/QtGlobal>

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QStorageInfo>

// std
#include <iostream>
#include <sstream>

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

  app->setOrganizationDomain("osmscout.karry.cz");
  app->setOrganizationName("cz.karry.osmscout"); // needed for Sailjail
  app->setApplicationName("OSMScout");
  app->setApplicationVersion(OSMSCOUT_SAILFISH_VERSION_STRING);

  Arguments args;
  {
    ArgParser argParser(app, argc, argv);

    osmscout::CmdLineParseResult argResult = argParser.Parse();
    if (argResult.HasError()) {
      std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
      std::cout << argParser.GetHelp() << std::endl;
      return 1;
    }

    args = argParser.GetArguments();
    if (args.help) {
      std::cout << argParser.GetHelp() << std::endl;
      return 0;
    }
    if (args.version) {
      std::cout << versionStrings() << std::endl;
      return 0;
    }
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
    // setup audio class for proper audio routing and interaction with audio player
    // see SFOS forum for problem description: https://forum.sailfishos.org/t/playing-audio-file-with-qmediaplayer-pause-media-player/2732/5

    // pre-defined classes: navigator, call, camera, game, player, event ( https://github.com/qt/qtmultimedia/commit/1c5ea95 )
    // pulseaudio behaviour is configured in /etc/pulse/xpolicy.conf
    // best class should be *navigator*, but this class is routed to device speaker even when bluetooth headset is connected
    // *player* class is routed (pulse route_audio flag), but system player is paused and not resumed after voice message
    // *game* class seems to be best for car navigation
    qputenv("NEMO_RESOURCE_CLASS_OVERRIDE", "game");
  }

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
  qRegisterMetaType<std::vector<Storage::WaypointNearby>>("std::vector<Storage::WaypointNearby>");
  qRegisterMetaType<std::optional<osmscout::Color>>("std::optional<osmscout::Color>");

  qmlRegisterType<CollectionListModel>("harbour.osmscout.map", 1, 0, "CollectionListModel");
  qmlRegisterType<CollectionModel>("harbour.osmscout.map", 1, 0, "CollectionModel");
  qmlRegisterType<CollectionStatisticsModel>("harbour.osmscout.map", 1, 0, "CollectionStatisticsModel");
  qmlRegisterType<CollectionTrackModel>("harbour.osmscout.map", 1, 0, "CollectionTrackModel");
  qmlRegisterType<CollectionMapBridge>("harbour.osmscout.map", 1, 0, "CollectionMapBridge");
  qmlRegisterType<Tracker>("harbour.osmscout.map", 1, 0, "Tracker");
  qmlRegisterType<SearchHistoryModel>("harbour.osmscout.map", 1, 0, "SearchHistoryModel");
  qmlRegisterType<NearWaypointModel>("harbour.osmscout.map", 1, 0, "NearWaypointModel");
  qmlRegisterType<LocFile>("harbour.osmscout.map", 1, 0, "LocFile");
  qmlRegisterType<TrackElevationChartWidget>("harbour.osmscout.map", 1, 0, "TrackElevationChart");
  qmlRegisterType<PositionSimulator>("harbour.osmscout.map", 1, 0, "PositionSimulator");

  qmlRegisterSingletonType<AppSettings>("harbour.osmscout.map", 1, 0, "AppSettings", appSettingsSingletontypeProvider);

  QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString docsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  QString dataDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  
  QStringList databaseLookupDirectories;

  // lookup Maps in "Downloads" directory
  databaseLookupDirectories << downloadDir + QDir::separator() + "Maps";

  // and in SD card root / Maps directory
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
    .WithSettingsStorage(new QSettings(AppSettings::settingFile(), QSettings::NativeFormat, app))
    .AddOnlineTileProviders(SailfishApp::pathTo("resources/online-tile-providers.json").toLocalFile())
    .AddOnlineTileProviders(dataDir + QDir::separator() + "online-tile-providers.json")
    .AddMapProviders(SailfishApp::pathTo("resources/map-providers.json").toLocalFile())
    .AddMapProviders(dataDir + QDir::separator() + "map-providers.json")
    .AddVoiceProviders(SailfishApp::pathTo("resources/voice-providers.json").toLocalFile())
    .AddVoiceProviders(dataDir + QDir::separator() + "voice-providers.json")
    .WithBasemapLookupDirectory(SailfishApp::pathTo("resources/world").toLocalFile())
    .WithMapLookupDirectories(databaseLookupDirectories)
    .AddCustomPoiType("_highlighted")
    .AddCustomPoiType("_waypoint")
    .AddCustomPoiType("_waypoint_red_circle")
    .AddCustomPoiType("_waypoint_green_circle")
    .AddCustomPoiType("_waypoint_blue_circle")
    .AddCustomPoiType("_waypoint_yellow_circle")
    .AddCustomPoiType("_waypoint_red_square")
    .AddCustomPoiType("_waypoint_green_square")
    .AddCustomPoiType("_waypoint_blue_square")
    .AddCustomPoiType("_waypoint_yellow_square")
    .AddCustomPoiType("_waypoint_red_triangle")
    .AddCustomPoiType("_waypoint_green_triangle")
    .AddCustomPoiType("_waypoint_blue_triangle")
    .AddCustomPoiType("_waypoint_yellow_triangle")
    .AddCustomPoiType("_track")
    .WithCacheLocation(cacheDir + QDir::separator() + "OsmTileCache")
    .WithIconDirectory(SailfishApp::pathTo("map-icons").toLocalFile())
    .WithStyleSheetDirectory(SailfishApp::pathTo("map-styles").toLocalFile())
    .WithTileCacheSizes(/* online */ args.desktop ?  60 : 50, /* offline */ args.desktop ? 200 : 60)
    .WithUserAgent("OSMScoutForSFOS", OSMSCOUT_SAILFISH_VERSION_STRING)
    .Init();

  if (!initSuccess) {
    std::cerr << "Cannot initialize OSMScoutQt" << std::endl;
    return 1;
  }

  Storage::initInstance(dataDir);
  MemoryManager memoryManager; // lives in UI thread

  int result;
  if (!args.desktop) {
    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty("OSMScoutVersionString", OSMSCOUT_SAILFISH_VERSION_STRING);
    view->rootContext()->setContextProperty("PositionSimulationTrack", args.positionSimulatorFile);
    view->engine()->addImageProvider(QLatin1String("harbour-osmscout"), new IconProvider());
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->showFullScreen();
    result=app->exec();
  } else {
    QQmlApplicationEngine window(SailfishApp::pathTo("qml/desktop.qml"));
    result=app->exec();
  }

  Storage::clearInstance();
  if (args.shutdownWait) {
    OSMScoutQt::GetInstance().waitForReleasingResources(100, std::numeric_limits<long>::max());
  }
  OSMScoutQt::FreeInstance();

  return result;
}
