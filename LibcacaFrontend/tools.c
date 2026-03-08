/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <string.h>
#include <caca.h>

#include "../common/logging.h"

#include "tools.h"

///////////////////////////////////////////////////////////////////////////////////////////
// Window tools

// Get active monitor size in pixels
// NOTE: you can have multiple monitors. We need the one on which the application runs.
// return error if ret != 0
static int getActiveMonitorSize(int *width, int *height)
{
    *width = 0, *height = 0;
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy)
        return 1;

    // Get cursor position
    int root_x, root_y;
    Window root_return, child_return;
    int win_x, win_y;
    unsigned int mask;

    // In my case:
    // monitor 1 - 2560×1440
    // monitor 2 - 1440×2560
    // DefaultScreen return virtual monitor size (2560+1440)x2560 = 4000x2560
    Window root = RootWindow(dpy, DefaultScreen(dpy));

    XQueryPointer(dpy, root, &root_return, &child_return,
                  &root_x, &root_y, &win_x, &win_y, &mask);

    // We get information about all screens via Xinerama (if any).
    int event_base, error_base;
    if (XineramaQueryExtension(dpy, &event_base, &error_base) &&
        XineramaIsActive(dpy))
    {
        int num_screens;
        XineramaScreenInfo *screens = XineramaQueryScreens(dpy, &num_screens);

        if (screens)
        {
            for (int i = 0; i < num_screens; i++)
            {
                if (root_x >= screens[i].x_org &&
                    root_x < screens[i].x_org + screens[i].width &&
                    root_y >= screens[i].y_org &&
                    root_y < screens[i].y_org + screens[i].height)
                {
                    *width = screens[i].width;
                    *height = screens[i].height;
                    break;
                }
            }
            XFree(screens);
        }
    }

    // If xinerama is not worked
    if (*width == 0)
        return 1;

    return 0;
}

// get terminal size in chars
static void getScreenCharSize(int *maxcwidth, int *maxcheight)
{
    struct winsize w;
    // WARNING: STDOUT_FILENO is uncorrectly worked
    ioctl(/*STDOUT_FILENO*/ 0, TIOCGWINSZ, &w);
    LOG_INFO("TERMINAL size in chars: columns: %d rows: %d\n", w.ws_col, w.ws_row);
    *maxcwidth = w.ws_col;
    *maxcheight = w.ws_row;
}

int setCharSize(int *cwidth, int *cheight, int cmdwidth, int cmdheight)
{
    int pixelsWidth, pixelsHeight;
    if (getActiveMonitorSize(&pixelsWidth, &pixelsHeight) != 0)
    {
        LOG_WARNING("getActiveMonitorSize return error...\n");
        return 1;
    }

    int maxcwidth, maxcheight;
    getScreenCharSize(&maxcwidth, &maxcheight);

    float w = cmdwidth / pixelsWidth;
    float h = cmdheight / pixelsHeight;

    *cwidth = w * maxcwidth;
    *cheight = h * maxcheight;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Dithering tools

int ditherSetAlgorithm(caca_dither_t const *dither, const char *algo)
{
    char const * const *list = caca_get_dither_algorithm_list(dither);
    int use = 0;
    for (int i = 0; list[i] != NULL; i += 2)
    {
        if (strcmp(list[i], algo) == 0)
        {
            LOG_INFO("Used %s dithering algorithm\n", algo);
            return caca_set_dither_algorithm(dither, algo);
        }
    }
    LOG_INFO("Use default dithering algorithm - 'Floyd-Steinberg dithering'\n");
    return 0;
}

int ditherSetCharSet(caca_dither_t const *dither, const char *set)
{
    char const * const *list = caca_get_dither_charset_list(dither);
    for (int i = 0; list[i] != NULL; i += 2)
    {
        if (strcmp(list[i], set) == 0)
        {
            LOG_INFO("Used %s dithering charset\n", set);
            return caca_set_dither_charset(dither, set);
        }
    }
    LOG_INFO("Use default dithering charset - 'ascii'\n");
    return 0;
}

int ditherSetColor(caca_dither_t const *dither, const char *clist)
{
    char const * const *list = caca_get_dither_color_list(clist);
    for (int i = 0; list[i] != NULL; i += 2)
    {
        if (strcmp(list[i], clist) == 0)
        {
            LOG_INFO("Used %s dithering color\n", clist);
            return caca_set_dither_color(dither, clist);
        }
    }
    LOG_INFO("Use default dithering color - 'full16'\n");
    return 0;
}
