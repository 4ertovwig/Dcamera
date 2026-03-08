/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stdio.h>
#include <jpeglib.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Decoder interface
// NOTE: BGR24 output frame format
void *jpeg_decode(void *encoded_data, unsigned int encoded_size);

#ifdef __cplusplus
}
#endif
