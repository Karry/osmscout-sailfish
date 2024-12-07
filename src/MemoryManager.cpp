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
#include "AppSettings.h"

#include <osmscoutclientqt/OSMScoutQt.h>

#include <QDebug>
#include <QtDBus/QtDBus>

#include <malloc.h>

using namespace osmscout;

MCIMemoryWatcher::MCIMemoryWatcher()
{
  QDBusConnection systemBus = QDBusConnection::systemBus();

  if (!systemBus.isConnected()) {
    qWarning() << "System bus is not connected!";
  } else {
    QString service = "com.nokia.mce";

    // subscribe
    systemBus.connect(service, "/com/nokia/mce/signal", "com.nokia.mce.signal", "sig_memory_level_ind",
                      this, SLOT(mciMemoryLevelChanged(const QString &)));

    // get current value
    QDBusInterface iface(service, "/com/nokia/mce/request", "com.nokia.mce.request", systemBus);
    if (!iface.isValid()) {
      qWarning() << "MCE interface is not valid";
    } else {
      QDBusReply<QString> reply = iface.call("get_memory_level", "");
      if (!reply.isValid()) {
        qWarning() << "Getting memory level fails with" << reply.error().message();
      } else {
        mciMemoryLevelChanged(reply.value());
      }
    }
  }
}

void MCIMemoryWatcher::mciMemoryLevelChanged(const QString &level)
{
  // https://sailfishos.org/wiki/Mce
  qDebug() << "Memory level:" << level;
  if (level == "critical") {
    emit memoryLevelChanged(MemoryLevel::Critical);
  } else if (level == "warning") {
    emit memoryLevelChanged(MemoryLevel::Warning);
  } else if (level == "normal") {
    emit memoryLevelChanged(MemoryLevel::Normal);
  } else {
    qWarning() << "Unsupported Memory level:" << level;
  }
}

namespace {
  constexpr uint64_t PageSize = 4096;
  constexpr uint64_t MiB = 1024*1024;

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
  constexpr QString::SplitBehavior SkipEmptyParts = QString::SplitBehavior::SkipEmptyParts;
#else
  constexpr Qt::SplitBehaviorFlags SkipEmptyParts = Qt::SkipEmptyParts;
#endif

  size_t parseLine(const QString &line) {
    QStringList arr = line.split(" ", SkipEmptyParts);
    if (arr.size() != 3) {
      qWarning() << "Can't parse memory line" << line;
      return 0;
    }
    if (arr[2] != "kB") {
      qWarning() << "Can't parse memory line" << line;
      return 0;
    }
    bool ok = true;
    size_t val = arr[1].toULongLong(&ok);
    if (!ok) {
      qWarning() << "Can't parse memory line" << line;
      return 0;
    }
    return val * 1024;
  }

  uint64_t availableMemory()
  {
    uint64_t available = 0;

    QFile inputFile("/proc/meminfo");
    if (!inputFile.open(QIODevice::ReadOnly)) {
      qWarning() << "Can't open /proc/meminfo";
      return 0;
    }

    QTextStream in(&inputFile);

    for (QString line = in.readLine(); !line.isEmpty(); line = in.readLine()) {
      if (line.startsWith("MemFree:") || line.startsWith("Cached:")) {
        available += parseLine(line);
      }
    }

    return available;
  }

  QString readLine(const QString &file)
  {
    QFile inputFile(file);
    if (!inputFile.open(QIODevice::ReadOnly)) {
      qWarning() << "Can't open file" << file;
      return "";
    }

    return inputFile.readLine();
  }

