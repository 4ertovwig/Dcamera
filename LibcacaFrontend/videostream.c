/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#ifdef VIDEO_BACKEND_UVC
#include "../video/uvc_stream.h"
#else
#include "../video/v4l_stream.h"
#endif

#include "../video/common.h"
#include "videostream.h"

// Video interface
extern VideoStreamProvider ifaceVideo;
// Parameters from camera
extern CamParams camParams;

void *CacaVideoStreamStart(void *arg)
{
    (void)arg;
    // For uvc need vid/pid
    if (ifaceVideo.configure(camParams))
    {
        LOG_ERROR("Error in libcaca video stream configure...\n");
        return NULL;
    }
    if (ifaceVideo.start())
    {
        LOG_ERROR("Error in libcaca start video stream...\n");
        return NULL;
    }
    return NULL;
}

void CacaVideoStreamStop(bool stop)
{
    LOG_DEBUG("|-----stop video-----|\n");
    if (stop)
        ifaceVideo.stop(!stop);
}
