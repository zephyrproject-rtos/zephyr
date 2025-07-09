/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>

LOG_MODULE_REGISTER(cpu_freq, CONFIG_CPU_FREQ_LOG_LEVEL);

static void cpu_freq_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(cpu_freq_timer, cpu_freq_timer_handler, NULL);

/*
 * Timer that expires periodically to execute the selected policy algorithm
 * and pass the next P-state to the P-state driver.
 */
static void cpu_freq_timer_handler(struct k_timer *timer)
{
	int ret;

	/* Get next performance state */
	const struct pstate *pstate_next;

	ret = cpu_freq_policy_select_pstate(&pstate_next);
	if (ret) {
		LOG_ERR("Failed to get pstate: %d", ret);
		return;
	}

	/* Set performance state using pstate driver */
	ret = cpu_freq_pstate_set(pstate_next);
	if (ret) {
		LOG_ERR("Failed to set performance state: %d", ret);
		return;
	}
}

static int cpu_freq_init(void)
{
	k_timer_start(&cpu_freq_timer, K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS),
		      K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS));
	LOG_INF("CPU frequency subsystem initialized with interval %d ms",
		CONFIG_CPU_FREQ_INTERVAL_MS);
	return 0;
}

SYS_INIT(cpu_freq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
