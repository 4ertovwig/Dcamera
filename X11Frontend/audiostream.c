/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stddef.h>

#include "audiostream.h" 

#if defined (AUDIO_BACKEND_PULSE)
#include "../audio/pulse_stream.h"
#elif defined (AUDIO_BACKEND_PIPEWIRE)
#include "../audio/pipewire_stream.h"
#elif defined (AUDIO_BACKEND_ALSA)
#include "../audio/alsa_stream.h"
#endif

#include "../common/logging.h"
#include "../audio/common.h"

// audio interface
extern AudioStreamProvider ifaceAudio;

void* X11AudioStreamStart(void* arg)
{
    (void)arg;
    if (ifaceAudio.configure())
    {
        LOG_ERROR("Error in audio stream configure...\n");
        return NULL;
    }
    if (ifaceAudio.start())
    {
        LOG_ERROR("Error in start audio stream...\n");
        return NULL;
    }
}

void X11AudioStreamStop(bool stop)
{
    LOG_DEBUG("!!!!!!! stop audio\n");
    ifaceAudio.stop(!stop);
}
