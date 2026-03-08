/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include "painter.h"

#ifdef VIDEO_BACKEND_UVC
#include "../video/uvc_stream.h"
#else
#include "../video/v4l_stream.h"
#endif

#include "../sync.h"

extern "C"
{
    extern SharedVideoFrame *sharedFrame;
    extern SynchronizeData syncData;
}

Painter::Painter()
{
    connect(this, &Painter::run, this, &Painter::getFrames);
    pthread_mutex_init(&syncData.mut, NULL);
    pthread_cond_init(&syncData.cond, NULL);
}
Painter::~Painter()
{
    pthread_cond_destroy(&syncData.cond);
    pthread_mutex_destroy(&syncData.mut);
}

void Painter::getFrames()
{
    while (!syncData.stopStream)
    {
        // process here
        pthread_mutex_lock(&syncData.mut);
        LOG_DEV("Before frameready\n");
        while (!syncData.frameReady && !syncData.stopStream)
        {
            LOG_DEV("Before condvar syncData.frameReady: %d syncData.stopStream: %d\n", syncData.frameReady, syncData.stopStream);
            pthread_cond_wait(&syncData.cond, &syncData.mut);
        }

        if (!sharedFrame)
        {
            LOG_ERROR("CameraFrame is null\n");
            break;
        }
        else
        {
            LOG_DEV("Ready frame sharedFrame->strideSize: %d\n", sharedFrame->strideSize);
            QImage image{(const unsigned char *)sharedFrame->data[sharedFrame->videobufSurface],
                         sharedFrame->width, sharedFrame->height, sharedFrame->strideSize, QImage::Format_BGR888};
            emit readyFrame(image);
        }
        syncData.frameReady = false;
        pthread_mutex_unlock(&syncData.mut);
    }
}
