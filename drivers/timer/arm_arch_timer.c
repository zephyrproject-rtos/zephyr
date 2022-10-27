/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/timer/arm_arch_timer.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/arch/cpu.h>

#define CYC_PER_TICK	((uint64_t)sys_clock_hw_cycles_per_sec() \
			/ (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS	INT32_MAX
#define MIN_DELAY	(1000)

static struct k_spinlock lock;
static uint64_t last_cycle;
#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = ARM_ARCH_TIMER_IRQ;
#endif

static void arm_arch_timer_compare_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

#ifdef CONFIG_ARM_ARCH_TIMER_ERRATUM_740657
	/*
	 * Workaround required for Cortex-A9 MPCore erratum 740657
	 * comp. ARM Cortex-A9 processors Software Developers Errata Notice,
	 * ARM document ID032315.
	 */

	if (!arm_arch_timer_get_int_status()) {
		/*
		 * If the event flag is not set, this is a spurious interrupt.
		 * DO NOT modify the compare register's value, DO NOT announce
		 * elapsed ticks!
		 */
		k_spin_unlock(&lock, key);
		return;
	}
#endif /* CONFIG_ARM_ARCH_TIMER_ERRATUM_740657 */

	uint64_t curr_cycle = arm_arch_timer_count();
	uint32_t delta_ticks = (uint32_t)((curr_cycle - last_cycle) / CYC_PER_TICK);

	last_cycle += delta_ticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t next_cycle = last_cycle + CYC_PER_TICK;

		if ((uint64_t)(next_cycle - curr_cycle) < MIN_DELAY) {
			next_cycle += CYC_PER_TICK;
		}
		arm_arch_timer_set_compare(next_cycle);
		arm_arch_timer_set_irq_mask(false);
	} else {
		arm_arch_timer_set_irq_mask(true);
#ifdef CONFIG_ARM_ARCH_TIMER_ERRATUM_740657
		/*
		 * In tickless mode, the compare register is normally not
		 * updated from within the ISR. Yet, to work around the timer's
		 * erratum, a new value *must* be written while the interrupt
		 * is being processed before the interrupt is acknowledged
		 * by the handling interrupt controller.
		 */
		arm_arch_timer_set_compare(~0ULL);
	}

	/*
	 * Clear the event flag so that in case the erratum strikes (the timer's
	 * vector will still be indicated as pending by the GIC's pending register
	 * after this ISR has been executed) the error will be detected by the
	 * check performed upon entry of the ISR -> the event flag is not set,
	 * therefore, no actual hardware interrupt has occurred.
	 */
	arm_arch_timer_clear_int_status();
#else
	}
#endif /* CONFIG_ARM_ARCH_TIMER_ERRATUM_740657 */

	k_spin_unlock(&lock, key);

	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
#if defined(CONFIG_TICKLESS_KERNEL)

	if (ticks == K_TICKS_FOREVER && idle) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : \
		MIN(MAX_TICKS,  MAX(ticks - 1,  0));

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t curr_cycle = arm_arch_timer_count();
	uint64_t req_cycle = ticks * CYC_PER_TICK;

	/*
	 * Round up to next tick boundary, but an edge case should be handled.
	 * Fast hardware with slow timer hardware can trigger and enter an
	 * interrupt and reach this spot before the counter has advanced.
	 * That defeats the "round up" logic such that we end up scheduling
	 * timeouts a tick too soon (e.g. if the kernel requests an interrupt
	 * at the "X" tick, we would end up computing a comparator value
	 * representing the "X-1" tick!). Choose the bigger one between 1 and
	 * "curr_cycle - last_cycle" to correct.
	 */
	req_cycle += MAX(curr_cycle - last_cycle, 1) + (CYC_PER_TICK - 1);

	req_cycle = (req_cycle / CYC_PER_TICK) * CYC_PER_TICK;

	if ((req_cycle + last_cycle - curr_cycle) < MIN_DELAY) {
		req_cycle += CYC_PER_TICK;
	}

	arm_arch_timer_set_compare(req_cycle + last_cycle);
	arm_arch_timer_set_irq_mask(false);
	k_spin_unlock(&lock, key);

#else  /* CONFIG_TICKLESS_KERNEL */
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = (uint32_t)((arm_arch_timer_count() - last_cycle)
		    / CYC_PER_TICK);

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)arm_arch_timer_count();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return arm_arch_timer_count();
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
void arch_busy_wait(uint32_t usec_to_wait)
{
	if (usec_to_wait == 0) {
		return;
	}

	uint64_t start_cycles = arm_arch_timer_count();

	uint64_t cycles_to_wait = sys_clock_hw_cycles_per_sec() / USEC_PER_SEC * usec_to_wait;

	for (;;) {
		uint64_t current_cycles = arm_arch_timer_count();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
}
#endif

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	/*
	 * set the initial status of timer0 of each secondary core
	 */
	arm_arch_timer_set_compare(arm_arch_timer_count() + CYC_PER_TICK);
	arm_arch_timer_enable(true);
	irq_enable(ARM_ARCH_TIMER_IRQ);
	arm_arch_timer_set_irq_mask(false);
}
#endif

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(ARM_ARCH_TIMER_IRQ, ARM_ARCH_TIMER_PRIO,
		    arm_arch_timer_compare_isr, NULL, ARM_ARCH_TIMER_FLAGS);
	arm_arch_timer_init();
	last_cycle = arm_arch_timer_count();
	arm_arch_timer_set_compare(last_cycle + CYC_PER_TICK);
	arm_arch_timer_enable(true);
	irq_enable(ARM_ARCH_TIMER_IRQ);
	arm_arch_timer_set_irq_mask(false);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
