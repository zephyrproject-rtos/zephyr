/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(audio_proc);

#include <zephyr.h>

#include "audio_core.h"

K_SEM_DEFINE(audio_proc_small_block_sem, 0, 1);
K_SEM_DEFINE(audio_proc_large_block_sem, 0, 1);

static void audio_proc_small_block_thread(void);
static void audio_proc_large_block_thread(void);

K_THREAD_DEFINE(audio_proc_sm_blk_tid, AUDIO_SM_BLOCK_PROC_THREAD_STACK_SIZE,
		audio_proc_small_block_thread, NULL, NULL, NULL,
		AUDIO_SM_BLOCK_PROC_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(audio_proc_lg_blk_tid, AUDIO_LG_BLOCK_PROC_THREAD_STACK_SIZE,
		audio_proc_large_block_thread, NULL, NULL, NULL,
		AUDIO_LG_BLOCK_PROC_THREAD_PRIORITY, 0, 0);

static void audio_proc_small_block_thread(void)
{
	LOG_INF("Starting small block processing thread ...");

	while (true) {
		k_sem_take(&audio_proc_small_block_sem, K_FOREVER);
		audio_core_process_small_blocks();
	}
}

static void audio_proc_large_block_thread(void)
{
	LOG_INF("Starting large block processing thread ...");

	while (true) {
		k_sem_take(&audio_proc_large_block_sem, K_FOREVER);
		audio_core_process_large_blocks();
	}
}
