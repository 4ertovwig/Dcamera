/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "main.h"
#include "audiostream.h"
#include "videostream.h"

#include "../sync.h"
#include "../common/args.h"
#include "wl_structs.h"
#include "xkb_common.h"
#include "wl_window.h"
#include "wl_display.h"

///////////////////////////////
// Global vars
SharedVideoFrame *sharedFrame;
extern CamParams camParams;
SynchronizeData syncData = {
    .stopStream = false,
    .frameReady = false};
KeyboardContext *keyboardContext;

int run(int argc, char *argv[])
{
    // struct sigaction sigint;
    Display *display;
    Window *window;
    int ret = 0;

    if ((ret = preparexkb(&keyboardContext)) == 1)
        exit(EXIT_FAILURE);

    sharedFrame = malloc(sizeof(SharedVideoFrame));
    if (!sharedFrame)
    {
        LOG_ERROR("Allocation error for shared buffer\n");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&syncData.mut, NULL);
    pthread_cond_init(&syncData.cond, NULL);
    pthread_mutex_init(&syncData.mutbuf, NULL);

    sharedFrame->width = camParams.geometry.width;
    sharedFrame->height = camParams.geometry.height;
    // first surface in double video buffering will be 0
    sharedFrame->videobufSurface = 0;
    for (int videoSurface = 0; videoSurface < 2; videoSurface++)
        sharedFrame->data[videoSurface] = NULL;
    sharedFrame->frameNumber = 0;

    // Run video stream
    pthread_t video_stream_id;
    if ((ret = pthread_create(&video_stream_id, NULL, &WaylandVideoStreamStart, NULL)) != 0)
    {
        LOG_ERROR("Error in creating thread for video strean\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(video_stream_id);
    // Run audio stream
    pthread_t audio_stream_id;
    if ((ret = pthread_create(&audio_stream_id, NULL, &WaylandAudioStreamStart, NULL)) != 0)
    {
        LOG_ERROR("Error in creating thread for audio strean\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(audio_stream_id);
    // Raw format of video frames from libuvc/v4l2 here
    display = createDisplay();

    display->window = window = createWindow(display);
    if (!window)
    {
        LOG_ERROR("Error in wl_display_roundtrip\n");
        exit(EXIT_FAILURE);
    }

    // Here we retrieve the linux-dmabuf objects, or error
    if ((ret = wl_display_roundtrip(display->display)) == -1)
    {
        LOG_ERROR("Error in wl_display_roundtrip\n");
        exit(EXIT_FAILURE);
    }

    window->initialized = true;

    if (!window->wait_for_configure)
        redrawWindow((void *)window, NULL, 0);

    while (!syncData.stopStream /*&& ret != -1*/)
    {
        LOG_DEBUG("wl_display_dispatch\n");
        ret = wl_display_dispatch(display->display);
    }
    LOG_INFO("Dcamera is exiting\n");
    releasexkb(&keyboardContext);
    destroyWindow(window);
    destroyDisplay(display);

    WaylandVideoStreamStop(true);
    WaylandAudioStreamStop(true);

    pthread_mutex_destroy(&syncData.mutbuf);
    pthread_cond_destroy(&syncData.cond);
    pthread_mutex_destroy(&syncData.mut);

    free(sharedFrame);
    return 0;
}
