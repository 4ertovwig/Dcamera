/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_atom.h>

#include "main.h"
#include "../sync.h"
#include "../common/logging.h"
#include "../common/args.h"

#include "videostream.h"
#include "audiostream.h"
#include "renderer.h"

////////////////////////////////////
// Global vars
SharedVideoFrame *sharedFrame;
extern CamParams camParams;
SynchronizeData syncData = {
    .stopStream = false,
    .frameReady = false
};

static xcb_format_t const* getFormat(xcb_connection_t *conn, unsigned depth)
{
    xcb_setup_t const *setup = xcb_get_setup(conn);
    xcb_format_iterator_t it = xcb_setup_pixmap_formats_iterator(setup);
    
    while (it.rem)
    {
        xcb_format_t *fmt = it.data;
        if (fmt->depth == depth)
            return fmt;
        xcb_format_next(&it);
    }
    return NULL;
}

int run(int argc, char *argv[])
{
    int ret = 0;
    xcb_generic_event_t *event;
    int depth = 24;

    sharedFrame = malloc(sizeof(SharedVideoFrame));
    if (!sharedFrame)
    {
        LOG_ERROR("Allocation error for shared buffer\n");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&syncData.mut, NULL);
    pthread_cond_init(&syncData.cond, NULL);
    pthread_mutex_init(&syncData.mutbuf, NULL);

    // try to connect to the X server and get screen
    xcb_connection_t *connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(connection))
    {
        LOG_ERROR("Can't connect to an X server\n");
        exit(EXIT_FAILURE);
    }
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    xcb_query_extension_reply_t *shmExtension = xcb_get_extension_data(connection, &xcb_shm_id);
    if (!shmExtension || !shmExtension->present)
    {
        LOG_ERROR("Error in xcb_get_extension_data\n");
        exit(EXIT_FAILURE);
    }

    sharedFrame->width = camParams.geometry.width;
    sharedFrame->height = camParams.geometry.height;
    // first surface in double video buffering will be 0
    sharedFrame->videobufSurface = 0;
    for (int videoSurface = 0; videoSurface < 2; videoSurface++)
        sharedFrame->data[videoSurface] = NULL;
    sharedFrame->frameNumber = 0;
    ConverterParams converterParams = createConverterParams(sharedFrame->width, sharedFrame->height,
                                                            sharedFrame->width, sharedFrame->height, AV_PIX_FMT_BGR24, AV_PIX_FMT_RGBA);
    struct SwsContext *xcbSwsContext;
    if (createContext(&xcbSwsContext, converterParams))
    {
        LOG_ERROR("Error in createContext\n");
        exit(EXIT_FAILURE);
    }

    uint32_t value_mask;
    uint32_t value_list[2];

    // create a window
    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_EVENT_MASK_KEY_PRESS;
    value_list[0] = screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_EXPOSURE;
    xcb_window_t window = xcb_generate_id(connection);
    xcb_create_window(
        connection,
        screen->root_depth,
        window,
        screen->root,
        0, 0,
        sharedFrame->width, sharedFrame->height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        value_mask, value_list);

    value_mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    value_list[0] = screen->black_pixel;
    value_list[1] = 0;

    xcb_format_t const *fmt = getFormat(connection, depth);
    xcb_image_t *image = xcb_image_create(sharedFrame->width, sharedFrame->height,
        XCB_IMAGE_FORMAT_Z_PIXMAP,
        fmt->scanline_pad,
        fmt->depth, fmt->bits_per_pixel, 0,
        XCB_IMAGE_ORDER_MSB_FIRST,
        XCB_IMAGE_ORDER_MSB_FIRST,
        NULL, ~0, 0);
    if (!image)
    {
        LOG_ERROR("Error in xcb_image_create\n  ");
        exit(EXIT_FAILURE);
    }

    size_t shm_size = image->stride * image->height;
    int shmemId = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | 0600); // just owner with rw-
    if (shmemId < 0)
    {
        LOG_ERROR("Error in shmget\n");
        exit(EXIT_FAILURE);
    }

    image->data = shmat(shmemId, NULL, 0);
    if (image->data == (void *)-1)
    {
        LOG_ERROR("Error in shmat. x-shm is not supported...\n");
        exit(EXIT_FAILURE);
    }

    // Register segment into X-server
    int shmemSegmentId = xcb_generate_id(connection);
    xcb_shm_attach(connection, shmemSegmentId, shmemId, 0);

    xcb_gcontext_t gcontext = xcb_generate_id(connection);
    xcb_create_gc(connection, gcontext, window, value_mask, value_list);

    xcb_map_window(connection, window);
    const char *winName = "XCamera";
    xcb_atom_t wmNameAtom = xcb_intern_atom_reply(connection,
            xcb_intern_atom(connection, 0, strlen("WM_NAME"), "WM_NAME"), NULL)->atom;
    xcb_atom_t wmIconNameAtom = xcb_intern_atom_reply(connection,
                xcb_intern_atom(connection, 0, strlen("WM_ICON_NAME"), "WM_ICON_NAME"), NULL)->atom;
    xcb_atom_t stringAtom = xcb_intern_atom_reply(connection,
            xcb_intern_atom(connection, 0, strlen("STRING"), "STRING"), NULL)->atom;

    xcb_change_property (connection, XCB_PROP_MODE_REPLACE, window,
        wmNameAtom, stringAtom, 8, strlen(winName), winName);
    xcb_change_property (connection, XCB_PROP_MODE_REPLACE, window,
        wmIconNameAtom, stringAtom, 8, strlen(winName), winName);

    // Run video stream
    pthread_t video_stream_id;
    if ((ret = pthread_create(&video_stream_id, NULL, &X11VideoStreamStart, NULL)) != 0)
    {
        LOG_ERROR("Error in creating thread for video strean\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(video_stream_id);
    // Run audio stream
    pthread_t audio_stream_id;
    if ((ret = pthread_create(&audio_stream_id, NULL, &X11AudioStreamStart, NULL)) != 0)
    {
        LOG_ERROR("Error in creating thread for audio strean\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(audio_stream_id);

    XCBRenderData renderData;
    renderData.data = image->data;
    renderData.width = sharedFrame->width;
    renderData.height = sharedFrame->height;
    renderData.converterParams = converterParams;
    renderData.swsContext = xcbSwsContext;
    renderData.depth = screen->root_depth;
    renderData.shmemSegId = shmemSegmentId;

    renderData.connection = connection;
    renderData.window = window;
    renderData.gcontext = gcontext;

    ///Run render
    pthread_t xcb_render_id;
    if ((ret = pthread_create(&xcb_render_id, NULL, &XCBRender, (void *)&renderData)) != 0)
    {
        LOG_ERROR("Error in creating thread for render\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(xcb_render_id);

    while (!syncData.stopStream && (event = xcb_wait_for_event(connection)))
    {
        switch (event->response_type)
        {
            case XCB_KEY_PRESS:
                LOG_WARNING("Keycode: %d\n", ((xcb_key_press_event_t *)event)->detail);
                syncData.stopStream = true;
                goto release;
        }
    }

release:
    LOG_INFO("Dcamera is exiting\n");

    xcb_destroy_window(connection, window);
    xcb_disconnect(connection);

    releaseContext(xcbSwsContext);
    pthread_mutex_destroy(&syncData.mutbuf);
    pthread_cond_destroy(&syncData.cond);
    pthread_mutex_destroy(&syncData.mut);

    free(sharedFrame);

    return 0;
}
