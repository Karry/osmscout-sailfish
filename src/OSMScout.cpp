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
#include "LocationInfoModel.h"
#include "RoutingModel.h"

// Application settings
#include "Settings.h"

// Application theming
#include "Theme.h"

#include <osmscout/util/Logger.h>
#include <osmscout/private/Config.h>

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

  app->setOrganizationDomain("libosmscout.sf.net");
  app->setApplicationName("harbour-osmscout"); // Harbour name have to be used - for correct cache dir
    
  int           result;  

#if defined(HAVE_MMAP)
  qDebug() << "Usage of memory mapped files is supported.";
#else
  qWarning() << "Usage of memory mapped files is NOT supported.";
#endif

  qRegisterMetaType<RenderMapRequest>();
  qRegisterMetaType<DatabaseLoadedResponse>();
  qRegisterMetaType<osmscout::TileRef>();

  qmlRegisterType<MapWidget>("harbour.osmscout.map", 1, 0, "Map");
  qmlRegisterType<Location>("harbour.osmscout.map", 1, 0, "Location");
  qmlRegisterType<LocationListModel>("harbour.osmscout.map", 1, 0, "LocationListModel");
  qmlRegisterType<LocationInfoModel>("harbour.osmscout.map", 1, 0, "LocationInfoModel");
  qmlRegisterType<RouteStep>("harbour.osmscout.map", 1, 0, "RouteStep");
  qmlRegisterType<RoutingListModel>("harbour.osmscout.map", 1, 0, "RoutingListModel");
  qmlRegisterType<QmlSettings>("harbour.osmscout.map", 1, 0, "Settings");

  qmlRegisterSingletonType<Theme>("harbour.osmscout.map", 1, 0, "Theme", ThemeProvider);

  osmscout::log.Debug(true);

  QThread thread;

  bool desktop = false;
  for (QString arg: app->arguments()){
      desktop |= (arg == "--desktop");
  }
 
  QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);  
  QString cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  if (!DBThread::InitializeInstance(
          docs + QDir::separator() + "Maps", 
          "/usr/share/harbour-osmscout", 
          cache + QDir::separator() + "OsmTileCache",
          /* onlineTileCacheSize  */ desktop ?  40 : 20,
          /* offlineTileCacheSize */ desktop ? 200 : 50
          )) { 
    
    std::cerr << "Cannot initialize DBThread" << std::endl;
    return 1;
  }

  DBThread* dbThread=DBThread::GetInstance();
  
  dbThread->moveToThread(&thread);

  dbThread->connect(&thread, SIGNAL(started()), SLOT(Initialize()));
  dbThread->connect(&thread, SIGNAL(finished()), SLOT(Finalize()));
 
  QQmlApplicationEngine *window = NULL;
  if (!desktop){
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->showFullScreen();
  }else{
    window = new QQmlApplicationEngine(SailfishApp::pathTo("qml/desktop.qml"));
  }
    
  thread.start();
  
  result=app->exec();
  
  if (window!=NULL)
      window->deleteLater();

  thread.quit();
  thread.wait();

  DBThread::FreeInstance();
  Settings::FreeInstance();

  return result;
}
