/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stdbool.h>
#include <pulse/pulseaudio.h>

#ifdef __cplusplus
extern "C"
{
#endif

// for my system is it so:
#define PULSEAUDIO_DEVICE_DEFAULT_SINK "alsa_output.usb-Generic_USB_Audio-00.3.HiFi__hw_Audio_1__sink"
#define PULSEAUDIO_DEVICE_DEFAULT_SOURCE "alsa_input.usb-046d_HD_Webcam_C525_2BC712D0-00.3.mono-fallback"
// This values can give as:
// $ ./Dcamera --device-list

    // PulseAudio audio base interface
    int pulseaudioConfiguration();
    int pulseaudioStartStream();
    void pulseaudioStopStream(bool vas_stopped);

#ifdef __cplusplus
}
#endif
