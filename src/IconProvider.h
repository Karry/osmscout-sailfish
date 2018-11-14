/**
 * Copyright (C) 2014	Kimmo Lindholm ( https://together.jolla.com/users/196/kimmoli/ )
 *
 * https://together.jolla.com/question/44325/iconbutton-how-to-use-own-icons-with-highlight/
 *
 *             DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 */

#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include <sailfishapp/sailfishapp.h>

#include <QQuickImageProvider>
#include <QPainter>
#include <QColor>

#include <cassert>
#include <QtSvg/QSvgRenderer>

class IconProvider : public QQuickImageProvider
{
public:
  IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap)
  {
  }

  QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
  {
    QStringList parts = id.split('?');
    assert(!parts.isEmpty());

    QString iconPath = SailfishApp::pathTo(parts.at(0)).toString(QUrl::RemoveScheme);
    qDebug() << "Loading icon " << iconPath;
    QPixmap sourcePixmap;
    if (iconPath.endsWith(".svg") && requestedSize.isValid()){
      QSvgRenderer renderer(iconPath);
      if (renderer.isValid()) {
        sourcePixmap = QPixmap(requestedSize);
        sourcePixmap.fill(Qt::transparent);

        QPainter painter(&sourcePixmap);
        renderer.render(&painter, sourcePixmap.rect());
        painter.end();
      }
    }else {
      sourcePixmap.load(iconPath);
    }

    if (size) {
      *size = sourcePixmap.size();
    }

    if (parts.length() > 1) {
      QString colorString = parts.at(1);
      if (QColor::isValidColor(colorString)) {
        QPainter painter(&sourcePixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(sourcePixmap.rect(), colorString);
        painter.end();
      }
    }

    if (requestedSize.width() > 0 && requestedSize.height() > 0) {
      return sourcePixmap.scaled(requestedSize.width(), requestedSize.height(), Qt::IgnoreAspectRatio);
    } else {
      return sourcePixmap;
    }
  }
};

#endif // ICONPROVIDER_H

