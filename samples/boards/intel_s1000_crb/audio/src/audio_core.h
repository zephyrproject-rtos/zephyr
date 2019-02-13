/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AUDIO_CORE__H
#define AUDIO_CORE__H

/* scheduling priority used by each thread */
#define AUDIO_DRIVER_THREAD_PRIORITY		2

/* size of stack area used by each thread */
#define AUDIO_DRIVER_THREAD_STACKSIZE		2048

/* input audio channels */
#define AUDIO_MIC_INPUT_CHANNEL_COUNT		8
#define AUDIO_HOST_INPUT_CHANNEL_COUNT		2
#define	AUDIO_INPUT_CHANNEL_COUNT		\
	(AUDIO_MIC_INPUT_CHANNEL_COUNT +	\
	 AUDIO_HOST_OUTPUT_CHANNEL_COUNT)

/* output audio channels */
#define AUDIO_SPEAKER_OUTPUT_CHANNEL_COUNT	2
#define AUDIO_HOST_OUTPUT_CHANNEL_COUNT		2
#define AUDIO_OUTPUT_CHANNEL_COUNT		\
	(AUDIO_SPEAKER_OUTPUT_CHANNEL_COUNT +	\
	 AUDIO_HOST_OUTPUT_CHANNEL_COUNT)

/* samples per frame */
#define AUDIO_SAMPLES_PER_FRAME			192

int audio_core_initialize(void);
int audio_core_process_mic_source(s32_t *buffer, int channels);
int audio_core_process_host_source(s32_t *buffer, int channels);
int audio_core_process_speaker_sink(s32_t *buffer, int channels);
int audio_core_process_host_sink(s32_t *buffer, int channels);
int audio_core_notify_frame_tick(void);

#endif /* AUDIO_CORE__H */
