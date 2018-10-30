/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WRAPPER_H__
#define __WRAPPER_H__

#include <kernel.h>
#include <cmsis_os2.h>

#define	TRUE	1
#define FALSE	0

struct cv2_thread {
	sys_dnode_t node;
	struct k_thread z_thread;
	struct k_poll_signal poll_signal;
	struct k_poll_event  poll_event;
	u32_t            signal_results;
	char name[16];
	u32_t state;
};

extern osThreadId_t get_cmsis_thread_id(k_tid_t tid);
extern void *is_cmsis_rtos_v2_thread(void *thread_id);

#endif
