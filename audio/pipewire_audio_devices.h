/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include "../common/logging.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // For example: wpctl status:

    // PipeWire 'pipewire-0' [1.0.5, uvenat@tree-MS-7D32, cookie:831172255]
    //  └─ Clients:
    //         32. pipewire                            [1.0.5, uvenat@tree-MS-7D32, pid:3590]
    //         34. WirePlumber                         [1.0.5, uvenat@tree-MS-7D32, pid:3588]
    //         35. WirePlumber [export]                [1.0.5, uvenat@tree-MS-7D32, pid:3588]
    //         76. GNOME Volume Control Media Keys     [1.0.5, uvenat@tree-MS-7D32, pid:4161]
    //         77. gnome-shell                         [1.0.5, uvenat@tree-MS-7D32, pid:3922]
    //         78. GNOME Shell Volume Control          [1.0.5, uvenat@tree-MS-7D32, pid:3922]
    //         79. xdg-desktop-portal                  [1.0.5, uvenat@tree-MS-7D32, pid:4582]
    //         80. Firefox                             [1.0.5, uvenat@tree-MS-7D32, pid:34538]
    //         82. Chromium input                      [1.0.5, uvenat@tree-MS-7D32, pid:53265]
    //        102. Telegram Desktop                    [1.0.5, uvenat@tree-MS-7D32, pid:862525]
    //        110. wpctl                               [1.0.5, uvenat@tree-MS-7D32, pid:898201]

    // Audio
    //  ├─ Devices:
    //  │      48. HDA NVidia                          [alsa]
    //  │      49. USB Audio                           [alsa]
    //  │      50. HD Webcam C525                      [alsa]
    //  │
    //  ├─ Sinks:
    //  │      53. HDA NVidia Digital Stereo (HDMI)    [vol: 0.40]
    //  │      54. USB Audio S/PDIF Output             [vol: 0.40]
    //  │  *   55. USB Audio Front Headphones          [vol: 1.35]
    //  │      56. USB Audio Speakers                  [vol: 0.93]
    //  │
    //  ├─ Sink endpoints:
    //  │
    //  ├─ Sources:
    //  │  *   33. HD Webcam C525 Mono                 [vol: 1.00]
    //  │      57. USB Audio Microphone                [vol: 1.00]
    //  │      58. USB Audio Line Input                [vol: 1.00]
    //  │
    //  ├─ Source endpoints:
    //  │
    //  └─ Streams:
    //         84. Firefox
    //              87. output_FL       > USB Audio #1:playback_FL     [init]
    //              96. output_FR       > USB Audio #1:playback_FR     [init]
    //         90. Firefox
    //              86. output_FL       > USB Audio #1:playback_FL     [active]
    //              91. output_FR       > USB Audio #1:playback_FR     [active]
    //         92. Firefox
    //              81. output_FR       > USB Audio #1:playback_FR     [init]
    //              97. output_FL       > USB Audio #1:playback_FL     [init]
    //         95. Firefox
    //              99. output_FL       > USB Audio #1:playback_FL     [paused]
    //             101. output_FR       > USB Audio #1:playback_FR     [paused]
    //        106. Firefox
    //             103. output_FL       > USB Audio #1:playback_FL     [paused]
    //             105. output_FR       > USB Audio #1:playback_FR     [paused]
    //        113. Firefox
    //             107. output_FL       > USB Audio #1:playback_FL     [paused]
    //             116. output_FR       > USB Audio #1:playback_FR     [paused]
    //        118. Firefox
    //             119. output_FL       > USB Audio #1:playback_FL     [active]
    //             120. output_FR       > USB Audio #1:playback_FR     [active]

    // Video
    //  ├─ Devices:
    //  │      43. HD Webcam C525                      [v4l2]
    //  │      44. HD Webcam C525                      [v4l2]
    //  │
    //  ├─ Sinks:
    //  │
    //  ├─ Sink endpoints:
    //  │
    //  ├─ Sources:
    //  │  *   51. HD Webcam C525 (V4L2)
    //  │
    //  ├─ Source endpoints:
    //  │
    //  └─ Streams:

    int getAudioDevicesInfo();

#ifdef __cplusplus
}
#endif
