/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <stdbool.h>
#include <stdint.h>

LOG_MODULE_REGISTER(wl80211_rtos, CONFIG_WIFI_LOG_LEVEL);

#define WL80211_TIMEOUT_INIT_MS 1000

typedef void *rtos_semaphore;
typedef void *rtos_mutex;

struct evt_work {
	struct k_work work;
	void (*handler)(void);
};

extern unsigned int timeouts_handler(void);

/* Single-thread only: blob calls rtos_lock/unlock from macsw task, never ISR. */
static unsigned int rtos_lock_key;

static void timeout_timer_handler(struct k_timer *timer);
static K_TIMER_DEFINE(wl80211_timeout_timer, timeout_timer_handler, NULL);

static void timeout_timer_handler(struct k_timer *timer)
{
	unsigned int next_ms = timeouts_handler();

	if (next_ms > 0) {
		k_timer_start(timer, K_MSEC(next_ms), K_NO_WAIT);
	}
}

static void evt_work_handler(struct k_work *work)
{
	struct evt_work *ew = CONTAINER_OF(work, struct evt_work, work);
	void (*handler)(void) = ew->handler;

	k_free(ew);
	if (handler != NULL) {
		handler();
	}
}

uint32_t rtos_ms2tick(int ms)
{
	return k_ms_to_ticks_ceil32(ms);
}

uint32_t rtos_now(bool isr)
{
	ARG_UNUSED(isr);
	return k_uptime_ticks();
}

void rtos_lock(void)
{
	rtos_lock_key = irq_lock();
}

void rtos_unlock(void)
{
	irq_unlock(rtos_lock_key);
}

int rtos_semaphore_create(rtos_semaphore *semaphore, int max_count, int init_count)
{
	struct k_sem *sem = k_malloc(sizeof(struct k_sem));

	if (sem == NULL) {
		return -1;
	}
	k_sem_init(sem, init_count, max_count);
	*semaphore = sem;
	return 0;
}

void rtos_semaphore_delete(rtos_semaphore semaphore)
{
	k_free(semaphore);
}

int rtos_semaphore_wait(rtos_semaphore semaphore, int timeout)
{
	k_timeout_t t;

	if (timeout < 0) {
		t = K_FOREVER;
	} else {
		t = K_MSEC(timeout);
	}

	return k_sem_take(semaphore, t) ? 1 : 0;
}

int rtos_semaphore_signal(rtos_semaphore semaphore, bool isr)
{
	ARG_UNUSED(isr);
	k_sem_give(semaphore);
	return 0;
}

int rtos_mutex_create(rtos_mutex *mutex)
{
	struct k_mutex *m = k_malloc(sizeof(struct k_mutex));

	if (m == NULL) {
		return -1;
	}
	k_mutex_init(m);
	*mutex = m;
	return 0;
}

void rtos_mutex_delete(rtos_mutex mutex)
{
	k_free(mutex);
}

void rtos_mutex_lock(rtos_mutex mutex)
{
	k_mutex_lock(mutex, K_FOREVER);
}

void rtos_mutex_unlock(rtos_mutex mutex)
{
	k_mutex_unlock(mutex);
}

void rtos_timeouts_init(void)
{
	k_timer_start(&wl80211_timeout_timer, K_MSEC(WL80211_TIMEOUT_INIT_MS), K_NO_WAIT);
}

void rtos_timeouts_start(unsigned int delay)
{
	k_timer_start(&wl80211_timeout_timer, K_MSEC(delay), K_NO_WAIT);
}

void rtos_start_evt_task(void (*handler)(void))
{
	struct evt_work *ew;

	if (handler == NULL) {
		return;
	}

	ew = k_malloc(sizeof(struct evt_work));
	if (ew == NULL) {
		LOG_ERR("Failed to alloc evt_work");
		return;
	}

	ew->handler = handler;
	k_work_init(&ew->work, evt_work_handler);
	k_work_submit(&ew->work);
}
