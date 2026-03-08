/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <wayland-client.h>

#include "../sync.h"
#include "../common/logging.h"
#include "wl_structs.h"

    // Create wl_shm via memfd from shared video frame(from video stream libuvc/v4l2).
    // Convert to weston format shared video frame.
    struct wl_buffer *create_wl_buffer_from_pixels(struct wl_shm *shm,
                                                   SharedVideoFrame *sharedFrame);

    // In v4l/libuvc we used bgr24 frame format.
    // My compositor(weston) support only XRGB8888(not XBGR8888)
    uint32_t *bgr24ToXRGB8888(const uint8_t *videoFrame,
                              int width, int height);

    // Generate video frame if video frame from stream is undefined or NULL
    uint32_t* generateDefaultFrame(int width, int height);

#ifdef __cplusplus
}
#endif
