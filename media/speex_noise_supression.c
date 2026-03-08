/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include "speex_noise_supression.h"
#include "../common/logging.h"

#include <speex/speex.h>

// Noise supression only for output audio frame
speexNsFormat outputAudiofilterData;

void* noiseSupressionSpeexInit(speexNsFormat outputFilterData)
{
    SpeexPreprocessState *state = speex_preprocess_state_init(1024, outputFilterData.rate);

    int denoise = 1;
    int noiseSuppress = -25;
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress);

    int i;
    i = 0;
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_AGC, &i);
    i = 80000;
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &i);
    i = 0;
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_DEREVERB, &i);
    // These methods don't really do anything
    // float f = 0;
    // speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
    // f = 0;
    // speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);
    return state;
}

void noiseProcessSpeex(speexNsFormat outputFilterData, void *sampleBuffer)
{
    // vad activity will be ignored here
    speex_preprocess_run(outputFilterData.state, (spx_int16_t *)(sampleBuffer));
}

void noiseSupressionSpeexDestroy(speexNsFormat outputFilterData)
{
    speex_preprocess_state_destroy(outputFilterData.state);
}