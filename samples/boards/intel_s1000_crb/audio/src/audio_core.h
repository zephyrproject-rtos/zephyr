/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AUDIO_CORE__H
#define AUDIO_CORE__H

/* scheduling priority used by each thread */
#define TUN_DRV_IO_THREAD_PRIORITY		-3
#define AUDIO_DRIVER_THREAD_PRIORITY		2
#define AUDIO_SM_BLOCK_PROC_THREAD_PRIORITY	3
#define AUDIO_LG_BLOCK_PROC_THREAD_PRIORITY	4
#define FRAMEWORK_BG_THREAD_PRIORITY		99

/* size of stack area used by each thread */
#define AUDIO_DRIVER_THREAD_STACKSIZE		2048
#define TUN_DRV_IO_THREAD_STACK_SIZE		1024
#define FRAMEWORK_BG_THREAD_STACK_SIZE		1024
#define AUDIO_SM_BLOCK_PROC_THREAD_STACK_SIZE	1024
#define AUDIO_LG_BLOCK_PROC_THREAD_STACK_SIZE	1024

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

/* tuning interface prototypes */
int audio_core_tuning_interface_init(u32_t *command_buffer,
		u32_t size_in_words);
int audio_core_notify_tuning_cmd(void);
bool audio_core_is_tuning_reply_ready(void);

/* audio driver prototypes */
int audio_driver_start(void);
int audio_driver_stop(void);

/* tick for processing background tasks, called from the background thread */
int audio_core_process_background_tasks(void);

/* tick for processing small audio blocks */
int audio_core_process_small_blocks(void);
int audio_core_process_large_blocks(void);

#endif /* AUDIO_CORE__H */
