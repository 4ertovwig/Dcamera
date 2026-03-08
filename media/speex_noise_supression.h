/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <speex/speex_preprocess.h>

// Converted data from alsa/pulseaudio/pipewire
typedef struct _speexNsFormat {
    uint32_t rate;
    uint8_t channels;
    SpeexPreprocessState *state;
} speexNsFormat;

// Speex noise suppression filter base interface
void* noiseSupressionSpeexInit(speexNsFormat outputFilterData);
void noiseProcessSpeex(speexNsFormat outputFilterData, void *sampleBuffer);
void noiseSupressionSpeexDestroy(speexNsFormat outputFilterData);

#ifdef __cplusplus
}
#endif
