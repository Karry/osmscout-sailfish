/*
  OSMScout for SFOS
  Copyright (C) 2021 Lukas Karas

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

#include <osmscoutclientqt/ElevationChartWidget.h>

#include "Storage.h"

class TrackElevationChartWidget : public osmscout::ElevationChartWidget
{
  Q_OBJECT
  Q_PROPERTY(QString trackId READ getTrackId WRITE setTrackId NOTIFY loadingChanged2)

signals:
  void trackDataRequest(Track track, std::optional<double>);
  void loadingChanged2();

public slots:
  void storageInitialised();
  void storageInitialisationError(QString);
  void onTrackDataLoaded(Track track, std::optional<double>, bool complete, bool ok);

public:
  TrackElevationChartWidget(QQuickItem* parent = nullptr);
  ~TrackElevationChartWidget() override = default;

  QString getTrackId() const;
  void setTrackId(QString id);

private:
  Track track;
  std::optional<double> accuracyFilter=100;
};
