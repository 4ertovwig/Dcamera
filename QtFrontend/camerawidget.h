/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include "qt_macros_fix.h"

#ifdef kill_dependency
#undef kill_dependency
#endif

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QByteArray>
#include <QThread>
#include <QtGui>
#include <QPushButton>
#include <QDebug>

#include "painter.h"
#include "videostream.h"
#include "audiostream.h"

class CameraWidget : public QWidget
{
    Q_OBJECT

public:
    CameraWidget(Painter *painter, VideoStream *videostream, AudioStream *audiostream, QPushButton *btn, QWidget *parent = nullptr);
    ~CameraWidget();

signals:
    void stop();

public slots:
    void setImage(const QImage &newImage);
    void onClicked(bool);

protected:
    void paintEvent(QPaintEvent *) override;
    void closeEvent(QCloseEvent *event) override; 

private:
    QImage m_image;
    Painter *m_painter;
    VideoStream *m_video_stream;
    AudioStream *m_audio_stream;
    QPushButton *m_btn;
    QThread *painterThread;
    QThread *VideoStreamThread;
    QThread *AudioStreamThread;
};
