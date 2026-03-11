/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSIF_ZEPHYR_H_
#define _OSIF_ZEPHYR_H_
#include <zephyr/kernel.h>

#define OSIF_TASK_MIN_PRIORITY 0
#define OSIF_TASK_MAX_PRIORITY 6

/* Timer Status Enumeration */
typedef enum {
	OSIF_TIMER_NOT_ACTIVE = 0,
	OSIF_TIMER_ACTIVE = 1
} osif_timer_status_t;

/* Timer Type Enumeration */
typedef enum {
	OSIF_TIMER_ONCE = 0,    /* One-shot timer */
	OSIF_TIMER_PERIODIC = 1 /* Repeating timer */
} osif_timer_type_t;

/* Scheduler State Enumeration */
typedef enum {
	OSIF_SCHEDULER_SUSPENDED = 0,
	OSIF_SCHEDULER_NOT_STARTED = 1,
	OSIF_SCHEDULER_RUNNING = 2
} osif_sched_state_t;

struct osif_timer {
	struct k_timer ztimer;
	uint32_t interval_ms;
	uint32_t timer_id;
	uint8_t type;
	uint8_t status;
	const char *name;
	bool allocated;
};

struct task_signal {
	struct k_poll_signal *poll_signal;
	struct k_poll_event *poll_event;
	int32_t signal_results;
};

struct osif_task {
	struct k_thread zthread;
	struct k_poll_signal poll_signal;
	struct k_poll_event poll_event;
	int32_t signal_results;
	void *stack_start;
	const char *name;
};

struct osif_task_delete_context {
	struct k_work work;
	struct osif_task *task_to_free;
	void *stack_to_free;
};

void os_zephyr_patch_init(void);

#endif
