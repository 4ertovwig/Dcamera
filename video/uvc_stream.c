/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include <poll.h>
#include <signal.h>
#include <fcntl.h>

#include "common.h"
#include "uvc_stream.h"
#include "../common/logging.h"
#include "../media/converter.h"
#ifndef USE_LIBJPEG_TURBO
#include "../media/decoders.h"
#else
#include "../media/jpeg_decoder.h"
#endif
#ifdef FRAME_COUNTER
#include "../media/frame_counter.h"
#endif

// interface
VideoStreamProvider ifaceVideo = {
    .configure = configureUVCVideoStream,
    .start = startUVCVideoStream,
    .stop = stopUVCVideoStream
};

////////////////////////////////////
// Global vars
extern SharedVideoFrame *sharedFrame;
extern SynchronizeData syncData;
#ifndef USE_LIBJPEG_TURBO
extern LibavVideoDecoder videoDecoder;
#endif
ConverterParams converterParams;
extern CamParams camParams;
static int stopLoop[2];
static bool oneHandle = true;
struct SwsContext *swsContext;

// Parameters
typedef void* (*decoder)(void *, unsigned int);
typedef struct _VideoParams {
    decoder decode;
    enum AVPixelFormat convertFormat;
} VideoParams;

static void changeStatusCallback(enum uvc_status_class status_class,
                                 int event,
                                 int selector,
                                 enum uvc_status_attribute status_attribute,
                                 void *data, size_t data_len,
                                 void *user_ptr)
{
    LOG_DEBUG("ChangeStatus status_class: %d, event: %d, selector %d, status_attribute: %d, data_len: %lu\n",
             status_class, event, selector, status_attribute, data_len);
}

