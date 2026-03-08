/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include "fft_noise_supression.h"
#include "../common/logging.h"

// Contexts for sink/source for noise supression
AVFilterContext *srcBuffera = NULL;
AVFilterContext *sinkBuffera = NULL;
AVFilterContext *nsFiltter = NULL;
// Sink/source buffer filter for noise supression
AVFilter const *bufferFiltera = NULL;
AVFilter const *buffersinkFiltera = NULL;
// Main filter
AVFilter const *denoiseFilter = NULL;
// Main filter graph for noise supression
AVFilterGraph *filterGrapha = NULL;
// Noise supression only for output audio frame
fftnsFormat outputAudiofilterData;

// Convert just for mono and stereo
static const char *chan2str(uint8_t chan)
{
    if (chan == 0)
        return NULL;
    else if (chan == 1)
        return "mono";
    else if (chan == 2)
        return "stereo";
    else
        return NULL;
}

// Convert just for s16, float32
static const char *fmt2str(enum AVSampleFormat fmt)
{
    switch (fmt)
    {
    case AV_SAMPLE_FMT_S16:
        return "s16";
    case AV_SAMPLE_FMT_S32:
        return "s32";
    case AV_SAMPLE_FMT_FLT:
        return "flt";
    default:
        return NULL;
    };
}

void noiseSupressfft(void *sampleBuffer, size_t length, fftnsFormat outputFilterData)
{
    AVFrame *frame = av_frame_alloc();
    if (frame == NULL)
    {
        LOG_ERROR("Error in input frame allocation\n");
        goto exit;
    }
    frame->nb_samples = length / (av_get_bytes_per_sample(outputFilterData.fmt) * outputFilterData.channels);
    frame->format = outputFilterData.fmt;
    frame->ch_layout.nb_channels = outputFilterData.channels;
    frame->linesize[0] = length;
    frame->sample_rate = outputFilterData.rate;
    av_channel_layout_default(&frame->ch_layout, outputFilterData.channels);
    int err = av_frame_get_buffer(frame, 0);
    if (err != 0)
    {
        char buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(err, buf, AV_ERROR_MAX_STRING_SIZE);
        LOG_ERROR("Error in av_frame_get_buffer: %s\n", buf);
        goto exit;
    }

    memcpy(frame->data[0], sampleBuffer, length);
    // frame to filter
    err = av_buffersrc_add_frame_flags(srcBuffera, frame, AV_BUFFERSRC_FLAG_PUSH/*AV_BUFFERSRC_FLAG_KEEP_REF*/);
    if (err != 0)
    {
        char buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(err, buf, AV_ERROR_MAX_STRING_SIZE);
        LOG_ERROR("Error in av_buffersrc_add_frame_flags: %s\n", buf);
        goto release_frame;
    }

    // copy frame after afftdn filter
    AVFrame *out_frame = av_frame_alloc();
    if (out_frame == NULL)
    {
        LOG_ERROR("Error in out frame allocation\n");
        goto release_frame;
    }
    int ret = av_buffersink_get_frame(sinkBuffera, out_frame);
    memcpy(sampleBuffer, out_frame->data[0], length);

    av_frame_free(&out_frame);
release_frame:
    av_frame_free(&frame);
exit:
}

int noiseSupressionFilterInit()
{
    filterGrapha = avfilter_graph_alloc();
    if (filterGrapha == NULL)
    {
        LOG_ERROR("Failed to allocate filters graph...\n");
        return 1;
    }

    filterGrapha->thread_type = 0;
    return 0;
}

int noiseSupressionFilterPrepare()
{
#if LIBAVFILTER_VERSION_MAJOR < 7
    avfilter_register_all();
#endif

    bufferFiltera = avfilter_get_by_name("abuffer");
    if (bufferFiltera == NULL)
    {
        LOG_ERROR("Failed to find ffmpeg abuffer for afftdn filter...\n");
        return 1;
    }
    buffersinkFiltera = avfilter_get_by_name("abuffersink");
    if (buffersinkFiltera == NULL)
    {
        LOG_ERROR("Failed to find ffmpeg abuffersink for afftdn filter...\n");
        return 1;
    }
    denoiseFilter = avfilter_get_by_name("afftdn");
    if (denoiseFilter == NULL)
    {
        LOG_ERROR("Failed to find ffmpeg afftdn filter...\n");
        return 1;
    }
    return 0;
}

int noiseSupressionConfigureFilterGraph(fftnsFormat outputFilterData)
{
    char filterIn[256] = {0};
    const char *layout = chan2str(outputFilterData.channels);
    if (!layout)
        return 1;
    const char *fmt = fmt2str(outputFilterData.fmt);
    if (!fmt)
        return 1;
    snprintf(filterIn, sizeof(filterIn), "sample_rate=%u:sample_fmt=%s:channel_layout=%s",
                outputFilterData.rate, fmt, layout);

    if (avfilter_graph_create_filter(&srcBuffera, bufferFiltera, "src", filterIn, NULL, filterGrapha) < 0)
    {
        LOG_ERROR("avfilter_graph_create_filter for src failed\n");
        return 1;
    }
    if (avfilter_graph_create_filter(&sinkBuffera, buffersinkFiltera, "sink", NULL, NULL, filterGrapha) < 0)
    {
        LOG_ERROR("avfilter_graph_create_filter for sink failed\n");
        goto src_ctx_err;
    }

    // add noise supression
    if (avfilter_graph_create_filter(&nsFiltter, denoiseFilter, "afftdn", "noise_reduction=15", NULL, filterGrapha) < 0)
    {
        LOG_ERROR("avfilter_graph_create_filter for noise supression filter failed!\n");
        goto sink_ctx_err;
    }

    // composite filters
    if (avfilter_link(srcBuffera, 0, nsFiltter, 0) < 0 || avfilter_link(nsFiltter, 0, sinkBuffera, 0) < 0)
    {
        LOG_ERROR("Error in avfilter_link\n");
        goto ns_ctx_err;
    }

    return avfilter_graph_config(filterGrapha, NULL);
ns_ctx_err:
    avfilter_free(nsFiltter);
    nsFiltter = NULL;
sink_ctx_err:
    avfilter_free(sinkBuffera);
    sinkBuffera = NULL;
src_ctx_err:
    avfilter_free(srcBuffera);
    srcBuffera = NULL;
}

void noiseSupressionFilterDeinit()
{
    avfilter_graph_free(&filterGrapha);
}
