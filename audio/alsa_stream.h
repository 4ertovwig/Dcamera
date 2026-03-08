/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <alsa/asoundlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _AlsaInfo
{
    snd_pcm_t *capture_handle;
    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *hw_params_capture;
    snd_pcm_hw_params_t *hw_params_playback;
    char *capture_buffer;
    snd_pcm_format_t format;
    uint32_t rate;
    uint32_t channels;
    uint32_t buffer_frames;
    snd_pcm_info_t *info;
} AlsaInfo;

// callback for SIGINT processing
void stoppedCallback();

// base ALSA audio interface
int alsaConfigurationPCM();
int startAlsaPCMStream();
void stopAlsaPCMStream(bool vas_stopped);

#ifdef __cplusplus
}
#endif