// Callback for grubbing videoframe and processing all video pipeline with him
static void processFrameCallback(uvc_frame_t *frame, void *data)
{
    uvc_frame_t *rawFrame;
    uvc_error_t ret;
    enum uvc_frame_format *frame_format = (enum uvc_frame_format *)data;

    VideoParams videoParams = {
#ifndef USE_LIBJPEG_TURBO
        .decode = decode,
        .convertFormat = AV_PIX_FMT_YUV422P
#else
        .decode = jpeg_decode,
        .convertFormat = AV_PIX_FMT_BGR24
#endif
    };

    // Need convert to BGR
    rawFrame = uvc_allocate_frame(frame->width * frame->height * 3);
    // WARNING: some troubles with mjpeg...
    // uint8_t *bytes = (uint8_t *)frame->data;
    // if (bytes[0] != 0xFF || bytes[1] != 0xD8)
    // {
    //     LOG_WARNING("SOI is absent, corrupted MJPEG frame\n");
    //     goto callbackEnd;
    // }
    // if (bytes[frame->data_bytes - 2] != 0xFF || bytes[frame->data_bytes - 1] != 0xD9)
    // {
    //     LOG_WARNING("EOI is absent, corrupted MJPEG frame\n");
    //     goto callbackEnd;
    // }

    // memset(sharedFrame, 0, sizeof(SharedVideoFrame)); // That's was bad idea for 'nullabling' video data
    pthread_mutex_lock(&syncData.mut);
#ifdef FRONTEND_WAYLAND
    pthread_mutex_lock(&syncData.mutbuf);
#endif
    // WARNING: 
    // Need circular buffer for video frames coz backend and frontend processed frame with another velocity.
    // Exceptionally for wayland...
    // We need to free this shared frame
    free(sharedFrame->data[sharedFrame->videobufSurface]);
    sharedFrame->data[sharedFrame->videobufSurface] = malloc(frame->width * frame->height * 3);
    sharedFrame->width = frame->width;
    sharedFrame->height = frame->height;
    LOG_DEV("processFrameCallback videobufSurface: %d\n", sharedFrame->videobufSurface);
    if (!rawFrame || !sharedFrame /*|| !sharedFrame->data[sharedFrame->videobufSurface]*/)
    {
        LOG_WARNING("unable to allocate camera frame!\n");
        goto callbackEnd;
    }

    LOG_DEV("callback! frame_format = %d, width = %d, height = %d, length = %lu, data = %p\n",
            frame->frame_format, frame->width, frame->height, frame->data_bytes, data);

    // linear buffer for bgr
    int bgr_size;
    // stride size
    void *dataStride;
    // Decoder data if needed
    void *decodedFrame;
    switch (frame->frame_format)
    {
    case UVC_FRAME_FORMAT_H264:
        LOG_DEV("H264 frame\n");
        syncData.videoType = VIDEO_FRAME_TYPE_H264;
        // TODO: i haven't webcam with H264
        break;
    case UVC_COLOR_FORMAT_MJPEG:
        LOG_DEV("MJPEG frame\n");
        syncData.videoType = VIDEO_FRAME_TYPE_MJPEG;
        converterParams = createConverterParams(frame->width, frame->height, frame->width, frame->height, videoParams.convertFormat, AV_PIX_FMT_BGR24);
        if (createContext(&swsContext, converterParams))
        {
            LOG_INFO("Exit...\n");
            return;
        }

        decodedFrame = videoParams.decode(frame->data, frame->data_bytes);
        if (decodedFrame == NULL)
        {
            LOG_ERROR("Decoded error in decode...\n");
            goto callbackEnd;
        }

        // NOTE: extra convert from raw buffer to avframe
        AVFrame *tempOut = av_frame_alloc();
        if (tempOut == NULL)
        {
            LOG_ERROR("Error in tempOut allocation...\n");
            goto callbackEnd;
        }
        //////////////////////////////////////////////////////////////
#ifdef FRAME_COUNTER
        if (camParams.frame_counter)
        {
            if (addFrameCount(frame->width, frame->height, &decodedFrame, tempOut) != 0)
            {
                av_frame_free(&tempOut);
                goto callbackEnd;
            }
        }
#endif
        //////////////////////////////////////////////////////////////

        dataStride = convert(swsContext, decodedFrame, converterParams, &bgr_size, &dataStride, &sharedFrame->strideSize);
        if (dataStride == NULL)
        {
            LOG_ERROR("Convert error in uvc callback\n");
            av_frame_free(&tempOut);
            goto callbackEnd;
        }
        memcpy(sharedFrame->data[sharedFrame->videobufSurface], dataStride, bgr_size);
        av_free(dataStride);
        av_frame_free(&tempOut);
#ifndef USE_LIBJPEG_TURBO
        av_free(videoDecoder.packet->data);
        av_free(videoDecoder.packet);
#endif
        syncData.frameReady = true;
        pthread_cond_broadcast(&syncData.cond);
        pthread_mutex_unlock(&syncData.mut);
        releaseContext(swsContext);
        break;
    case UVC_COLOR_FORMAT_YUYV:
        LOG_DEV("YUYV frame\n");
        // Do the BGR conversion
        ret = uvc_any2bgr(frame, rawFrame);
        if (ret)
        {
            LOG_ERROR("In processFrameCallback...\n");
            uvc_perror(ret, "uvc_any2bgr");
            goto callbackEnd;
        }
        syncData.videoType = VIDEO_FRAME_TYPE_BGR888;
        pthread_mutex_lock(&syncData.mut);
        memcpy(sharedFrame->data[sharedFrame->videobufSurface], rawFrame->data, rawFrame->data_bytes);
        syncData.frameReady = true;
        pthread_cond_broadcast(&syncData.cond);
        uvc_free_frame(rawFrame);
        pthread_mutex_unlock(&syncData.mut);
        break;
    default:
        break;
    }
    if (frame->sequence % camParams.fps == 0)
    {
        LOG_INFO(" * got image %u\n", frame->sequence);
    }
#ifdef FRONTEND_WAYLAND
    pthread_mutex_unlock(&syncData.mutbuf);
#endif
    LOG_DEV("Change videobufSurface: %d\n", sharedFrame->videobufSurface);
    return; // End of logic

callbackEnd:
    pthread_mutex_lock(&syncData.mut);
#ifndef USE_LIBJPEG_TURBO
    av_free(decodedFrame);
#else
    free(decodedFrame);
#endif
    syncData.videoType = VIDEO_FRAME_TYPE_BGR888;
    syncData.frameReady = true;
    pthread_cond_broadcast(&syncData.cond);
    uvc_free_frame(rawFrame);
    pthread_mutex_unlock(&syncData.mut);
    releaseContext(swsContext);
#ifdef FRONTEND_WAYLAND
    pthread_mutex_unlock(&syncData.mutbuf);
#endif
    LOG_INFO("Change from error videobufSurface: %d\n", sharedFrame->videobufSurface);
}

////////////////////////////
// Global variables
static uvc_device_t *dev;
static uvc_device_handle_t *devh;
static uvc_context_t *ctx;
static struct pollfd pipeds;
static uvc_stream_ctrl_t ctrl;
static uvc_error_t res;

