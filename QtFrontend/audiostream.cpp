/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#if defined (AUDIO_BACKEND_PULSE)
#include "../audio/pulse_stream.h"
#elif defined (AUDIO_BACKEND_PIPEWIRE)
#include "../audio/pipewire_stream.h"
#elif defined (AUDIO_BACKEND_ALSA)
#include "../audio/alsa_stream.h"
#endif

#include "audiostream.h" 

extern AudioStreamProvider ifaceAudio;

void AudioStream::AudioStream::start()
{
    if (ifaceAudio.configure())
    {
        LOG_ERROR("Error in audio stream configure...\n");
        return;
    }
    if (ifaceAudio.start())
    {
        LOG_ERROR("Error in start audio stream...\n");
        return;
    }
}

void AudioStream::AudioStream::stopped()
{
    LOG_DEBUG("!!!!!!! stop\n");
    ifaceAudio.stop(m_stopped); //stopAudioStream(m_stopped);
    m_stopped = true;
}
