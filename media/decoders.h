/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include "libavcodec/avcodec.h" 

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct LibavVideoDecoder_ {
    AVCodec *codec;
    AVCodecContext *context;
    AVPacket *packet;
} LibavVideoDecoder;

// Base libav decoder interface
int decoderInit(enum VIDEO_FRAME_TYPE type);
void *decode(void *encoded_data, unsigned int encoded_size/*, unsigned int* decodedSize*/);

#ifdef __cplusplus
}
#endif
