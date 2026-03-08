/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#ifdef __cplusplus
extern "C" 
{
#endif

#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavutil/opt.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

// Frame counter realization via avfilter
// Init avfilter
int frameCounterFilterInit();

// extra settings for avfilter
int frameCounterFilterPrepare();

// Configure filter graph
int frameCounterConfigureFilterGraph(int width, int height, int fps);

// Main method - add frame number to decoded frame
int addFrameCount(int width, int height, void **decodedFrame, AVFrame *tempOut);

// Deinitialization
void frameCounterFilterDeinit();

#ifdef __cplusplus
}
#endif
