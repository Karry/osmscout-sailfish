/*
  OSMScout for SFOS
  Copyright (C) 2021  Lukáš Karas

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

#include "Migration.h"

#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>

#include <cassert>

Migration::Migration(const QString &home,
                     const QString &oldOrganization,
                     const QString &oldAppName,
                     const QString &newOrganization,
                     const QString &newAppName):
  home(home),
  oldOrganization(oldOrganization),
  oldAppName(oldAppName),
  newOrganization(newOrganization),
  newAppName(newAppName)
{}

Migration::Migration(const QString &oldOrganization,
                     const QString &oldAppName):
  Migration(QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
            oldOrganization,
            oldAppName,
            QCoreApplication::organizationName(),
            QCoreApplication::applicationName())
{}

QString Migration::cacheDir(const QString &organization, const QString &appName) const
{
  return home.absolutePath() + QDir::separator() +
         ".cache" + QDir::separator() +
         (organization.isEmpty() ? appName : organization) + QDir::separator() +
         appName;
}

QString Migration::configFile(const QString &organization, const QString &appName, bool defaultQtConfig) const
{
  return home.absolutePath() + QDir::separator() +
         ".config" + QDir::separator() +
         (organization.isEmpty() ? appName : organization) + QDir::separator() +
         (defaultQtConfig ? "" : appName + QDir::separator()) +
         appName + ".conf";
}

QString Migration::localDir(const QString &organization, const QString &appName) const
{
  return home.absolutePath() + QDir::separator() +
        ".local" + QDir::separator() +
        "share" + QDir::separator() +
         (organization.isEmpty() ? appName : organization) + QDir::separator() +
         appName;
}

bool Migration::migrate(const QString &oldLocation, const QString &newLocation) const
{
  qDebug() << "Considering migration" << oldLocation << "to" << newLocation;
  QFileInfo oldInfo(oldLocation);
  QFileInfo newInfo(newLocation);
  if (oldInfo.exists() && !newInfo.exists()){
    QDir parent = newInfo.dir();
    if (!parent.mkpath(parent.absolutePath())){
      qWarning() << "Failed to create path" << parent.absolutePath();
      return false;
    }
    if (!QFile::rename(oldLocation, newLocation)){
      qWarning() << "Failed to move" << oldLocation << "to" << newLocation;
      return false;
    }
    qDebug() << "Migrate" << oldLocation << "to" << newLocation;
    return true;
  }
  return false;
}

bool Migration::migrateConfig() const
{
  // try  ~/.config/harbour-osmscout/harbour-osmscout.conf -> ~/.config/cz.karry.osmscout/OSMScout/OSMScout.conf
  if (migrate(configFile(oldOrganization, oldAppName, true),
              configFile(newOrganization, newAppName, false))) {
    return true;
  }
  // try ~/.config/cz.karry.osmscout/OSMScout.conf -> ~/.config/cz.karry.osmscout/OSMScout/OSMScout.conf
  if (migrate(configFile(newOrganization, newAppName, true),
              configFile(newOrganization, newAppName, false))) {
    return true;
  }
  return false;
}

bool Migration::migrateLocal() const
{
  return migrate(localDir(oldOrganization, oldAppName),
                 localDir(newOrganization, newAppName));
}

bool Migration::wipeOldCache() const
{
  QString cache = cacheDir(oldOrganization, oldAppName);
  qDebug() << "Considering cache wipe" << cache;
  assert(!cache.isEmpty());
  assert(cache != "/");
  assert(cache != ".");
  assert(cache != home.absolutePath());
  if (QFileInfo(cache).exists()) {
    qDebug() << "Wiping old application cache" << cache;
    return QDir(cache).removeRecursively();
  }
  return false;
}
