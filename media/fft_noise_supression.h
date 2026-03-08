/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>

#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>

// Converted data from alsa/pulseaudio/pipewire
typedef struct _fftnsFormat {
    uint32_t rate;
    uint8_t channels;
    enum AVSampleFormat fmt;
} fftnsFormat;

// Base fft noise suppression via avfilter:
// Do suppression
void noiseSupressfft(void *sampleBuffer, size_t length, fftnsFormat outputFilterData);

// Init avfilter
int noiseSupressionFilterInit();

// extra settings for avfilter
int noiseSupressionFilterPrepare();

// Configure filter graph
int noiseSupressionConfigureFilterGraph(fftnsFormat outputFilterData);

// Deinitialization
void noiseSupressionFilterDeinit();

#ifdef __cplusplus
}
#endif
