/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>

#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>

#include "../common/logging.h"
#include "../common/args.h"
#include "pipewire_stream.h"
#include "common.h"

#if defined(USE_FFT_NOISE_SUPRESSION)
#include "../media/fft_noise_supression.h"
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
#include "../media/speex_noise_supression.h"
#endif

AudioStreamProvider ifaceAudio = {
    .configure = pipewireAudioDetailed,
    .start = pipewireStartAudioStream,
    .stop = pipewireStopAudioStream
};

#ifdef __cplusplus
extern "C"
{
#endif

#define CLEANUP_PIPEWIRE                                  \
    do                                                    \
    {                                                     \
        if (pipewireData.loop)                            \
            pw_main_loop_destroy(pipewireData.loop);      \
        if (source_props)                                 \
            pw_properties_free(source_props);             \
        if (output_props)                                 \
            pw_properties_free(output_props);             \
        if (pipewireData.sourceStream)                    \
            pw_stream_destroy(pipewireData.sourceStream); \
        if (pipewireData.sinkStream)                      \
            pw_stream_destroy(pipewireData.sinkStream);   \
        return 1;                                         \
    } while (0);

    // pipewire data for configuration
    PipewireData pipewireData;
    // Common camera params from command args
    extern CamParams camParams;

#if defined(USE_FFT_NOISE_SUPRESSION)
    extern fftnsFormat outputAudiofilterData;
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
    extern speexNsFormat outputAudiofilterData;
#endif

    static void echoLoopCallback(void *userdata)
    {
        PipewireData *data = userdata;
        struct pw_buffer *bSource, *bSink;

        if ((bSource = pw_stream_dequeue_buffer(data->sourceStream)) == NULL)
        {
            LOG_ERROR("No samples from source\n");
            return;
        }

        struct spa_buffer *source_buf = bSource->buffer;
        if (source_buf->datas[0].data)
        {
            // Pass data to sink stream
            if ((bSink = pw_stream_dequeue_buffer(data->sinkStream)) != NULL)
            {
                struct spa_data *sink_buf = &bSink->buffer->datas[0];
                struct spa_data *audio_data = &source_buf->datas[0];
                uint32_t size = source_buf->datas[0].chunk->size;
                // WARNING: BUG - noise supression with pipewire don't work
                camParams.noise_supression = false;
                atomic_thread_fence(memory_order_seq_cst);
                if (camParams.noise_supression)
                {
                    // reduce noise from outputBuffer
#if defined(USE_FFT_NOISE_SUPRESSION)
                    // WARNING: Just spikes... only errors
                    noiseSupressfft(source_buf->datas[0].data, SPA_MIN(size, sink_buf->maxsize), outputAudiofilterData);
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
                    // bad quality
                    noiseProcessSpeex(outputAudiofilterData, source_buf->datas[0].data);
#endif
                }
                if (sink_buf->data && audio_data->data && size > 0)
                {
                    // WARNING: I don't really understand why regular buffer relocation actually works.
                    // I think we should at least use a resampler.
                    memcpy(sink_buf->data, audio_data->data, SPA_MIN(size, sink_buf->maxsize));
                    sink_buf->chunk->offset = 0;
                    sink_buf->chunk->stride = audio_data->chunk->stride;
                    sink_buf->chunk->size = SPA_MIN(size, sink_buf->maxsize);
                }
                pw_stream_queue_buffer(data->sinkStream, bSink);
            }
            else
            {
                LOG_ERROR("No samples from sink...\n");
                return;
            }
        }

        pw_stream_queue_buffer(data->sourceStream, bSource);
    }

    static void stateChangedCallback(void *userdata, enum pw_stream_state old,
                                     enum pw_stream_state state, const char *error)
    {
        const char *stream_name = userdata;

        LOG_DEBUG("Stream '%s' state: %s -> %s\n", stream_name,
                  pw_stream_state_as_string(old),
                  pw_stream_state_as_string(state));

        if (error)
            LOG_ERROR(" (%s)\n", error);
    }

    static void streamParamChangedCallback(void *_data, uint32_t id, const struct spa_pod *param)
    {
        PipewireData *data = _data;

        // NULL means to clear the format
        if (param == NULL || id != SPA_PARAM_Format)
            return;

        if (spa_format_parse(param, &data->format.media_type, &data->format.media_subtype) < 0)
            return;

        // only accept raw audio
        if (data->format.media_type != SPA_MEDIA_TYPE_audio ||
            data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
            return;

        // call a helper function to parse the format for us.
        spa_format_audio_raw_parse(param, &data->format.info.raw);

        LOG_INFO("capturing rate:%d channels:%d\n",
                 data->format.info.raw.rate, data->format.info.raw.channels);
    }

    static const struct pw_stream_events sourceEvents = {
        PW_VERSION_STREAM_EVENTS,
        .param_changed = streamParamChangedCallback,
        .state_changed = stateChangedCallback,
        .process = echoLoopCallback,
    };

    static void sourceStreamCallback(void *userdata)
    {
        PipewireData *data = userdata;
        struct pw_buffer *buf = pw_stream_dequeue_buffer(data->sourceStream);
        if (buf)
        {
            pw_stream_queue_buffer(data->sourceStream, buf);
        }
        else
        {
            LOG_WARNING("Empty data from sourceStream\n");
        }
    }

    static const struct pw_stream_events sinkEvents = {
        PW_VERSION_STREAM_EVENTS,
        .state_changed = stateChangedCallback,
        .process = sourceStreamCallback,
    };

    static void do_quit(void *userdata, int signal_number)
    {
        PipewireData *data = userdata;
        pw_main_loop_quit(data->loop);
        quick_exit(EXIT_FAILURE);
    }

    int pipewireAudioDetailed()
    {
        // TODO may be no?
        // setenv("PIPEWIRE_DEBUG", "debug", 1);
        // setenv("PIPEWIRE_CPU", ...);

        LOG_DEBUG("Start pipewire audio streams\n");
        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
        struct pw_properties *source_props, *output_props;

        pw_init(NULL, NULL);

        pipewireData.sourceStream = NULL;
        pipewireData.sinkStream = NULL;
        pipewireData.rate = PIPEWIRE_DEFAULT_RATE;
        pipewireData.channels = PIPEWIRE_DEFAULT_CHANNELS;

        // make a main loop. If you already have another main loop, you can add
        // the fd of this pipewire mainloop to it.
        pipewireData.loop = pw_main_loop_new(NULL);
        if (!pipewireData.loop)
            CLEANUP_PIPEWIRE

        // pw_loop_add_signal(pw_main_loop_get_loop(pipewireData.loop), SIGINT, do_quit, &pipewireData);
        // pw_loop_add_signal(pw_main_loop_get_loop(pipewireData.loop), SIGTERM, do_quit, &pipewireData);

        source_props = pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Capture",
            PW_KEY_MEDIA_ROLE, "Communication",
            "stream.name", "Microphone Capture",
            NULL);
        if (!source_props)
            CLEANUP_PIPEWIRE

        output_props = pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Communication",
            "stream.name", "Headphones Output",
            NULL);
        if (!output_props)
            CLEANUP_PIPEWIRE

        // Set stream target if given on command line
        pipewireData.sourceStream = pw_stream_new_simple(
            pw_main_loop_get_loop(pipewireData.loop),
            "audio-source",
            source_props,
            &sourceEvents,
            &pipewireData);
        if (!pipewireData.sourceStream)
            CLEANUP_PIPEWIRE

        pipewireData.sinkStream = pw_stream_new_simple(
            pw_main_loop_get_loop(pipewireData.loop),
            "audio-output",
            output_props,
            &sinkEvents,
            &pipewireData);
        if (!pipewireData.sinkStream)
            CLEANUP_PIPEWIRE

        struct spa_audio_info_raw audio_info = {
            .format = SPA_AUDIO_FORMAT_S16, // 16-bit short
            .flags = SPA_AUDIO_FLAG_NONE,
            .rate = pipewireData.rate,
            .channels = pipewireData.channels,
            .position = {
                SPA_AUDIO_CHANNEL_FL,
                SPA_AUDIO_CHANNEL_FR}};
        if (camParams.noise_supression)
        {
#if defined(USE_FFT_NOISE_SUPRESSION) || defined(USE_SPEEX_NOISE_SUPRESSION)
            outputAudiofilterData.rate = PIPEWIRE_DEFAULT_RATE;
            outputAudiofilterData.channels = PIPEWIRE_DEFAULT_CHANNELS;
#endif
#if defined(USE_FFT_NOISE_SUPRESSION)
            outputAudiofilterData.fmt = AV_SAMPLE_FMT_S16;
#endif
        }
        params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audio_info);

        pipewireData.format.info.raw.channels = pipewireData.channels;
        pipewireData.buffer_size = 8192; // random value, not needed

        // Connect to source stream. We ask that our process function is
        // called in a realtime thread.
        if (pw_stream_connect(pipewireData.sourceStream,
                              PW_DIRECTION_INPUT,
                              PW_ID_ANY,
                              PW_STREAM_FLAG_AUTOCONNECT |
                                  PW_STREAM_FLAG_MAP_BUFFERS
                                  // WARNING: RT decrease quality
                                  /*|
                                  PW_STREAM_FLAG_RT_PROCESS*/
                              ,
                              params, 1) < 0)
        {
            LOG_ERROR("Error in pw_stream_connect sourceStream\n");
            CLEANUP_PIPEWIRE
            return 1;
        }

        // Connect to sink stream. We ask that our process function is
        // called in a realtime thread.
        if (pw_stream_connect(pipewireData.sinkStream,
                              PW_DIRECTION_OUTPUT,
                              PW_ID_ANY,
                              PW_STREAM_FLAG_AUTOCONNECT |
                                  PW_STREAM_FLAG_MAP_BUFFERS 
                                  // WARNING: RT decrease quality.
                                  /* |
                                  PW_STREAM_FLAG_RT_PROCESS */
                              ,
                              params, 1) < 0)
        {
            LOG_ERROR("Error in pw_stream_connect sinkStream\n");
            CLEANUP_PIPEWIRE
            return 1;
        }
        sleep(2);
        return 0;
    }

    int pipewireStartAudioStream()
    {
        if (camParams.noise_supression)
        {
#if defined(USE_FFT_NOISE_SUPRESSION)
            if (noiseSupressionFilterPrepare() != 0)
                return 1;

            if (noiseSupressionFilterInit() != 0)
                return 1;

            if (noiseSupressionConfigureFilterGraph(outputAudiofilterData) != 0)
                return 1;
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
            outputAudiofilterData.state = noiseSupressionSpeexInit(outputAudiofilterData);
            if (outputAudiofilterData.state == NULL)
            {
                LOG_ERROR("Bad configuration speex dsp\n");
                return 1;
            }
#endif
        }
        if (pipewireData.loop)
        {
            /* and wait while we let things run */
            pw_main_loop_run(pipewireData.loop);
        }
    }

    void pipewireStopAudioStream(bool vas_stopped)
    {
        // WARNING: BUG - noise supression with pipewire don't work
        camParams.noise_supression = false;
        atomic_thread_fence(memory_order_seq_cst);

        if (camParams.noise_supression)
        {
#if defined(USE_FFT_NOISE_SUPRESSION)
            noiseSupressionFilterDeinit();
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
            noiseSupressionSpeexDestroy(outputAudiofilterData);
#endif
        }
        if (pipewireData.sinkStream)
            pw_stream_destroy(pipewireData.sinkStream);
        if (pipewireData.sourceStream)
            pw_stream_destroy(pipewireData.sourceStream);
        if (pipewireData.loop)
            pw_main_loop_destroy(pipewireData.loop);
        pw_deinit();
    }

#ifdef __cplusplus
}
#endif
