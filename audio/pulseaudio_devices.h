/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <pulse/pulseaudio.h>

#include "../common/logging.h"

#ifdef __cplusplus
extern "C"
{
#endif
    
    /**
     * Function is a simplified analogue of the 'pactl list sinks/sources' utility.
     * Function only works with a flag '--device-list' and then app Dcamera does not run.
     * 
     * Example output:    
     * [INFO] 00003362 ==============================
     * OUTPUT DEVICE: 0
     * Sink Name:        alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio_3__sink
     * Description:      USB Audio S/PDIF Output
     * Sample Rate:      48000 Hz
     * Channels:         2
     * Format:           PCM16bitLE
     * Channel Map:      front-left,front-right
     * Muted:            No
     * [INFO] 00000051 ==============================
     * OUTPUT DEVICE: 1
     * Sink Name:        alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio_1__sink
     * Description:      USB Audio Front Headphones
     * Sample Rate:      48000 Hz
     * Channels:         2
     * Format:           SignedPCM32bitLE
     * Channel Map:      front-left,front-right
     * Muted:            No
     * [INFO] 00000045 ==============================
     * INPUT DEVICE: 0
     * Source Name:        alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio_3__sink.monitor
     * Description:        Monitor of USB Audio S/PDIF Output
     * Sample Rate:        48000 Hz
     * Channels:           2
     * Format:             PCM16bitLE
     * Card index:         48

     * [INFO] 00000026 ==============================
     * INPUT DEVICE: 1
     * Source Name:        alsa_output.usb-Generic_USB_Audio-00.HiFi__hw_Audio_1__sink.monitor
     * Description:        Monitor of USB Audio Front Headphones
     * Sample Rate:        48000 Hz
     * Channels:           2
     * Format:             SignedPCM32bitLE
     * Card index:         48
     */
    int getAudioDevicesInfo();

#ifdef __cplusplus
}
#endif
