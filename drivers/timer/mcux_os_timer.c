/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_os_timer

#include <limits.h>

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/pm/pm.h>
#include "fsl_ostimer.h"
#ifndef CONFIG_SOC_MCXN236
#include "fsl_power.h"
#endif

#define CYC_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec()	\
			      / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define CYC_PER_US ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec()	\
			      / (uint64_t)USEC_PER_SEC))
#define MAX_CYC INT_MAX
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1000

#define TICKLESS IS_ENABLED(CONFIG_TICKLESS_KERNEL)

static struct k_spinlock lock;
static uint64_t last_count;
static OSTIMER_Type *base;
/* Total cycles of the timer compensated to include the time lost in "sleep/deep sleep" modes.
 * This maintains the timer count to account for the case if the OS Timer is reset in
 * certain deep sleep modes and the time elapsed when it is powered off.
 */
static uint64_t cyc_sys_compensated;
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM
/* This is the counter device used when OS timer is not available in
 * standby mode.
 */
static const struct device *counter_dev;
/* Indicates if the counter is running. */
bool counter_running;
#endif

static uint64_t mcux_lpc_ostick_get_compensated_timer_value(void)
{
	return (OSTIMER_GetCurrentTimerValue(base) + cyc_sys_compensated);
}

void mcux_lpc_ostick_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = mcux_lpc_ostick_get_compensated_timer_value();
	uint32_t dticks = (uint32_t)((now - last_count) / CYC_PER_TICK);

	/* Clear interrupt flag by writing 1. */
	base->OSEVENT_CTRL &= ~OSTIMER_OSEVENT_CTRL_OSTIMER_INTENA_MASK;

	last_count += dticks * CYC_PER_TICK;

	if (!TICKLESS) {
		uint64_t next = last_count + CYC_PER_TICK;

		if ((int64_t)(next - now) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		OSTIMER_SetMatchValue(base, next, NULL);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM

/* The OS Timer is disabled in certain low power modes and cannot wakeup the system
 * on timeout. This function will be called by the low power code to allow the
 * OS Timer to save off the count if needed and also start a wakeup counter
 * that would wakeup the system from deep power down modes.
 */
static uint32_t mcux_lpc_ostick_set_counter_timeout(int32_t curr_timeout)
{
	uint32_t ret = 0;

	if (counter_dev) {
		uint32_t timeout;
		int32_t ticks;
		struct counter_top_cfg top_cfg = { 0 };
		timeout = k_ticks_to_us_near32(curr_timeout);

		ticks = counter_us_to_ticks(counter_dev, timeout);
		ticks = CLAMP(ticks, 1, counter_get_max_top_value(counter_dev));

		top_cfg.ticks = ticks;
		top_cfg.callback = NULL;
		top_cfg.user_data = NULL;
		top_cfg.flags = 0;
		if (counter_set_top_value(counter_dev, &top_cfg) != 0) {
			/* Setting top value failed, try setting an alarm */
			struct counter_alarm_cfg alarm_cfg;

			alarm_cfg.ticks = ticks;
			alarm_cfg.callback = NULL;
			alarm_cfg.user_data = NULL;
			alarm_cfg.flags = 0;

			if (counter_set_channel_alarm(counter_dev, 0, &alarm_cfg) != 0) {
				ret = 1;
				goto done;
			}
		}

		/* Counter is set to wakeup the system after the requested time */
		if (counter_start(counter_dev) != 0) {
			ret = 1;
		}
		counter_running = true;
#if CONFIG_MCUX_OS_TIMER_PM_POWERED_OFF
		/* Capture the current timer value for cases where it loses its state
		 * in low power modes.
		 */
		cyc_sys_compensated += OSTIMER_GetCurrentTimerValue(base);
#endif
	} else {
		ret = 1;
	}

done:
	return ret;
}

/* After exit from certain low power modes where the OS Timer was disabled, the
 * current tick value should be updated to account for the period when the OS Timer
 * was disabled. Also in certain cases, the OS Timer might lose its state and needs
 * to be reinitialized.
 */
static uint32_t mcux_lpc_ostick_compensate_system_timer(void)
{
	uint32_t ret = 0;

	if (counter_dev) {
		uint32_t slept_time_ticks;
		uint32_t slept_time_us;

		if (counter_running) {
			counter_stop(counter_dev);
			counter_running = false;
			counter_get_value(counter_dev, &slept_time_ticks);

			if (!(counter_is_counting_up(counter_dev))) {
				slept_time_ticks = counter_get_top_value(counter_dev) -
						   slept_time_ticks;
			}
			slept_time_us = counter_ticks_to_us(counter_dev, slept_time_ticks);
			cyc_sys_compensated += CYC_PER_US * slept_time_us;
#if CONFIG_MCUX_OS_TIMER_PM_POWERED_OFF
			/* Reset the OS Timer to a known state */
			RESET_PeripheralReset(kOSEVENT_TIMER_RST_SHIFT_RSTn);
			/* Reactivate os_timer for cases where it loses its state */
			OSTIMER_Init(base);
#endif
			/* Announce the time slept to the kernel*/
			mcux_lpc_ostick_isr(NULL);
		}
	} else {
		ret = 1;
	}

	return ret;
}

#endif

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Only for tickless kernel system */
		return;
	}

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM
	if (idle) {
		/* OS Timer may not be able to wakeup in certain low power modes.
		 * For these cases, we start a counter that can wakeup
		 * from low power modes.
		 */
		if (pm_state_next_get(0)->state == PM_STATE_STANDBY) {
			if (mcux_lpc_ostick_set_counter_timeout(ticks) == 0) {
				/* A low power counter has been started. No need to
				 * go further, simply return
				 */
				return;
			}
		}
	}
#endif

	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = mcux_lpc_ostick_get_compensated_timer_value();
	uint32_t adj, cyc = ticks * CYC_PER_TICK;

	/* Round up to next tick boundary. */
	adj = (uint32_t)(now - last_count) + (CYC_PER_TICK - 1);
	if (cyc <= MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = MAX_CYC;
	}
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;

	if ((int32_t)(cyc + last_count - now) < MIN_DELAY) {
		cyc += CYC_PER_TICK;
	}

	OSTIMER_SetMatchValue(base, cyc + last_count - cyc_sys_compensated, NULL);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful kernel system */
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = ((uint32_t)mcux_lpc_ostick_get_compensated_timer_value() -
					(uint32_t)last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)mcux_lpc_ostick_get_compensated_timer_value();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return mcux_lpc_ostick_get_compensated_timer_value();
}

void sys_clock_idle_exit(void)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM
	/* The tick should be compensated for states where the
	 * OS Timer is disabled
	 */
	if (pm_state_next_get(0)->state == PM_STATE_STANDBY) {
		mcux_lpc_ostick_compensate_system_timer();
	}
#endif
}

static int sys_clock_driver_init(void)
{

	/* Configure event timer's ISR */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
					mcux_lpc_ostick_isr, NULL, 0);

	base = (OSTIMER_Type *)DT_INST_REG_ADDR(0);

#if (DT_INST_PROP(0, wakeup_source))
	EnableDeepSleepIRQ(DT_INST_IRQN(0));
#endif

	/* Initialize the OS timer, setting clock configuration. */
	OSTIMER_Init(base);

	last_count = mcux_lpc_ostick_get_compensated_timer_value();
	OSTIMER_SetMatchValue(base, last_count + CYC_PER_TICK, NULL);

	/* Enable event timer interrupt */
	irq_enable(DT_INST_IRQN(0));

/* On some SoC's, OS Timer cannot wakeup from low power mode in standby modes */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM
	counter_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(0, deep_sleep_counter));
#endif

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
