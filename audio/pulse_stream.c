/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdio.h>
#include <limits.h>

#include <pulse/error.h>

#include "pulse_stream.h"
#include "../common/args.h"

#ifdef USE_LIBSOXR
#include <soxr.h>
#include "../media/soxr_resampler.h"
#endif

#if defined(USE_FFT_NOISE_SUPRESSION)
#include "../media/fft_noise_supression.h"
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
#include "../media/speex_noise_supression.h"
#endif

#include "common.h"
#include "../common/logging.h"

AudioStreamProvider ifaceAudio = {
    .configure = pulseaudioConfiguration,
    .start = pulseaudioStartStream,
    .stop = pulseaudioStopStream
};

// Common camera params from command args
extern CamParams camParams;
pa_mainloop *mainloop;
pa_context *context;
pa_stream *outStream;
pa_stream *inputStream;

// WARNING: In fact, it makes no difference what audio signal parameters you specify
// Because Pulseaudio do resampling automatically, here:
// file:///pulseaudio/src/pulsecore/resampler.c:
// static const char * const resample_methods[] = {
//     "src-sinc-best-quality",
//     "src-sinc-medium-quality",
//     "src-sinc-fastest",
//     "src-zero-order-hold",
//     "src-linear",
//     "trivial",
//     "speex-float-0",
//     "speex-float-1",
//     "speex-float-2",
//     "speex-float-3",
//     "speex-float-4",
//     "speex-float-5",
//     "speex-float-6",
//     "speex-float-7",
//     "speex-float-8",
//     "speex-float-9",
//     "speex-float-10",
//     "speex-fixed-0",
//     "speex-fixed-1",
//     "speex-fixed-2",
//     "speex-fixed-3",
//     "speex-fixed-4",
//     "speex-fixed-5",
//     "speex-fixed-6",
//     "speex-fixed-7",
//     "speex-fixed-8",
//     "speex-fixed-9",
//     "speex-fixed-10",
//     "ffmpeg",
//     "auto",
//     "copy",
//     "peaks",
//     "soxr-mq",
//     "soxr-hq",
//     "soxr-vhq"
// };

pa_sample_spec specOut = {
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 1};

pa_sample_spec specIn = {
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 1};

// pa_sample_spec specOut = {
//     .format = PA_SAMPLE_S32LE,
//     //.format = PA_SAMPLE_S16LE,
//     .rate = 48000,
//     //.rate = 44100,
//     .channels = 2};
//     //.channels = 1};

// pa_sample_spec specIn = {
//     .format = PA_SAMPLE_S16LE,
//     //.rate = 48000,
//     .rate = 44100,
//     .channels = 1};

#if defined(USE_FFT_NOISE_SUPRESSION)
extern fftnsFormat outputAudiofilterData;
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
extern speexNsFormat outputAudiofilterData;
#endif

#ifdef USE_LIBSOXR
static soxr_datatype_t pulseFormat2soxr(pa_sample_format_t format)
{
    // soxr and pulseaudio use interleaved samples always
    switch (format)
    {
    case PA_SAMPLE_U8:
        return -1;
    case PA_SAMPLE_ALAW:
        return -1;
    case PA_SAMPLE_ULAW:
        return -1;
    case PA_SAMPLE_S16LE:
        return SOXR_INT16_I;
    case PA_SAMPLE_S16BE:
        return SOXR_INT16_I;
    case PA_SAMPLE_FLOAT32LE:
        return SOXR_FLOAT32_I;
    case PA_SAMPLE_FLOAT32BE:
        return SOXR_FLOAT32_I;
    case PA_SAMPLE_S32LE:
        return SOXR_INT32_I;
    case PA_SAMPLE_S32BE:
        return SOXR_INT32_I;
    case PA_SAMPLE_S24LE:
        return SOXR_INT32_I; // NOTE: this format in soxr_io_spec
    case PA_SAMPLE_S24BE:
        return SOXR_INT32_I;
    case PA_SAMPLE_S24_32LE:
        return SOXR_INT32_I; // NOTE: in x86 really we must do it: sample &= 0x00FFFFFF
    case PA_SAMPLE_S24_32BE:
        return SOXR_INT32_I; // NOTE: in x86 really we must do it: sample &= 0xFFFFFF00
    case PA_SAMPLE_MAX:
        return -1;
    case PA_SAMPLE_INVALID:
        return -1;
    }
}
#endif

