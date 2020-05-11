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

#pragma once

#include <osmscout/util/String.h>
#include <osmscout/util/Distance.h>

#include <QVariant>
#include <QDateTime>

namespace converters {

inline QDateTime timestampToDateTime(const std::optional<osmscout::Timestamp> timestamp)
{
  if (timestamp.has_value()){
    using namespace std::chrono;
    milliseconds millisSinceEpoch = duration_cast<milliseconds>(timestamp->time_since_epoch());
    return QDateTime::fromMSecsSinceEpoch(millisSinceEpoch.count());
  }
  return QDateTime();
}

inline QString dateTimeToSQL(const QDateTime &dateTime)
{
  // sqlite store timestamp as string (omfg...)
  // so, do it in UTC to avoid troubles with timezones

  // QtSQL don't store timezone (Qt < 5.12),
  // we have to serialise QDateTime to string yourself
  // https://bugreports.qt.io/browse/QTBUG-60456

  // use Qt::ISODateWithMs with newer Qt
  return dateTime.toUTC().toString("yyyy-MM-ddTHH:mm:ss.zzzZ");
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

inline std::optional<std::string> varToStringOpt(const QVariant &var)
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::QString)) {
    return var.toString().toStdString();
  }

  return std::nullopt;
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

/**
 * there is slow conversion: QString -> QDateTime
 * consider to use SQL strftime('%s', ...) and varLongToOptTimestamp then
 */
inline QDateTime varToDateTime(const QVariant &var, QDateTime def = QDateTime())
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::QDateTime)) {
    return var.toDateTime();
  }

  return def;
}

inline std::optional<osmscout::Timestamp> varLongToOptTimestamp(const QVariant &var)
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::LongLong)) {

    auto duration = std::chrono::seconds(varToLong(var));
    static_assert(std::is_same<osmscout::Timestamp::clock, std::chrono::system_clock>::value, "Timestamp clock have use unix epoch");
    return osmscout::Timestamp(duration);
  }

  return std::nullopt;
}

inline osmscout::Timestamp dateTimeToTimestamp(const QDateTime &datetime)
{
  int64_t millis = datetime.toMSecsSinceEpoch();
  auto duration = std::chrono::milliseconds(millis);
  static_assert(std::is_same<osmscout::Timestamp::clock, std::chrono::system_clock>::value, "Timestamp clock have use unix epoch");
  return osmscout::Timestamp(duration);
}

inline std::optional<osmscout::Timestamp> dateTimeToTimestampOpt(const QDateTime &datetime)
{
  if (datetime.isValid()){
    return dateTimeToTimestamp(datetime);
  } else {
    return std::nullopt;
  }
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

inline std::optional<double> varToDoubleOpt(const QVariant &var)
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::Double)) {
    return var.toDouble();
  }

  return std::nullopt;
}

inline std::optional<osmscout::Distance> varToDistanceOpt(const QVariant &var)
{
  auto m = varToDoubleOpt(var);
  if (m.has_value()) {
    return osmscout::Distance::Of<osmscout::Meter>(*m);
  }
  return std::nullopt;
}

}
