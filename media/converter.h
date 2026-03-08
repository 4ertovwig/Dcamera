/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavutil/pixfmt.h"
#include "libavutil/frame.h"
#include "libavutil/mem.h"

// Parameters for swscale context
typedef struct _ConverterParams
{
    int src_width;
    int src_height;
    int dst_width;
    int dst_height;
    enum AVPixelFormat src_format;
    enum AVPixelFormat dst_format;

} ConverterParams;

// Create converter parameters
ConverterParams createConverterParams(int src_height, int src_width, int dst_height, int dst_width,
                                        enum AVPixelFormat src_format,
                                        enum AVPixelFormat dst_format);

// Base swscale converter interface
int createContext(struct SwsContext **ctx, ConverterParams params);
unsigned char *convert(struct SwsContext *ctx, void *data, ConverterParams params, int *outDataSize,
                        void **data_plane, int *linesize);
void releaseContext(struct SwsContext *ctx);

#ifdef __cplusplus
}
#endif
