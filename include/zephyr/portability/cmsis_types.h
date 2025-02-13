/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CMSIS_TYPES_H_
#define ZEPHYR_INCLUDE_CMSIS_TYPES_H_

#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_os2.h>

struct cv2_thread {
	sys_dnode_t node;
	struct k_thread z_thread;
	struct k_poll_signal poll_signal;
	struct k_poll_event poll_event;
	uint32_t signal_results;
	char name[16];
	uint32_t attr_bits;
	struct k_sem join_guard;
	char has_joined;
};

struct cv2_timer {
	struct k_timer z_timer;
	osTimerType_t type;
	uint32_t status;
	char name[16];
	void (*callback_function)(void *argument);
	void *arg;
};

struct cv2_mutex {
	struct k_mutex z_mutex;
	char name[16];
	uint32_t state;
};

struct cv2_sem {
	struct k_sem z_semaphore;
	char name[16];
};

struct cv2_mslab {
	struct k_mem_slab z_mslab;
	void *pool;
	char is_dynamic_allocation;
	char name[16];
};

struct cv2_msgq {
	struct k_msgq z_msgq;
	void *pool;
	char is_dynamic_allocation;
	char name[16];
};

struct cv2_event_flags {
	struct k_poll_signal poll_signal;
	struct k_poll_event poll_event;
	uint32_t signal_results;
	char name[16];
};

#endif
