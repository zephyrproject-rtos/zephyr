/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_os_timer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include "fsl_ostimer.h"
#include "fsl_power.h"

#define CYC_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec()	\
			      / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define MAX_CYC INT_MAX
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1000

#define TICKLESS IS_ENABLED(CONFIG_TICKLESS_KERNEL)

static struct k_spinlock lock;
static uint64_t last_count;
static OSTIMER_Type *base;

void mcux_lpc_ostick_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = OSTIMER_GetCurrentTimerValue(base);
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

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Only for tickless kernel system */
		return;
	}

	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = OSTIMER_GetCurrentTimerValue(base);
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

	OSTIMER_SetMatchValue(base, cyc + last_count, NULL);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful kernel system */
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = ((uint32_t)OSTIMER_GetCurrentTimerValue(base) -
					(uint32_t)last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)OSTIMER_GetCurrentTimerValue(base);
}

uint64_t sys_clock_cycle_get_64(void)
{
	return OSTIMER_GetCurrentTimerValue(base);
}

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Configure event timer's ISR */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
					mcux_lpc_ostick_isr, NULL, 0);

	base = (OSTIMER_Type *)DT_INST_REG_ADDR(0);

	EnableDeepSleepIRQ(DT_INST_IRQN(0));

	/* Initialize the OS timer, setting clock configuration. */
	OSTIMER_Init(base);

	last_count = OSTIMER_GetCurrentTimerValue(base);
	OSTIMER_SetMatchValue(base, last_count + CYC_PER_TICK, NULL);

	/* Enable event timer interrupt */
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
