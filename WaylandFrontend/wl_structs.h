/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stdbool.h>
#include <wayland-client.h>
#include <wayland-cursor.h>

struct _Window;

typedef struct _Display
{
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_seat *seat;
    struct wl_pointer *pointer;
    struct wl_keyboard *keyboard;
    struct wl_shm *shm;
    struct wl_cursor_theme *cursor_theme;
    struct wl_cursor *default_cursor;
    struct wl_surface *cursor_surface;
    struct xdg_wm_base *wm_base;
    struct wp_viewporter *viewporter;
    bool requested_format_found;

    struct _Window *window;
    // Double buffering
    struct wl_buffer *videobuf[2];
    int videobufSurface;
} Display;

typedef struct _Window
{
    Display *display;
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    struct wl_callback *callback;
    struct wp_viewport *viewport;
    bool wait_for_configure;
    bool initialized;
    bool fullscreen;
    bool fullscreen_cursor;
} Window;
