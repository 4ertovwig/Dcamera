/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include "caca.h"
#include "../media/converter.h"

typedef struct SwsContext Context;

// Struct for passing pthread data
typedef struct _CacaRenderData
{
    uint32_t canvasWidth;  // in character cells
    uint32_t canvasHeight; // in character cells
    caca_display_t **dp;
    caca_canvas_t **cv;
    caca_dither_t **dither;
    ConverterParams converterParams;
    Context *swsContext;
} CacaRenderData;

// libcaca rendering
void *CacaRender(void *arg);

