/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

#include "../sync.h"
#include "../common/logging.h"
#include "renderer.h"

// Shared video frame from video backend
extern SharedVideoFrame *sharedFrame;
// Shared sync data
extern SynchronizeData syncData;

static void bgr2rgba(uint8_t *bgr, uint8_t *rgba, int width, int height)
{
    for (int y = 0; y < height; y++)
    {
        uint8_t *bgrLine = bgr + y * width * 3;
        uint8_t *rgbaLine = rgba + y * width * 4;
        for (int x = 0; x < width; x++)
        {
            int bgrIdx = x * 3;
            int rgbaIdx = x * 4;

            rgbaLine[rgbaIdx] = bgrLine[bgrIdx + 2];     // R
            rgbaLine[rgbaIdx + 1] = bgrLine[bgrIdx + 1]; // G
            rgbaLine[rgbaIdx + 2] = bgrLine[bgrIdx];     // B
            rgbaLine[rgbaIdx + 3] = 255;                 // A
        }
    }
}

void *XCBRender(void *arg)
{
    XCBRenderData *r = (XCBRenderData *)arg;
    while (!syncData.stopStream)
    {
        // process here
        pthread_mutex_lock(&syncData.mut);
        LOG_DEV("Before frameready\n");
        while (!syncData.frameReady && !syncData.stopStream)
        {
            LOG_DEV("Before condvar syncData.frameReady: %d syncData.stopStream: %d\n",
                    syncData.frameReady, syncData.stopStream);
            pthread_cond_wait(&syncData.cond, &syncData.mut);
        }

        if (!sharedFrame)
        {
            LOG_ERROR("Frame from camera is empty\n");
            break;
        }
        else
        {
            bgr2rgba(sharedFrame->data[sharedFrame->videobufSurface], r->data,
                r->width, r->height);
            xcb_shm_put_image(r->connection,
                r->window, r->gcontext,
                r->width, r->height,
                0, 0, r->width, r->height,
                0, 0, r->depth,
                XCB_IMAGE_FORMAT_Z_PIXMAP,
                0, r->shmemSegId, 0);
            xcb_flush(r->connection);
        }
        syncData.frameReady = false;
        pthread_mutex_unlock(&syncData.mut);
    }
    return NULL;
}
