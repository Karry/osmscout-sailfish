/*
  OSMScout for SFOS
  Copyright (C) 2018 Lukas Karas

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

#ifndef OSMSCOUT_SAILFISH_QVARIANTCONVERTERS_H
#define OSMSCOUT_SAILFISH_QVARIANTCONVERTERS_H

#include <osmscout/util/String.h>
#include <osmscout/util/Distance.h>
#include <osmscout/gpx/Optional.h>

#include <QVariant>
#include <QDateTime>

namespace converters {

inline QDateTime timestampToDateTime(const osmscout::gpx::Optional<osmscout::Timestamp> timestamp)
{
  if (timestamp.hasValue()){
    using namespace std::chrono;
    milliseconds millisSinceEpoch = duration_cast<milliseconds>(timestamp.get().time_since_epoch());
    return QDateTime::fromMSecsSinceEpoch(millisSinceEpoch.count());
  }
  return QDateTime();
}

inline qlonglong varToLong(const QVariant &var, qlonglong def = -1)
{
  bool ok;
  qlonglong val = var.toLongLong(&ok);
  return ok ? val : def;
}

inline QString varToString(const QVariant &var, QString def = "")
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::QString)) {
    return var.toString();
  }

  return def;
}

inline osmscout::gpx::Optional<std::string> varToStringOpt(const QVariant &var)
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::QString)) {
    return osmscout::gpx::Optional<std::string>::of(var.toString().toStdString());
  }

  return osmscout::gpx::Optional<std::string>();
}

inline bool varToBool(const QVariant &var, bool def = false)
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::Bool)) {
    return var.toBool();
  }

  return def;
}

inline QDateTime varToDateTime(const QVariant &var, QDateTime def = QDateTime())
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::QDateTime)) {
    return var.toDateTime();
  }

  return def;
}

inline osmscout::Timestamp dateTimeToTimestamp(const QDateTime &datetime)
{
  int64_t millis = datetime.toMSecsSinceEpoch();
  auto duration = std::chrono::milliseconds(millis);
  return osmscout::Timestamp(duration);
}

inline double varToDouble(const QVariant &var, double def = 0)
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::Double)) {
    return var.toDouble();
  }

  return def;
}

inline osmscout::gpx::Optional<double> varToDoubleOpt(const QVariant &var)
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::Double)) {
    return osmscout::gpx::Optional<double>::of(var.toDouble());
  }

  return osmscout::gpx::Optional<double>();
}

inline osmscout::gpx::Optional<osmscout::Distance> varToDistanceOpt(const QVariant &var)
{
  auto m = varToDoubleOpt(var);
  if (m.hasValue()) {
    return osmscout::gpx::Optional<osmscout::Distance>::of(osmscout::Distance::Of<osmscout::Meter>(m.get()));
  }
  return osmscout::gpx::Optional<osmscout::Distance>();
}

}

#endif //OSMSCOUT_SAILFISH_QVARIANTCONVERTERS_H