  /**
  * see statm in `man proc`
  */
  struct StatM {
    uint64_t size{0};      //!< total program size
    //!< (same as VmSize in /proc/[pid]/status)
    uint64_t resident{0};  //!< resident set size
    //!< (same as VmRSS in /proc/[pid]/status)
    uint64_t shared{0};    //!< size of resident shared memory (i.e., backed by a file)
    //!< (same as RssFile+RssShmem in /proc/[pid]/status)
    uint64_t text{0};      //!< text (code)
    uint64_t lib{0};       //!< library (unused since Linux 2.6; always 0)
    uint64_t data{0};      //!< data + stack
    uint64_t dt{0};        //!< dirty pages (unused since Linux 2.6; always 0)
  };


  bool readStatM(StatM &statm) {
    QFile inputFile("/proc/self/statm");
    if (!inputFile.open(QIODevice::ReadOnly)) {
      qWarning() << "Can't open sattm file";
      return false;
    }
    QTextStream in(&inputFile);

    QString line = in.readLine();
    if (line.isEmpty()){
      qWarning() << "Can't parse statm";
      return false;
    }

    QStringList arr = line.split(" ", SkipEmptyParts);
    if (arr.size() < 7){
      qWarning() << "Can't parse statm";
      return false;
    }
    QVector<uint64_t> arri;
    arri.reserve(arr.size());
    for (const auto &numStr:arr){
      bool ok;
      arri.push_back(numStr.toULongLong(&ok) * 4096);
      if (!ok){
        qWarning() << "Can't parse statm";
        return false;
      }
    }

    statm.size = arri[0];
    statm.resident = arri[1];
    statm.shared = arri[2];
    statm.text = arri[3];
    statm.lib = arri[4];
    statm.data = arri[5];
    statm.dt = arri[6];

    return true;
  }

}

ProcMemoryWatcher::ProcMemoryWatcher(const QSettings &setting)
{
  // default values for critical are 110% of low-memory-killer's minfree from Xperia 10.II
  // and 120% for warning level (in MiB)
  QStringList adj = setting.value("MemoryManager/adj", "0,58,147,529,1000").toString().split(',');
  QStringList warningLevels = setting.value("MemoryManager/warning", "549,657,765,873,1202").toString().split(',');
  QStringList criticalLevels = setting.value("MemoryManager/critical", "503,602,701,800,1102").toString().split(',');

  if (adj.size() != warningLevels.size() || adj.size() != criticalLevels.size() || adj.empty() || warningLevels.empty()) {
    qWarning() << "memory watcher configuration is weird:" << adj << warningLevels;
  }
  for (int i = 0; i < std::min(std::min(adj.size(), warningLevels.size()), criticalLevels.size()); ++i) {
    int adjVal = adj[i].toInt();
    FreeSpaceLevel levels{
      double(warningLevels[i].toLongLong() * MiB),
      double(criticalLevels[i].toLongLong() * MiB)
    };

    qDebug() << "ProcMemoryWatcher configuration" << adjVal
             << "warning:" << QString::fromStdString(ByteSizeToString(levels.warning))
             << "critical:" << QString::fromStdString(ByteSizeToString(levels.critical));

    levelMap[adjVal] = levels;
  }

  if (!levelMap.empty()) {
    timer.setSingleShot(false);
    connect(&timer, &QTimer::timeout, this, &ProcMemoryWatcher::onTimeout);
    timer.start(4000);
  }
}

