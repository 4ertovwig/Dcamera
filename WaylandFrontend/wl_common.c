/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#define _GNU_SOURCE 
#include <errno.h>
#include <stdlib.h>
#include <linux/input.h>
#include <linux/mman.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "wl_common.h"
#include "../sync.h"

extern SharedVideoFrame *sharedFrame;
extern SynchronizeData syncData;

const char *dump_format(uint32_t format, char out[4])
{
#if BYTE_ORDER == BIG_ENDIAN
    format = bswap32(format);
#endif
    memcpy(out, &format, 4);
    return out;
}

uint32_t* generateDefaultFrame(int width, int height)
{
    uint32_t *defaultData = malloc(width * height * sizeof(uint32_t));
    memset(defaultData, 0, width * height * sizeof(uint32_t));
    return defaultData;
}

uint32_t *bgr24ToXRGB8888(const uint8_t *videoFrame,
                          int width, int height)
{
    if (!videoFrame)
    {
        LOG_ERROR("Video frame from video stream is NULL\n");
        return NULL;
    }

    // XRGB8888 use 32 bit on pixel
    uint32_t *xrgb_data = malloc(width * height * sizeof(uint32_t));

    for (int i = 0; i < width * height; i++)
    {
        uint8_t b = videoFrame[i * 3 + 0]; // B
        uint8_t g = videoFrame[i * 3 + 1]; // G
        uint8_t r = videoFrame[i * 3 + 2]; // R

        // XRGB8888: 0x00RRGGBB (alpha=0)
        xrgb_data[i] = (0x00 << 24) | (r << 16) | (g << 8) | b;
    }

    return xrgb_data;
}

struct wl_buffer *create_wl_buffer_from_pixels(struct wl_shm *shm,
                                                      SharedVideoFrame *sharedFrame)
{
    void *xrgb8888data = NULL;
    // if (sharedFrame->data[sharedFrame->videobufSurface])
    // {
    //     xrgb8888data = bgr24ToXRGB8888(sharedFrame->data[sharedFrame->videobufSurface], sharedFrame->width, sharedFrame->height);
    // }
    // else 
    // {
    //     int videoSurface = sharedFrame->data[sharedFrame->videobufSurface] == 0 ? 1 : 0;
    //     if (sharedFrame->data[videoSurface])
    //         xrgb8888data = bgr24ToXRGB8888(sharedFrame->data[videoSurface], sharedFrame->width, sharedFrame->height);
    //     else
    //         xrgb8888data = generateDefaultFrame(sharedFrame->width, sharedFrame->height);
    // }
    xrgb8888data = bgr24ToXRGB8888(sharedFrame->data[sharedFrame->videobufSurface], sharedFrame->width, sharedFrame->height);
    if (!xrgb8888data)
    {
        LOG_INFO("Generate default frame...\n");
        xrgb8888data = generateDefaultFrame(sharedFrame->width, sharedFrame->height);
    }

    int width = sharedFrame->width;
    int height = sharedFrame->height;
    // set stride for 8888 extra bytes in width
    int stride = sharedFrame->strideSize == 4 * sharedFrame->width ? sharedFrame->strideSize : 4 * sharedFrame->width;
    int size = stride * height;
    LOG_DEV("create_wl_buffer_from_pixels width: %d height: %d stride: %d \n", width, height, stride);
    int fd = memfd_create("dcam-shm", MFD_CLOEXEC);
    if (fd < 0)
    {
        LOG_ERROR("memfd_create is NULL\n");
        goto bad_situation;
    }

    if (ftruncate(fd, size) < 0)
    {
        close(fd);
        LOG_ERROR("ftruncate error\n");
        goto bad_situation;
    }

    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
    {
        close(fd);
        LOG_ERROR("Error in mmap %s size: %d\n", strerror(errno), size);
        goto bad_situation;
    }

    // Copy pixel
    // data already in compositor
    memcpy(data, xrgb8888data, size);
    munmap(data, size);
    free(xrgb8888data);

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    close(fd);

    struct wl_buffer *buffer = wl_shm_pool_create_buffer(
        pool,
        0,                     // offset
        width, height,         // size
        stride,                // stride
        WL_SHM_FORMAT_XRGB8888 // my weston support just XRGB8888
    );

    if (!buffer)
    {
        LOG_ERROR("buffer error\n");
        goto bad_situation;
    }

    wl_shm_pool_destroy(pool);
    return buffer;
bad_situation:
    return NULL;
}
