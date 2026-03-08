/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct _SharedVideoFrame
    {
        // Double buffering for shared video frame from backend and frontend
        void *data[2];
        // TODO: add double buffering support
        int videobufSurface;
        // Size of raw video frame
        size_t data_size;
        // Frame params
        int width;
        int height;
        int strideSize;

        // used to display frame number for libcaca frontend
        unsigned int frameNumber;
    } SharedVideoFrame;

    enum VIDEO_FRAME_TYPE
    {
        VIDEO_FRAME_TYPE_BGR888,
        VIDEO_FRAME_TYPE_MJPEG,
        VIDEO_FRAME_TYPE_H264,
        VIDEO_FRAME_TYPE_YUYV,
        VIDEO_FRAME_TYPE_YUV
    };

    typedef struct _SynchronizeData
    {
        enum VIDEO_FRAME_TYPE videoType;
        bool stopStream;
        bool frameReady;
        // synchronization for backend and fronnted
        pthread_mutex_t mut;
        pthread_cond_t cond;
        // synchronization mutex for double buffering
        pthread_mutex_t mutbuf;
    } SynchronizeData;

#ifdef __cplusplus
}
#endif
