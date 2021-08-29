/*
  QMediaPlaeyr test
  Copyright (C) 2020  Lukáš Karas

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

// SFOS
#include <sailfishapp/sailfishapp.h>

// Qt includes
#include <QGuiApplication>
#include <QStandardPaths>
#include <QtCore/QtGlobal>
#include <QObject>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QAudioDeviceInfo>
#include <QAudioOutputSelectorControl>
#include <QMediaService>

#include <MediaPlayer.h>

#include <cassert>

void MediaPlayerTest::playerStateChanged(QMediaPlayer::State state){
  if (state==QMediaPlayer::StoppedState) {
    qDebug() << "Stopped";
    qDebug() << "Player support" << mediaPlayer->supportedAudioRoles().size() << "roles";
    for (auto role : mediaPlayer->supportedAudioRoles()){
      qDebug() << "role:" << role;
    }
    app->quit();
  } else if (state==QMediaPlayer::PausedState) {
    qDebug() << "Paused";
  } else if (state==QMediaPlayer::PlayingState) {
    qDebug() << "Playing";
  }
}

MediaPlayerTest::MediaPlayerTest(QGuiApplication *app,
                                 const QString &file):
  app(app)
{
  mediaPlayer = new QMediaPlayer(this);
  QMediaPlaylist *currentPlaylist = new QMediaPlaylist(mediaPlayer);
  connect(mediaPlayer, &QMediaPlayer::stateChanged, this, &MediaPlayerTest::playerStateChanged);
  mediaPlayer->setPlaylist(currentPlaylist);

  qDebug() << "device count:" << QAudioDeviceInfo::availableDevices(QAudio::AudioOutput ).size();
  for (const auto &dev: QAudioDeviceInfo::availableDevices(QAudio::AudioOutput )){
    qDebug() << "QAudioDeviceInfo:" << dev.deviceName();
  }

  QMediaService *mediaService = mediaPlayer->service();
  assert(mediaService);
  QAudioOutputSelectorControl* ctl = mediaService->requestControl<QAudioOutputSelectorControl*>();
  if(ctl){
    for (const auto &out: ctl->availableOutputs()){
      qDebug() << "device:" << out;
    }
    mediaService->releaseControl(ctl);
  } else {
    qWarning() << "QAudioOutputSelectorControl is not available";
  }

  auto sampleUrl = QUrl::fromLocalFile(file);
  qDebug() << "Adding to playlist:" << sampleUrl;
  currentPlaylist->addMedia(sampleUrl);

  currentPlaylist->setCurrentIndex(0);
  mediaPlayer->play();
}

Q_DECL_EXPORT int main(int argc, char* argv[]){
#ifdef Q_WS_X11
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif
  QGuiApplication *app = SailfishApp::application(argc, argv);

  if (app->arguments().size()!=2){
    return 1;
  }

  MediaPlayerTest player(app,
                         app->arguments()[1]);

  return app->exec();
}
