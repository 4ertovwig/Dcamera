/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdlib.h>
#include <string.h>

#include "jpeg_decoder.h"
#include "../common/logging.h"

void *jpeg_decode(void *encoded_data, unsigned int encoded_size)
{
    struct jpeg_decompress_struct *cinfo = (struct jpeg_decompress_struct *)malloc(sizeof(struct jpeg_decompress_struct));
    if (cinfo == NULL)
    {
        LOG_ERROR("Error in allocate decoded jpeg_decompress_struct...\n");
        return NULL;
    }
    struct jpeg_error_mgr jerr;
    // NOTE: crash without it
    cinfo->err = jpeg_std_error(&jerr);

    jpeg_create_decompress(cinfo);

    jpeg_mem_src(cinfo, encoded_data, encoded_size);

    (void)jpeg_read_header(cinfo, FALSE /*TRUE*/);
    (void)jpeg_start_decompress(cinfo);

    // physical row width in output buffer
    int row_stride = cinfo->output_width * cinfo->output_components;
    JSAMPARRAY buffer = (*cinfo->mem->alloc_sarray)((j_common_ptr)cinfo, JPOOL_IMAGE, row_stride, 1);

    int seek = 0;
    void *decodedFrame = malloc(cinfo->output_width * cinfo->output_height * 3); // allocate for BGR24
    if (decodedFrame == NULL)
    {
        LOG_ERROR("Error in allocate decoded frame...\n");
        goto finish;
    }

    while (cinfo->output_scanline < cinfo->output_height)
    {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could ask for
         * more than one scanline at a time if that's more convenient.
         */
        (void)jpeg_read_scanlines(cinfo, buffer, 1);
        memcpy(decodedFrame + seek, buffer[0], row_stride);
        seek += row_stride;
    }

finish:
    (void)jpeg_finish_decompress(cinfo);
    jpeg_destroy_decompress(cinfo);

    return decodedFrame;
}
