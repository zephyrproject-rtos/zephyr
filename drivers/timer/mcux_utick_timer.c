/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_utick

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include "fsl_utick.h"

LOG_MODULE_REGISTER(mcux_utick_timer, LOG_LEVEL_ERR);

/* Timer cycles per tick */
#define CYC_PER_TICK                                                                               \
	((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() /                                      \
		    (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
/* UTICK: DELAYVAL is 31-bit; actual delay = (DELAYVAL+1) cycles */
#define MAX_CYC   (UTICK_CTRL_DELAYVAL_MASK)
/* The minimum usable tick interval is one clock cycle, for a delay of two timer clocks. */
#define MIN_CYC   (1u)
#define MAX_TICKS ((MAX_CYC + 1u) / CYC_PER_TICK)

static struct k_spinlock lock;
/* UTICK did not provide a readable counter register, so use accumulate software cycles */
static volatile uint64_t sw_cycle64;
/* Ticks programmed for the next interrupt (used as elapsed on IRQ) */
static volatile uint32_t programmed_ticks;

static UTICK_Type *base = (UTICK_Type *)DT_INST_REG_ADDR(0);

static inline uint32_t ticks_to_cycles(uint32_t ticks)
{
	uint64_t cyc = (uint64_t)ticks * (uint64_t)CYC_PER_TICK;

	if (cyc < MIN_CYC + 1) {
		cyc = MIN_CYC + 1;
	}
	if (cyc > MAX_CYC + 1) {
		cyc = MAX_CYC + 1;
	}
	return (uint32_t)(cyc - 1);
}

static void utick_cb(void)
{
	uint32_t elapsed = programmed_ticks ? programmed_ticks : 1u;

	sw_cycle64 += (uint64_t)elapsed * (uint64_t)CYC_PER_TICK;
	programmed_ticks = 0u;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		programmed_ticks = 1u;
		UTICK_SetTick(base, kUTICK_Onetime, ticks_to_cycles(programmed_ticks), utick_cb);
		sys_clock_announce(1);
	} else {
		sys_clock_announce((int32_t)elapsed);
	}
}

static void mcux_utick_isr(const void *arg)
{
	ARG_UNUSED(arg);

	UTICK_HandleIRQ(base, utick_cb);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Not supported on tickful kernels */
		return;
	}
	ARG_UNUSED(idle);

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP((ticks - 1), 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);

	programmed_ticks = (uint32_t)ticks;
	UTICK_SetTick(base, kUTICK_Onetime, ticks_to_cycles(programmed_ticks), utick_cb);
	k_spin_unlock(&lock, key);
}

void sys_clock_disable(void)
{
	UTICK_Deinit(base);
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)sw_cycle64;
}

uint64_t sys_clock_cycle_get_64(void)
{
	return sw_cycle64;
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), mcux_utick_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));
	UTICK_Init(base);

	programmed_ticks = IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? MAX_TICKS : 1u;
	UTICK_SetTick(base, kUTICK_Onetime, ticks_to_cycles(programmed_ticks), utick_cb);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