#ifdef USE_FFT_NOISE_SUPRESSION
static enum AVSampleFormat pulseFormat2libav(pa_sample_format_t format)
{
    // pulseaudio use interleaved samples, libav - planar or intreleaved
    switch (format)
    {
    case PA_SAMPLE_U8:
        return -1;
    case PA_SAMPLE_ALAW:
        return -1;
    case PA_SAMPLE_ULAW:
        return -1;
    case PA_SAMPLE_S16LE:
        return AV_SAMPLE_FMT_S16;
    case PA_SAMPLE_S16BE:
        return AV_SAMPLE_FMT_S16P;
    case PA_SAMPLE_FLOAT32LE:
        return AV_SAMPLE_FMT_FLTP;
    case PA_SAMPLE_FLOAT32BE:
        return AV_SAMPLE_FMT_FLTP;
    case PA_SAMPLE_S32LE:
        return AV_SAMPLE_FMT_S32P;
    case PA_SAMPLE_S24LE:
        return AV_SAMPLE_FMT_S32P;
    case PA_SAMPLE_S24BE:
        return -1;
    case PA_SAMPLE_S24_32LE:
        return -1;
    case PA_SAMPLE_S24_32BE:
        return -1;
    case PA_SAMPLE_MAX:
        return -1;
    case PA_SAMPLE_INVALID:
        return -1;
    }
}
#endif

// Functions for stream

void freeBuffer(void *buffer)
{
#ifdef USE_LIBSOXR
    free(buffer);
#else
#endif
}

void readCallback(pa_stream *s, size_t length, void *userdata)
{
    const void *data;
    int res = pa_stream_peek(s, &data, &length);
    if (res < 0)
    {
        LOG_ERROR("Error in pa_stream_peek: %s\n", pa_strerror(res));
        return;
    }

    if (!data)
    {
        LOG_ERROR("Data after pa_stream_peek is NULL...\n");
        return;
    }

#ifdef USE_LIBSOXR
    size_t clips = 0;
    soxr_error_t error;
    soxr_t soxr;
    double inputRate = specIn.rate;
    double outputRate = specOut.rate;

    soxr_datatype_t itype = pulseFormat2soxr(specIn.format);
    soxr_datatype_t otype = pulseFormat2soxr(specOut.format);
    if (otype == -1 || itype == -1)
    {
        LOG_ERROR("Unsupported sample format in libsoxr...\n");
        goto dont_work;
    }

    size_t outSize;
    if (configureResampler(data, length, inputRate, outputRate, specIn.channels, specOut.channels, itype, otype, &outSize))
    {
        LOG_ERROR("Error in soxr resampler configure...\n");
        goto dont_work;
    }

    void *outputBuffer = soxrConvert(outSize);
    if (!outputBuffer)
    {
        LOG_ERROR("Error in soxrConvert...\n");
        goto dont_work;
    }
#else
    // In this pulseaudio does everything for us...
    void *outputBuffer = data;
    size_t outSize = length;
#endif

    if (camParams.noise_supression)
    {
        // reduce noise from outputBuffer
#if defined(USE_FFT_NOISE_SUPRESSION)
        noiseSupressfft(outputBuffer, outSize, outputAudiofilterData);
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
        noiseProcessSpeex(outputAudiofilterData, outputBuffer);
#endif
    }
    // Write to output device
    if (outStream && (res = pa_stream_write(outStream, outputBuffer, outSize, NULL, 0, PA_SEEK_RELATIVE) < 0))
    {
        LOG_ERROR("Error in pa_stream_write %s\n", pa_strerror(res));
        freeBuffer(outputBuffer);
        goto dont_work;
    }

    freeBuffer(outputBuffer);
    pa_stream_drop(s);
dont_work:
}

void streamStateCallback(pa_stream *stream, void *userData)
{
    switch (pa_stream_get_state(stream))
    {
    case PA_STREAM_UNCONNECTED:
        LOG_INFO("Stream %s is unconnected\n", userData ? (char *)userData : "unknown");
        break;
    case PA_STREAM_CREATING:
        LOG_INFO("Stream %s is creating\n", userData ? (char *)userData : "unknown");
        break;
    case PA_STREAM_READY:
        LOG_INFO("Stream %s is ready\n", userData ? (char *)userData : "unknown");
        break;
    case PA_STREAM_FAILED:
        LOG_INFO("Stream %s is failed\n", userData ? (char *)userData : "unknown");
        break;
    case PA_STREAM_TERMINATED:
        LOG_INFO("Stream %s is terminated\n", userData ? (char *)userData : "unknown");
        break;
    }
}

