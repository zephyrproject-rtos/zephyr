/*
 * Copyright (c) 2024 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <limits.h>

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <opensbi.h>

static struct k_spinlock lock;
static uint64_t last_count;
static uint64_t last_ticks;
static uint32_t last_elapsed;

#define TIMER_IRQN   0x5
#define CYC_PER_TICK (100000)

static uint64_t xtime(void)
{
	unsigned int current_time;

	__asm__ volatile("rdtime %0" : "=r"(current_time));

	return current_time;
}

static void set_xtimecmp(uint64_t next_time)
{
	uint32_t time_low = (uint32_t)next_time;
	uint32_t time_high = (uint32_t)(next_time >> 32);

	sbi_console_set_timer(time_low, time_high);
}

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t now = xtime();

	set_xtimecmp(now + CYC_PER_TICK);

	k_spin_unlock(&lock, key);
	sys_clock_announce(1);
}

uint32_t sys_clock_cycle_get_32(void)
{
	return xtime();
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = xtime();
	uint64_t dcycles = now - last_count;
	uint32_t dticks = (uint32_t)dcycles / CYC_PER_TICK;

	last_elapsed = dticks;
	k_spin_unlock(&lock, key);
	return dticks;
}

static int sys_clock_driver_init(void)
{

	IRQ_CONNECT(TIMER_IRQN, 0, timer_isr, NULL, 0);

	last_ticks = xtime();
	set_xtimecmp(last_ticks + CYC_PER_TICK);
	irq_enable(TIMER_IRQN);

	arch_irq_unlock(XSTATUS_IEN);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
