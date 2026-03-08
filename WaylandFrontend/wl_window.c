/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wl_window.h"

extern SynchronizeData syncData;
extern SharedVideoFrame *sharedFrame;

static void xdg_surface_handle_configure(void *data, struct xdg_surface *surface,
                                         uint32_t serial)
{
    Window *window = data;

    xdg_surface_ack_configure(surface, serial);

    if (window->initialized && window->wait_for_configure)
        redrawWindow((void *)window, NULL, 0);
    window->wait_for_configure = false;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_handle_configure,
};

// fwd declaration
static const struct wl_callback_listener frame_listener;


static void xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *toplevel,
                                          int32_t width, int32_t height,
                                          struct wl_array *states)
{
    Window *window = data;
    uint32_t *p;

    window->fullscreen = 0;
    wl_array_for_each(p, states)
    {
        uint32_t state = *p;
        switch (state)
        {
        case XDG_TOPLEVEL_STATE_FULLSCREEN:
            window->fullscreen = 1;
            break;
        }
    }

    if (!window->viewport)
        return;

    if (window->fullscreen)
    {
        float ratio_w = (float)width / sharedFrame->width;
        float ratio_h = (float)height / sharedFrame->height;
        int32_t viewport_w;
        int32_t viewport_h;

        if (ratio_w > ratio_h)
        {
            viewport_w = width / ratio_w * ratio_h;
            viewport_h = height;
        }
        else
        {
            viewport_w = width;
            viewport_h = height / ratio_h * ratio_w;
        }

        wp_viewport_set_destination(window->viewport, viewport_w,
                                    viewport_h);
    }
    else
    {
        wp_viewport_set_destination(window->viewport, -1, -1);
    }
}

static void xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
    syncData.stopStream = true;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_handle_configure,
    .close = xdg_toplevel_handle_close,
};

////////////////////////////////////////////////////////////////////////
Window *createWindow(Display *display)
{
    Window *window = (Window *)malloc(sizeof *window);
    if (!window)
    {
        LOG_ERROR("Window allocation error\n");
        return NULL;
    }
    memset(window, 0, sizeof *window);

    window->callback = NULL;
    window->display = display;
    window->surface = wl_compositor_create_surface(display->compositor);

    if (display->wm_base)
    {
        // First buffer is 0
        window->display->videobufSurface = 0;

        if (display->viewporter)
        {
            window->viewport =
                wp_viewporter_get_viewport(display->viewporter,
                                           window->surface);
        }

        window->xdg_surface =
            xdg_wm_base_get_xdg_surface(display->wm_base,
                                        window->surface);

        assert(window->xdg_surface);

        xdg_surface_add_listener(window->xdg_surface,
                                 &xdg_surface_listener, window);

        window->xdg_toplevel =
            xdg_surface_get_toplevel(window->xdg_surface);

        assert(window->xdg_toplevel);

        xdg_toplevel_add_listener(window->xdg_toplevel,
                                  &xdg_toplevel_listener, window);

        xdg_toplevel_set_title(window->xdg_toplevel, "wcamera");
        xdg_toplevel_set_app_id(window->xdg_toplevel, "wcameraid");

        window->wait_for_configure = true;
        wl_surface_commit(window->surface);
    }
    else
    {
        assert(0);
    }

    LOG_INFO("Create window\n");
    return window;
}

////////////////////////////////////////////////////////////////////////
void destroyWindow(Window *window)
{
    if (window->callback)
        wl_callback_destroy(window->callback);

    if (window->viewport)
        wp_viewport_destroy(window->viewport);

    if (window->xdg_toplevel)
        xdg_toplevel_destroy(window->xdg_toplevel);
    if (window->xdg_surface)
        xdg_surface_destroy(window->xdg_surface);
    wl_surface_destroy(window->surface);

    free(window);

    LOG_INFO("Destroy window\n");
}

////////////////////////////////////////////////////////////////////////
void redrawWindow(void *data, struct wl_callback *callback, uint32_t time)
{
    LOG_DEV("redraWindow time: %u\n", time);
    Window *window = data;

    pthread_mutex_lock(&syncData.mut);
    LOG_DEV("Before frameready\n");
    while (!syncData.frameReady && !syncData.stopStream)
    {
        LOG_DEV("Before condvar syncData.frameReady: %d syncData.stopStream: %d\n", syncData.frameReady, syncData.stopStream);
        pthread_cond_wait(&syncData.cond, &syncData.mut);
    }
    
    pthread_mutex_lock(&syncData.mutbuf);
    if (!sharedFrame)
    {
        LOG_ERROR("CameraFrame is null\n");
        syncData.frameReady = false;
        pthread_mutex_unlock(&syncData.mut);
        pthread_mutex_unlock(&syncData.mutbuf);
        return;
    }

    window->display->videobuf[window->display->videobufSurface] = create_wl_buffer_from_pixels(window->display->shm, sharedFrame);
    if (!window->display->videobuf[window->display->videobufSurface])
    {
        LOG_WARNING("video wl_buffer is null\n");
        syncData.frameReady = false;
        pthread_mutex_unlock(&syncData.mut);
        pthread_mutex_unlock(&syncData.mutbuf);
        return;
    }

    syncData.frameReady = false;
    pthread_mutex_unlock(&syncData.mut);

    wl_surface_attach(window->surface, window->display->videobuf[window->display->videobufSurface], 0, 0);
    wl_surface_damage(window->surface, 0, 0, INT32_MAX, INT32_MAX);

    if (callback)
        wl_callback_destroy(callback);

    window->callback = wl_surface_frame(window->surface);
    wl_callback_add_listener(window->callback, &frame_listener, window);
    wl_surface_commit(window->surface);
    // change surface for double buffer
    pthread_mutex_unlock(&syncData.mutbuf);
    window->display->videobufSurface = window->display->videobufSurface == 0 ? 1 : 0;
    if (window->display->videobuf[window->display->videobufSurface])
        wl_buffer_destroy(window->display->videobuf[window->display->videobufSurface]);
}

static const struct wl_callback_listener frame_listener = {
    // Notify the client when the related request is done
    .done = redrawWindow
};
