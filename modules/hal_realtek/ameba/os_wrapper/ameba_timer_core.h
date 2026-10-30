/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef AMEBA_TIMER_CORE_H
#define AMEBA_TIMER_CORE_H

#include <stdbool.h>
#include <stdint.h>

/* All operations require the caller's lock. Only the daemon dispatches and
 * completes callbacks. Opaque handles remain valid until delete is accepted;
 * reusing a deleted handle after allocation reuses its address is caller misuse.
 */
struct ameba_timer {
	struct ameba_timer *next;
	void (*callback)(void *p_context);
	uint32_t id;
	uint32_t interval_ms;
	uint32_t generation;
	uint32_t dispatched_generation;
	int64_t deadline_ms;
	bool periodic;
	bool active;
	bool running;
	bool retired;
};

struct ameba_timer_core {
	struct ameba_timer *head;
};

enum ameba_timer_delete_result {
	AMEBA_TIMER_DELETE_INVALID = -1,
	AMEBA_TIMER_DELETE_DEFERRED = 0,
	AMEBA_TIMER_DELETE_RECLAIM = 1,
};

void ameba_timer_add(struct ameba_timer_core *core, struct ameba_timer *timer);
struct ameba_timer *ameba_timer_find(struct ameba_timer_core *core, const void *handle);
bool ameba_timer_arm(struct ameba_timer_core *core, const void *handle,
		     uint32_t interval_ms, int64_t now_ms);
bool ameba_timer_stop(struct ameba_timer_core *core, const void *handle);
enum ameba_timer_delete_result ameba_timer_retire(struct ameba_timer_core *core,
						  const void *handle);
struct ameba_timer *ameba_timer_dispatch(struct ameba_timer_core *core,
					 int64_t now_ms, int64_t *wait_ms);
bool ameba_timer_complete(struct ameba_timer_core *core, struct ameba_timer *timer,
			  int64_t now_ms);

#endif
