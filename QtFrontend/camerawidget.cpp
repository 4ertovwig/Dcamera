/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <unistd.h>
#include "camerawidget.h"

CameraWidget::CameraWidget(Painter *painter, VideoStream *videostream, AudioStream *audiostream, QPushButton *btn, QWidget *parent)
    : QWidget(parent), m_painter(painter), m_video_stream(videostream), m_audio_stream(audiostream), m_btn(btn)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(1280, 720);
    setWindowFlags(Qt::Window);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    painterThread = new QThread(this);
    m_painter->moveToThread(painterThread);

    VideoStreamThread = new QThread(this);
    m_video_stream->moveToThread(VideoStreamThread);

    AudioStreamThread = new QThread(this);
    m_audio_stream->moveToThread(AudioStreamThread);

    connect(painterThread, &QThread::started, m_painter, &Painter::getFrames);
    connect(m_painter, &Painter::readyFrame, this, &CameraWidget::setImage /*, Qt::QueuedConnection*/);
    connect(VideoStreamThread, &QThread::started, m_video_stream, &VideoStream::start);
    connect(AudioStreamThread, &QThread::started, m_audio_stream, &AudioStream::start);
    connect(m_btn, &QPushButton::clicked, this, &CameraWidget::onClicked);
    connect(this, &CameraWidget::stop, m_video_stream, &VideoStream::stopped, Qt::QueuedConnection); //!
    connect(this, &CameraWidget::stop, m_audio_stream, &AudioStream::stopped, Qt::QueuedConnection);

    AudioStreamThread->start();
    VideoStreamThread->start();
    painterThread->start();
    LOG_DEBUG("CameraWidget ctor\n");
}

CameraWidget::~CameraWidget()
{
    painterThread->quit();
    painterThread->wait();
    VideoStreamThread->quit();
    VideoStreamThread->wait();
    AudioStreamThread->quit();
    AudioStreamThread->wait();
    LOG_DEBUG("CameraWidget dctor\n");
}

void CameraWidget::setImage(const QImage &newImage)
{
    m_image = newImage;
    LOG_DEV("repaint request\n");
    update(); // repaint request
}

void CameraWidget::onClicked(bool)
{
    m_video_stream->stopped();
    m_audio_stream->stopped();
    painterThread->quit();
    painterThread->wait();
    VideoStreamThread->quit();
    VideoStreamThread->wait();
    AudioStreamThread->quit();
    AudioStreamThread->wait();
    LOG_DEBUG("Click for stream stoping\n");
    // emit stop();
}

void CameraWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (!m_image.isNull())
    {
        LOG_DEV("gooood\n");
        painter.drawImage(rect(), m_image);
    }
    else
    {
        LOG_DEV("baaaad\n");
        painter.fillRect(rect(), Qt::gray); // Gray background if error
    }
}

void CameraWidget::closeEvent(QCloseEvent *event)
{
    LOG_WARNING("Close by user\n");
    event->accept();
    exit(EXIT_SUCCESS);
}