void ProcMemoryWatcher::onTimeout()
{
  int adj = readLine("/proc/self/oom_score_adj").toInt();
  StatM statm;
  if (!readStatM(statm)) {
    qWarning() << "Failed to parse statm";
  }
  qDebug() << "Self oom_score_adj:" << adj
           << "Rss:" << QString::fromStdString(ByteSizeToString(statm.resident))
           << "Anon:" << QString::fromStdString(ByteSizeToString(statm.resident - statm.shared));

  auto it = levelMap.upper_bound(adj);
  MemoryLevel currentLevel = MemoryLevel::Normal;
  assert(!levelMap.empty());
  if (it != levelMap.begin()) {
    FreeSpaceLevel &levels = (--it)->second;
    auto available = double(availableMemory());
    qDebug() << "warning level:" << QString::fromStdString(ByteSizeToString(levels.warning))
             << "critical level:" << QString::fromStdString(ByteSizeToString(levels.critical))
             << "available:" << QString::fromStdString(ByteSizeToString(available));
    if (available < levels.critical) {
      qDebug() << "Memory level: critical";
      currentLevel = MemoryLevel::Critical;
    } else if (available < levels.warning) {
      qDebug() << "Memory level: warning";
      currentLevel = MemoryLevel::Warning;
    }
  }
  if (currentLevel != level) {
    level = currentLevel;
    if (level==MemoryLevel::Normal) {
      timer.setInterval(4000);
    } else if (level==MemoryLevel::Warning) {
      timer.setInterval(2000);
    } else {
      assert(level==MemoryLevel::Critical);
      timer.setInterval(1000);
    }
    emit memoryLevelChanged(level);
  }
}

MemoryManager::MemoryManager(QQmlEngine* engine)
  : qmlEngine(engine)
{
  assert(qmlEngine!=nullptr);
  memoryLevelChanged(MemoryLevel::Normal);

  // MCE is using CGroup API for memory notifications when memnotify is not available
  // (all phones newer than Jolla 1). But CGroup api is not usable, it takes reclaimable
  // memory into account. So, we prefer to watch system memory ourself.
  // see https://www.karry.cz/karry/blog/2021/11/07/sailfishos_memory/

  // Ideal would be to autodetect low-memory-killer configuration from
  // /sys/module/lowmemorykiller/parameters/(adj|minfree) files,
  // but this configuration is not available from Sailjail!
  // So we need to rely on configuration with some feasible defaults
  QSettings setting(AppSettings::settingFile(), QSettings::NativeFormat, this);

  if (setting.value("MemoryManager/type", "proc").toString() == "proc") {
    qDebug() << "Using Proc memory watcher";
    watcher = std::make_unique<ProcMemoryWatcher>(setting);
  } else {
    qDebug() << "Using MCI memory watcher";
    watcher = std::make_unique<MCIMemoryWatcher>();
  }

  connect(watcher.get(), &MemoryWatcher::memoryLevelChanged, this, &MemoryManager::memoryLevelChanged);

  connect(&timer, &QTimer::timeout, this, &MemoryManager::onTimeout);
  timer.setSingleShot(false);

  auto dbThread=OSMScoutQt::GetInstance().GetDBThread();
  assert(dbThread);
  flushCachesRequest.Connect(dbThread->flushCaches);
}

void MemoryManager::onTimeout()
{
  using namespace std::chrono;
  malloc_stats();
  flushCachesRequest.Emit(duration_cast<milliseconds>(cacheValidity));
  if (callGc) {
    qmlEngine->collectGarbage();
  }
  if (trimAlloc){
    if (malloc_trim(0) == 0){
      // The malloc_trim() function returns 1 if memory was actually released
      // back to the system, or 0 if it was not possible to release any
      // memory.
      qWarning() << "No memory can be returned";
    }
  }
}

void MemoryManager::memoryLevelChanged(const MemoryLevel &level)
{
  using namespace std::chrono;

  // https://sailfishos.org/wiki/Mce
  if (level == MemoryLevel::Critical) {
    cacheValidity=seconds(1);
    callGc=true;
    trimAlloc=true;
    timer.start(duration_cast<milliseconds>(cacheValidity).count());
    onTimeout();
  } else if (level == MemoryLevel::Warning) {
    cacheValidity=seconds(10);
    callGc=false;
    trimAlloc=false;
    timer.start(duration_cast<milliseconds>(cacheValidity).count());
    onTimeout();
  } else if (level == MemoryLevel::Normal) {
    callGc=false;
    trimAlloc=false;
    timer.stop();
  } else {
    qWarning() << "Unsupported Memory level:" << int(level);
    assert(false);
  }
}
