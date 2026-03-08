/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include "../common/args.h"
#include "../sync.h"
#include "../media/converter.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        void *data;
        size_t data_bytes;
    } v4l_buffer;

    // Base v4l2 backend stream interface
    int configureV4lVideoStream(CamParams m_camParams);
    int startV4lVideoStream();
    void stopV4lVideoStream(bool);

#ifdef __cplusplus
}
#endif