// fwd declaration
void stopUVCVideoStream(bool);
// after SIGTERM we must stopped stream and attach to uvcvideo usb driver
void attach_to_kernel_driver(int signal)
{
    if (signal != SIGINT)
    {
        LOG_WARNING("SIGNAL %d was occurred\n", signal);
        return;
    }
    LOG_INFO("attach_to_kernel_driver\n");
    if (!oneHandle)
    {
        LOG_WARNING("SIGINT was processing early...\n");
        pthread_mutex_lock(&syncData.mut);
        syncData.stopStream = true;
        syncData.frameReady = false;
        pthread_cond_broadcast(&syncData.cond);
        pthread_mutex_unlock(&syncData.mut);

        exit(EXIT_SUCCESS);
        return;
    }
    oneHandle = false;
    stopUVCVideoStream(false);
}

void stopUVCVideoStream(bool flag)
{
    if (flag != false && syncData.stopStream == true)
    {
        LOG_WARNING("Stream was stopped early...\n");
        return;
    }

    if (!devh)
    {
        if (ctx)
            goto exit;
        else
            goto stopLoop;
    }

    LOG_DEV("-----------------StopStream-----------------\n");
    uvc_stop_streaming(devh);
    LOG_INFO("UVC Stream stop.\n");
    uvc_close(devh);
    LOG_DEBUG("Device close\n");
    uvc_unref_device(dev);
    LOG_DEBUG("Release the device descriptor\n");
exit:
    uvc_exit(ctx);
stopLoop:
    write(stopLoop[1], "q", 1); // stop loop
    pthread_mutex_lock(&syncData.mut);

    syncData.stopStream = true;
    syncData.frameReady = false;
    pthread_cond_broadcast(&syncData.cond);
    pthread_mutex_unlock(&syncData.mut);
#ifdef FRAME_COUNTER
    if (camParams.frame_counter)
        frameCounterFilterDeinit();
#endif
    LOG_INFO("UVC exited\n");
}

int configureUVCVideoStream(CamParams camParams)
{
    struct sigaction action;
    action.sa_handler = attach_to_kernel_driver;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    if (pipe(stopLoop) == -1)
    {
        LOG_ERROR("pipe error\n");
        return 1;
    }

    // struct pollfd pipeds;
    pipeds.fd = stopLoop[0];
    pipeds.events = POLLIN;

    // Initialize a UVC service context. Libuvc will set up its own libusb
    // context. Replace NULL with a libusb_context pointer to run libuvc
    // from an existing libusb context.
    res = uvc_init(&ctx, NULL);

    if (res < 0)
    {
        uvc_perror(res, "uvc_init");
        return 1;
    }

    // Open device with fixed vid/pid
    res = uvc_find_device(
        ctx, &dev,
        camParams.vid, camParams.pid, NULL); // filter devices: vendor_id, product_id, "serial_num"

    if (res < 0)
    {
        uvc_perror(res, "uvc_find_device");
        // no devices found. I think need to exit application...
        // WARNING: here was infinitly loop:
        // https://elixir.bootlin.com/glibc/glibc-2.39/source/stdlib/exit.c#L53
        // In this place were 5 atexit functions and some dtors that blocked each other.
        // I decide not to call tls dtors here:
        // https://elixir.bootlin.com/glibc/glibc-2.39/source/stdlib/exit.c#L41
        // and use quick_exit instead of exit...
        quick_exit(EXIT_FAILURE);
    }

    // Try to open the device: requires exclusive access
    res = uvc_open(dev, &devh);
    if (dev == NULL || devh == NULL)
    {
        LOG_ERROR("uvc_open error\n");
        quick_exit(EXIT_FAILURE);
    }

    LOG_INFO("UVC initialized\n");
    LOG_INFO("Video device found\n");

    uvc_set_status_callback(devh,
                            changeStatusCallback,
                            NULL);

    if (res < 0)
    {
        // unable to open device
        uvc_perror(res, "uvc_open");
        return 1;
    }

    LOG_DEBUG("Device opened\n");

    // WARNING: Uncomment if u want to see:
    // VideoStreaming(1):
    // bEndpointAddress: 129
    // Formats:
    // UncompressedFormat(1)
    //           bits per pixel: 16
    //           GUID: 5955593200001000800000aa00389b71 (YUY2)
    //           default frame: 1
    //           aspect ratio: 0x0
    //           interlace flags: 00
    //           copy protect: 00
    //                 FrameDescriptor(1)
    //                   capabilities: 01
    //                   size: 640x480
    //                   bit rate: 24576000-147456000
    //                   max frame size: 614400
    //                   default interval: 1/30
    //                   interval[0]: 1/30
    //                   interval[1]: 1/24
    //                   interval[2]: 1/20
    //                   interval[3]: 1/15
    //                   interval[4]: 1/10
    //                   interval[5]: 1/7
    //                   interval[6]: 1/5

    if (false)
    {
        // Print out a message containing all the information that libuvc
        // knows about the device
        uvc_print_diag(devh, stderr);
    }
    const uvc_format_desc_t *format_desc = uvc_get_format_descs(devh);
    const uvc_frame_desc_t *frame_desc = format_desc->frame_descs;
    enum uvc_frame_format frame_format;
    // waited params. But really libuvc will determine it itself
    int width = camParams.geometry.width;
    int height = camParams.geometry.height;
    int fps = camParams.fps;
    LOG_INFO("Set %dx%d %d fps\n", width, height, fps);

    switch (format_desc->bDescriptorSubtype)
    {
    case UVC_VS_FORMAT_MJPEG:
        frame_format = UVC_COLOR_FORMAT_MJPEG;
        break;
    case UVC_VS_FORMAT_FRAME_BASED:
        frame_format = UVC_FRAME_FORMAT_H264;
        break;
    default:
        frame_format = UVC_FRAME_FORMAT_YUYV;
        break;
    }

    LOG_INFO("\nFirst format: (%4s) %dx%d %dfps\n", format_desc->fourccFormat, width, height, fps);

    // frame_format = UVC_COLOR_FORMAT_YUYV;  // don't work
    frame_format = UVC_COLOR_FORMAT_MJPEG;
    syncData.videoType = VIDEO_FRAME_TYPE_MJPEG;
    // Try to negotiate first stream profile
    res = uvc_get_stream_ctrl_format_size(
        devh, &ctrl, // result stored in ctrl
        frame_format, width, height, fps);

    // Print out the result
    uvc_print_stream_ctrl(&ctrl, stderr);

#ifdef FRAME_COUNTER
    if (camParams.frame_counter)
    {
        if (frameCounterFilterPrepare() != 0)
            return 1;

        if (frameCounterFilterInit() != 0)
            return 1;

        if (frameCounterConfigureFilterGraph(width, height, fps) != 0)
            return 1;
    }
#endif
    return 0;
}

