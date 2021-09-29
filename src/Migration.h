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

#pragma once

#include <QString>
#include <QDir>

/**
 * Linux specific utility for migrating Qt directories after change application name or organization.
 */
class Migration {
private:
  QDir home;
  QString oldOrganization;
  QString oldAppName;
  QString newOrganization;
  QString newAppName;

public:
  Migration(const QString &home,
            const QString &oldOrganization,
            const QString &oldAppName,
            const QString &newOrganization,
            const QString &newAppName);

  Migration(const QString &oldOrganization,
            const QString &oldAppName);

  virtual ~Migration() = default;

  QString cacheDir(const QString &organization, const QString &appName) const;

  /**
   * config file used by QSettings
   * @param organization
   * @param appName
   * @return
   */
  QString configFile(const QString &organization, const QString &appName) const;

  QString localDir(const QString &organization, const QString &appName) const;

  bool migrate(const QString &oldLocation, const QString &newLocation) const;
  bool migrateConfig() const;
  bool migrateLocal() const;
  bool wipeOldCache() const;
};
