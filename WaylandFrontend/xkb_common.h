/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <xkbcommon/xkbcommon.h>

// Context for xkbcommon
typedef struct _KeyboardContext
{
    struct xkb_context *xkb_context;
    struct xkb_keymap *keymap;
    struct xkb_state *state;
} KeyboardContext;

// allocations, configuration xkbcommon data
int preparexkb(KeyboardContext **keyboardContext);

// deallocations xkbcommon data
void releasexkb(KeyboardContext **keyboardContext);

// Convert wayland key to symbol
char *keycode_to_text(KeyboardContext *ctx, uint32_t keycode);
