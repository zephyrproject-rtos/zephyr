/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Backend-independent CPU load feature layer: current-CPU getter, periodic
 * report timer and threshold callback. The actual measurement is provided by
 * the selected backend through cpu_load_get_cpu().
 */

#include <zephyr/sys/cpu_load.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_load, CONFIG_CPU_LOAD_LOG_LEVEL);

#ifndef CONFIG_CPU_LOAD_LOG_PERIODICALLY
#define CONFIG_CPU_LOAD_LOG_PERIODICALLY 0
#endif

static cpu_load_cb_t load_cb;
static uint8_t cpu_load_threshold_percent;

int cpu_load_get(bool reset)
{
	unsigned int cpu_id = 0U;

#ifdef CONFIG_SMP
	cpu_id = arch_curr_cpu()->id;
#endif

	return cpu_load_get_cpu(cpu_id, reset);
}

/* Start a fresh measurement window on every CPU. Only safe once the secondary
 * CPUs are up, i.e. not from a POST_KERNEL initialization.
 */
static void cpu_load_reset_all(void)
{
	for (unsigned int cpu = 0U; cpu < arch_num_cpus(); cpu++) {
		(void)cpu_load_get_cpu(cpu, true);
	}
}

/* Report every CPU, not just the one this timer happens to expire on: which CPU
 * runs the timer is arbitrary on SMP, and it is usually not the loaded one.
 */
static void cpu_load_log_fn(struct k_timer *dummy)
{
	for (unsigned int cpu = 0U; cpu < arch_num_cpus(); cpu++) {
		int load = cpu_load_get_cpu(cpu, true);
		uint32_t percent;
		uint32_t fraction;

		if (load < 0) {
			continue;
		}

		percent = load / 10;
		fraction = load % 10;

		LOG_INF("CPU%u load: %u.%u%%", cpu, percent, fraction);

		if (load_cb != NULL && percent >= cpu_load_threshold_percent) {
			load_cb(percent);
		}
	}
}

K_TIMER_DEFINE(cpu_load_timer, cpu_load_log_fn, NULL);

void cpu_load_log_control(bool enable)
{
	if (CONFIG_CPU_LOAD_LOG_PERIODICALLY == 0) {
		return;
	}
	if (enable) {
		cpu_load_reset_all();
		k_timer_start(&cpu_load_timer, K_MSEC(CONFIG_CPU_LOAD_LOG_PERIODICALLY),
			      K_MSEC(CONFIG_CPU_LOAD_LOG_PERIODICALLY));
	} else {
		k_timer_stop(&cpu_load_timer);
	}
}

int cpu_load_cb_reg(cpu_load_cb_t cb, uint8_t threshold_percent)
{
	if (threshold_percent > 100) {
		return -EINVAL;
	}

	cpu_load_threshold_percent = threshold_percent;
	load_cb = cb;
	return 0;
}

#if CONFIG_CPU_LOAD_LOG_PERIODICALLY > 0
static int cpu_load_log_init(void)
{
	/* Only the current CPU may be touched here: the secondary CPUs are
	 * initialized at INIT_LEVEL_SMP, which runs after POST_KERNEL, so their
	 * per-CPU kernel state does not exist yet. They simply start their first
	 * measurement window at the first report.
	 */
	(void)cpu_load_get(true);
	k_timer_start(&cpu_load_timer, K_MSEC(CONFIG_CPU_LOAD_LOG_PERIODICALLY),
		      K_MSEC(CONFIG_CPU_LOAD_LOG_PERIODICALLY));

	return 0;
}

SYS_INIT(cpu_load_log_init, POST_KERNEL, 0);
#endif
