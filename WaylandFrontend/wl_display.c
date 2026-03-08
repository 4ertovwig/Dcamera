/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdlib.h>
#include <string.h>

#include "wl_display.h"

//From wl_handle.h
extern const struct wl_registry_listener registry_listener;

Display *createDisplay()
{
    Display *display;

    display = malloc(sizeof *display);
    memset(display, 0, sizeof *display);
    if (display == NULL)
    {
        LOG_ERROR("out of memory for display\n");
        exit(EXIT_FAILURE);
    }
    display->display = wl_display_connect(NULL);
    if (!display->display)
    {
        LOG_ERROR("Error int wl_display_connect. Try WAYLAND_DISPLAY=wayland-1 or WAYLAND_DISPLAY=wayland-0\n");
        exit(EXIT_FAILURE);
    }

    display->registry = wl_display_get_registry(display->display);
    wl_registry_add_listener(display->registry,
                             &registry_listener, display);
 
    wl_display_roundtrip(display->display);

    display->cursor_surface =
        wl_compositor_create_surface(display->compositor);

    LOG_INFO("Create wayland display\n");
    return display;
}

void destroyDisplay(Display *display)
{
    wl_surface_destroy(display->cursor_surface);

    if (display->viewporter)
        wp_viewporter_destroy(display->viewporter);

    if (display->wm_base)
        xdg_wm_base_destroy(display->wm_base);

    if (display->compositor)
        wl_compositor_destroy(display->compositor);

    wl_registry_destroy(display->registry);
    wl_display_flush(display->display);
    wl_display_disconnect(display->display);
    free(display);
    LOG_INFO("Destroy display\n");
}
