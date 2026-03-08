/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdio.h>

#include <pulse/error.h>

#include "pulseaudio_devices.h"

pa_context *dcontext;

//// TODO may be need to use it:
// pa_context_exit_daemon(c, simple_callback, NULL);
// But i don't know a moment

static void exit_signal_callback(pa_mainloop_api *m, pa_signal_event *e, int sig, void *userdata)
{
    LOG_WARNING("Got SIGINT, exiting.\n");
    // TODO: fixed it
    exit(EXIT_SUCCESS);
}

static char *pulseFormat2str(pa_sample_format_t format)
{
    switch (format)
    {
    case PA_SAMPLE_U8:
        return "PCM8bit";
    case PA_SAMPLE_ALAW:
        return "ALaw8bit";
    case PA_SAMPLE_ULAW:
        return "ULaw8bit";
    case PA_SAMPLE_S16LE:
        return "PCM16bitLE";
    case PA_SAMPLE_S16BE:
        return "PCM16bitBE";
    case PA_SAMPLE_S32BE:
        return "SIGNED32bitBE";
    case PA_SAMPLE_FLOAT32LE:
        return "FLOAT32bitLE";
    case PA_SAMPLE_FLOAT32BE:
        return "FLOAT32bitBE";
    case PA_SAMPLE_S32LE:
        return "SignedPCM32bitLE";
    case PA_SAMPLE_S24LE:
        return "SignedPCM24bitLE";
    case PA_SAMPLE_S24BE:
        return "SignedPCM24bitBE";
    case PA_SAMPLE_S24_32LE:
        return "Signed24LSB32bitLE";
    case PA_SAMPLE_S24_32BE:
        return "Signed24LSB32bitBE";
    case PA_SAMPLE_MAX:
        return "Upper limit of sample size";
    case PA_SAMPLE_INVALID:
    default:
        return "Invalid sample format";
    }
}

// Struct just for passing in callbacks
typedef struct _PulseCbData
{
    pa_mainloop *mainloop;     // main loop
    pa_time_event *timeout_id; // event id
    int printInfo;             // default YES
    int inputNumber;           // number of input devices
    int outputNumber;          // number of output devices
} PulseCbData;

static void context_drain_complete(pa_context *c, void *userdata)
{
    pa_context_disconnect(c);
}

static void getInfoTimeoutCallback(pa_mainloop_api *api, pa_time_event *event, struct timeval *tv, void *userData)
{
    PulseCbData *data = (PulseCbData *)userData;
    LOG_DEBUG("Timeout for getAudioDevicesInfo was occured...\n");
    pa_operation *o;
    if (!(o = pa_context_drain(dcontext, context_drain_complete, NULL)))
        pa_context_disconnect(dcontext);
    else
        pa_operation_unref(o);
    if (data->mainloop)
    {
        pa_signal_done();
        // pa_mainloop_free(data->mainloop);
    }
}

static void infoOutputDevicesCallback(pa_context *c, const pa_sink_info *info, int eol, void *userData)
{
    PulseCbData *data = (PulseCbData *)userData;
    pa_mainloop *mainloop = data->mainloop;
    if (eol > 0)
    {
        // pa_mainloop_quit(mainloop, 0);
        return;
    }

    if (data->printInfo)
    {
        char channelMaps[PA_CHANNEL_MAP_SNPRINT_MAX] = {0};
        char *channelMap = (info != NULL) ? pa_channel_map_snprint(channelMaps, sizeof(channelMaps), &info->channel_map) : "";
        char outputData[8 * 1024] = {0};
        snprintf(outputData, sizeof(outputData),
                 "Sink Name:        %s\n"
                 "Description:      %s\n"
                 "Sample Rate:      %u Hz\n"
                 "Channels:         %u\n"
                 "Format:           %s\n"
                 "Channel Map:      %s\n"
                 "Muted:            %s\n\n",
                 info->name, info->description, info->sample_spec.rate, info->sample_spec.channels,
                 pulseFormat2str(info->sample_spec.format), channelMap, info->mute ? "Yes" : "No");
        LOG_INFO("==============================\nOUTPUT DEVICE: %d\n %s", data->outputNumber++, outputData);
    }
}

