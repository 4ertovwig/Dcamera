/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "logging.h"

/**
 * Params for camera as device.
 */
typedef struct _CamParams {
    LogLevel lvl;
    int vid;
    int pid;
    char dev[16];
    bool frame_counter;
    bool noise_supression;
    struct Geomertry {
        uint32_t width;
        uint32_t height;
    } geometry;
    int fps;
    bool use_best_quality;
    const char* cacaBackend;
    struct DitherParams {
        char algo[16];
        char charset[16];
        char color[16];
    } dither_param;
} CamParams;

#ifdef __cplusplus
extern "C" 
{
#endif

    int parse_args(int argc, char **argv, CamParams *camParams);

    static inline void help()
    {
        const char *help_message = "Usage:\n"
                                "Dcamera [OPTIONS]\n\n"
                                "Options:\n"
                                "  -l  --device-list                get audio sink/source devices list\n"
                                "  -e, --devel                      get development logging\n"
                                "  -d, --debug                      get debug logging\n"
                                "  -s, --silent                     only fatal log output\n"
                                "  -v, --vid=0x123                  vid of usb camera. Must be in HEX format. Actually just for libuvc backend\n"
                                "  -p, --pid=0xeeff                 pid of usb camera. Must be in HEX format. Actually just for libuvc backend\n"
                                "  -f, --frame                      add frame counter on camera frames surface\n"
                                "  -n, --noise-supression           add noise supression\n"
                                "  --geometry                       set ${width}x${height} for output video\n"
                                "  --fps                            set fps for output video\n"
                                "                                   WARNING: low fps can cause audio artifacts\n"
                                "  --no-pretty                      do not output colored messages to the terminal\n\n"
                                " Just for v4l2 video backend:\n"
                                "  --best-quality                   camera uses the best possible resolution. Actually just for video4linux backend\n"
                                "  -c, --camera=/dev/videoX         camera device in /dev/videoX. Actually just for video4linux backend\n\n"
                                " Just for libcaca frontend:\n"
                                "  --caca-backend={x11,slang}       Use selected backend for libcaca. Your libcaca library must supported selected backend\n"
                                "  --dither-algo                    Use dither algorithm:\n"
                                "                                       none - without dithering\n"
                                "                                       ordered2 - Bayer matrix 2x2\n"
                                "                                       ordered4 - Bayer matrix 4x4\n"
                                "                                       ordered8 - Bayer matrix 8x8\n"
                                "                                       random - random dithering\n"
                                "                                       fstein - Floyd-Steinberg dithering\n"
                                "  --dither-charset                 Use dither charset:\n"
                                "                                       ascii - just ascii symbols\n"
                                "                                       shades - Unicode halftone characters (U+2591, U+2592, U+2593)\n"
                                "                                       block - Unicode Quarter-Block Combinations\n"
                                "  --dither-color                   Use dither color:\n"
                                "                                       mono - Light gray on a black background\n"
                                "                                       fullgray - Black, white and two grays for symbols and background\n"
                                "                                       full16 - 16 ANSI colors for characters and backgrounds\n\n"
                                "  -h, --help                       display this help\n\n";

        printf("%s", help_message);
        fflush(stdout);
    }

#ifdef __cplusplus
}
#endif
