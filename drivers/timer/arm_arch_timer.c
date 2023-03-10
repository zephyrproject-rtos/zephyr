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

#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
/* precompute CYC_PER_TICK at driver init to avoid runtime double divisions */
static uint32_t cyc_per_tick;
#define CYC_PER_TICK cyc_per_tick
#else
#define CYC_PER_TICK (uint32_t)(sys_clock_hw_cycles_per_sec() \
				/ CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#endif

/* the unsigned long cast limits divisors to native CPU register width */
#define cycle_diff_t unsigned long

static struct k_spinlock lock;
static uint64_t last_cycle;
static uint64_t last_tick;
static uint32_t last_elapsed;

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
	uint64_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = (cycle_diff_t)delta_cycles / CYC_PER_TICK;

	last_cycle += (cycle_diff_t)delta_ticks * CYC_PER_TICK;
	last_tick += delta_ticks;
	last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t next_cycle = last_cycle + CYC_PER_TICK;

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

	if (ticks == K_TICKS_FOREVER) {
		if (idle) {
			return;
		}
		ticks = INT32_MAX;
	}

	/*
	 * Clamp the max period length to a number of cycles that can fit
	 * in half the range of a cycle_diff_t for native width divisions
	 * to be usable elsewhere. Also clamp it to half the range of an
	 * int32_t as this is the type used for elapsed tick announcements.
	 * The half range gives us one bit of extra room to cope with the
	 * unavoidable IRQ servicing latency (we never need as much but this
	 * is simple). The compiler should optimize away the least restrictive
	 * of those tests automatically.
	 */
	ticks = CLAMP(ticks, 0, (cycle_diff_t)-1 / 2 / CYC_PER_TICK);
	ticks = CLAMP(ticks, 0, INT32_MAX / 2);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t next_cycle = (last_tick + last_elapsed + ticks) * CYC_PER_TICK;

	arm_arch_timer_set_compare(next_cycle);
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
	uint64_t curr_cycle = arm_arch_timer_count();
	uint64_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = (cycle_diff_t)delta_cycles / CYC_PER_TICK;

	last_elapsed = delta_ticks;
	k_spin_unlock(&lock, key);
	return delta_ticks;
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
	arm_arch_timer_set_compare(last_cycle + CYC_PER_TICK);
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
#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
	cyc_per_tick = sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#endif
	arm_arch_timer_enable(true);
	last_tick = arm_arch_timer_count() / CYC_PER_TICK;
	last_cycle = last_tick * CYC_PER_TICK;
	arm_arch_timer_set_compare(last_cycle + CYC_PER_TICK);
	irq_enable(ARM_ARCH_TIMER_IRQ);
	arm_arch_timer_set_irq_mask(false);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
