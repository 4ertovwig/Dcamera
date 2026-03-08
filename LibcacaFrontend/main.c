/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <sys/ioctl.h>
#include <caca.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "main.h"
#include "audiostream.h"
#include "videostream.h"
#include "renderer.h"
#include "tools.h"

#include "../sync.h"
#include "../common/args.h"

///////////////////////////////
// Global vars
SharedVideoFrame *sharedFrame;
extern CamParams camParams;
SynchronizeData syncData = {
    .stopStream = false,
    .frameReady = false};

caca_display_t *dp = NULL;
caca_canvas_t *cv = NULL;

// Default size in character cells
const int defaultWidth = 200;
const int defaultHeight = 100;

void cleanCaca()
{
    // TODO: add mutex here
    if (cv)
    {
        caca_free_canvas(cv);
        cv = NULL;
    }
    if (dp)
        caca_free_display(dp);
}

int run(int argc, char *argv[])
{
    int ret = 0;
    sharedFrame = malloc(sizeof(SharedVideoFrame));
    if (!sharedFrame)
    {
        LOG_ERROR("Allocation error for shared buffer\n");
        exit(EXIT_FAILURE);
    }

    at_quick_exit(cleanCaca);
    // WARNING: Dcamera work just with x11 or slang backends.
    // Your libcaca library must supported x11 or slang backend.
    const char *cacaBackend = camParams.cacaBackend;

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
    ConverterParams converterParams = createConverterParams(sharedFrame->width, sharedFrame->height, sharedFrame->width,
                                            sharedFrame->height, AV_PIX_FMT_BGR24, AV_PIX_FMT_RGBA);
    struct SwsContext *cacaSwsContext;
    if (createContext(&cacaSwsContext, converterParams))
    {
        LOG_ERROR("Error in createContext\n");
        exit(EXIT_FAILURE);
    }
    // NOTE: context released in uvc/v4l streams

    // Run video stream
    pthread_t video_stream_id;
    if ((ret = pthread_create(&video_stream_id, NULL, &CacaVideoStreamStart, NULL)) != 0)
    {
        LOG_ERROR("Error in creating thread for video strean\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(video_stream_id);
    // Run audio stream
    pthread_t audio_stream_id;
    if ((ret = pthread_create(&audio_stream_id, NULL, &CacaAudioStreamStart, NULL)) != 0)
    {
        LOG_ERROR("Error in creating thread for audio strean\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(audio_stream_id);

    int cwidth = defaultWidth, cheight = defaultHeight;
    if (strcmp(cacaBackend, "x11") == 0)
    {
        if (setCharSize(&cwidth, &cheight, camParams.geometry.width, camParams.geometry.height) != 0)
            LOG_WARNING("Set default size for canvas: %dx%d in character cells\n", defaultWidth, defaultHeight);
        else
            LOG_WARNING("Set size for canvas: %dx%d in character cells\n", cwidth, cheight);
    }

    // create canvas in character cells
    cv = caca_create_canvas(cwidth, cheight);
    if (cv == NULL)
    {
        LOG_ERROR("Failed to create canvas\n");
        exit(EXIT_FAILURE);
    }

    dp = caca_create_display(cv);
    if (!dp)
    {
        LOG_ERROR("Cannot create caca display\n");
        exit(EXIT_FAILURE);
    }

    // caca_set_color_ansi(cv, CACA_WHITE, CACA_BLACK);
    char const *const *list;
    list = caca_get_display_driver_list();
    LOG_INFO("Available libcaca backends:\n");
    bool findedBackend = false;
    for (int i = 0; list[i] != NULL; i += 2)
    {
        if (!strcmp(list[i], cacaBackend))
            findedBackend = true;
        LOG_INFO("  driver: %s, full name: %s\n", list[i], list[i + 1]);
    }
    if (!findedBackend)
    {
        LOG_ERROR("Backend '%s' is not supported in your libcaca library...\n");
        quick_exit(EXIT_FAILURE);
    }

    if (caca_set_display_driver(dp, cacaBackend) != 0)
    {
        LOG_ERROR("Error in caca_set_display_driver with backend: %s...\n", cacaBackend);
        quick_exit(EXIT_FAILURE);
    }

    caca_set_display_time(dp, 40000);
    caca_set_cursor(dp, 0);
    caca_set_display_title(dp, "Dcamera");
    caca_refresh_display(dp);

    uint32_t rmask = 0x00FF0000, gmask = 0x0000FF00, bmask = 0x000000FF, amask = 0xFF000000;
    caca_dither_t *dither = caca_create_dither(32, sharedFrame->width, sharedFrame->height,
                                               4 * sharedFrame->width, rmask, gmask, bmask, amask);

    ditherSetAlgorithm(dither, camParams.dither_param.algo);
    ditherSetCharSet(dither, camParams.dither_param.charset);
    ditherSetColor(dither, camParams.dither_param.color);

    CacaRenderData renderData;
    renderData.canvasWidth = caca_get_canvas_width(cv);
    renderData.canvasHeight = caca_get_canvas_height(cv);
    renderData.dp = &dp;
    renderData.cv = &cv;
    renderData.dither = &dither;
    renderData.converterParams = converterParams;
    renderData.swsContext = cacaSwsContext;

    // Run render
    pthread_t caca_render_id;
    if ((ret = pthread_create(&caca_render_id, NULL, &CacaRender, (void *)&renderData)) != 0)
    {
        LOG_ERROR("Error in creating thread for render\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(caca_render_id);
    while (!syncData.stopStream)
    {
        caca_event_t ev;
        while (caca_get_event(dp, CACA_EVENT_KEY_PRESS | CACA_EVENT_QUIT, &ev, 1))
        {
            if (caca_get_event_type(&ev))
                goto release;
        }
    }

    LOG_INFO("Dcamera is exiting\n");

release:
    caca_free_dither(dither);
    caca_free_display(dp);

    CacaVideoStreamStop(true);
    CacaAudioStreamStop(true);

    releaseContext(cacaSwsContext);

    pthread_mutex_destroy(&syncData.mutbuf);
    pthread_cond_destroy(&syncData.cond);
    pthread_mutex_destroy(&syncData.mut);

    free(sharedFrame);
    return 0;
}
