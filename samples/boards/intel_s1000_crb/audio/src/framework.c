/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(framework);

#include <zephyr.h>

#include "audio_core.h"

extern int tun_drv_packet_handler(void);
static int framework_background_thread(void);

K_THREAD_DEFINE(framework_bg_thread_id, FRAMEWORK_BG_THREAD_STACK_SIZE,
		framework_background_thread, NULL, NULL, NULL,
		FRAMEWORK_BG_THREAD_PRIORITY, 0, 0);

static int framework_background_thread(void)
{
	LOG_INF("Starting framework background thread ...");

	while (true) {
		tun_drv_packet_handler();
		audio_core_process_background_tasks();
	}

	return 0;
}
