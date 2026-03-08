/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <stdbool.h>

#include <pipewire/pipewire.h>
#include <spa/param/audio/raw.h>
#include <spa/param/audio/format.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define PIPEWIRE_DEFAULT_RATE 48000
#define PIPEWIRE_DEFAULT_CHANNELS 2

// for 48Khz/2 channels, pw-top for my streams:
// S   ID  QUANT   RATE    WAIT    BUSY   W/Q   B/Q  ERR FORMAT           NAME
// S   53      0      0    ---     ---   ---   ---     0                  alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio_3__sink
// R   54   1024  48000  23.8us   8.2us  0.00  0.00    6    S32LE 2 48000 alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio_1__sink
// R   96      0      0  10.6us   6.6us  0.00  0.00    3    S32LE 2 48000  + audio-output
// S   55      0      0    ---     ---   ---   ---     0                  alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio__sink
// S   56      0      0    ---     ---   ---   ---     0                  alsa_input.usb-Generic_USB_Audio-00.HiFi__hw_Audio_2__source
// S   57      0      0    ---     ---   ---   ---     0                  alsa_input.usb-Generic_USB_Audio-00.HiFi__hw_Audio_1__source
// R   33   1024  48000  29.6us   0.5us  0.00  0.00    0    S16LE 1 48000 alsa_input.usb-046d_HD_Webcam_C525_2BC712D0-00.mono-fallback
// R   88      0      0  13.7us   9.9us  0.00  0.00    0    S32LE 2 48000  + audio-source

// for 44100hz/1 channel, pw-top for my streams:
// S   ID  QUANT   RATE    WAIT    BUSY   W/Q   B/Q  ERR FORMAT           NAME 
// S   53      0      0    ---     ---   ---   ---     0                  alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio_3__sink
// R   54   1024  48000  46.6us   8.3us  0.00  0.00    6    S32LE 2 48000 alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio_1__sink
// R   98      0      0   9.8us  31.3us  0.00  0.00    4    S32LE 1 44100  + audio-output
// S   55      0      0    ---     ---   ---   ---     0                  alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio__sink
// S   56      0      0    ---     ---   ---   ---     0                  alsa_input.usb-Generic_USB_Audio-00.HiFi__hw_Audio_2__source
// S   57      0      0    ---     ---   ---   ---     0                  alsa_input.usb-Generic_USB_Audio-00.HiFi__hw_Audio_1__source
// R   33   1024  48000  37.4us   0.6us  0.00  0.00    0    S16LE 1 48000 alsa_input.usb-046d_HD_Webcam_C525_2BC712D0-00.mono-fallback
// R   95      0      0  12.8us  20.4us  0.00  0.00    0    S32LE 1 44100  + audio-source

    // Main data structure
    typedef struct _PipewireData
    {
        struct pw_main_loop *loop;
        struct pw_stream *sourceStream; // input stream in pipewire terminology
        struct pw_stream *sinkStream;   // output strema in pipewire terminology

        struct spa_audio_info format;
        int buffer_size;
        uint32_t rate;
        uint32_t channels;
    } PipewireData;

    // Pipeware base audio interface
    int pipewireAudioDetailed();
    int pipewireStartAudioStream();
    void pipewireStopAudioStream(bool vas_stopped);

#ifdef __cplusplus
}
#endif
