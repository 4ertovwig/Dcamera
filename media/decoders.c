/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include "libavutil/pixfmt.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"

#include "../sync.h"
#include "../common/logging.h"
#include "decoders.h"

LibavVideoDecoder videoDecoder;

int decoderInit(enum VIDEO_FRAME_TYPE type)
{
    // find the mpeg1 or H254 video decoder
    switch (type)
    {
    case VIDEO_FRAME_TYPE_MJPEG:
        videoDecoder.codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
        break;
    case VIDEO_FRAME_TYPE_H264:
        videoDecoder.codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        break;
    default:
        videoDecoder.codec = NULL;
        break;
    }
    if (!videoDecoder.codec)
    {
        LOG_ERROR("AVcodec not found...\n");
        return 1;
    }

    videoDecoder.context = avcodec_alloc_context3(videoDecoder.codec);
    if (!videoDecoder.context)
    {
        LOG_ERROR("Cannot alloc context...\n");
        return 1;
    }
    videoDecoder.context->codec_type = AVMEDIA_TYPE_VIDEO;
    // if (videoDecoder.codec->capabilities & CODEC_CAP_TRUNCATED)
    //     videoDecoder.context->flags |= CODEC_FLAG_TRUNCATED; // we do not send complete frames
    if (avcodec_open2(videoDecoder.context, videoDecoder.codec, NULL) < 0)
    {
        LOG_ERROR("Could not open codec...\n");
        return 1;
    }
    
    return 0;
}

void *decode(void *encodedData, unsigned int encodedSize)
{
    int got_picture, frameLen, frame;
    AVFrame *decodedFrame = av_frame_alloc();
    if (!decodedFrame)
    {
        LOG_ERROR("Error in allocate decoded av frame\n");
        return NULL;
    };

    videoDecoder.packet = av_packet_alloc();
    videoDecoder.packet->data = av_malloc(encodedSize);

    // TODO: for H264 need processing I,P,B-frames.
    // Need so decoded time and etc.

    // Construct av_packet from frame_info data
    memcpy(videoDecoder.packet->data, encodedData, encodedSize);
    videoDecoder.packet->size = encodedSize;

    // Decode video frame
    int res = avcodec_send_packet(videoDecoder.context, videoDecoder.packet);
    if (res != 0)
    {
        LOG_ERROR("Error in send packet...\n");
        return NULL;
    }

    res = avcodec_receive_frame(videoDecoder.context, decodedFrame);
    if (res != 0)
    {
        LOG_ERROR("Error in receive frame...\n");
        return NULL;
    }

    LOG_DEBUG("Decoded frame width: %d height: %d pixel format: %d\n", decodedFrame->width, decodedFrame->height, decodedFrame->format);

    bool frame_finished = res == 0;
    if (!frame_finished && res != AVERROR(EAGAIN))
    {
        LOG_ERROR("Error in send packet...\n");
        return NULL;
    }

    // Not needed for MJPEG:
    // Indicate a decoded frame
    //if (frame_finished)
    //  pts_rep = last_frame->reordered_opaque;

    return decodedFrame->data[0];
}
