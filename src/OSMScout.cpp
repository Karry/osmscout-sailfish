/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings

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

// Qt includes
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QStandardPaths>

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>

// Custom QML objects
#include "MapWidget.h"
#include "SearchLocationModel.h"
#include "RoutingModel.h"

// Application settings
#include "Settings.h"

// Application theming
#include "Theme.h"

#include <osmscout/util/Logger.h>

Q_DECLARE_METATYPE(osmscout::TileRef)

static QObject *ThemeProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    Theme *theme = new Theme();

    return theme;
}

int main(int argc, char* argv[])
{
#ifdef Q_WS_X11
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

  QGuiApplication *app = SailfishApp::application(argc, argv);
  QScopedPointer<QQuickView> view(SailfishApp::createView());  
  
  SettingsRef     settings;
  int             result;

  app->setOrganizationName("libosmscout");
  app->setOrganizationDomain("libosmscout.sf.net");
  app->setApplicationName("OSMScout");

  qRegisterMetaType<RenderMapRequest>();
  qRegisterMetaType<DatabaseLoadedResponse>();
  qRegisterMetaType<osmscout::TileRef>();

  qmlRegisterType<MapWidget>("harbour.osmscout.map", 1, 0, "Map");
  qmlRegisterType<Location>("harbour.osmscout.map", 1, 0, "Location");
  qmlRegisterType<LocationListModel>("harbour.osmscout.map", 1, 0, "LocationListModel");
  qmlRegisterType<RouteStep>("harbour.osmscout.map", 1, 0, "RouteStep");
  qmlRegisterType<RoutingListModel>("harbour.osmscout.map", 1, 0, "RoutingListModel");

  qmlRegisterSingletonType<Theme>("harbour.osmscout.map", 1, 0, "Theme", ThemeProvider);

  osmscout::log.Debug(true);

  settings=std::make_shared<Settings>();

  QThread thread;

  QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);  
  QString cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  if (!DBThread::InitializeInstance(
          docs + QDir::separator() + "Maps", 
          "/usr/share/harbour-osmscout", 
          cache + QDir::separator() + "OsmTileCache", 
          /* It seems that Jolla reports incorrect DPI (100), 
           * maybe related with Andorid bug: https://bugreports.qt.io/browse/QTBUG-35701
           * 
           * It should be possible to override by environment variable in new Sailfish OS (2.0.1)
           * QT_WAYLAND_FORCE_DPI
           * https://together.jolla.com/question/126893/changelog-201taalojarvi/
           * 
           * But it doesn't work in current stable version (2.0.0.10), so we hardcode it in code :-(
           */
          245.0
          )) { 
    
    std::cerr << "Cannot initialize DBThread" << std::endl;
    return 1;
  }

  DBThread* dbThread=DBThread::GetInstance();
  
  dbThread->moveToThread(&thread);

  dbThread->connect(&thread, SIGNAL(started()), SLOT(Initialize()));
  dbThread->connect(&thread, SIGNAL(finished()), SLOT(Finalize()));

  view->setSource(SailfishApp::pathTo("qml/main.qml"));
  view->showFullScreen();
    
  thread.start();
  
  result=app->exec();

  thread.quit();
  thread.wait();

  DBThread::FreeInstance();

  return result;
}
