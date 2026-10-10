/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include "ameba_timer_core.h"

void ameba_timer_add(struct ameba_timer_core *core, struct ameba_timer *timer)
{
	timer->next = core->head;
	core->head = timer;
}

struct ameba_timer *ameba_timer_find(struct ameba_timer_core *core, const void *handle)
{
	for (struct ameba_timer *timer = core->head; timer != NULL; timer = timer->next) {
		if (timer == handle) {
			return timer;
		}
	}
	return NULL;
}

bool ameba_timer_arm(struct ameba_timer_core *core, const void *handle,
		     uint32_t interval_ms, int64_t now_ms)
{
	struct ameba_timer *timer = ameba_timer_find(core, handle);

	if (timer == NULL || timer->retired) {
		return false;
	}
	timer->interval_ms = interval_ms;
	timer->generation++;
	/* Timers use millisecond deadlines. Zero schedules a one-shot for 1 ms
	 * later, even if reload was requested; actual dispatch is subject to
	 * kernel tick granularity and daemon scheduling.
	 */
	timer->deadline_ms = now_ms + (interval_ms != 0 ? (int64_t)interval_ms : 1);
	timer->active = true;
	return true;
}

bool ameba_timer_stop(struct ameba_timer_core *core, const void *handle)
{
	struct ameba_timer *timer = ameba_timer_find(core, handle);

	if (timer == NULL || timer->retired) {
		return false;
	}
	timer->active = false;
	timer->generation++;
	return true;
}

static void unlink_timer(struct ameba_timer_core *core, struct ameba_timer *timer)
{
	struct ameba_timer **link = &core->head;

	while (*link != NULL && *link != timer) {
		link = &(*link)->next;
	}
	if (*link == timer) {
		*link = timer->next;
		timer->next = NULL;
	}
}

enum ameba_timer_delete_result ameba_timer_retire(struct ameba_timer_core *core,
						  const void *handle)
{
	struct ameba_timer *timer = ameba_timer_find(core, handle);

	if (timer == NULL || timer->retired) {
		return AMEBA_TIMER_DELETE_INVALID;
	}
	timer->retired = true;
	timer->generation++;
	timer->active = false;
	if (timer->running) {
		return AMEBA_TIMER_DELETE_DEFERRED;
	}
	unlink_timer(core, timer);
	return AMEBA_TIMER_DELETE_RECLAIM;
}

struct ameba_timer *ameba_timer_dispatch(struct ameba_timer_core *core,
					 int64_t now_ms, int64_t *wait_ms)
{
	struct ameba_timer *next = NULL;

	for (struct ameba_timer *timer = core->head; timer != NULL; timer = timer->next) {
		if (!timer->active || timer->retired || timer->running) {
			continue;
		}
		if (next == NULL || timer->deadline_ms < next->deadline_ms) {
			next = timer;
		}
	}
	if (next == NULL) {
		*wait_ms = -1;
		return NULL;
	}
	if (next->deadline_ms > now_ms) {
		*wait_ms = next->deadline_ms - now_ms;
		return NULL;
	}
	next->running = true;
	next->dispatched_generation = next->generation;
	next->active = next->periodic && next->interval_ms != 0;
	/* Arm before releasing the lock. Callback or concurrent stop/change/start
	 * commands can replace this deadline without completion overwriting them.
	 */
	next->deadline_ms = now_ms + (int64_t)next->interval_ms;
	*wait_ms = 0;
	return next;
}

bool ameba_timer_complete(struct ameba_timer_core *core, struct ameba_timer *timer,
			  int64_t now_ms)
{
	timer->running = false;
	if (timer->retired) {
		unlink_timer(core, timer);
		return true;
	}
	/* Coalesce missed periods after a slow callback: schedule one full period
	 * from completion instead of a catch-up burst. An explicit callback or
	 * concurrent start/change command retains its own deadline.
	 */
	if (timer->periodic && timer->active && timer->generation == timer->dispatched_generation &&
	    timer->deadline_ms <= now_ms) {
		timer->deadline_ms = now_ms + (int64_t)timer->interval_ms;
	}
	return false;
}
