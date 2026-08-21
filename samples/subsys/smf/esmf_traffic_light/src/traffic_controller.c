/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "traffic_events.h"

#define THREAD_STACK_SIZE 1024
#define THREAD_PRIORITY   5

#define LOOP_PERIOD_MS 1000

static void traffic_controller_thread(void *p1, void *p2, void *p3)
{
	int loop_count = 0;

	while (1) {
		(void)traffic_post_event(TRIG_TICK, "tick(auto)", K_NO_WAIT);

		if ((loop_count % 3) == 0) {
			(void)traffic_post_event(TRIG_TIMER, "timer(auto)", K_NO_WAIT);
		}

		if ((loop_count % 7) == 0) {
			(void)traffic_post_event(TRIG_DIAG_TICK, "diag(auto)", K_NO_WAIT);
		}

		if ((loop_count % 11) == 0) {
			(void)traffic_post_event(TRIG_PED_BUTTON, "ped(auto)", K_NO_WAIT);
		}

		if (loop_count == 14) {
			(void)traffic_post_event(TRIG_EMERGENCY, "emergency(auto)", K_NO_WAIT);
		}

		if (loop_count == 18) {
			(void)traffic_post_event(TRIG_RESET, "reset(auto)", K_NO_WAIT);
		}

		loop_count++;
		k_sleep(K_MSEC(LOOP_PERIOD_MS));
	}
}

K_THREAD_DEFINE(traffic_controller, THREAD_STACK_SIZE, traffic_controller_thread, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, 0);
