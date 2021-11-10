/*
  OSMScout for SFOS
  Copyright (C) 2020 Lukas Karas

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

#include "MemoryManager.h"

#include <osmscoutclientqt/OSMScoutQt.h>

#include <QDebug>
#include <QtDBus/QtDBus>

#include <malloc.h>

using namespace osmscout;

MemoryManager::MemoryManager()
{
  memoryLevelChanged("normal");

  QDBusConnection systemBus = QDBusConnection::systemBus();

  if (!systemBus.isConnected()) {
    qWarning() << "System bus is not connected!";
  } else {
    QString service = "com.nokia.mce";

    // subscribe
    systemBus.connect(service, "/com/nokia/mce/signal", "com.nokia.mce.signal", "sig_memory_level_ind",
      this, SLOT(memoryLevelChanged(const QString &)));

    // get current value
    QDBusInterface iface(service, "/com/nokia/mce/request", "com.nokia.mce.request", systemBus);
    if (!iface.isValid()) {
      qWarning() << "MCE interface is not valid";
    } else {
      QDBusReply<QString> reply = iface.call("get_memory_level", "");
      if (!reply.isValid()) {
        qWarning() << "Getting memory level fails with" << reply.error().message();
      } else {
        memoryLevelChanged(reply.value());
      }
    }
  }

  connect(&timer, &QTimer::timeout, this, &MemoryManager::onTimeout);
  timer.setSingleShot(false);

  auto dbThread=OSMScoutQt::GetInstance().GetDBThread();
  assert(dbThread);
  connect(this, &MemoryManager::flushCachesRequest,
          dbThread.get(), &DBThread::FlushCaches,
          Qt::QueuedConnection);
}

void MemoryManager::onTimeout()
{
  using namespace std::chrono;
  malloc_stats();
  emit flushCachesRequest(duration_cast<milliseconds>(cacheValidity).count());
  if (trimAlloc){
    if (malloc_trim(0) == 0){
      // The malloc_trim() function returns 1 if memory was actually released
      // back to the system, or 0 if it was not possible to release any
      // memory.
      qWarning() << "No memory can be returned";
    }
  }
}

void MemoryManager::memoryLevelChanged(const QString &level)
{
  using namespace std::chrono;

  // https://sailfishos.org/wiki/Mce
  qDebug() << "Memory level:" << level;
  if (level == "critical") {
    cacheValidity=seconds(20);
    trimAlloc=true;
    timer.start(duration_cast<milliseconds>(cacheValidity).count());
    onTimeout();
  } else if (level == "warning") {
    cacheValidity=minutes(1);
    trimAlloc=false;
    timer.start(duration_cast<milliseconds>(cacheValidity).count());
    onTimeout();
  } else if (level == "normal") {
    trimAlloc=false;
    timer.stop();
  } else {
    qWarning() << "Unsupported Memory level:" << level;
  }
}
