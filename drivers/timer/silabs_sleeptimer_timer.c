/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/logging/log.h>

#include <sl_sleeptimer.h>

LOG_MODULE_REGISTER(silabs_sleeptimer_timer);

/* Maximum time interval between timer interrupts (in hw_cycles) */
#define MAX_TIMEOUT_CYC (UINT32_MAX >> 1)

#define DT_RTC DT_COMPAT_GET_ANY_STATUS_OKAY(silabs_gecko_stimer)

/* Ensure interrupt names don't expand to register interface struct pointers */
#undef RTCC

/* With CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME, this global variable holds the clock frequency,
 * and must be written by the driver at init.
 */
extern unsigned int z_clock_hw_cycles_per_sec;

/* Global timer state */
struct sleeptimer_timer_data {
	uint32_t cyc_per_tick;      /* Number of hw_cycles per 1 kernel tick */
	uint32_t max_timeout_ticks; /* MAX_TIMEOUT_CYC expressed as ticks */
	atomic_t last_count;        /* Value of counter when the previous tick was announced */
	struct k_spinlock lock;     /* Spinlock to sync between ISR and updating the timeout */
	bool initialized;           /* Set to true when timer is initialized */
	sl_sleeptimer_timer_handle_t handle; /* Timer handle for system timer */
};
static struct sleeptimer_timer_data g_sleeptimer_timer_data = {0};

static void sleeptimer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	ARG_UNUSED(handle);
	struct sleeptimer_timer_data *timer = data;

	uint32_t curr = sl_sleeptimer_get_tick_count();
	uint32_t prev = atomic_get(&timer->last_count);
	uint32_t pending = curr - prev;

	/* Number of unannounced ticks since the last announcement */
	uint32_t unannounced = pending / timer->cyc_per_tick;

	atomic_set(&timer->last_count, prev + unannounced * timer->cyc_per_tick);

	sys_clock_announce(unannounced);
}

static void sleeptimer_clock_set_timeout(int32_t ticks, struct sleeptimer_timer_data *timer)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? timer->max_timeout_ticks : ticks;
	ticks = CLAMP(ticks, 0, timer->max_timeout_ticks);

	k_spinlock_key_t key = k_spin_lock(&timer->lock);

	uint32_t curr = sl_sleeptimer_get_tick_count();
	uint32_t pending = curr % timer->cyc_per_tick;
	uint32_t next = ticks * timer->cyc_per_tick;

	/* Next timeout is N ticks in the future, minus the current progress
	 * into the tick if using multiple cycles per tick. The HAL API must
	 * be called with a timeout of at least one cycle.
	 */
	if (next == 0) {
		next = timer->cyc_per_tick;
	}
	next -= pending;

	sl_sleeptimer_restart_timer(&timer->handle, next, sleeptimer_cb, timer, 0, 0);
	k_spin_unlock(&timer->lock, key);
}

static uint32_t sleeptimer_clock_elapsed(struct sleeptimer_timer_data *timer)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL) || !timer->initialized) {
		/* No unannounced ticks can have elapsed if not in tickless mode */
		return 0;
	} else {
		return (sl_sleeptimer_get_tick_count() - atomic_get(&timer->last_count)) /
		       timer->cyc_per_tick;
	}
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	sleeptimer_clock_set_timeout(ticks, &g_sleeptimer_timer_data);
}

uint32_t sys_clock_elapsed(void)
{
	return sleeptimer_clock_elapsed(&g_sleeptimer_timer_data);
}

uint32_t sys_clock_cycle_get_32(void)
{
	return g_sleeptimer_timer_data.initialized ? sl_sleeptimer_get_tick_count() : 0;
}

static int sleeptimer_init(void)
{
	sl_status_t status = SL_STATUS_OK;
	struct sleeptimer_timer_data *timer = &g_sleeptimer_timer_data;

	IRQ_CONNECT(DT_IRQ(DT_RTC, irq), DT_IRQ(DT_RTC, priority),
		    CONCAT(DT_STRING_UPPER_TOKEN_BY_IDX(DT_RTC, interrupt_names, 0), _IRQHandler),
		    0, 0);

	sl_sleeptimer_init();

	z_clock_hw_cycles_per_sec = sl_sleeptimer_get_timer_frequency();

	BUILD_ASSERT(CONFIG_SYS_CLOCK_TICKS_PER_SEC > 0,
		     "Invalid CONFIG_SYS_CLOCK_TICKS_PER_SEC value");

	timer->cyc_per_tick = z_clock_hw_cycles_per_sec / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	timer->max_timeout_ticks = MAX_TIMEOUT_CYC / timer->cyc_per_tick;
	timer->initialized = true;

	atomic_set(&timer->last_count,
		   ROUND_DOWN(sl_sleeptimer_get_tick_count(), timer->cyc_per_tick));

	/* Start the timer and announce 1 kernel tick */
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		status = sl_sleeptimer_start_timer(&timer->handle, timer->cyc_per_tick,
						   sleeptimer_cb, timer, 0, 0);
	} else {
		status = sl_sleeptimer_start_periodic_timer(&timer->handle, timer->cyc_per_tick,
							    sleeptimer_cb, timer, 0, 0);
	}
	if (status != SL_STATUS_OK) {
		return -ENODEV;
	}

	return 0;
}

SYS_INIT(sleeptimer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
