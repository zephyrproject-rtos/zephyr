/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_systim_timer

/*
 * TI SimpleLink CC23X0 timer driver based on SYSTIM
 */

#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_systim.h>
#include <inc/hw_evtsvt.h>

/* Kernel tick period in microseconds (same timebase as systim) */
#define TICK_PERIOD_MICRO_SEC (1000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * Max number of systim ticks into the future
 *
 * Under the hood, the kernel timer uses the SysTimer whose events trigger
 * immediately if the compare value is less than 2^22 systimer ticks in the past
 * (4.194sec at 1us resolution). Therefore, the max number of SysTimer ticks you
 * can schedule into the future is 2^32 - 2^22 - 1 ticks (~= 4290 sec at 1us
 * resolution).
 */
#define SYSTIM_TIMEOUT_MAX 0xFFBFFFFFU

/* Set systim interrupt to lowest priority */
#define SYSTIM_ISR_PRIORITY 3U

/* Keep track of systim counter at previous announcement to the kernel */
static uint32_t last_systim_count;

static void systim_isr(const void *arg);
static int sys_clock_driver_init(void);

/*
 * Set system clock timeout.
 */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	/* If timeout is necessary */
	if (ticks != K_TICKS_FOREVER) {
		/* Get current value as early as possible */
		uint32_t now_tick = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);
		uint32_t timeout = ticks * TICK_PERIOD_MICRO_SEC;

		if (timeout > SYSTIM_TIMEOUT_MAX) {
			timeout = SYSTIM_TIMEOUT_MAX;
		}
		/* This should wrap around */
		HWREG(SYSTIM_BASE + SYSTIM_O_CH0CC) = now_tick + timeout;
	}
}

uint32_t sys_clock_elapsed(void)
{
	/* Get current value as early as possible */
	uint32_t current_systim_count = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);
	uint32_t elapsed_systim;

	if (current_systim_count >= last_systim_count) {
		elapsed_systim = current_systim_count - last_systim_count;
	} else {
		elapsed_systim = (UINT32_MAX - last_systim_count) + current_systim_count;
	}

	int32_t elapsed_ticks = elapsed_systim / TICK_PERIOD_MICRO_SEC;

	return elapsed_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);
}

void systim_isr(const void *arg)
{
	/* Get current value as early as possible */
	uint32_t current_systim_count = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);
	uint32_t elapsed_systim;

	if (current_systim_count >= last_systim_count) {
		elapsed_systim = current_systim_count - last_systim_count;
	} else {
		elapsed_systim = (UINT32_MAX - last_systim_count) + current_systim_count;
	}

	int32_t elapsed_ticks = elapsed_systim / TICK_PERIOD_MICRO_SEC;

	sys_clock_announce(elapsed_ticks);

	last_systim_count = current_systim_count;

	/* Do not re-arm systim. Zephyr will do so through sys_clock_set_timeout */
}

static int sys_clock_driver_init(void)
{
	uint32_t now_tick;

	/* Get current value as early as possible */
	now_tick = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);
	last_systim_count = now_tick;

	/* Clear any pending interrupts on SysTimer channel 0 */
	HWREG(SYSTIM_BASE + SYSTIM_O_ICLR) = SYSTIM_ICLR_EV0_CLR;

	/*
	 * Configure SysTimer channel 0 to compare mode with timer
	 * resolution of 1 us.
	 */
	HWREG(SYSTIM_BASE + SYSTIM_O_CH0CFG) = 0;

	/* Make SysTimer halt on CPU debug halt */
	HWREG(SYSTIM_BASE + SYSTIM_O_EMU) = SYSTIM_EMU_HALT_STOP;

	HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ16SEL) = EVTSVT_CPUIRQ16SEL_PUBID_SYSTIM0;

	/*
	 * Set IMASK for channel 0. IMASK is used by the power driver to know
	 * which systimer channels are active.
	 */
	HWREG(SYSTIM_BASE + SYSTIM_O_IMSET) = SYSTIM_IMSET_EV0_SET;

	/* This should wrap around and set a maximum timeout */
	HWREG(SYSTIM_BASE + SYSTIM_O_CH0CC) = now_tick + SYSTIM_TIMEOUT_MAX;

	/* Take configurable interrupt IRQ16 for systimer */
	IRQ_CONNECT(CPUIRQ16_IRQn, SYSTIM_ISR_PRIORITY, systim_isr, 0, 0);
	irq_enable(CPUIRQ16_IRQn);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
