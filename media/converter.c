/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"

#include "converter.h"
#include "../common/logging.h"

ConverterParams createConverterParams(int src_width, int src_height, int dst_width, int dst_height,
                                      enum AVPixelFormat src_format,
                                      enum AVPixelFormat dst_format)
{
    return (ConverterParams){
        .src_width = src_width,
        .src_height = src_height,
        .dst_width = dst_width,
        .dst_height = dst_height,
        .src_format = src_format,
        .dst_format = dst_format};
}

int createContext(struct SwsContext **ctx, ConverterParams params)
{
    *ctx = sws_getContext(params.dst_width, params.dst_height, params.src_format, params.dst_width,
                                      params.dst_height, params.dst_format, SWS_BILINEAR, NULL, NULL, NULL);
    if (!*ctx)
    {
        LOG_ERROR("Error in create sws context\n");
        return 1;
    }
    return 0;
}

static AVFrame *createAVframe(uint8_t *inputBuf, int width, int height, enum AVPixelFormat pix_fmt)
{
    AVFrame *frame = av_frame_alloc();
    if (!frame)
    {
        LOG_ERROR("Error in allocate av frame\n");
        return NULL;
    }

    frame->width = width;
    frame->height = height;
    frame->format = pix_fmt;

    av_image_fill_arrays(frame->data, frame->linesize, inputBuf, pix_fmt, width, height, 1);

    return frame;
}

unsigned char *convert(struct SwsContext *ctx, void *inputData, ConverterParams params, int *outDataSize, void **data_stride, int *dst_stride)
{
    int outBufferSize = av_image_get_buffer_size(params.dst_format, params.dst_width, params.dst_height, 1);
    *outDataSize = outBufferSize;
    // bgr for uvc, or rgba for libcaca
    unsigned char *outBuffer = av_malloc(outBufferSize);
    if (!outBuffer)
    {
        LOG_ERROR("Error in allocate bgr output frame\n");
        return NULL;
    }
    // Create avframe from raw yuyv for uvc
    AVFrame *srcFrame = createAVframe(inputData, params.src_width, params.src_height, params.src_format);

    // output frame with bgr888 for uvc, or rgba8888 for libcaca
    AVFrame *dstFrame = createAVframe(outBuffer, params.dst_width, params.dst_height, params.dst_format);
    *dst_stride = dstFrame->linesize[0];

    // attempt to fix 'unable to decode APP fields: Invalid data found when processing input'
    // sws_setColorspaceDetails(
    //     ctx,
    //     sws_getCoefficients(SWS_CS_DEFAULT),
    //     srcFrame->color_range == AVCOL_RANGE_JPEG ? 0 : 1, // src range: 0 = full, 1 = limited
    //     sws_getCoefficients(SWS_CS_DEFAULT), 0, 0, 0, 1);

    // convertation
    sws_scale(ctx,
              (const uint8_t *const *)srcFrame->data,
              srcFrame->linesize,
              0, params.dst_height,
              dstFrame->data,
              dstFrame->linesize);

    *data_stride = av_malloc(outBufferSize);
    if (!data_stride)
    {
        LOG_ERROR("Error in allocate data_plane\n");
        return NULL;
    }
    memcpy(*data_stride, dstFrame->data[0], *outDataSize);
    av_free(outBuffer);
    av_frame_free(&dstFrame);
    av_frame_free(&srcFrame);

    return *data_stride;
}

void releaseContext(struct SwsContext* ctx)
{
    sws_freeContext(ctx);
}
