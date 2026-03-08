/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <unistd.h>

#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <alsa/error.h>

#include "alsa_devices.h"
#include "../common/logging.h"

static snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

static int list_audio_cards()
{
    int card_num = -1;
    snd_ctl_card_info_t *card_info;
    snd_pcm_t *pcm_handle;
    int devices_count = 0;

    // allocate on stack !!
    snd_ctl_card_info_alloca(&card_info);

    // List all cards
    while (snd_card_next(&card_num) >= 0 && card_num >= 0)
    {
        snd_ctl_t *ctl_handle;
        char name[32];
        int err;

        snprintf(name, sizeof(name), "hw:%d", card_num);

        // Open card controller
        if ((err = snd_ctl_open(&ctl_handle, name, 0)) < 0)
        {
            LOG_ERROR("Error opening card %d: %s\n", card_num, snd_strerror(err));
            continue;
        }

        // Get info about card
        if ((err = snd_ctl_card_info(ctl_handle, card_info)) < 0)
        {
            LOG_ERROR("Error getting card info %d: %s\n", card_num, snd_strerror(err));
            snd_ctl_close(ctl_handle);
            continue;
        }

        // Main card info:
        LOG_INFO("\n===========================================\n");
        LOG_INFO("  Card %d:\n", card_num);
        LOG_INFO("  ID: %s\n", snd_ctl_card_info_get_id(card_info));
        LOG_INFO("  Name: %s\n", snd_ctl_card_info_get_name(card_info));
        LOG_INFO("  Long Name: %s\n", snd_ctl_card_info_get_longname(card_info));
        LOG_INFO("  Driver: %s\n", snd_ctl_card_info_get_driver(card_info));
        LOG_SYMBOL('\n');

        // Get all PCM devices
        int device_num = -1;
        while (snd_ctl_pcm_next_device(ctl_handle, &device_num) >= 0 && device_num >= 0)
        {
            snd_pcm_info_t *pcm_info;
            // allocate on stack !!
            snd_pcm_info_alloca(&pcm_info);

            snd_pcm_info_set_device(pcm_info, device_num);

            LOG_INFO("   PLAYBACK DEVICES:\n");
            snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);
            if (snd_ctl_pcm_info(ctl_handle, pcm_info) >= 0)
            {
                LOG_INFO("      Playback Device %d: %s\n", device_num,
                         snd_pcm_info_get_name(pcm_info));

                // subdevices
                unsigned int subdevices_count = snd_pcm_info_get_subdevices_count(pcm_info);
                LOG_INFO("       Subdevices: %d available, %d available\n",
                         subdevices_count,
                         snd_pcm_info_get_subdevices_avail(pcm_info));

                // open as playback device
                err = snd_pcm_open(&pcm_handle, name, SND_PCM_STREAM_PLAYBACK, 0);
                if (err < 0)
                {
                    LOG_ERROR("Cannot open for capture: %s\n", snd_strerror(err));
                    continue;
                }

                snd_pcm_hw_params_t *hw_params;
                // allocate on stack !!
                snd_pcm_hw_params_alloca(&hw_params);
                snd_pcm_hw_params_any(pcm_handle, hw_params);

                unsigned int min_rate, max_rate;
                unsigned int min_channels, max_channels;
                snd_pcm_uframes_t min_period_size, max_period_size;
                snd_pcm_uframes_t min_buffer_size, max_buffer_size;
                int dir;

                // supported sample rate
                snd_pcm_hw_params_get_rate_min(hw_params, &min_rate, &dir);
                snd_pcm_hw_params_get_rate_max(hw_params, &max_rate, &dir);
                LOG_INFO("      Sample rate range: %u - %u Hz\n", min_rate, max_rate);

                // supported channels
                snd_pcm_hw_params_get_channels_min(hw_params, &min_channels);
                snd_pcm_hw_params_get_channels_max(hw_params, &max_channels);
                LOG_INFO("      Channels range: %u - %u\n", min_channels, max_channels);

                // supported sample formats
                LOG_INFO("      Supported formats:\n");
                for (int fmt = 0; fmt <= SND_PCM_FORMAT_LAST; fmt++)
                {
                    if (snd_pcm_hw_params_test_format(pcm_handle, hw_params, fmt) == 0)
                    {
                        LOG_INFO("       - %s\n", snd_pcm_format_name(fmt));
                    }
                }
                snd_pcm_close(pcm_handle);
            }

            LOG_INFO("   CAPTURE DEVICES:\n");
            snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_CAPTURE);
            if (snd_ctl_pcm_info(ctl_handle, pcm_info) >= 0)
            {
                LOG_INFO("       Capture Device %d: %s\n", device_num,
                         snd_pcm_info_get_name(pcm_info));

                unsigned int subdevices_count = snd_pcm_info_get_subdevices_count(pcm_info);
                LOG_INFO("       Subdevices: %d available, %d available\n",
                         subdevices_count,
                         snd_pcm_info_get_subdevices_avail(pcm_info));

                // open as capture device
                err = snd_pcm_open(&pcm_handle, name, SND_PCM_STREAM_CAPTURE, 0);
                if (err < 0)
                {
                    LOG_ERROR("Cannot open for capture: %s\n", snd_strerror(err));
                    continue;
                }

                snd_pcm_hw_params_t *hw_params;
                // allocate on stack !!
                snd_pcm_hw_params_alloca(&hw_params);
                snd_pcm_hw_params_any(pcm_handle, hw_params);

                unsigned int min_rate, max_rate;
                unsigned int min_channels, max_channels;
                snd_pcm_uframes_t min_period_size, max_period_size;
                snd_pcm_uframes_t min_buffer_size, max_buffer_size;
                int dir;

                // supported sample rate
                snd_pcm_hw_params_get_rate_min(hw_params, &min_rate, &dir);
                snd_pcm_hw_params_get_rate_max(hw_params, &max_rate, &dir);
                LOG_INFO("      Sample rate range: %u - %u Hz\n", min_rate, max_rate);

                // supported channels
                snd_pcm_hw_params_get_channels_min(hw_params, &min_channels);
                snd_pcm_hw_params_get_channels_max(hw_params, &max_channels);
                LOG_INFO("      Channels range: %u - %u\n", min_channels, max_channels);

                // supported sample formats
                LOG_INFO("      Supported formats:\n");
                for (int fmt = 0; fmt <= SND_PCM_FORMAT_LAST; fmt++)
                {
                    if (snd_pcm_hw_params_test_format(pcm_handle, hw_params, fmt) == 0)
                    {
                        LOG_INFO("       - %s\n", snd_pcm_format_name(fmt));
                    }
                }
                snd_pcm_close(pcm_handle);
            }
            devices_count++;
        }

        // ignored MIDI devices...
    }
    if (devices_count == 0)
    {
        LOG_INFO("All audio devices cannot be processed by alsa-lib\n");
        return 1;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////

static void pcm_list()
{
    void **hints, **n;
    char *name, *descr, *descr1, *io;
    const char *filter;

    if (snd_device_name_hint(-1, "pcm", &hints) < 0)
        return;
    n = hints;
    filter = stream == SND_PCM_STREAM_CAPTURE ? "Input" : "Output";
    while (*n != NULL)
    {
        name = snd_device_name_get_hint(*n, "NAME");
        descr = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if (io != NULL && strcmp(io, filter) != 0)
            goto list_end;
        LOG_INFO("%s\n", name);
        if ((descr1 = descr) != NULL)
        {
            LOG_INFO("    \n");
            while (*descr1)
            {
                if (*descr1 == '\n')
                    LOG_INFO("\n    ");
                else
                    LOG_SYMBOL(*descr1);
                descr1++;
            }
            LOG_SYMBOL('\n');
        }
    list_end:
        free(name);
        free(descr);
        free(io);
        n++;
    }
    snd_device_name_free_hint(hints);
}

/////////////////////////////////////////////////////////////////////////////////

static void device_list(void)
{
    snd_ctl_t *handle;
    int card, err, dev, idx;
    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcminfo;
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);

    card = -1;
    if (snd_card_next(&card) < 0 || card < 0)
    {
        LOG_ERROR("no soundcards found...");
        return;
    }
    LOG_INFO("**** List of %s Hardware Devices ****\n", snd_pcm_stream_name(stream));
    while (card >= 0)
    {
        char name[32];
        sprintf(name, "hw:%d", card);
        if ((err = snd_ctl_open(&handle, name, 0)) < 0)
        {
            LOG_ERROR("control open (%i): %s", card, snd_strerror(err));
            goto next_card;
        }
        if ((err = snd_ctl_card_info(handle, info)) < 0)
        {
            LOG_ERROR("control hardware info (%i): %s", card, snd_strerror(err));
            snd_ctl_close(handle);
            goto next_card;
        }
        dev = -1;
        while (1)
        {
            unsigned int count;
            if (snd_ctl_pcm_next_device(handle, &dev) < 0)
                LOG_ERROR("snd_ctl_pcm_next_device");
            if (dev < 0)
                break;
            snd_pcm_info_set_device(pcminfo, dev);
            snd_pcm_info_set_subdevice(pcminfo, 0);
            snd_pcm_info_set_stream(pcminfo, stream);
            if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0)
            {
                if (err != -ENOENT)
                    LOG_ERROR("control digital audio info (%i): %s", card, snd_strerror(err));
                continue;
            }
            LOG_INFO("card %i: %s [%s], device %i: %s [%s]\n",
                     card, snd_ctl_card_info_get_id(info), snd_ctl_card_info_get_name(info),
                     dev, snd_pcm_info_get_id(pcminfo), snd_pcm_info_get_name(pcminfo));
            count = snd_pcm_info_get_subdevices_count(pcminfo);
            LOG_INFO("  Subdevices: %i/%i\n", snd_pcm_info_get_subdevices_avail(pcminfo), count);
            for (idx = 0; idx < (int)count; idx++)
            {
                snd_pcm_info_set_subdevice(pcminfo, idx);
                if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0)
                {
                    LOG_ERROR("control digital audio playback info (%i): %s", card, snd_strerror(err));
                }
                else
                {
                    LOG_INFO("  Subdevice #%i: %s\n", idx, snd_pcm_info_get_subdevice_name(pcminfo));
                }
            }
        }
        snd_ctl_close(handle);
    next_card:
        if (snd_card_next(&card) < 0)
        {
            LOG_ERROR("snd_card_next");
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////

int getAudioDevicesInfo()
{
    // Uncomment if you are interested
    // from aplay code:
    // pcm_list();
    // device_list();
    return list_audio_cards();
}
