/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "logging.h"

#if defined(AUDIO_BACKEND_PULSE)
#include "../audio/pulseaudio_devices.h"
#elif defined(AUDIO_BACKEND_PIPEWIRE)
#include "../audio/pipewire_audio_devices.h"
#elif defined(AUDIO_BACKEND_ALSA)
#include "../audio/alsa_devices.h"
#endif

// Basic log level
LogLevel log_lvl = INFO;
// colorized output
bool ColorOutput = true;

CamParams camParams = {
    .dev = { '/','d','e','v','/','v','i','d','e','o','0' },
    .lvl = INFO,
    .vid = 0x123,
    .pid = 0x123,
    .frame_counter = false,
    .noise_supression = false,
    .geometry = {
        .width = 640u,
        .height = 480u
    },
    .dither_param = {
        .algo = { 'f','s', 't', 'e', 'i', 'n', '\0' },
        .charset = { 'a', 's', 'c', 'i', 'i', '\0' },
        .color = { 'f', 'u', 'l', 'l', '1', '6', '\0'}
    },
    .use_best_quality = false};

char const *supportedBackends[] = {"x11", "slang", NULL};
static const char* ditherAlgoList[] = { "none", "ordered2", "ordered4", "ordered8", "random", "fstein", NULL };
static const char* ditherCharsetList[] = { "ascii", "shades", "block", NULL };
static const char* ditherColorList[] = { "mono", "fullgray", "full16", NULL };

enum
{
    NO_PRETTY_OPT = 224,
    FPS_OPT = 225,
    BEST_QUALITY_OPT = 226,
    CACA_BACKEND_OPT = 227,
    DITHER_ALGO_OPT = 228,
    DITHER_CHARSET_OPT = 229,
    DITHER_COLOR_OPT = 230
};

static int set_video(char *optarg, CamParams *p)
{
    if (!optarg)
    {
        LOG_WARNING("Use --camera option instead\n");
        return -1;
    }

    size_t lcam = strlen(optarg) + 1;
    memset(p->dev, 0, sizeof(p->dev)/sizeof(p->dev[0]));
    if (lcam > sizeof(p->dev))
        strlcpy(p->dev, optarg, sizeof("/dev/videoxx"));
    else
        strlcpy(p->dev, optarg, lcam);
    LOG_INFO("camera device: %s\n", p->dev);
    return 0;
}

static int set_geometry(char *optarg, CamParams *p)
{
    if (!optarg)
        return -1;

    const char *rpos = strchr(optarg, 'x');
    if (!rpos)
    {
        LOG_ERROR("invalid format\n");
        return -1;
    }

    int width = atoi(optarg);
    if (width < 0 || width > 3840u)
    {
        LOG_ERROR("Width incorrect: %u", width);
        return -1;
    }

    int height = atoi(rpos + 1);
    if (height < 0 || height > 2160u)
    {
        LOG_ERROR("Height incorrect: %u", height);
        return -1;
    }

    p->geometry.width = width;
    p->geometry.height = height;
    LOG_INFO("Geometry: %ux%u\n", width, height);

    return 0;
}

static void set_dither_param(char *optarg, void *p, const char* list[])
{
    for (int i = 0; list[i] != NULL; ++i)
    {
        if (!strcmp(list[i], optarg))
        {
            memset(p, 0, 16);
            strlcpy(p, list[i], strlen(list[i]) + 1);
            return;
        }
    }
}

static int parse_opt(int key, const char *arg, CamParams *camParams)
{
    (void)arg;
    switch (key)
    {
    case 'e':
        LOG_INFO("Set 'DEVEL' log level\n");
        LOG_SET_DEVEL();
        break;
    case 'd':
        LOG_INFO("Set 'DEBUG' log level\n");
        LOG_SET_DEBUG();
        break;
    case 's':
        LOG_INFO("Set 'FATAL' log level\n");
        LOG_SET_CRITICAL();
        break;
    case 'v':
        camParams->vid = strtol(optarg, NULL, 16);
        LOG_INFO("vid: 0x%04x\n", camParams->vid);
        break;
    case 'p':
        camParams->pid = strtol(optarg, NULL, 16);
        LOG_INFO("pid: 0x%04x\n", camParams->pid);
        break;
    case 'c':
        if (set_video(optarg, camParams) == -1)
            return -1;
        break;
    case 'f':
        camParams->frame_counter = true;
        LOG_INFO("frame counter added\n");
        break;
    case 'n':
        camParams->noise_supression = true;
        LOG_INFO("noise supression turn on\n");
        break;
    case 'g':
        if (set_geometry(optarg, camParams) == -1)
            return -1;
        break;
    case 'l':
        if (getAudioDevicesInfo())
            LOG_ERROR("getAudioDevicesInfo was failed\n");
        LOG_INFO("Stop app. Just captured devices and print their...\n");
        return -1;
        break;
    case NO_PRETTY_OPT:
        ColorOutput = false;
        break;
    case FPS_OPT:
        camParams->fps = atoi(optarg);
        break;
    case BEST_QUALITY_OPT:
        camParams->use_best_quality = true; // Actually just for v4l2
        break;
    case CACA_BACKEND_OPT:
        for (int i = 0; supportedBackends[i] != NULL; ++i)
        {
            if (!strcmp(supportedBackends[i], optarg))
            {
                camParams->cacaBackend = optarg;
                goto cacaBackend;
            }
        }
        LOG_ERROR("Current libcaca backend; %s is not supported in Dcamera...\n", optarg);
        return -1;
    cacaBackend:
        break;
    case DITHER_ALGO_OPT:
        set_dither_param(optarg, camParams->dither_param.algo, ditherAlgoList);
        break;
    case DITHER_CHARSET_OPT:
        set_dither_param(optarg, camParams->dither_param.charset, ditherCharsetList);
        break;
    case DITHER_COLOR_OPT:
        set_dither_param(optarg, camParams->dither_param.color, ditherColorList);
        break;
    case 'h':
        help();
        return -1;
    default:
        LOG_ERROR("Failed to parse key: %d", key);
        return -1;
    }
    return 0;
}

int parse_args(int argc, char **argv, CamParams *camParams)
{
    int c;

    static struct option long_options[] = {
        {"devel", no_argument, NULL, 'e'},
        {"device-list", no_argument, NULL, 'l'},
        {"debug", no_argument, NULL, 'd'},
        {"silent", no_argument, NULL, 's'},
        {"vid", required_argument, NULL, 'v'},
        {"pid", required_argument, NULL, 'p'},
        {"camera", required_argument, NULL, 'c'},
        {"frame", no_argument, NULL, 'f'},
        {"noise-supression", no_argument, NULL, 'n'},
        {"geometry", required_argument, NULL, 'g'},
        {"fps", required_argument, NULL, FPS_OPT},
        {"no-pretty", no_argument, NULL, NO_PRETTY_OPT},
        {"best-quality", no_argument, NULL, BEST_QUALITY_OPT},
        {"caca-backend", required_argument, NULL, CACA_BACKEND_OPT},
        {"dither-algo", required_argument, NULL, DITHER_ALGO_OPT},
        {"dither-charset", required_argument, NULL, DITHER_CHARSET_OPT},
        {"dither-color", required_argument, NULL, DITHER_COLOR_OPT},
        {"help", no_argument, NULL, 'h'},
        {0, 0, NULL, 0}};

    while (1)
    {
        int option_index = 0;
        c = getopt_long(argc, argv, "eldsvpcfngh", long_options, &option_index);
        if (c == -1)
            return 0;

        if (parse_opt(c, optarg, camParams) == -1)
            return -1;
    }

    return 0;
}
