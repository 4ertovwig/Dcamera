/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stdbool.h>

// Start audio pulse/pipewire/alsa stream
void *X11AudioStreamStart(void*);

// Stop audio pulse/pipewire/alsa stream
void X11AudioStreamStop(bool);