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

#include "LocFile.h"

#include <osmscout/async/Breaker.h>
#include <osmscoutgpx/Utils.h>

#include <QTemporaryFile>
#include <QDebug>

#include <libxml/xmlwriter.h>

#include <sstream>

class LocWritter {
private:
  static constexpr char const *Encoding = "utf-8";

  xmlTextWriterPtr writer;
  osmscout::gpx::ProcessCallbackRef callback;
  osmscout::BreakerRef breaker;

private:
  bool StartElement(const char *name)
  {
    if (writer==nullptr){
      return false;
    }
    if (xmlTextWriterStartElement(writer, (const xmlChar *)name) < 0) {
      if (callback) {
        callback->Error("Error at xmlTextWriterStartElement");
      }
      return false;
    }
    return true;
  }

  bool WriteAttribute(const char *name, const char *content)
  {
    if (writer==nullptr){
      return false;
    }
    if (xmlTextWriterWriteAttribute(writer, (const xmlChar *)name, (const xmlChar *)content) < 0) {
      if (callback) {
        callback->Error("Error at xmlTextWriterWriteAttribute");
      }
      return false;
    }
    return true;
  }

  bool WriteAttribute(const char *name, double value, std::streamsize precision = 6)
  {
    std::ostringstream stream;
    stream.setf(std::ios::fixed,std::ios::floatfield);
    stream.imbue(std::locale("C"));
    stream.precision(precision);
    stream << value;

    return WriteAttribute(name, stream.str().c_str());
  }


  bool WriteTextElement(const char *elementName, const std::string &text)
  {
    if (writer==nullptr){
      return false;
    }
    if (xmlTextWriterWriteElement(writer, (const xmlChar *)elementName, (const xmlChar *)text.c_str()) < 0) {
      if (callback) {
        callback->Error("Error at xmlTextWriterWriteElement");
      }
      return false;
    }
    return true;
  }

  bool StartDocument()
  {
    if (xmlTextWriterStartDocument(writer, nullptr, Encoding, nullptr) < 0) {
      if (callback) {
        callback->Error("Error at xmlTextWriterStartDocument");
      }
      return false;
    }
    return true;
  }

  bool EndDocument()
  {
    if (xmlTextWriterEndDocument(writer) < 0) {
      if (callback) {
        callback->Error("Error at xmlTextWriterEndDocument");
      }
      return false;
    }
    return true;
  }

  bool EndElement()
  {
    if (writer==nullptr){
      return false;
    }
    if (xmlTextWriterEndElement(writer) < 0) {
      if (callback) {
        callback->Error("Error at xmlTextWriterStartElement");
      }
      return false;
    }
    return true;
  }

  bool WriteWaypoint(double lat, double lon, const QString &name)
  {
    return StartElement("waypoint") &&
           WriteTextElement("name", name.toStdString()) &&
           StartElement("coord") &&
           WriteAttribute("lat", lat) &&
           WriteAttribute("lon", lon) &&
           EndElement() && // coord
           EndElement(); // waypoint
  }

public:
  LocWritter(const std::string &filePath,
             osmscout::BreakerRef breaker,
             osmscout::gpx::ProcessCallbackRef callback):
    writer(nullptr),
    callback(callback),
    breaker(breaker)
  {
    /* Create a new XmlWriter for uri, with no compression. */
    writer = xmlNewTextWriterFilename(filePath.c_str(), 0);
    if (writer == nullptr && callback) {
      callback->Error("Error creating the xml writer");
    }
    if (writer) {
      if (xmlTextWriterSetIndent(writer, 1) < 0){
        callback->Error("Error at xmlTextWriterSetIndent");
      }
    }
  }

  ~LocWritter()
  {
    if (writer!=nullptr){
      xmlFreeTextWriter(writer);
      writer=nullptr;
    }
  }

  bool Process(double lat, double lon, const QString &name) {
    if (writer == nullptr) {
      return false;
    }

    //
    return StartDocument() &&
           StartElement("loc") &&
           WriteAttribute("version", "1.0") &&
           WriteAttribute("src", "OSM Scout for Sailfish OS") &&
           WriteWaypoint(lat, lon, name) &&
           EndDocument();
  }
};

QString LocFile::writeLocFile(double placeLat, double placeLon, const QString &name)
{
  QTemporaryFile *tmpFile = new QTemporaryFile("osmscout-XXXXXX.loc", this); // destructed and file deleted with LocFile
  if (!tmpFile->open()) {
    return "";
  }
  tmpFile->close();

  LocWritter writter(
    tmpFile->fileName().toStdString(),
    nullptr,
    std::make_shared<osmscout::gpx::ProcessCallback>());

  if (!writter.Process(placeLat, placeLon, name)){
    return "";
  }

  return tmpFile->fileName();
}
