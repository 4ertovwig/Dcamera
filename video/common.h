/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stdbool.h>
#include "../common/args.h"

// Video interface for video backends
typedef int (*configureVideoStream)(CamParams m_camParams);
typedef int (*startVideoStream)();
typedef void (*stopVideoStream)(bool);

typedef struct _VideoStreamProvider
{
    // configuratuion streams
    configureVideoStream configure;
    // start stream
    startVideoStream start;
    // stop stream
    stopVideoStream stop;
} VideoStreamProvider;
