/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <errno.h>

#include "common.h"
#include "v4l_stream.h"
#include "../media/converter.h"
#ifndef USE_LIBJPEG_TURBO
#include "../media/decoders.h"
#else
#include "../media/jpeg_decoder.h"
#endif
#ifdef FRAME_COUNTER
#include "../media/frame_counter.h"
#endif

// extra video device
const char *vid1 = "/dev/video0";
const char *vid2 = "/dev/video1";

// interface
VideoStreamProvider ifaceVideo = {
    .configure = configureV4lVideoStream,
    .start = startV4lVideoStream,
    .stop = stopV4lVideoStream
};

/////////////////////////////////////////////////
// Global vars
extern CamParams camParams;
// swscale context
struct SwsContext *swsContext;

// v4l2 data
static int fdvideo;
static const int buffersCount = 4; // recommended
static v4l_buffer *sharedBuffers;
static enum v4l2_buf_type typeOfCapture = V4L2_BUF_TYPE_VIDEO_CAPTURE;
struct v4l2_format fmt = {0};

ConverterParams converterParams;
extern SharedVideoFrame *sharedFrame;
extern SynchronizeData syncData;
#ifndef USE_LIBJPEG_TURBO
extern LibavVideoDecoder videoDecoder;
#endif


// Set maximum size of screen
static void setMaximumResolution(int fdvideo, int *width, int *height, unsigned int *pixelFormat)
{
    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    int max_width = 0, max_height = 0;
    LOG_INFO("Supported formats:\n");
    while (ioctl(fdvideo, VIDIOC_ENUM_FMT, &fmtdesc) == 0)
    {
        LOG_DEBUG("  [%d]: %s\n", fmtdesc.index, fmtdesc.description);

        // For all formats
        struct v4l2_frmsizeenum frmsize = {0};
        frmsize.pixel_format = fmtdesc.pixelformat;

        for (frmsize.index = 0; ioctl(fdvideo, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0; frmsize.index++)
        {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
            {
                if (strcmp(fmtdesc.description, "Motion-JPEG") == 0)
                {
                    LOG_DEV("YUYV description\n");
                    *pixelFormat = V4L2_PIX_FMT_MJPEG;
                }
                else if (strcmp(fmtdesc.description, "YUYV") == 0)
                {
                    LOG_DEV("motion jpeg description\n");
                    *pixelFormat = V4L2_PIX_FMT_YUYV;
                }
                LOG_DEV("    Size: %dx%d\n", frmsize.discrete.width, frmsize.discrete.height);
                if (frmsize.discrete.width > max_width)
                    max_width = frmsize.discrete.width;
                if (frmsize.discrete.height > max_height)
                    max_height = frmsize.discrete.height;
            }
        }
        fmtdesc.index++;
    }
    *width = max_width;
    *height = max_height;
    LOG_DEBUG("Maximum width: %d height %d", *width, *height);
}

int configureV4lVideoStream(CamParams camParams)
{
    fdvideo = open(camParams.dev, O_RDWR);
    if (fdvideo < 0)
    {
        LOG_ERROR("Cannot open %s device. Try another\n", camParams.dev);
        fdvideo = open(vid1, O_RDWR);
        if (fdvideo < 0)
        {
            fdvideo = open(vid2, O_RDWR);
            if (fdvideo < 0)
            {
                LOG_FATAL("Cannot open /dev/video0 and /dev/video1 devices. Exit...\n");
                return 1;
            }
        }
    }
    LOG_INFO("Open device: %s\n", camParams.dev);

    sharedBuffers = malloc(sizeof(v4l_buffer) * buffersCount);
    if (!sharedBuffers)
    {
        LOG_ERROR("Error in malloc for video sharedBuffers. Exit...\n");
        return 1;
    }

    // waited params, but getSize change to maximal
    int width = camParams.geometry.width, height = camParams.geometry.height;
    unsigned int pixelFormat;

    if (camParams.use_best_quality)
        setMaximumResolution(fdvideo, &width, &height, &pixelFormat);

    // Set format
    // struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    sharedFrame->width = width;
    sharedFrame->height = height;

    // here use just raw or mjpeg formats
    // if (strstr(pixelFormat, "JPEG") != NULL)
    //     fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    // else
    //     fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    LOG_INFO("Width: %d Height: %d, pixelFormat: %u\n", width, height, pixelFormat);

    if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {
#ifndef USE_LIBJPEG_TURBO
        // TODO: really YUYV ???
        if (decoderInit(VIDEO_FRAME_TYPE_YUYV))
            return 1;
        converterParams = createConverterParams(width, height, width, height, AV_PIX_FMT_YUYV422, AV_PIX_FMT_BGR24);
#else
        // Use swscale converter
        converterParams = createConverterParams(width, height, width, height, AV_PIX_FMT_BGR24, AV_PIX_FMT_BGR24);
#endif
    }
    else
    {
#ifndef USE_LIBJPEG_TURBO
        if (decoderInit(VIDEO_FRAME_TYPE_MJPEG))
            return 1;
        // TODO: really AV_PIX_FMT_YUVJ422P ???
        converterParams = createConverterParams(width, height, width, height, AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_BGR24);
#else
        // Use swscale converter
        converterParams = createConverterParams(width, height, width, height, AV_PIX_FMT_BGR24, AV_PIX_FMT_BGR24);
#endif
    }
    if (createContext(&swsContext, converterParams))
    {
        LOG_ERROR("Exit...\n");
        return 1;
    }

    if (ioctl(fdvideo, VIDIOC_S_FMT, &fmt) < 0)
    {
        LOG_ERROR("VIDIOC_S_FMT error. Exit...\n");
        return 1;
    }

    struct v4l2_requestbuffers reqBuffers = {0};
    reqBuffers.count = buffersCount;
    reqBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqBuffers.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fdvideo, VIDIOC_REQBUFS, &reqBuffers) < 0)
    {
        LOG_ERROR("VIDIOC_REQBUFS error. Exit...\n");
        return 1;
    }

    for (int i = 0; i < buffersCount; ++i)
    {
        struct v4l2_buffer buffer = {0};
        buffer.index = i;
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fdvideo, VIDIOC_QUERYBUF, &buffer) < 0)
        {
            LOG_ERROR("VIDIOC_QUERYBUF error. Exit...\n");
            return 1;
        }

        sharedBuffers[i].data_bytes = buffer.length;
        sharedBuffers[i].data = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fdvideo, buffer.m.offset);
        if (sharedBuffers[i].data == MAP_FAILED)
        {
            LOG_ERROR("mmap error. Exit...");
            return 1;
        }

        // buffer to queue
        if (ioctl(fdvideo, VIDIOC_QBUF, &buffer) < 0)
        {
            LOG_ERROR("VIDIOC_QBUF error. Exit...");
            return 1;
        }
    }

