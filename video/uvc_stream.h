/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <libuvc/libuvc.h>
#include "../sync.h"
#include "../common/args.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Attach usb camera to uvcvideo linux usb backend
void attach_to_kernel_driver(int signal);

// UVC video backend stream interface
int configureUVCVideoStream(CamParams m_camParams);
int startUVCVideoStream();
void stopUVCVideoStream(bool);

#ifdef __cplusplus
}
#endif

