/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_freq_sample, LOG_LEVEL_INF);

typedef enum {
	CPU_SCENARIO_0,
	CPU_SCENARIO_1,
	CPU_SCENARIO_2,
	CPU_SCENARIO_4
} cpu_scenario_t;

static cpu_scenario_t cpu_scenario = CPU_SCENARIO_0;

static void update_sleep_time(struct k_timer *timer_id)
{
	cpu_scenario = (cpu_scenario + 1) % 4;
}

K_TIMER_DEFINE(timer, update_sleep_time, NULL);

int main(void)
{
	LOG_INF("Starting CPU Freq Subsystem Sample!\n");

	volatile int counter = 0;

	/* Set timer to change cpu scenario periodically */
	k_timer_start(&timer, K_MSEC(10000), K_MSEC(10000));

	while (1) {
		switch (cpu_scenario) {
		case CPU_SCENARIO_0:
			/* CPU is always in sleep */
			k_usleep(100000);
			break;
		case CPU_SCENARIO_1:
			/* CPU doing some intermittent processing */
			for (int i = 0; i < 1000; i++) {
				counter++;
			}
			k_usleep(200);
			break;
		case CPU_SCENARIO_2:
			/* CPU doing some more intense processing */
			for (int i = 0; i < 1000; i++) {
				counter++;
			}
			k_usleep(1);
			break;
		case CPU_SCENARIO_4:
			/* CPU doing lots of calculations */
			counter++;
			break;
		default:
			LOG_ERR("Unknown CPU scenario: %d", cpu_scenario);
			return -1;
		}
	}
}
