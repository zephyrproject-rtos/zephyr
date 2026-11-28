/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* AMD ACP DSP0 External Timer1 system clock driver */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#include <acp7x_chip_reg.h>
#include <acp7x_chip_offsets.h>
#include "../interrupt_controller/intc_acp.h"

#define DT_DRV_COMPAT amd_acp_timer

/* CNTL[31:30]: 00=disable, 10=periodic */
#define TIMER_COUNT_MASK 0x3FFFFFFFUL
#define TIMER_MODE_SHIFT 30
#define TIMER_PERIOD_US  500U

#define TIMER_IRQ      DT_INST_IRQN(0)
#define TIMER_CLK_FREQ DT_INST_PROP(0, clock_frequency)

/* CPU cycles per kernel tick */
#define CYC_PER_TICK    (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
/* Countdown for a fixed 500 us periodic interrupt */
#define TIMER_COUNTDOWN ((TIMER_CLK_FREQ / 1000000U) * TIMER_PERIOD_US)

BUILD_ASSERT(CYC_PER_TICK > 0, "CYC_PER_TICK must be > 0");
BUILD_ASSERT(TIMER_CLK_FREQ % 1000000U == 0, "timer clock must be a whole number of MHz");
BUILD_ASSERT(TIMER_COUNTDOWN > 0 && TIMER_COUNTDOWN <= TIMER_COUNT_MASK,
	     "TIMER_COUNTDOWN out of range");

/* CCOUNT at the last announced tick, extended to 64 bits */
static uint64_t last_cycle;

static ALWAYS_INLINE uint32_t ccount(void)
{
	uint32_t v;

	__asm__ volatile("rsr.CCOUNT %0" : "=r"(v));
	return v;
}

/*
 * Start the timer in periodic mode; it reloads itself after each interrupt.
 * Write 0 first, since the hardware only loads a new count when re-enabled.
 */
static ALWAYS_INLINE void timer_arm_periodic(void)
{
	uint32_t val = ((uint32_t)ACP_INTC_TIMER_CNTL_PERIODIC << TIMER_MODE_SHIFT) |
		       (TIMER_COUNTDOWN & TIMER_COUNT_MASK);

	io_reg_write((PU_REGISTER_BASE + ACP_DSP0_EXT_TIMER1_CNTL), 0);
	io_reg_write((PU_REGISTER_BASE + ACP_DSP0_EXT_TIMER1_CNTL), val);
}

static ALWAYS_INLINE void timer_stop(void)
{
	io_reg_write((PU_REGISTER_BASE + ACP_DSP0_EXT_TIMER1_CNTL), 0);
}

/* Clear the write-1-to-clear Timer1 timeout status */
static ALWAYS_INLINE void timer_clear_timeout_status(void)
{
	io_reg_write((PU_REGISTER_BASE + ACP_DSP0_INTR_STAT), ACP_EXT_TIMER1_INTR_BIT);
}

/* Advance last_cycle by whole elapsed ticks and return that tick count */
static uint32_t sys_clock_elapsed_ticks(uint32_t curr)
{
	uint32_t dticks = (curr - (uint32_t)last_cycle) / CYC_PER_TICK;

	last_cycle += (uint64_t)dticks * CYC_PER_TICK;
	return dticks;
}

static void acp_timer_isr(const void *arg)
{
	k_spinlock_key_t key;
	uint32_t dticks;

	ARG_UNUSED(arg);
	timer_clear_timeout_status();

	key = sys_clock_lock();
	dticks = sys_clock_elapsed_ticks(ccount());
	sys_clock_announce_locked(dticks, key);
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);

	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	/* Periodic hardware timer auto-reloads; nothing to program here */
}

uint32_t sys_clock_elapsed(void)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}
	return (ccount() - (uint32_t)last_cycle) / CYC_PER_TICK;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return ccount();
}

uint64_t sys_clock_cycle_get_64(void)
{
	k_spinlock_key_t key = sys_clock_lock();
	uint64_t ret = last_cycle + (uint32_t)(ccount() - (uint32_t)last_cycle);

	sys_clock_unlock(key);
	return ret;
}

static int amd_acp_timer_init(void)
{
	IRQ_CONNECT(TIMER_IRQ, 0, acp_timer_isr, NULL, 0);

	timer_stop();
	timer_clear_timeout_status();
	last_cycle = ccount();
	timer_arm_periodic();

	return 0;
}

/* Must run after acp_intc_init() (POST_KERNEL, 0) has routed and unmasked the IRQ */
static int amd_acp_timer_post(void)
{
	timer_clear_timeout_status();
	timer_arm_periodic();
	irq_enable(TIMER_IRQ);

	return 0;
}

SYS_INIT(amd_acp_timer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
SYS_INIT(amd_acp_timer_post, POST_KERNEL, 50);
