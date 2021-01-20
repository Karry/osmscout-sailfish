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

#include "TrackElevationChartWidget.h"

TrackElevationChartWidget::TrackElevationChartWidget(QQuickItem* parent)
  :osmscout::ElevationChartWidget(parent)
{
  Storage *storage = Storage::getInstance();
  assert(storage);

  connect(storage, &Storage::initialised,
          this, &TrackElevationChartWidget::storageInitialised,
          Qt::QueuedConnection);

  connect(storage, &Storage::initialisationError,
          this, &TrackElevationChartWidget::storageInitialisationError,
          Qt::QueuedConnection);

  connect(this, &TrackElevationChartWidget::trackDataRequest,
          storage, &Storage::loadTrackData,
          Qt::QueuedConnection);

  connect(storage, &Storage::trackDataLoaded,
          this, &TrackElevationChartWidget::onTrackDataLoaded,
          Qt::QueuedConnection);

  connect(this, &TrackElevationChartWidget::loadingChanged,
          this, &TrackElevationChartWidget::loadingChanged2);
}

void TrackElevationChartWidget::storageInitialised()
{
  if (track.id > 0) {
    loading = true;
    emit trackDataRequest(track, accuracyFilter);
    emit loadingChanged();
  }
}

void TrackElevationChartWidget::storageInitialisationError(QString)
{
  storageInitialised();
}

void TrackElevationChartWidget::onTrackDataLoaded(Track track, std::optional<double>, bool complete, bool /*ok*/)
{
  using namespace osmscout;
  if (track.id != this->track.id || accuracyFilter != this->accuracyFilter){
    return;
  }
  // TODO: error handling when !ok
  loading = !complete;
  if (complete) {
    QElapsedTimer timer;
    timer.start();
    points.clear();
    Distance distance;
    for (const auto &segment : track.data->segments) {
      points.reserve(points.size()+segment.points.size());
      std::optional<gpx::TrackPoint> previousPoint;
      for (const auto &point : segment.points) {
        if (previousPoint.has_value()){
          distance+=GetEllipsoidalDistance(previousPoint->coord, point.coord);
        }
        if (point.elevation.has_value()) {
          ElevationPoint pt{distance, Meters(point.elevation.value()), point.coord, nullptr};
          // qDebug() << "On" << pt.distance.AsMeter() << "ele" << pt.elevation.AsMeter();
          points.push_back(pt);
          if (!lowest.has_value() || lowest->elevation > pt.elevation){
            lowest=pt;
          }
          if (!highest.has_value() || highest->elevation < pt.elevation){
            highest=pt;
          }
        }
        previousPoint=point;
      }
    }
    // be careful with elapsed time, method is processed in UI thread
    qDebug() << "Preparing elevation profile took" << timer.elapsed() << "ms";
  }
  update();
  emit loadingChanged();
}

QString TrackElevationChartWidget::getTrackId() const
{
  return QString::number(track.id);
}

void TrackElevationChartWidget::setTrackId(QString id)
{
  bool ok;
  track.id = id.toLongLong(&ok);
  if (!ok)
    track.id = -1;
  storageInitialised();
}
