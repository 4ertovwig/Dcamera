/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../sync.h"
#include "../common/logging.h"
#include "renderer.h"

// TODO: fix crash with libav convertor
#define USE_SIMPLE_CONVERT

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

void *CacaRender(void *arg)
{
    CacaRenderData *r = (CacaRenderData *)arg;
#ifdef USE_SIMPLE_CONVERT
    // libcaca used just BGRA frame format
    uint8_t *bgraBuf = malloc(sharedFrame->width * sharedFrame->height * 4);
    if (!bgraBuf)
    {
        LOG_ERROR("Error in allocation BGRA buffer for libcaca...\n");
        exit(EXIT_FAILURE);
    }

    memset(bgraBuf, 0, sharedFrame->width * sharedFrame->height * 4);
#endif
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
#ifdef USE_SIMPLE_CONVERT
            bgr2rgba(sharedFrame->data[sharedFrame->videobufSurface], bgraBuf,
                     sharedFrame->width, sharedFrame->height);
            caca_dither_bitmap(*r->cv, 0, 0, r->canvasWidth, r->canvasHeight,
                               *r->dither, (uint8_t *)bgraBuf);
#else
            // // linear buffer for bgr
            int bgraSize;
            void *dataStride;
            dataStride = convert(r->swsContext, sharedFrame->data[sharedFrame->videobufSurface], r->converterParams,
                                 &bgraSize, &dataStride, &sharedFrame->strideSize);
            if (dataStride == NULL)
            {
                LOG_ERROR("Convert error in libcaca renderer\n");
                syncData.stopStream = true;
                return NULL;
            }

            memcpy(sharedFrame->data[sharedFrame->videobufSurface], dataStride, bgraSize);
            av_free(dataStride);

            caca_dither_bitmap(*r->cv, 0, 0, r->canvasWidth, r->canvasHeight,
                            *r->dither, sharedFrame->data[sharedFrame->videobufSurface]);
#endif
            caca_draw_line(*r->cv, 0, 2, 15, 2, ' ');
            caca_printf(*r->cv, 2, 2, "Frame number: %d", sharedFrame->frameNumber++);
            caca_refresh_display(*r->dp);
        }
        syncData.frameReady = false;
        pthread_mutex_unlock(&syncData.mut);
    }
#ifdef USE_SIMPLE_CONVERT
    free(bgraBuf);
#endif
    return NULL;
}
