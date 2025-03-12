/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <platform/argus_timer.h>
#include <api/argus_status.h>

#include <zephyr/kernel.h>

#include <../drivers/sensor/broadcom/afbr_s50/platform.h>

struct platform_argus_timer {
	struct k_timer *timer;
	uint32_t dt_microseconds;
	struct {
		timer_cb_t handler;
		void *param;
		struct k_mutex *lock;
	} cb;
};

static void argus_timer_handler(struct k_timer *timer);

K_TIMER_DEFINE(argus_timer, argus_timer_handler, NULL);
K_MUTEX_DEFINE(argus_handler_mutex);

static struct platform_argus_timer platform_timer = {
	.timer = &argus_timer,
	.cb.lock = &argus_handler_mutex,
};

static void argus_timer_handler(struct k_timer *timer)
{
	struct platform_argus_timer *p_timer = &platform_timer;

	(void)k_mutex_lock(p_timer->cb.lock, K_FOREVER);

	if (p_timer->cb.handler) {
		p_timer->cb.handler(p_timer->cb.param);
	}

	(void)k_mutex_unlock(p_timer->cb.lock);
}

void Timer_GetCounterValue(uint32_t *hct, uint32_t *lct)
{
	int64_t uptime_ticks = k_uptime_ticks();
	uint64_t uptime_us = k_ticks_to_us_floor64((uint64_t)uptime_ticks);

	/* hct in seconds, lct in microseconds */
	*hct = uptime_us / USEC_PER_SEC;
	*lct = uptime_us % USEC_PER_SEC;
}

status_t Timer_SetCallback(timer_cb_t f)
{
	__ASSERT_NO_MSG(false);
	/** Since the handler may be set to NULL, we're guarding this operation
	 * with a mutex.
	 */
	(void)k_mutex_lock(platform_timer.cb.lock, K_FOREVER);

	platform_timer.cb.handler = f;

	(void)k_mutex_unlock(platform_timer.cb.lock);

	return STATUS_OK;
}

/** The API description talks about multiple timers going on at once,
 * distinguished by the parameter passed on, however there does not appear to
 * be a mention on what's the requirement, neither do the other platform
 * implementation in the upstream SDK follow a multi-timer pattern.
 * For starters, this implementation just covers single-timer use case.
 */
status_t Timer_SetInterval(uint32_t dt_microseconds, void *param)
{
	__ASSERT_NO_MSG(false);

	if (dt_microseconds == 0) {
		k_timer_stop(platform_timer.timer);
		platform_timer.dt_microseconds = 0;
		platform_timer.cb.param = NULL;
	} else if (dt_microseconds != platform_timer.dt_microseconds) {
		platform_timer.dt_microseconds = dt_microseconds;
		platform_timer.cb.param = param;
		k_timer_start(platform_timer.timer,
			      K_USEC(platform_timer.dt_microseconds),
			      K_USEC(platform_timer.dt_microseconds));
	}

	return STATUS_OK;
}

static int timer_init(struct afbr_s50_platform_data *data)
{
	ARG_UNUSED(data);

	return 0;
}

static struct afbr_s50_platform_init_node timer_init_node = {
	.init_fn = timer_init,
};

static int timer_platform_init_hooks(void)
{
	afbr_s50_platform_init_hooks_add(&timer_init_node);

	return 0;
}

SYS_INIT(timer_platform_init_hooks, POST_KERNEL, 0);
