/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <linux/input.h>
#include <stdlib.h>
#include <unistd.h>

#include "xdg-shell-client-protocol.h"
#include "viewporter-client-protocol.h"
#include <wayland-client-protocol.h>

#include "wl_structs.h"
#include "wl_common.h"
#include "../sync.h"
#include "../common/logging.h"
#include "xkb_common.h"

extern SynchronizeData syncData;
extern SharedVideoFrame *sharedFrame;
extern KeyboardContext *keyboardContext;

void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                                   uint32_t format, int fd, uint32_t size)
{
    // Just so we don’t leak the keymap fd
    close(fd);
}

void keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                                  uint32_t serial, struct wl_surface *surface,
                                  struct wl_array *keys)
{
}

void keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                                  uint32_t serial, struct wl_surface *surface)
{
}

void keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                                uint32_t serial, uint32_t time, uint32_t key,
                                uint32_t state)
{
    Display *d = data;

    if (!d->wm_base)
        return;

    // TODO: need to rescale sharedVideoFrame via libav in backend
    // if (key == KEY_F11 && state)
    // {
    //     if (d->window->fullscreen)
    //         xdg_toplevel_unset_fullscreen(d->window->xdg_toplevel);
    //     else
    //         xdg_toplevel_set_fullscreen(d->window->xdg_toplevel, NULL);
    // }
    if ((key == KEY_ESC || key == KEY_Q) && state)
    {
        char *key_str = keycode_to_text(keyboardContext, key);
        /// WARNING:
        // It's very unstable...
        // You need to move your camera frame to any place in the compositor
        LOG_INFO("~~~~~~~~~~~~~~~~~~~~%s pressed: stop stream and exit~~~~~~~~~~~~~~~~~~~\n", key_str);
        syncData.stopStream = true;
        free(key_str);
    }
}

void keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                                      uint32_t serial, uint32_t mods_depressed,
                                      uint32_t mods_latched, uint32_t mods_locked,
                                      uint32_t group)
{
}

const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_handle_keymap,
    .enter = keyboard_handle_enter,
    .leave = keyboard_handle_leave,
    .key = keyboard_handle_key,
    .modifiers = keyboard_handle_modifiers,
};