/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

// Output for example:
// ===========================================
// [INFO] 00000002   Card 1:
// [INFO] 00000001   ID: NVidia
// [INFO] 00000002   Name: HDA NVidia
// [INFO] 00000002   Long Name: HDA NVidia at 0x81080000 irq 17
// [INFO] 00000002   Driver: HDA-Intel

// [INFO] 00000019    PLAYBACK DEVICES:
// [INFO] 00000044       Playback Device 3: LQ27F180
// [INFO] 00000006        Subdevices: 1 available, 1 available
// [ERROR] 00000071 list_audio_cards() Cannot open for capture: No such file or directory
// [INFO] 00000004    PLAYBACK DEVICES:
// [INFO] 00000010       Playback Device 7: LQ27F180L-Q
// [INFO] 00000002        Subdevices: 1 available, 1 available
// [ERROR] 00000032 list_audio_cards() Cannot open for capture: No such file or directory
// [INFO] 00000003    PLAYBACK DEVICES:
// [INFO] 00000009       Playback Device 8: HDMI 2
// [INFO] 00000003        Subdevices: 1 available, 1 available
// [ERROR] 00000027 list_audio_cards() Cannot open for capture: No such file or directory
// [INFO] 00000004    PLAYBACK DEVICES:
// [INFO] 00000007       Playback Device 9: HDMI 3
// [INFO] 00000003        Subdevices: 1 available, 1 available
// [ERROR] 00000026 list_audio_cards() Cannot open for capture: No such file or directory
// [INFO] 00000019 
// ===========================================
// [INFO] 00000003   Card 2:
// [INFO] 00000002   ID: C525
// [INFO] 00000002   Name: HD Webcam C525
// [INFO] 00000003   Long Name: HD Webcam C525 at usb-0000:00:14.0-3.2, high speed
// [INFO] 00000002   Driver: USB-Audio

// [INFO] 00000002    PLAYBACK DEVICES:
// [INFO] 00000002    CAPTURE DEVICES:
// [INFO] 00000003        Capture Device 0: USB Audio
// [INFO] 00000002        Subdevices: 1 available, 1 available
// [INFO] 00057784       Sample rate range: 16000 - 48000 Hz
// [INFO] 00000011       Channels range: 1 - 1
// [INFO] 00000002       Supported formats:
// [INFO] 00000004        - S16_LE
// [INFO] 00000051 
// ===========================================

#ifdef __cplusplus
extern "C"
{
#endif

// get audio device info
// use: aplay, arecord
int getAudioDevicesInfo();

#ifdef __cplusplus
}
#endif
