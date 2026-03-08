/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include "videostream.h"

#ifdef VIDEO_BACKEND_UVC
#include "../video/uvc_stream.h"
#else
#include "../video/v4l_stream.h"
#endif

#include "../video/common.h"

extern VideoStreamProvider ifaceVideo;

void VideoStream::VideoStream::start()
{
    // For uvc need vid/pid
    if (ifaceVideo.configure(m_camParams))
    {
        LOG_ERROR("Error in Qt video stream configure...\n");
        return;
    }
    if (ifaceVideo.start())
    {
        LOG_ERROR("Error in Qt start video stream...\n");
        return;
    }
}
void VideoStream::VideoStream::stopped()
{
    LOG_DEBUG("|-----stop video-----|\n");
    if (!m_stopped)
        ifaceVideo.stop(m_stopped);
    m_stopped = true;
}