int startUVCVideoStream()
{
    if (res < 0)
    {
        // device doesn't provide a matching stream
        uvc_perror(res, "Error in get_mode");
        return 1;
    }

    if (syncData.videoType == VIDEO_FRAME_TYPE_MJPEG)
    {
#ifndef USE_LIBJPEG_TURBO
        if (decoderInit(VIDEO_FRAME_TYPE_MJPEG))
            return 1;
#endif
    }

    // Start the video stream. The library will call user function callback with random last value
    res = uvc_start_streaming(devh, &ctrl, processFrameCallback, (void *)4, 0);

    if (res < 0)
    {
        // unable to start stream
        uvc_perror(res, "start_streaming");
        return 1;
    }

    LOG_INFO("Streaming...\n");

    // enable auto exposure - see uvc_set_ae_mode documentation
    LOG_INFO("Enabling auto exposure ...\n");
    const uint8_t UVC_AUTO_EXPOSURE_MODE_AUTO = 2; // 1280;
    {
        LOG_WARNING(" ... enabled auto exposure\n");
    }
    if (res == UVC_ERROR_PIPE)
    {
        // this error indicates that the camera does not support the full AE mode;
        // try again, using aperture priority mode (fixed aperture, variable exposure time)
        LOG_WARNING(" ... full AE not supported, trying aperture priority mode\n");
        const uint8_t UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY = 8;
        res = uvc_set_ae_mode(devh, UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY);
        if (res < 0)
        {
            uvc_perror(res, " ... uvc_set_ae_mode failed to enable aperture priority mode");
        }
        else
        {
            LOG_WARNING(" ... enabled aperture priority auto exposure mode\n");
        }
    }
    else
    {
        uvc_perror(res, " ... uvc_set_ae_mode failed to enable auto exposure mode");
    }

    while (1)
    {
        // infinitly wait
        int ret = poll(&pipeds, 1, -1);
        if (ret > 0)
        {
            LOG_DEV("------------------------------------------------------\n");
            if (pipeds.revents & POLLIN)
            {
                LOG_DEV("Stop stream...\n");
                break;
            }
        }
        else
        {
            LOG_ERROR("Bad poll...\n");
        }
    }

    return 0;
}
