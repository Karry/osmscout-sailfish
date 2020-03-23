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

#pragma once

#include <osmscout/OSMScoutQt.h>

/**
 * This class listen for events about memory state from MCE daemon
 * (Mode Control Entity, https://sailfishos.org/wiki/Mce)
 * and tries to release some cahe when system memory level
 * is "warning" or "critical" (third state is "normal").
 */
class MemoryManager: public QObject {
  Q_OBJECT

public slots:
  void memoryLevelChanged(const QString &arg);
  void onTimeout();

signals:
  void flushCachesRequest(qint64 idleMs);

public:
  MemoryManager();
  virtual ~MemoryManager() = default;

private:
  QTimer timer;
  std::chrono::milliseconds cacheValidity=std::chrono::minutes(10);
  bool trimAlloc{false};
};
