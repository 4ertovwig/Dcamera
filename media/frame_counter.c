/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include "frame_counter.h"
#include "../common/logging.h"

// Contexts for sink/source for frame counter
AVFilterContext *srcBufferv = NULL;
AVFilterContext *sinkBufferv = NULL;
// Sink/source buffer filter for frame counter
AVFilter const *bufferFilterv = NULL;
AVFilter const *buffersinkFilterv = NULL;
// Main filter graph for frame counter
AVFilterGraph *filterGraphv = NULL;
// Presentation time stamp
// need to increment manually because jpegturbo decoder produces raw buffer
int decodedPts = 0;

static enum AVPixelFormat getPixelFormat()
{
#ifndef USE_LIBJPEG_TURBO
    return AV_PIX_FMT_YUV422P;
#else
    return AV_PIX_FMT_BGR24;
#endif
}

// Register all ffmpeg builtin filters and prepares needed filter definitions
int frameCounterFilterPrepare()
{
#if LIBAVFILTER_VERSION_MAJOR < 7 
    avfilter_register_all();
#endif

    bufferFilterv = avfilter_get_by_name("buffer");
    if (bufferFilterv == NULL)
    {
        LOG_ERROR("Failed to find ffmpeg source buffer filter...\n");
        return 1;
    }
    buffersinkFilterv = avfilter_get_by_name("buffersink");
    if (buffersinkFilterv == NULL)
    {
        LOG_ERROR("Failed to find ffmpeg sink buffer filter...\n");
        return 1;
    }
    return 0;
}

// Allocates ffmpeg filter graph or throws an exception with information about problem
int frameCounterFilterInit()
{
    filterGraphv = avfilter_graph_alloc();
    if (filterGraphv == NULL)
    {
        LOG_ERROR("Failed to allocate filters graph...\n");
        return 1;
    }

    filterGraphv->thread_type = 0;
    return 0;
}

int frameCounterConfigureFilterGraph(int width, int height, int fps)
{
    int ret = 0;
    {
        char filterIn[256];
        snprintf(filterIn, sizeof(filterIn), "video_size=%dx%d:pix_fmt=%d:time_base=1/%d:pixel_aspect=1/1",
                 width, height, getPixelFormat(), fps);
        enum AVPixelFormat pix_fmts[] = {getPixelFormat(), AV_PIX_FMT_NONE};
        ret = avfilter_graph_create_filter(&srcBufferv, bufferFilterv, "in", filterIn, NULL, filterGraphv);
        av_opt_set_int_list(srcBufferv, "pix_fmts", pix_fmts,
                            AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0)
        {
            LOG_ERROR("Failed to create sink filter...\n");
            return 1;
        }
    }
    {
        enum AVPixelFormat pix_fmts[] = {getPixelFormat(), AV_PIX_FMT_NONE};
        ret = avfilter_graph_create_filter(&sinkBufferv, buffersinkFilterv, "out", NULL, NULL, filterGraphv);
        av_opt_set_int_list(sinkBufferv, "pix_fmts", pix_fmts,
                            AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0)
        {
            LOG_ERROR("Failed to create sink filter...\n");
            return 1;
        }
    }

    AVFilterInOut *inputs = avfilter_inout_alloc(), *outputs = avfilter_inout_alloc();
    if (!inputs || !outputs)
    {
        LOG_ERROR("Failed to allocate inout filters..\n");
        return 1;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = srcBufferv;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = sinkBufferv;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    char *frameCounterFilter = "drawtext=text='Frame\\: %{n}':x=10:y=10:fontsize=24:fontcolor=red";
    ret = avfilter_graph_parse_ptr(filterGraphv, frameCounterFilter, &inputs, &outputs, NULL);
    if (ret < 0)
    {
        LOG_ERROR("Failed to parse '%s' filter\n", frameCounterFilter);
        return 1;
    }

    ret = avfilter_graph_config(filterGraphv, NULL);
    if (ret < 0)
    {
        LOG_ERROR("Failed to configure filter\n");
        return 1;
    }

    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);

    return 0;
}

int addFrameCount(int width, int height, void **decodedFrame, AVFrame *tempOut)
{
    int ret = 0;
    // NOTE: extra convert from raw buffer to avframe
    AVFrame *tempIn = av_frame_alloc();
    tempIn->width = width;
    tempIn->height = height;
    tempIn->format = getPixelFormat();
    av_image_fill_arrays(tempIn->data, tempIn->linesize,
                         *decodedFrame, tempIn->format, width, height, 1);

    tempIn->pts = decodedPts++;
    ret = av_buffersrc_add_frame_flags(srcBufferv, tempIn, AV_BUFFERSRC_FLAG_PUSH);
    if (ret < 0)
    {
        LOG_ERROR("Failed to add frame to source buffer...\n");
        return 1;
    }
    av_frame_free(&tempIn);

    //tempOut = av_frame_alloc();
    for (int i = 0; i < 100; ++i)
    {
        ret = av_buffersink_get_frame(sinkBufferv, tempOut);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        if (ret < 0)
        {
            LOG_ERROR("Failed to get frame from sink buffer...\n");
            return 1;
        }
    }

    //free(*decodedFrame);
    *decodedFrame = tempOut->data[0];

    return 0;
}

void frameCounterFilterDeinit()
{
    avfilter_graph_free(&filterGraphv);
}
