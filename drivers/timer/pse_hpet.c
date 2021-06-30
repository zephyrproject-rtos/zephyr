/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include "pse_hpet.h"

static inline void wait_for_idle(uint32_t bits)
{
	while (CONTROL_AND_STATUS_REG & bits) {
		;
	}
}

static struct k_spinlock lock;
static unsigned int max_ticks;
static unsigned int cyc_per_tick;
static uint64_t last_count;

/**
 *
 * @brief Safely read the main HPET up counter
 *
 * This routine simulates an atomic read of the 64-bit system clock on CPUs
 * that only support 32-bit memory accesses. The most significant word
 * of the counter is read twice to ensure it doesn't change while the least
 * significant word is being retrieved (as per HPET documentation).
 *
 * @return current 64-bit counter value
 */
static uint64_t hpet_main_counter_atomic(void)
{
	uint32_t high_bits;
	uint32_t low_bits;

	do {
		high_bits = MAIN_COUNTER_MSW_REG;
		low_bits = MAIN_COUNTER_LSW_REG;
	} while (high_bits != MAIN_COUNTER_MSW_REG);

	return ((uint64_t)high_bits << 32) | low_bits;
}

#if defined(CONFIG_TICKLESS_KERNEL) && !defined(CONFIG_QEMU_TICKLESS_WORKAROUND)
static uint64_t hpet_tick2counter(uint64_t ticks)
{
	/*
	 * HPET clock is 32768Hz, so for 10ms tick, we can't just
	 * use 32768/100 = 327 as a tick interval, there will be
	 * 32768 - 327 * 100 = 68 (~2075us) error in 1 second.
	 *
	 * currently we try to fix the error by adjusting the counter
	 * 4 times in 1 second.
	 */

	uint64_t no_err_ticks = ticks / HPET_OS_TICKS_PER_ERR_FIX;
	uint64_t err_ticks = ticks - (no_err_ticks * HPET_OS_TICKS_PER_ERR_FIX);

	uint64_t no_err_counters = no_err_ticks * HPET_COUNTERS_PER_ERR_FIX;
	uint64_t err_counters = err_ticks * HPET_COUNTERS_PER_OS_TICK;

	return no_err_counters + err_counters;
}
#endif

static uint64_t hpet_counter2tick(uint64_t counters)
{
	uint64_t no_err_counters = counters / HPET_COUNTERS_PER_ERR_FIX;
	uint64_t err_counters = counters - no_err_counters *
				HPET_COUNTERS_PER_ERR_FIX;

	uint64_t no_err_ticks = no_err_counters * HPET_OS_TICKS_PER_ERR_FIX;
	uint64_t err_ticks = err_counters / HPET_COUNTERS_PER_OS_TICK;

	return no_err_ticks + err_ticks;
}


static void _hpet_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	wait_for_idle(GENERAL_INT_STATUS);
	GENERAL_INT_STATUS_REG = 1;

	uint64_t now = hpet_main_counter_atomic();
	uint64_t dticks = hpet_counter2tick(now - last_count);

	last_count = now;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL) ||
	    IS_ENABLED(CONFIG_QEMU_TICKLESS_WORKAROUND)) {
		uint64_t next = last_count + cyc_per_tick;

		if ((int64_t)(next - now) < MIN_DELAY) {
			next += cyc_per_tick;
		}
		wait_for_idle(TIMER0_CONFIG_CAPS | TIMER0_COMPARATOR);
		TIMER0_COMPARATOR_REG = next;
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}