#ifdef FRAME_COUNTER
    if (camParams.frame_counter)
    {
        if (frameCounterFilterPrepare() != 0)
            return 1;

        if (frameCounterFilterInit() != 0)
            return 1;

        if (frameCounterConfigureFilterGraph(width, height, 30) != 0)
        {
            return 1;
        }
    }
#endif

    return 0;
}

int startV4lVideoStream()
{
    // enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fdvideo, VIDIOC_STREAMON, &typeOfCapture) < 0)
    {
        LOG_ERROR("VIDIOC_STREAMON error. Exit...");
        return 1;
    }

    // YUYV = 2 bytes per pixel
    // need convert frame to bgr24
    sharedFrame->data[sharedFrame->videobufSurface] = malloc(sharedFrame->width * sharedFrame->height * 3);
    if (sharedFrame->data[sharedFrame->videobufSurface] == NULL)
    {
        LOG_ERROR("Error in allocation raw bgr24 data. Exit...");
        return 1;
    }
    while (!syncData.stopStream)
    {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fdvideo, VIDIOC_DQBUF, &buf) < 0)
        {
            LOG_ERROR("VIDIOC_DQBUF error. Exit...\n");
            return 1;
        }

        // size of linear buffer
        int bgr_size;
        void *dataStride;
        void *decodedFrame;
        if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
        {
#ifndef USE_LIBJPEG_TURBO
            decodedFrame = decode(sharedBuffers[buf.index].data, buf.bytesused);
#else
            decodedFrame = jpeg_decode(sharedBuffers[buf.index].data, buf.bytesused);
#endif
        }

        // NOTE: extra convert from raw buffer to avframe
        AVFrame *tempOut = av_frame_alloc();
        if (tempOut == NULL)
        {
            LOG_ERROR("Error in tempOut allocation...\n");
            goto filterError;
        }
        //////////////////////////////////////////////////////////////

#ifdef FRAME_COUNTER
        if (camParams.frame_counter)
        {
            if (addFrameCount(sharedFrame->width, sharedFrame->height, &decodedFrame, tempOut) != 0)
            {
                av_frame_free(&tempOut);
                goto filterError;
            }
        }
#endif
        //////////////////////////////////////////////////////////////

        dataStride = convert(swsContext, decodedFrame, converterParams, &bgr_size, &dataStride, &sharedFrame->strideSize);
        LOG_DEBUG("Start stream, buf index: %d bytesused: %d offset: %d output data size: %d\n", buf.index, buf.bytesused, buf.m.offset, bgr_size);
        pthread_mutex_lock(&syncData.mut);
        memcpy(sharedFrame->data[sharedFrame->videobufSurface], dataStride, bgr_size);
        //av_frame_free(&tempOut);
filterError:
#ifndef USE_LIBJPEG_TURBO
        av_free(decodedFrame);
        av_free(dataStride);
        av_free(videoDecoder.packet->data);
        av_free(videoDecoder.packet);
#else
        free(decodedFrame);
        free(dataStride);
#endif
        // TODO: crash
        //av_frame_free(&tempOut);
        syncData.frameReady = true;
        pthread_cond_broadcast(&syncData.cond);
        pthread_mutex_unlock(&syncData.mut);

        if (ioctl(fdvideo, VIDIOC_QBUF, &buf) < 0)
        {
            LOG_ERROR("VIDIOC_QBUF error. Exit...\n");
            return 1;
        }
    }
}

void stopV4lVideoStream(bool)
{
    pthread_mutex_lock(&syncData.mut);
    // Stopped
    if (ioctl(fdvideo, VIDIOC_STREAMOFF, &typeOfCapture) < 0)
        LOG_ERROR("Stop stream error... strange\n");

    free(sharedFrame->data[sharedFrame->videobufSurface]);
    sharedFrame->data[sharedFrame->videobufSurface] = NULL;
    syncData.stopStream = true;
    syncData.frameReady = false;
    pthread_cond_broadcast(&syncData.cond);
    // Release shared buffers
    if (sharedBuffers)
    {
        for (int i = 0; i < buffersCount; ++i)
            munmap(sharedBuffers[i].data, sharedBuffers[i].data_bytes);
        free(sharedBuffers);
    }
    pthread_mutex_unlock(&syncData.mut);
#ifdef FRAME_COUNTER
if (camParams.frame_counter)
        frameCounterFilterDeinit();
#endif
    releaseContext(swsContext);
    close(fdvideo);
}
