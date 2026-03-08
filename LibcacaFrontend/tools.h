/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <sys/ioctl.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

// NOTE: my configuration: i have two monitors - one horizontal and one vertical
// $ xrandr -d :1
// Screen 0: minimum 8 x 8, current 4000 x 2560, maximum 32767 x 32767
// HDMI-0 disconnected (normal left inverted right x axis y axis)
// DP-0 connected primary 2560x1440+0+0 (normal left inverted right x axis y axis) 600mm x 340mm
//    2560x1440     60.00 + 180.00   165.00*  144.00   120.00
//    3840x2160     59.94    50.00
//    1920x1080    119.99    60.00    59.94    50.00
//    1680x1050     59.95
//    1600x900      60.00
//    1440x900      59.89
//    1280x1024     75.02    60.02
//    1280x960      60.00
//    1280x720      60.00    59.94    50.00
//    1152x864      75.00
//    1024x768      75.03    60.00
//    800x600       75.00    60.32
//    720x576       50.00
//    720x480       59.94
//    640x480       75.00    59.94    59.93
// DP-1 disconnected (normal left inverted right x axis y axis)
// DP-2 disconnected (normal left inverted right x axis y axis)
// DP-3 disconnected (normal left inverted right x axis y axis)
// DP-4 connected 1440x2560+2560+0 left (normal left inverted right x axis y axis) 600mm x 340mm
//    2560x1440     60.00 + 180.00*  165.00   144.00   120.00
//    3840x2160     59.94    29.97
//    1920x1200     59.88
//    1920x1080    119.88    60.00    60.00    59.94    50.00
//    1680x1050     59.95
//    1600x1200     60.00
//    1440x900      59.89
//    1280x1024     75.02    60.02
//    1280x960      60.00
//    1280x800      59.81
//    1280x720      60.00    59.94    50.00
//    1152x864      75.00
//    1024x768      75.03    60.00
//    800x600       75.00    60.32
//    720x576       50.00
//    720x480       59.94
//    640x480       75.00    59.94    59.93
// DP-5 disconnected (normal left inverted right x axis y axis)

int setCharSize(int *cwidth, int *cheight, int cmdwidth, int cmdheight);

// Dither params setter
int ditherSetAlgorithm(caca_dither_t const *dither, const char *algo);
int ditherSetCharSet(caca_dither_t const *dither, const char *set);
int ditherSetColor(caca_dither_t const *dither, const char *clist);
