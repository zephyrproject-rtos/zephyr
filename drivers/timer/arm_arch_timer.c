/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/drivers/timer/arm_arch_timer.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys/clock.h>
#include <zephyr/arch/cpu.h>

/*
 * Free-running 64-bit system counter plus an absolute compare register: a
 * COMPARE_ORDERED backend. Arming also unmasks the compare interrupt, which
 * the ISR masks in tickless mode.
 */
#define TIMER_CORE_BACKEND_COMPARE_ORDERED
#define TIMER_CORE_COUNTER_WIDTH 64

static inline uint64_t timer_driver_cycle_get(void)
{
	return arm_arch_timer_count();
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	arm_arch_timer_set_compare(cycles);
	arm_arch_timer_set_irq_mask(false);
}

#include "system_timer_generic.h"

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = ARM_ARCH_TIMER_IRQ;
#endif

static void arm_arch_timer_compare_isr(const void *arg)
{
	ARG_UNUSED(arg);

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
		return;
	}
#endif /* CONFIG_ARM_ARCH_TIMER_ERRATUM_740657 */

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * The compare stays asserted until it is reprogrammed, so mask
		 * the interrupt now. timer_driver_set_compare() unmasks it when the
		 * timer is rearmed after the announce.
		 */
		arm_arch_timer_set_irq_mask(true);
	}

#ifdef CONFIG_ARM_ARCH_TIMER_ERRATUM_740657
	/*
	 * The event flag sets again as long as the counter is past the compare,
	 * so push the compare out of reach before clearing it. The core programs
	 * the real deadline in the announce below, in either tick mode.
	 */
	arm_arch_timer_set_compare(~0ULL);

	/*
	 * Clear the event flag so that in case the erratum strikes (the timer's
	 * vector will still be indicated as pending by the GIC's pending register
	 * after this ISR has been executed) the error will be detected by the
	 * check performed upon entry of the ISR -> the event flag is not set,
	 * therefore, no actual hardware interrupt has occurred.
	 */
	arm_arch_timer_clear_int_status();
#endif /* CONFIG_ARM_ARCH_TIMER_ERRATUM_740657 */

	timer_core_announce();
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
void arch_busy_wait(uint32_t usec_to_wait)
{
	if (usec_to_wait == 0) {
		return;
	}

	uint64_t start_cycles = arm_arch_timer_count();
	uint64_t cycles_to_wait = k_us_to_cyc_ceil64(usec_to_wait);

#ifdef CONFIG_ARM64
	if (is_wfxt_implemented()) {
		uint64_t deadline = start_cycles + cycles_to_wait;

		do {
			wfet(deadline);
		} while (arm_arch_timer_count() < deadline);
		return;
	}
#endif

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
	timer_core_smp_prime();
	arm_arch_timer_enable(true);
	irq_enable(ARM_ARCH_TIMER_IRQ);
}
#endif

static int sys_clock_driver_init(void)
{

	IRQ_CONNECT(ARM_ARCH_TIMER_IRQ, ARM_ARCH_TIMER_PRIO,
		    arm_arch_timer_compare_isr, NULL, ARM_ARCH_TIMER_FLAGS);
	arm_arch_timer_init();
	timer_core_init();
	arm_arch_timer_enable(true);
	irq_enable(ARM_ARCH_TIMER_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
