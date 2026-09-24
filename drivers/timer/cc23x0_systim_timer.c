/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_systim_timer

/*
 * TI SimpleLink CC23X0 timer driver based on the ClockP module from hal_ti
 */

#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

#include <ti/drivers/dpl/ClockP.h>

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_systim.h>
#include <inc/hw_evtsvt.h>

/* Kernel tick period in ClockP ticks */
#define CLOCKP_TICKS_PER_SYS_CLOCK_TICK                                                            \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * Max number of clockP ticks into the future
 *
 * Under the hood, the kernel timer uses the SysTimer whose events trigger
 * immediately if the compare value is less than 2^22 systimer ticks in the past
 * (4.194sec at 1us resolution). Therefore, the max number of SysTimer ticks you
 * can schedule into the future is 2^32 - 2^22 - 1 ticks (~= 4290 sec at 1us
 * resolution).
 */
#define SYSCLOCK_TIMEOUT_MAX 0xFFBFFFFFU

/* Set systim interrupt to lowest priority */
#define SYSCLOCK_ISR_PRIORITY IRQ_PRIO_LOWEST

static struct k_spinlock lock;

/* Keep track of clockp ticks at previous announcement to the kernel */
static uint32_t last_clockp_tick;

static void sys_clock_isr(uintptr_t arg);
static int sys_clock_driver_init(void);

/* ClockP object used for system clock */
static ClockP_Struct sysClockObj;

/*
 * Set system clock timeout.
 */
void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* If timeout is necessary */
	if (ticks != K_TICKS_FOREVER) {

		ClockP_stop(&sysClockObj);

		uint32_t timeout;

		if (ticks < 1) {
			/* Fire next tick as soon as possible */
			timeout = 0;
		} else {
			uint32_t now_tick = ClockP_getSystemTicks();

			/* Microseconds elapsed within the current tick period */
			uint32_t clockp_tick_delta = now_tick % CLOCKP_TICKS_PER_SYS_CLOCK_TICK;

			timeout = ticks * CLOCKP_TICKS_PER_SYS_CLOCK_TICK;

			if (timeout > SYSCLOCK_TIMEOUT_MAX) {
				timeout = SYSCLOCK_TIMEOUT_MAX;
				/* Align timeout to CLOCKP_TICKS_PER_SYS_CLOCK_TICK */
				timeout -= timeout % CLOCKP_TICKS_PER_SYS_CLOCK_TICK;
			}

			/*
			 * We remove the delta since last tick boundary to get the
			 * correct timeout in HW-cycles until the specified tick
			 */
			timeout -= clockp_tick_delta;
		}

		ClockP_setTimeout(&sysClockObj, timeout);

		ClockP_start(&sysClockObj);
	}

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t current_clockp_tick = ClockP_getSystemTicks();

	uint32_t elapsed_kernel_ticks =
		(current_clockp_tick - last_clockp_tick) / CLOCKP_TICKS_PER_SYS_CLOCK_TICK;

	k_spin_unlock(&lock, key);

	return elapsed_kernel_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return ClockP_getSystemTicks();
}

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
uint64_t sys_clock_cycle_get_64(void)
{
	return ClockP_getSystemTicks64();
}
#endif /* CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER */

void sys_clock_isr(uintptr_t arg)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	/* Get current value as early as possible */
	uint32_t current_clockp_tick = ClockP_getSystemTicks();
	uint32_t elapsed_kernel_ticks =
		(current_clockp_tick - last_clockp_tick) / CLOCKP_TICKS_PER_SYS_CLOCK_TICK;

	last_clockp_tick += elapsed_kernel_ticks * CLOCKP_TICKS_PER_SYS_CLOCK_TICK;

	k_spin_unlock(&lock, key);

	sys_clock_announce(elapsed_kernel_ticks);

	/* Do not re-arm sysClock. Zephyr will do so through sys_clock_set_timeout */
}

static int sys_clock_driver_init(void)
{
	/* Get current value as early as possible */
	uint32_t nowTick;

	nowTick = ClockP_getSystemTicks();
	last_clockp_tick = nowTick;

	/* Parameters for the system clock ClockP object */
	ClockP_Params sysClockParams;

	sysClockParams.period = 0;
	sysClockParams.startFlag = false;

	/* Construct the systimers ClockP object */
	ClockP_construct(&sysClockObj, sys_clock_isr, SYSCLOCK_TIMEOUT_MAX, &sysClockParams);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
