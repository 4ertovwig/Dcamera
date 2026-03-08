/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdbool.h>
#include <signal.h>

#include "alsa_stream.h"

#include "../common/logging.h"
#include "common.h"

AlsaInfo alsaInfo;

void sigintProcess(int signal)
{
    snd_pcm_drop(alsaInfo.capture_handle);
    LOG_DEV("stoppedCallback\n");
}

AudioStreamProvider ifaceAudio = {
    .configure = alsaConfigurationPCM,
    .start = startAlsaPCMStream,
    .stop = stopAlsaPCMStream,
    .cbSigint = sigintProcess
};


static int sndConfigure(snd_pcm_t **handle, snd_pcm_hw_params_t *hwParams, uint32_t rate,
                        uint32_t channels, snd_pcm_format_t format, snd_pcm_stream_t type)
{
    int err;
    if ((err = snd_pcm_open(handle, "default", type, 0)) < 0)
    {
        LOG_ERROR("cannot open audio device (%s)\n", snd_strerror(err));
        return 1;
    }
    LOG_INFO("Open audio interface\n");

    if ((err = snd_pcm_hw_params_malloc(&hwParams)) < 0)
    {
        LOG_ERROR("cannot allocate hardware parameter structure (%s)\n",
                  snd_strerror(err));
        return 1;
    }

    if ((err = snd_pcm_hw_params_any(*handle, hwParams)) < 0)
    {
        LOG_ERROR("cannot initialize hardware parameter structure (%s)\n",
                  snd_strerror(err));
        return 1;
    }

    if ((err = snd_pcm_hw_params_set_access(*handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        LOG_ERROR("cannot set access type (%s)\n",
                  snd_strerror(err));
        return 1;
    }

    if ((err = snd_pcm_hw_params_set_format(*handle, hwParams, format)) < 0)
    {
        LOG_ERROR("cannot set sample format (%s)\n",
                snd_strerror(err));
        return 1;
    }

    if ((err = snd_pcm_hw_params_set_rate_near(*handle, hwParams, &rate, 0)) < 0)
    {
        LOG_ERROR("cannot set sample rate (%s)\n",
                snd_strerror(err));
        return 1;
    }

    if ((err = snd_pcm_hw_params_set_channels(*handle, hwParams, channels)) < 0)
    {
        LOG_ERROR("cannot set channel count (%s)\n",
                snd_strerror(err));
        return 1;
    }

    if ((err = snd_pcm_hw_params(*handle, hwParams)) < 0)
    {
        LOG_ERROR("cannot set parameters (%s)\n",
                snd_strerror(err));
        return 1;
    }

    snd_pcm_hw_params_free(hwParams);

    if ((err = snd_pcm_prepare(*handle)) < 0)
    {
        LOG_ERROR("cannot prepare audio interface for use (%s)\n",
                  snd_strerror(err));
        return 1;
    }
    return 0;
}

int alsaConfigurationPCM()
{
    int err;
    alsaInfo.format = SND_PCM_FORMAT_S16_LE;
    alsaInfo.rate = 44100;
    alsaInfo.channels = 1;
    alsaInfo.buffer_frames = 1024 / 8;

    // configure capture stream
    if (sndConfigure(&alsaInfo.capture_handle, alsaInfo.hw_params_capture,
                       alsaInfo.rate, alsaInfo.channels, alsaInfo.format, SND_PCM_STREAM_CAPTURE) == 1)
    {
        LOG_ERROR("Error in configuratuion capture stream\n");
        return 1;
    }

    // configure playback stream
    if (sndConfigure(&alsaInfo.playback_handle, alsaInfo.hw_params_playback,
        alsaInfo.rate, alsaInfo.channels, alsaInfo.format, SND_PCM_STREAM_PLAYBACK) == 1)
    {
        LOG_ERROR("Error in configuratuion playback stream\n");
        return 1;
    }

    LOG_INFO("audio interface prepared\n");
    return 0;
}

int startAlsaPCMStream()
{
    alsaInfo.capture_buffer = malloc(128 * snd_pcm_format_width(alsaInfo.format) / 8 * 2);
    if (!alsaInfo.capture_buffer)
    {
        LOG_ERROR("Allocation problem for bytes: %d\n", 128 * snd_pcm_format_width(alsaInfo.format) / 8 * 2);
        return 1;
    }

    int audio_frame = 0;
    while (true)
    {
        int err;
        if ((err = snd_pcm_readi(alsaInfo.capture_handle, alsaInfo.capture_buffer, alsaInfo.buffer_frames)) != alsaInfo.buffer_frames)
        {
            LOG_ERROR("read from audio interface failed (%s)\n", snd_strerror(err));
            return 1;
        }
        //LOG_DEV("alsaInfo.buffer_frames: %d\n", alsaInfo.buffer_frames);
        if ((err = snd_pcm_writei(alsaInfo.playback_handle, alsaInfo.capture_buffer, alsaInfo.buffer_frames)) != alsaInfo.buffer_frames)
        {
            LOG_ERROR("write to audio interface failed (%s)\n", snd_strerror(err));
            return 1;
        }
        //LOG_DEV("Audio_frame: %d\n", audio_frame++);
    }
    return 0;
}

void stopAlsaPCMStream(bool vas_stopped)
{
    if (vas_stopped)
        return;

    snd_pcm_drop(alsaInfo.capture_handle);
    free(alsaInfo.capture_buffer);

    if (alsaInfo.capture_handle)
        snd_pcm_close(alsaInfo.capture_handle);
    if (alsaInfo.playback_handle)
        snd_pcm_close(alsaInfo.playback_handle);

    LOG_INFO("ALSA audio interfaces closed\n");
}
