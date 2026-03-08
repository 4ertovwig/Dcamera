/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
// #include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "viewporter-client-protocol.h"
#include <wayland-client-protocol.h>

#include "wl_structs.h"
#include "wl_common.h"
#include "../sync.h"
#include "../common/logging.h"

    // Create window with current display
    Window *createWindow(Display *display);
    // redraw window - main callback
    void redrawWindow(void *data, struct wl_callback *callback, uint32_t time);
    // Release all resources for
    void destroyWindow(Window *window);

#ifdef __cplusplus
}
#endif
