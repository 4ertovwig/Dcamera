/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdio.h>
#include <stdlib.h>

#include "soxr_resampler.h"

#include "../common/logging.h"

typedef struct soxrContext_
{
    soxr_t soxr;
    void *data;
    size_t length;
    double inputRate;
    double outputRate;
    unsigned inputChannels;
    unsigned outputChannels;
    soxr_datatype_t inputType;
    soxr_datatype_t outputType;
} soxrContext;
soxrContext soxrCtx;

int configureResampler(void *data, size_t length, double inputRate, double outputRate, unsigned inputChannels, unsigned outputChannels,
                       soxr_datatype_t inputType, soxr_datatype_t outputType, size_t *outSize)
{
    soxr_error_t error;
    soxrCtx = (soxrContext){
        .soxr = NULL,
        .data = data,
        .length = length,
        .inputRate = inputRate,
        .outputRate = outputRate,
        .inputChannels = inputChannels,
        .outputChannels = outputChannels,
        .inputType = inputType,
        .outputType = outputType};
    soxr_io_spec_t io_spec = soxr_io_spec(inputType, outputType);
    // Create a stream resampler
    soxrCtx.soxr = soxr_create(inputRate, outputRate, inputChannels, &error, &io_spec, NULL, NULL);
    if (error)
    {
        LOG_ERROR("Error in soxr_create...\n");
        return 1;
    }
    *outSize = (float)(soxrCtx.length * soxr_datatype_size(soxrCtx.outputType) * soxrCtx.outputChannels * soxrCtx.outputRate) /
               (soxr_datatype_size(soxrCtx.inputType) * soxrCtx.inputChannels * soxrCtx.inputRate);
    return 0;
}

void *soxrConvert(size_t outSize)
{
    size_t clips = 0;
    void *outputBuffer = malloc(outSize);
    size_t inSamples = soxrCtx.length / (soxr_datatype_size(soxrCtx.inputType) * soxrCtx.inputChannels);
    size_t outSamples = outSize / (soxr_datatype_size(soxrCtx.outputType) * soxrCtx.outputChannels);
    size_t inDone = 0, outDone = 0;
    soxr_error_t error = soxr_process(soxrCtx.soxr, soxrCtx.data, inSamples, &inDone, outputBuffer, outSamples, &outDone);
    LOG_DEV("outDone: %lu inDone: %lu outSize: %lu\n", outDone, inDone, outSize);
    if (error)
    {
        LOG_ERROR("Error in processing: %s\n", soxr_strerror(error));
        free(outputBuffer);
        return NULL;
    }

    return outputBuffer;
}

void soxrRelease()
{
    soxr_delete(soxrCtx.soxr);
}
