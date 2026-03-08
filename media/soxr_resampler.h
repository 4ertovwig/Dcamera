/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <soxr.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Base libsoxr interface:
    // resemper configuration
    int configureResampler(void *data, size_t length, double inputRate, double outputRate,
                           unsigned inputChannels, unsigned outputChannels, soxr_datatype_t inputType, soxr_datatype_t outputType, size_t *outSize);

    // convertation
    void *soxrConvert(size_t outSize);

    // Release soxr resempler
    void soxrRelease();

#ifdef __cplusplus
}
#endif
