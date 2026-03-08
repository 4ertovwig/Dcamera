/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stdbool.h>

// Start libuvc/v4l2 video stream
void *WaylandVideoStreamStart(void*);

// Stop libuvc/v4l2 video stream
void WaylandVideoStreamStop(bool);
