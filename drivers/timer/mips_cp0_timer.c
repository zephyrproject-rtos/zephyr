/*
 * Copyright (c) 2020, 2021 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (c) 2021 Remy Luisant <remy@luisant.ca>
 *
 * Based on riscv_machine_timer.c and xtensa_sys_timer.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <soc.h>
#include <mips/mipsregs.h>

#define CYC_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() \
				 / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define MAX_CYC INT_MAX
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1000

#define TICKLESS IS_ENABLED(CONFIG_TICKLESS_KERNEL)

static struct k_spinlock lock;
static uint32_t last_count;

static ALWAYS_INLINE void set_cp0_compare(uint32_t time)
{
	_mips_write_32bit_c0_register(CP0_COMPARE, time);
}

static ALWAYS_INLINE uint32_t get_cp0_count(void)
{
	return _mips_read_32bit_c0_register(CP0_COUNT);
}

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t now = get_cp0_count();
	uint32_t dticks = ((now - last_count) / CYC_PER_TICK);

	last_count = now;

	if (!TICKLESS) {
		uint32_t next = last_count + CYC_PER_TICK;

		if (next - now < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_cp0_compare(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(TICKLESS ? dticks : 1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!TICKLESS) {
		return;
	}

	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t current_count = get_cp0_count();
	uint32_t delay_wanted = ticks * CYC_PER_TICK;

	/* Round up to next tick boundary. */
	uint32_t adj = (current_count - last_count) + (CYC_PER_TICK - 1);

	if (delay_wanted <= MAX_CYC - adj) {
		delay_wanted += adj;
	} else {
		delay_wanted = MAX_CYC;
	}
	delay_wanted = (delay_wanted / CYC_PER_TICK) * CYC_PER_TICK;

	if ((int32_t)(delay_wanted + last_count - current_count) < MIN_DELAY) {
		delay_wanted += CYC_PER_TICK;
	}

	set_cp0_compare(delay_wanted + last_count);
	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!TICKLESS) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ticks_elapsed = (get_cp0_count() - last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ticks_elapsed;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return get_cp0_count();
}

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(MIPS_MACHINE_TIMER_IRQ, 0, timer_isr, NULL, 0);
	last_count = get_cp0_count();

	/*
	 * In a tickless system the first tick might possibly be pushed
	 * much further into the future than is being done here.
	 */
	set_cp0_compare(last_count + CYC_PER_TICK);

	irq_enable(MIPS_MACHINE_TIMER_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
