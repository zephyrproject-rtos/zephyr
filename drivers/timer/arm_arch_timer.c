/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/timer/arm_arch_timer.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <arch/cpu.h>

#define CYC_PER_TICK	((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() \
			       / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))

#define MAX_TICKS	((0xffffffffu - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY	(1000)

static struct k_spinlock lock;
static volatile uint64_t last_cycle;

static void arm_arch_timer_compare_isr(void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t curr_cycle = arm_arch_timer_count();
	uint32_t delta_ticks = (uint32_t)((curr_cycle - last_cycle) / CYC_PER_TICK);

	last_cycle += delta_ticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t next_cycle = last_cycle + CYC_PER_TICK;

		if ((int64_t)(next_cycle - curr_cycle) < MIN_DELAY) {
			next_cycle += CYC_PER_TICK;
		}
		arm_arch_timer_set_compare(next_cycle);
	}

	k_spin_unlock(&lock, key);

	z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? delta_ticks : 1);
}

int z_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	IRQ_CONNECT(ARM_ARCH_TIMER_IRQ, ARM_ARCH_TIMER_PRIO,
		    arm_arch_timer_compare_isr, NULL, ARM_ARCH_TIMER_FLAGS);
	arm_arch_timer_set_compare(arm_arch_timer_count() + CYC_PER_TICK);
	arm_arch_timer_enable(true);
	irq_enable(ARM_ARCH_TIMER_IRQ);

	return 0;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)

	if (idle) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = MAX(MIN(ticks - 1, (int32_t)MAX_TICKS), 0);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t curr_cycle = arm_arch_timer_count();
	uint32_t req_cycle = ticks * CYC_PER_TICK;

	/* Round up to next tick boundary */
	req_cycle += (uint32_t)(curr_cycle - last_cycle) + (CYC_PER_TICK - 1);
	req_cycle = (req_cycle / CYC_PER_TICK) * CYC_PER_TICK;

	if ((int32_t)(req_cycle + last_cycle - curr_cycle) < MIN_DELAY) {
		req_cycle += CYC_PER_TICK;
	}

	arm_arch_timer_set_compare(req_cycle + last_cycle);
	k_spin_unlock(&lock, key);

#endif
}

uint32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = ((uint32_t)arm_arch_timer_count() - (uint32_t)last_cycle)
		    / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t z_timer_cycle_get_32(void)
{
	return (uint32_t)arm_arch_timer_count();
}
