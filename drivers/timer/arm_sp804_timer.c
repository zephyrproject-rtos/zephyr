/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel tick source based on the ARM SP804 dual timer
 *
 * Timer 1 of the block free runs periodically at the tick rate; timer 2 is
 * left free running as the cycle counter and is also what the interrupt uses
 * to work out how many ticks actually went by. Both timers count down, so the
 * cycle counter is the bitwise complement of the raw value.
 */

#define DT_DRV_COMPAT arm_sp804

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>

#define TIMER_BASE  DT_INST_REG_ADDR(0)
#define TIMER1_BASE (TIMER_BASE + 0x00)
#define TIMER2_BASE (TIMER_BASE + 0x20)

#define TIMER_LOAD    0x00
#define TIMER_VALUE   0x04
#define TIMER_CONTROL 0x08
#define TIMER_INTCLR  0x0c
#define TIMER_RIS     0x10
#define TIMER_MIS     0x14
#define TIMER_BGLOAD  0x18

#define CTRL_ONESHOT  BIT(0)
#define CTRL_32BIT    BIT(1)
#define CTRL_IE       BIT(5)
#define CTRL_PERIODIC BIT(6)
#define CTRL_ENABLE   BIT(7)

#define CYCLES_PER_TICK (sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

BUILD_ASSERT(DT_INST_PROP(0, clock_frequency) == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "SP804 clock-frequency must match CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC");

static struct k_spinlock lock;
/* Cycle count of the last tick boundary already handed to the kernel. */
static uint32_t last_tick_cycle;

/* Timer 2 counts down, so invert it to get a monotonic up-counter. */
static inline uint32_t free_running_cycles(void)
{
	return ~sys_read32(TIMER2_BASE + TIMER_VALUE);
}

static void sp804_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	if ((sys_read32(TIMER1_BASE + TIMER_MIS) & 0x1U) == 0U) {
		k_spin_unlock(&lock, key);
		return;
	}

	sys_write32(1U, TIMER1_BASE + TIMER_INTCLR);

	/*
	 * Count whole ticks off the free running timer rather than assuming
	 * one per interrupt. If servicing was delayed past a tick boundary -
	 * a long irq_lock(), or the host descheduling the emulator - those
	 * ticks still have to reach the kernel, otherwise kernel time drifts
	 * behind the same counter that k_busy_wait() and k_cycle_get_32()
	 * read.
	 */
	uint32_t elapsed = free_running_cycles() - last_tick_cycle;
	uint32_t ticks = elapsed / CYCLES_PER_TICK;

	last_tick_cycle += ticks * CYCLES_PER_TICK;

	k_spin_unlock(&lock, key);

	if (ticks != 0U) {
		sys_clock_announce((int32_t)ticks);
	}
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);

	/* Tickful only: the periodic timer always fires once per tick. */
}

uint32_t sys_clock_elapsed(void)
{
	/*
	 * Whole ticks that have passed since the last announcement but that
	 * the interrupt has not reported yet.
	 */
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t elapsed = free_running_cycles() - last_tick_cycle;

	k_spin_unlock(&lock, key);

	return elapsed / CYCLES_PER_TICK;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return free_running_cycles();
}

static int sp804_init(void)
{
	/* Timer 1: periodic tick source. */
	sys_write32(0U, TIMER1_BASE + TIMER_CONTROL);
	sys_write32(1U, TIMER1_BASE + TIMER_INTCLR);
	sys_write32(CYCLES_PER_TICK, TIMER1_BASE + TIMER_LOAD);
	sys_write32(CTRL_ENABLE | CTRL_PERIODIC | CTRL_IE | CTRL_32BIT,
		    TIMER1_BASE + TIMER_CONTROL);

	/* Timer 2: free running cycle counter. */
	sys_write32(0U, TIMER2_BASE + TIMER_CONTROL);
	sys_write32(0xffffffffU, TIMER2_BASE + TIMER_LOAD);
	sys_write32(CTRL_ENABLE | CTRL_32BIT, TIMER2_BASE + TIMER_CONTROL);

	last_tick_cycle = free_running_cycles();

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), sp804_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

SYS_INIT(sp804_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
