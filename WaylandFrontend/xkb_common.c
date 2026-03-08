/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../common/logging.h"
#include "xkb_common.h"

int preparexkb(KeyboardContext **keyboardContext)
{
    *keyboardContext = malloc(sizeof(KeyboardContext));
    if (!keyboardContext)
    {
        LOG_ERROR("Allocation error for keyboardContext\n");
        return 1;
    }

    (*keyboardContext)->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!(*keyboardContext)->xkb_context)
    {
        LOG_ERROR("Allocation error for xkb_context_new\n");
        return 1;
    }

    struct xkb_rule_names rule_names = {
        .rules = NULL,
        .model = "pc105", 
        .layout = "us",
        .variant = "",
        .options = ""
    };

    (*keyboardContext)->keymap = xkb_keymap_new_from_names((*keyboardContext)->xkb_context, &rule_names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!(*keyboardContext)->keymap)
    {
        LOG_ERROR("Allocation error for xkb_keymap_new_from_names\n");
        return 1;
    }

    (*keyboardContext)->state = xkb_state_new((*keyboardContext)->keymap);
    if (!(*keyboardContext)->state)
    {
        LOG_ERROR("Allocation error for xkb_state_new\n");
        return 1;
    }

    LOG_INFO("Prepare xkbcommon\n");
    return 0;
}

void releasexkb(KeyboardContext **keyboardContext)
{
    xkb_state_unref((*keyboardContext)->state);
    xkb_keymap_unref((*keyboardContext)->keymap);
    xkb_context_unref((*keyboardContext)->xkb_context);
    free(*keyboardContext);
    LOG_INFO("Release xkbcommon\n");
}

// Convert wayland key to symbol
char *keycode_to_text(KeyboardContext *ctx, uint32_t keycode) 
{
    // keycode in Wayland = keycode + 8
    uint32_t xkb_keycode = keycode + 8;
    xkb_keysym_t keysym = xkb_state_key_get_one_sym(ctx->state, xkb_keycode);

    char buffer[64] = {0};
    int size = xkb_keysym_to_utf8(keysym, buffer, sizeof(buffer));

    if (size > 0) {
        return strdup(buffer);
    }

    char buf[64] = {0};
    // if symbol absent, return unknown buf
    int res = xkb_keysym_get_name(keysym, buf, 64);
    if (res != -1)
        return strdup(buf);
    else
        return strdup("Unknown");
}
