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
  bool help{false};
  bool version{false};
  bool debug{false};
  bool desktop{false};
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
            "version",
            "Display application version and exits",
            false);

  AddOption(osmscout::CmdLineFlag([this](const bool& value) {
              args.debug=value;
            }),
            "debug",
            "Enable debug output",
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