void streamCallback(pa_context *context, void *userData)
{
    switch (pa_context_get_state(context))
    {
    case PA_CONTEXT_READY:
        outStream = pa_stream_new(context, "Output audio stream", &specOut, NULL);
        pa_stream_set_state_callback(outStream, streamStateCallback, "Output stream");
        pa_stream_connect_playback(outStream, PULSEAUDIO_DEVICE_DEFAULT_SINK, NULL,
                                   PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);

        inputStream = pa_stream_new(context, "Input audio stream", &specIn, NULL);
        pa_stream_set_read_callback(inputStream, readCallback, "Input audio stream");
        pa_stream_set_state_callback(inputStream, streamStateCallback, "Input audio stream");
        pa_stream_connect_record(inputStream, PULSEAUDIO_DEVICE_DEFAULT_SOURCE, NULL,
                                 PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE);
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        pa_mainloop_quit(mainloop, 1);
        break;
    case PA_CONTEXT_UNCONNECTED:
        LOG_INFO("Unconnected from Pulseaudio server\n");
        break;
    case PA_CONTEXT_CONNECTING:
        LOG_INFO("Connecting to Pulseaudio server\n");
        break;
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    default:
        break;
    }
}

int pulseaudioConfiguration()
{
    int res = 0;
    mainloop = pa_mainloop_new();
    if (mainloop == NULL)
    {
        LOG_ERROR("Error in pa_mainloop_new\n");
        return 1;
    }
    pa_mainloop_api *api = pa_mainloop_get_api(mainloop);
    context = pa_context_new(api, "Echo loop context");
    if (context == NULL)
    {
        LOG_ERROR("Error in pa_context_new\n");
        res = 1;
        goto _mainloop_fail;
    }

    pa_context_set_state_callback(context, streamCallback, NULL);
    res = pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
    if (res < 0)
    {
        LOG_ERROR("Error in pa_context_connect: %s\n", pa_strerror(res));
        res = 1;
        goto _ctx_fail;
    }

    return res;

_ctx_fail:
    pa_context_unref(context);
_mainloop_fail:
    pa_mainloop_free(mainloop);
    return res;
}

int pulseaudioStartStream()
{
    LOG_DEBUG("Start pulse audio streams\n");
    if (camParams.noise_supression)
    {
#if defined(USE_FFT_NOISE_SUPRESSION)
        if (noiseSupressionFilterPrepare() != 0)
            return 1;

        if (noiseSupressionFilterInit() != 0)
            return 1;

        outputAudiofilterData.rate = specOut.rate;
        outputAudiofilterData.channels = specOut.channels;
        outputAudiofilterData.fmt = pulseFormat2libav(specOut.format);

        if (noiseSupressionConfigureFilterGraph(outputAudiofilterData) != 0)
            return 1;
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
        outputAudiofilterData.rate = specOut.rate;
        outputAudiofilterData.channels = specOut.channels;
        outputAudiofilterData.state = noiseSupressionSpeexInit(outputAudiofilterData);
        if (outputAudiofilterData.state == NULL)
        {
            LOG_ERROR("Bad configuration speex dsp\n");
            return 1;
        }
#endif
    }
    return pa_mainloop_run(mainloop, NULL);
}

void pulseaudioStopStream(bool vas_stopped)
{
    if (vas_stopped)
        return;

    int res = pa_stream_disconnect(outStream);
    if (res != 0)
    {
        LOG_ERROR("Error in out stream disconnect: %s\n", pa_strerror(res));
    }
    res = pa_stream_disconnect(inputStream);
    if (res != 0)
    {
        LOG_ERROR("Error in input stream disconnect: %s\n", pa_strerror(res));
    }
    if (camParams.noise_supression)
    {
#if defined(USE_FFT_NOISE_SUPRESSION)
        noiseSupressionFilterDeinit();
#elif defined(USE_SPEEX_NOISE_SUPRESSION)
        noiseSupressionSpeexDestroy(outputAudiofilterData);
#endif
    }
    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_mainloop_free(mainloop);
    soxrRelease();
    LOG_INFO("Stop audio streams\n");
}
