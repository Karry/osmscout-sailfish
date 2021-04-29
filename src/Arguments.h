/*
  OSMScout for SFOS
  Copyright (C) 2019  Lukáš Karas

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

#include <osmscout/util/CmdLineParsing.h>
#include <iostream>

struct Arguments {
  enum class LogLevel: int {
    None = 0,
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4
  };

  bool help{false};
  bool version{false};
  LogLevel logLevel{LogLevel::Warn};
  bool desktop{false};
  QString positionSimulatorFile;
};

class ArgParser: public osmscout::CmdLineParser
{
private:
  Arguments args;

public:
ArgParser(QGuiApplication *app,
          int argc, char* argv[])
          : osmscout::CmdLineParser(app->applicationName().toStdString(), argc, argv)
{
  AddOption(osmscout::CmdLineFlag([this](const bool& value) {
              args.help=value;
            }),
            std::vector<std::string>{"h","help"},
            "Display help and exits",
            true);

  AddOption(osmscout::CmdLineFlag([this](const bool& value) {
              args.version=value;
            }),
            std::vector<std::string>{"v","version"},
            "Display application version and exits",
            false);

  AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
              auto str=osmscout::UTF8StringToLower(value);
              if (str=="debug"){
                args.logLevel=Arguments::LogLevel::Debug;
              } else if (str=="info"){
                args.logLevel=Arguments::LogLevel::Info;
              } else if (str=="warn"){
                args.logLevel=Arguments::LogLevel::Warn;
              } else if (str=="error") {
                args.logLevel = Arguments::LogLevel::Error;
              } else if (str=="none") {
                args.logLevel = Arguments::LogLevel::None;
              } else {
                osmscout::log.Error() << "Unknown log level " << value;
              }
            }),
            "log",
            "Set logging level (debug, info, warn, error, none). Default warn.",
            false);

  AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
              args.positionSimulatorFile = QString::fromStdString(value);
            }),
            "simulate-position",
            "Simulate position by record from gpx file",
            false);

  AddOption(osmscout::CmdLineFlag([this](const bool& value) {
              args.desktop=value;
            }),
            "desktop",
            "Use desktop UI (Qt Quick instead of Silica)",
            false);

}

Arguments GetArguments() const
{
  return args;
}

};
