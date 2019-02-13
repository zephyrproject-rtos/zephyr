/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_ERR
#include <logging/log.h>
LOG_MODULE_REGISTER(audio_core);

#include <zephyr.h>
#include "audio_core.h"

/*
 * stereo mic in
 * stereo host in
 * stereo speaker out
 * stereo host out
 */
#define AUDIO_ACTIVE_CHANNELS	2

/* core audio processing buffers with interleaved samples */
static s32_t mic_input[AUDIO_ACTIVE_CHANNELS][AUDIO_SAMPLES_PER_FRAME];
static s32_t host_input[AUDIO_ACTIVE_CHANNELS][AUDIO_SAMPLES_PER_FRAME];

int audio_core_initialize(void)
{
	return 0;
}

int audio_core_process_mic_source(s32_t *buffer, int channels)
{
	int sample;
	int channel;
	s32_t *read;

	read = buffer;

	/* copy audio samples from mic source buffer to mic_input */
	for (sample = 0; sample < AUDIO_SAMPLES_PER_FRAME; sample++) {
		for (channel = 0; channel < AUDIO_ACTIVE_CHANNELS; channel++) {
			mic_input[channel][sample] = read[channel];
		}
		read += channels;
	}

	return 0;
}

int audio_core_process_host_source(s32_t *buffer, int channels)
{
	int sample;
	int channel;
	s32_t *read;

	read = buffer;

	/* copy audio samples from host source buffer to host_input */
	for (sample = 0; sample < AUDIO_SAMPLES_PER_FRAME; sample++) {
		for (channel = 0; channel < AUDIO_ACTIVE_CHANNELS; channel++) {
			host_input[channel][sample] = read[channel];
		}
		read += channels;
	}

	return 0;
}

int audio_core_process_speaker_sink(s32_t *buffer, int channels)
{
	int sample;
	int channel;
	s32_t *write;

	write = buffer;

	/* copy audio samples from host_input to speaker sink buffer */
	for (sample = 0; sample < AUDIO_SAMPLES_PER_FRAME; sample++) {
		for (channel = 0; channel < AUDIO_ACTIVE_CHANNELS; channel++) {
			write[channel] = host_input[channel][sample];
		}
		write += channels;
	}

	return 0;
}

int audio_core_process_host_sink(s32_t *buffer, int channels)
{
	int sample;
	int channel;
	s32_t *write;

	write = buffer;

	/* copy audio samples from mic_input to host sink buffer */
	for (sample = 0; sample < AUDIO_SAMPLES_PER_FRAME; sample++) {
		for (channel = 0; channel < AUDIO_ACTIVE_CHANNELS; channel++) {
			write[channel] = mic_input[channel][sample];
		}
		write += channels;
	}

	return 0;
}

int audio_core_notify_frame_tick(void)
{
	return 0;
}
