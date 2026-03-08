/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <string.h>
#include "xdg-shell-client-protocol.h"
#include "viewporter-client-protocol.h"
#include <wayland-client-protocol.h>

#include "wl_structs.h"
#include "wl_common.h"
#include "../sync.h"
#include "../common/logging.h"

extern const struct wl_keyboard_listener keyboard_listener;
extern const struct wl_pointer_listener pointer_listener;

void seat_handle_capabilities(void *data, struct wl_seat *seat,
                                     enum wl_seat_capability caps)
{
    Display *d = data;

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !d->pointer)
    {
        d->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(d->pointer, &pointer_listener, d);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && d->pointer)
    {
        wl_pointer_destroy(d->pointer);
        d->pointer = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !d->keyboard)
    {
        d->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(d->keyboard, &keyboard_listener, d);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && d->keyboard)
    {
        wl_keyboard_destroy(d->keyboard);
        d->keyboard = NULL;
    }
}

const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
};

void xdg_wm_base_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

const struct xdg_wm_base_listener wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t id, const char *interface, uint32_t version)
{
    Display *d = data;

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        LOG_DEV("wl_compositor_interface.name interface: %s\n", wl_compositor_interface.name);
        d->compositor =
            wl_registry_bind(registry,
                             id, &wl_compositor_interface, 1);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        LOG_DEV("wl_seat_interface.name interface: %s\n", wl_seat_interface.name);
        d->seat = wl_registry_bind(registry,
                                   id, &wl_seat_interface, 1);
        wl_seat_add_listener(d->seat, &seat_listener, d);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        LOG_DEV("wl_shm_interface.name interface: %s\n", wl_shm_interface.name);
        d->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        d->cursor_theme = wl_cursor_theme_load(NULL, 32, d->shm);
        if (!d->cursor_theme)
        {
            LOG_ERROR("unable to load default theme\n");
            return;
        }
        d->default_cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "left_ptr");
        if (!d->default_cursor)
        {
            LOG_ERROR("unable to load default left pointer\n");
        }
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        LOG_DEV("xdg_wm_base_interface.name interface: %s\n", xdg_wm_base_interface.name);
        d->wm_base = wl_registry_bind(registry,
                                      id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(d->wm_base, &wm_base_listener, d);
    }
    // TODO: may be later i can use dmabuf from v4l, but now v4l copy frame to userspace(sharedFrame)
    // else if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0)
    // {
    //     d->dmabuf = wl_registry_bind(registry,
    //                                  id, &zwp_linux_dmabuf_v1_interface, 3);
    //     zwp_linux_dmabuf_v1_add_listener(d->dmabuf, &dmabuf_listener,
    //                                      d);
    // }
    // else if (strcmp(interface, weston_direct_display_v1_interface.name) == 0)
    // {
    //     d->direct_display = wl_registry_bind(registry,
    //                                          id, &weston_direct_display_v1_interface, 1);
    // }
    else if (strcmp(interface, wp_viewporter_interface.name) == 0)
    {
        LOG_DEV("wp_viewporter_interface.name interface: %s\n", wp_viewporter_interface.name);
        d->viewporter = wl_registry_bind(registry, id,
                                         &wp_viewporter_interface,
                                         1);
    }
    else
    {
        LOG_DEBUG("registry_handle_global interface: %s\n", interface);
    }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
                                          uint32_t name)
{
}

// global registry
const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove
};
