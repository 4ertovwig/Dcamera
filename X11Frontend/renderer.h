/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include "../media/converter.h"

typedef struct SwsContext Context;

// Struct for passing pthread data
typedef struct _XCBRenderData
{
    void *data;
    uint32_t width;
    uint32_t height;
    xcb_connection_t *connection;
    xcb_pixmap_t *pixmap;
    xcb_window_t window;
    xcb_gcontext_t gcontext;
    int depth;
    int shmemSegId;
    ConverterParams converterParams;
    Context *swsContext;
} XCBRenderData;

// libxcb rendering
void *XCBRender(void *arg);
