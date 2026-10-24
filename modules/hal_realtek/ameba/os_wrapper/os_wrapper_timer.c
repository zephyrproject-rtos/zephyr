/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * Vendor software timer callbacks may take mutexes, including Wi-Fi
 * authentication callbacks. Execute them on a dedicated preemptible thread,
 * with asynchronous stop/delete commands, rather than in k_timer expiry
 * interrupt context or on the shared system workqueue.
 */
#include <stdint.h>
#include <string.h>
#include <os_wrapper_timer.h>
#include <rtk_status.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "ameba_timer_core.h"

LOG_MODULE_REGISTER(os_if_timer);

BUILD_ASSERT(CONFIG_REALTEK_AMEBA_TIMER_PRIORITY < CONFIG_NUM_PREEMPT_PRIORITIES,
	     "Ameba timer priority must name a configured preemptible priority");

static struct ameba_timer_core timer_core;
static struct k_spinlock timer_lock;
K_SEM_DEFINE(timer_wake, 0, 1);

static void timer_daemon(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	for (;;) {
		int64_t wait_ms;
		k_spinlock_key_t key = k_spin_lock(&timer_lock);
		struct ameba_timer *timer =
			ameba_timer_dispatch(&timer_core, k_uptime_get(), &wait_ms);

		k_spin_unlock(&timer_lock, key);
		if (timer == NULL) {
			/* A command arriving between unlock and take leaves a semaphore
			 * token, so an earlier deadline cannot be missed.
			 */
			(void)k_sem_take(&timer_wake, wait_ms < 0 ? K_FOREVER : K_MSEC(wait_ms));
			continue;
		}
		/* running retains the object through stop/delete/self-delete. No
		 * lock is held across vendor code, and no shared workqueue is used.
		 */
		timer->callback(timer);
		key = k_spin_lock(&timer_lock);
		bool reclaim = ameba_timer_complete(&timer_core, timer, k_uptime_get());

		k_spin_unlock(&timer_lock, key);
		if (reclaim) {
			k_free(timer);
		}
	}
}

K_THREAD_DEFINE(ameba_timer_daemon, CONFIG_REALTEK_AMEBA_TIMER_STACK_SIZE, timer_daemon,
		NULL, NULL, NULL, K_PRIO_PREEMPT(CONFIG_REALTEK_AMEBA_TIMER_PRIORITY), 0, 0);

int rtos_timer_create(rtos_timer_t *handle, const char *name, uint32_t id,
		      uint32_t interval_ms, uint8_t reload, void (*callback)(void *p_context))
{
	struct ameba_timer *timer;

	ARG_UNUSED(name);
	if (handle == NULL || callback == NULL) {
		return RTK_FAIL;
	}
#if K_HEAP_MEM_POOL_SIZE > 0
	timer = k_malloc(sizeof(*timer));
	if (timer == NULL) {
		LOG_ERR("Cannot allocate a vendor software timer");
		return RTK_FAIL;
	}
#else
	LOG_ERR("Vendor software timers require a heap memory pool");
	return RTK_FAIL;
#endif

	memset(timer, 0, sizeof(*timer));
	timer->id = id;
	timer->interval_ms = interval_ms;
	timer->periodic = reload != 0;
	timer->callback = callback;
	k_spinlock_key_t key = k_spin_lock(&timer_lock);

	ameba_timer_add(&timer_core, timer);
	k_spin_unlock(&timer_lock, key);
	*handle = timer;
	return RTK_SUCCESS;
}

int rtos_timer_create_static(rtos_timer_t *handle, const char *name, uint32_t id,
			     uint32_t interval_ms, uint8_t reload,
			     void (*callback)(void *p_context))
{
	return rtos_timer_create(handle, name, id, interval_ms, reload, callback);
}

int rtos_timer_delete(rtos_timer_t handle, uint32_t wait_ms)
{
	/* wait_ms is queue-enqueue wait, not permission to wait for a callback
	 * holding/awaiting the caller's Wi-Fi mutex. Commands apply immediately.
	 */
	ARG_UNUSED(wait_ms);
	k_spinlock_key_t key = k_spin_lock(&timer_lock);
	enum ameba_timer_delete_result result = ameba_timer_retire(&timer_core, handle);

	k_spin_unlock(&timer_lock, key);
	if (result == AMEBA_TIMER_DELETE_RECLAIM) {
		k_free(handle);
	}
	k_sem_give(&timer_wake);
	return result == AMEBA_TIMER_DELETE_INVALID ? RTK_FAIL : RTK_SUCCESS;
}

int rtos_timer_delete_static(rtos_timer_t handle, uint32_t wait_ms)
{
	return rtos_timer_delete(handle, wait_ms);
}

int rtos_timer_start(rtos_timer_t handle, uint32_t wait_ms)
{
	ARG_UNUSED(wait_ms);
	k_spinlock_key_t key = k_spin_lock(&timer_lock);
	struct ameba_timer *timer = ameba_timer_find(&timer_core, handle);
	/* A zero period schedules a one-shot with a 1 ms deadline. */
	bool ok = timer != NULL &&
		ameba_timer_arm(&timer_core, handle, timer->interval_ms, k_uptime_get());

	k_spin_unlock(&timer_lock, key);
	k_sem_give(&timer_wake);
	return ok ? RTK_SUCCESS : RTK_FAIL;
}

int rtos_timer_stop(rtos_timer_t handle, uint32_t wait_ms)
{
	ARG_UNUSED(wait_ms);
	k_spinlock_key_t key = k_spin_lock(&timer_lock);
	bool ok = ameba_timer_stop(&timer_core, handle);

	k_spin_unlock(&timer_lock, key);
	k_sem_give(&timer_wake);
	return ok ? RTK_SUCCESS : RTK_FAIL;
}

int rtos_timer_change_period(rtos_timer_t handle, uint32_t interval_ms, uint32_t wait_ms)
{
	ARG_UNUSED(wait_ms);
	k_spinlock_key_t key = k_spin_lock(&timer_lock);
	bool ok = ameba_timer_arm(&timer_core, handle, interval_ms, k_uptime_get());

	k_spin_unlock(&timer_lock, key);
	k_sem_give(&timer_wake);
	return ok ? RTK_SUCCESS : RTK_FAIL;
}

uint32_t rtos_timer_is_timer_active(rtos_timer_t handle)
{
	k_spinlock_key_t key = k_spin_lock(&timer_lock);
	struct ameba_timer *timer = ameba_timer_find(&timer_core, handle);
	bool active = timer != NULL && timer->active && !timer->retired;

	k_spin_unlock(&timer_lock, key);
	return active ? 1U : 0U;
}

uint32_t rtos_timer_get_id(rtos_timer_t handle)
{
	k_spinlock_key_t key = k_spin_lock(&timer_lock);
	struct ameba_timer *timer = ameba_timer_find(&timer_core, handle);
	uint32_t id = timer != NULL ? timer->id : 0;

	k_spin_unlock(&timer_lock, key);
	return id;
}

__weak void init_timer_wrapper(void)
{
	LOG_ERR("Not Support");
}