static void infoInputDevicesCallback(pa_context *context, const pa_source_info *info, int eol, void *userData)
{
    PulseCbData *data = (PulseCbData *)userData;
    pa_mainloop *mainloop = data->mainloop;
    if (eol > 0)
    {
        // pa_mainloop_quit(mainloop, 0);
        return;
    }

    if (data->printInfo)
    {
        char inputData[8 * 1024] = {0};
        snprintf(inputData, sizeof(inputData),
                 "Source Name:        %s\n"
                 "Description:        %s\n"
                 "Sample Rate:        %u Hz\n"
                 "Channels:           %u\n"
                 "Format:             %s\n"
                 "Card index:         %d\n\n",
                 info->name, info->description, info->sample_spec.rate, info->sample_spec.channels,
                 pulseFormat2str(info->sample_spec.format), info->card);
        LOG_INFO("==============================\nINPUT DEVICE: %d\n %s", data->inputNumber++, inputData);
    }
}

static void stateCallbackDeviceInfo(pa_context *context, void *userData)
{
    PulseCbData *data = (PulseCbData *)userData;
    pa_mainloop *mainloop = data->mainloop;
    switch (pa_context_get_state(context))
    {
    case PA_CONTEXT_READY:
        pa_context_get_sink_info_list(context, infoOutputDevicesCallback, data);
        pa_context_get_source_info_list(context, infoInputDevicesCallback, data);
        break;
    case PA_CONTEXT_FAILED:
        pa_context_unref(context);
        pa_context_disconnect(context);
        break;
    case PA_CONTEXT_TERMINATED:
        pa_mainloop_quit(mainloop, 1);
        break;
    default:
        break;
    }
}

int getAudioDevicesInfo()
{
    int res = 0;
    // Here we create pulse main loop, use info, and exit from main loop.
    // This Loop is not used by main audio stream
    pa_mainloop *mainloop = pa_mainloop_new();
    if (mainloop == NULL)
    {
        LOG_FATAL("Error in pa_mainloop_new\n");
        return 1;
    }

    PulseCbData userData = {
        .mainloop = mainloop,
        .printInfo = 1,
        .inputNumber = 0,
        .outputNumber = 0};
    pa_signal_new(SIGTERM, exit_signal_callback, NULL);
    pa_mainloop_api *api = pa_mainloop_get_api(mainloop);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    tv.tv_sec += 2; // 2 second from this time
    userData.timeout_id = api->time_new(api, &tv, (pa_time_event_cb_t)getInfoTimeoutCallback, &userData);

    /*pa_context **/ dcontext = pa_context_new(api, "Audio devicelist");
    if (dcontext == NULL)
    {
        LOG_FATAL("Error in pa_context_new\n");
        res = 1;
        goto mainloop_fail;
    }

    pa_context_set_state_callback(dcontext, stateCallbackDeviceInfo, &userData);
    res = pa_context_connect(dcontext, NULL, PA_CONTEXT_NOFLAGS, NULL);
    if (res < 0)
    {
        LOG_FATAL("Error in pa_context_connect: %s\n", pa_strerror(res));
        res = 1;
        goto ctx_fail;
    }

    // Run main loop
    res = pa_mainloop_run(mainloop, NULL);
    if (res < 0)
    {
        LOG_FATAL("Error in pa_mainloop_run: %s\n", pa_strerror(res));
        pa_mainloop_free(mainloop);
        return 1;
    }

    // pa_mainloop_run return 1 in good case
    if (res == 1)
        res = 0;
    return res;

mainloop_fail:
    if (mainloop)
    {
        pa_signal_done();
        pa_mainloop_free(mainloop);
    }
ctx_fail:
    pa_context_unref(dcontext);
    pa_context_disconnect(dcontext);

    return res;
}
