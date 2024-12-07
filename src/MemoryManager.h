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

#include <osmscoutclientqt/OSMScoutQt.h>

#include <QQmlEngine>

#include <memory>

enum class MemoryLevel
{
  Normal,
  Warning,
  Critical
};

class MemoryWatcher: public QObject {
  Q_OBJECT

signals:
  void memoryLevelChanged(const MemoryLevel &level);
};

/**
 * This watcher implementation listen for events about memory state from MCE daemon
 * (Mode Control Entity, https://sailfishos.org/wiki/Mce).
 */

class MCIMemoryWatcher: public MemoryWatcher {
  Q_OBJECT

public slots:
  void mciMemoryLevelChanged(const QString &level);

public:
  MCIMemoryWatcher();
  ~MCIMemoryWatcher() override = default;
};

struct FreeSpaceLevel
{
  double warning{0};
  double critical{0};
};

class ProcMemoryWatcher: public MemoryWatcher {
  Q_OBJECT

public slots:
  void onTimeout();

public:
  explicit ProcMemoryWatcher(const QSettings &setting);
  ~ProcMemoryWatcher() override = default;

private:
  // map of process oom score adjustment to free space levels
  std::map<int, FreeSpaceLevel> levelMap;
  QTimer timer;
  MemoryLevel level{MemoryLevel::Normal};
};


/**
 * This class listen for events about memory state pressure
 * and tries to release some cache when system memory level
 * is "warning" or "critical" (third state is "normal").
 */
class MemoryManager: public QObject {
  Q_OBJECT

public slots:
  void memoryLevelChanged(const MemoryLevel &level);
  void onTimeout();

public:
  explicit MemoryManager(QQmlEngine* engine);
  ~MemoryManager() override = default;

private:
  std::unique_ptr<MemoryWatcher> watcher;
  QQmlEngine* qmlEngine;
  QTimer timer;
  std::chrono::milliseconds cacheValidity=std::chrono::minutes(10);
  bool trimAlloc{false};
  bool callGc{false};
  osmscout::Signal<std::chrono::milliseconds> flushCachesRequest;
};