int sys_clock_driver_init(const struct device *dev)
{
	extern int z_clock_hw_cycles_per_sec;

	ARG_UNUSED(dev);

	wait_for_idle(GENERAL_CONFIG | MAIN_COUNTER_VALUE |
		      TIMER0_CONFIG_CAPS | TIMER0_COMPARATOR);

	/*
	 * Initial state of HPET is unknown, so put it back in a reset-like
	 * state (i.e. set main counter to 0 and disable interrupts)
	 */

	GENERAL_CONF_REG &= ~HPET_ENABLE_CNF;
	MAIN_COUNTER_REG = 0;

	uint32_t hz = (uint32_t)(PISECONDS_PER_SECOND / CLK_PERIOD_REG);

	z_clock_hw_cycles_per_sec = hz;
	cyc_per_tick = hz / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	max_ticks = TICK_MAX;
	last_count = 0;

	/*
	 * HPET timers IRQ field is 5 bits wide, and hence, can support only
	 * IRQ's up to 31. Some platforms, however, use IRQs greater than 31.
	 * In this case program leaves the IRQ fields blank.
	 *
	 * HPET is set to ONE-SHOT mode
	 */

	TIMER0_CONF_REG =
		(TIMER0_CONF_REG & ~HPET_Tn_INT_ROUTE_CNF_MASK) |
		(CONFIG_HPET_TIMER_IRQ << HPET_Tn_INT_ROUTE_CNF_SHIFT)
		| HPET_Tn_INT_TYPE_CNF;

	TIMER0_COMPARATOR_REG = cyc_per_tick;

	/*
	 * Although the stub has already been "connected", the vector number
	 * still has to be programmed into the interrupt controller.
	 */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    _hpet_isr, 0, 0);

	irq_enable(CONFIG_HPET_TIMER_IRQ);

	/* enable other hpet interrupt router before overall enable */
	wait_for_idle(TIMER1_CONFIG_CAPS);
	TIMER1_CONF_REG =
		(TIMER1_CONF_REG & ~HPET_Tn_INT_ROUTE_CNF_MASK)
		| HPET_Tn_INT_TYPE_CNF;
	wait_for_idle(TIMER2_CONFIG_CAPS);
	TIMER2_CONF_REG =
		(TIMER2_CONF_REG & ~HPET_Tn_INT_ROUTE_CNF_MASK) |
		(TIMER2_INT_ROUTE << HPET_Tn_INT_ROUTE_CNF_SHIFT)
		| HPET_Tn_INT_TYPE_CNF;

	/* enable the HPET generally, and timer0 specifically */
	wait_for_idle(GENERAL_CONFIG | TIMER0_CONFIG_CAPS);

	GENERAL_CONF_REG |= (HPET_ENABLE_CNF | HPET_LEGACY_RT_CNF);
	TIMER0_CONF_REG |= HPET_Tn_INT_ENB_CNF;

	return 0;
}

void smp_timer_init(void)
{
	/* Noop, the HPET is a single system-wide device and it's
	 * configured to deliver interrupts to every CPU, so there's
	 * nothing to do at initialization on auxiliary CPUs.
	 */
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL) && !defined(CONFIG_QEMU_TICKLESS_WORKAROUND)
	wait_for_idle(GENERAL_CONFIG);
	if (ticks == K_TICKS_FOREVER && idle) {
		GENERAL_CONF_REG &= ~HPET_ENABLE_CNF;
		return;
	}

	ticks = ticks == K_TICKS_FOREVER ? max_ticks : ticks;
	ticks = MAX(MIN(ticks - 1, (int32_t)max_ticks), 0);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = hpet_main_counter_atomic(), cyc;

	/* Round up to next tick boundary */
	cyc = now + hpet_tick2counter(ticks);

	if ((cyc - now) < MIN_DELAY) {
		cyc += cyc_per_tick;
	}

	wait_for_idle(TIMER0_COMPARATOR | TIMER0_CONFIG_CAPS);
	TIMER0_CONF_REG |= TCONF_INT_ENABLE;
	TIMER0_COMPARATOR_REG = cyc;
	k_spin_unlock(&lock, key);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = (hpet_main_counter_atomic() - last_count) / cyc_per_tick;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)hpet_main_counter_atomic();
}

void sys_clock_idle_exit(void)
{
	wait_for_idle(GENERAL_CONFIG);
	GENERAL_CONF_REG |= HPET_ENABLE_CNF;
}
