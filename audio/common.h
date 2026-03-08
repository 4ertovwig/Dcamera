/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stdbool.h>

// Audio interface for audio backends
typedef void (*sigintCallback)(int signal);
typedef int (*configureAudioStream)();
typedef int (*startAudioStream)();
typedef void (*stopAudioStream)(bool vas_stopped);

typedef struct _AudioStreamProvider
{
    // configuratuion streams
    configureAudioStream configure;
    // start stream
    startAudioStream start;
    // stop stream
    stopAudioStream stop;
    // stopped callback
    sigintCallback cbSigint;
} AudioStreamProvider;
