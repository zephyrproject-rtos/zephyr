/*
 * Copyright (c) 2025 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_freq_pressure_sample, LOG_LEVEL_INF);

#define SAMPLE_SLEEP_MS 1500

#define STACK_SIZE 1024
#define LOW_PRIO 10
#define MED_PRIO 5
#define HIGH_PRIO 1

K_THREAD_STACK_DEFINE(low_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(med_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(high_stack, STACK_SIZE);

struct k_thread low_thread_data;
struct k_thread med_thread_data;
struct k_thread high_thread_data;

static void thread_fn(void *arg1, void *arg2, void *arg3)
{
	(void)arg1;
	(void)arg2;
	(void)arg3;

	while (1) {
		k_busy_wait(10000000);
	}

	k_yield();
}

int main(void)
{
	LOG_INF("Starting CPU Freq Pressure Policy Sample!");

	/* Create threads with varying priorities and switch between them
	 * to increase/decrease pressure in the queue
	 */
	k_thread_create(&low_thread_data, low_stack, STACK_SIZE,
			thread_fn, NULL, NULL, NULL,
			LOW_PRIO, 0, K_NO_WAIT);

	k_thread_create(&med_thread_data, med_stack, STACK_SIZE,
			thread_fn, NULL, NULL, NULL,
			MED_PRIO, 0, K_NO_WAIT);

	k_thread_create(&high_thread_data, high_stack, STACK_SIZE,
			thread_fn, NULL, NULL, NULL,
			HIGH_PRIO, 0, K_NO_WAIT);

	static int mode;

	mode = 0;

	while (1) {
		mode = (mode % 3) + 1;

		switch (mode) {
		case 1:
			k_thread_resume(&low_thread_data);
			k_thread_suspend(&med_thread_data);
			k_thread_suspend(&high_thread_data);
			LOG_INF("Mode 1: Low only");
			break;
		case 2:
			k_thread_resume(&low_thread_data);
			k_thread_resume(&med_thread_data);
			k_thread_suspend(&high_thread_data);
			LOG_INF("Mode 2: Low + Medium");
			break;
		case 3:
			k_thread_resume(&low_thread_data);
			k_thread_resume(&med_thread_data);
			k_thread_resume(&high_thread_data);
			LOG_INF("Mode 3: Low + Medium + High");
			break;
		default:
			LOG_ERR("Unknown mode!");
			return -1;
		}

		k_msleep(SAMPLE_SLEEP_MS);
	}
}
