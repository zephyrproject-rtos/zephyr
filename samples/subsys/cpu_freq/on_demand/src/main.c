/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_freq_sample, LOG_LEVEL_INF);

#define MS_TO_US(x)      ((x) * 1000)
#define PERCENT_SLEEP_MS (CONFIG_CPU_FREQ_INTERVAL_MS / 100)

typedef enum {
	CPU_SCENARIO_0,
	CPU_SCENARIO_1,
	CPU_SCENARIO_2,
	CPU_SCENARIO_3,
	CPU_SCENARIO_4,
	CPU_SCENARIO_COUNT
} cpu_scenario_t;

static int curr_work_time_ms;
static cpu_scenario_t cpu_scenario = CPU_SCENARIO_0;

static void update_sleep_time(struct k_timer *timer_id)
{
	cpu_scenario = (cpu_scenario + 1) % CPU_SCENARIO_COUNT;

	switch (cpu_scenario) {
	case CPU_SCENARIO_0:
		/* CPU is always in sleep */
		curr_work_time_ms = 0;
		break;
	case CPU_SCENARIO_1:
		/* CPU doing some intermittent processing */
		curr_work_time_ms = PERCENT_SLEEP_MS * 10;
		break;
	case CPU_SCENARIO_2:
		/* CPU doing some more intense processing */
		curr_work_time_ms = PERCENT_SLEEP_MS * 25;
		break;
	case CPU_SCENARIO_3:
		/* CPU doing some more intense processing */
		curr_work_time_ms = PERCENT_SLEEP_MS * 50;
		break;
	case CPU_SCENARIO_4:
		/* CPU doing lots of calculations */
		curr_work_time_ms = PERCENT_SLEEP_MS * 100;
		break;
	default:
		LOG_ERR("Unknown CPU scenario: %d", cpu_scenario);
	}
}

K_TIMER_DEFINE(timer, update_sleep_time, NULL);

int main(void)
{
	LOG_INF("Starting CPU Freq Subsystem Sample!");

	curr_work_time_ms = 0;

	/* Set timer to change cpu scenario periodically */
	k_timer_start(&timer, K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS),
		      K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS));

	while (1) {
		k_busy_wait(MS_TO_US(curr_work_time_ms));
		k_msleep(CONFIG_CPU_FREQ_INTERVAL_MS - curr_work_time_ms);
	}
}
