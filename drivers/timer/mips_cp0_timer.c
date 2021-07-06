/*
 * Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on riscv_machine_timer.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <soc.h>

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_CYC 0xffffffffu
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1000

static struct k_spinlock lock;
static uint32_t last_count;

static ALWAYS_INLINE void set_cp0_compare(uint32_t time)
{
	mips_setcompare(time);
}

static ALWAYS_INLINE uint32_t get_cp0_count(void)
{
	return mips_getcount();
}

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t now = get_cp0_count();
	uint32_t dticks = (uint32_t)((now - last_count) / CYC_PER_TICK);

	last_count += dticks * CYC_PER_TICK;

	uint32_t next = last_count + CYC_PER_TICK;

	if (next - now < MIN_DELAY) {
		next += CYC_PER_TICK;
	}
	set_cp0_compare(next);

	k_spin_unlock(&lock, key);
	z_clock_announce(1);
}

int z_clock_driver_init(const struct device *device)
{
	ARG_UNUSED(device);

	IRQ_CONNECT(MIPS_MACHINE_TIMER_IRQ, 0, timer_isr, NULL, 0);
	last_count = get_cp0_count();
	set_cp0_compare(last_count + CYC_PER_TICK);

	irq_enable(MIPS_MACHINE_TIMER_IRQ);

	return 0;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
	return;
}

uint32_t z_clock_elapsed(void)
{
	return 0;
}

uint32_t z_timer_cycle_get_32(void)
{
	return get_cp0_count();
}
