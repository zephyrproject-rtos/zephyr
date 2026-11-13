/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include "timer.h"

#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>

#if !DT_NODE_HAS_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
#include <fsl_clock.h>
#endif

/*
 * timing_cycles_get() counts CPU cycles (DWT CYCCNT), so the cycles->us
 * conversion needs the exact core frequency, not the system-timer rate.
 *
 * Prefer the CPU node's clock-frequency when devicetree supplies it: on
 * i.MX RT2660 SysTick counts an external reference, so timing_freq_get()
 * would report the timer rate rather than the ~1 GHz core/DWT rate.
 *
 * When the CPU node has no clock-frequency (MCXN947, i.MX RT700, where the
 * core rate depends on board clock setup), query the MCUX HAL, which derives
 * the exact rate from the clock registers. This avoids the timing subsystem
 * fallback, which only runtime-calibrates an approximate DWT rate on those
 * SoCs.
 */
uint64_t timer_cpu_hz(void)
{
#if DT_NODE_HAS_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
	return (uint64_t)DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);
#else
	return (uint64_t)CLOCK_GetCoreSysClkFreq();
#endif
}

void timer_start(struct timer *t)
{
	timing_init();
	timing_start();
	t->start = timing_counter_get();
}

void timer_stop(struct timer *t, struct timer_result *res)
{
	t->end = timing_counter_get();
	timing_stop();

	res->cycles = timing_cycles_get(&t->start, &t->end);
	res->us = (res->cycles * 1000000ULL) / timer_cpu_hz();
}

void timer_report(const char *label, const struct timer_result *res)
{
	printk("%s: %llu us (%llu cpu cycles @ %llu MHz)\n", label, res->us, res->cycles,
	       timer_cpu_hz() / 1000000ULL);
}
