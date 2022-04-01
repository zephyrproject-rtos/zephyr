/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "measurement_event.h"
#include "control_event.h"
#include "ack_event.h"
#include "config_event.h"

#define SENSOR_SIMULATED_THREAD_STACK_SIZE 800
#define SENSOR_SIMULATED_THREAD_PRIORITY 1
#define SENSOR_SIMULATED_THREAD_SLEEP 500
#define MODULE sensor_sim

static K_THREAD_STACK_DEFINE(sensor_simulated_thread_stack,
			     SENSOR_SIMULATED_THREAD_STACK_SIZE);
static struct k_thread sensor_simulated_thread;

static int8_t value1;
static int16_t value2;
static int32_t value3;

static void measure_update(void)
{
	value2 += value1;
	value3 += value2;
}

static void measure(void)
{
	measure_update();

	struct measurement_event *event = new_measurement_event();

	event->value1 = value1;
	event->value2 = value2;
	event->value3 = value3;
	EVENT_SUBMIT(event);
}

static void sensor_simulated_thread_fn(void)
{
	while (true) {
		measure();
		k_sleep(K_MSEC(SENSOR_SIMULATED_THREAD_SLEEP));
	}
}

static void init(void)
{
	k_thread_create(&sensor_simulated_thread,
			sensor_simulated_thread_stack,
			SENSOR_SIMULATED_THREAD_STACK_SIZE,
			(k_thread_entry_t)sensor_simulated_thread_fn,
			NULL, NULL, NULL,
			SENSOR_SIMULATED_THREAD_PRIORITY,
			0, K_NO_WAIT);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_control_event(eh)) {
		value1 = -value1;
		struct ack_event *ack = new_ack_event();

		EVENT_SUBMIT(ack);
		return false;
	}

	if (is_config_event(eh)) {
		struct config_event *ce = cast_config_event(eh);

		value1 = ce->init_value1;
		init();
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, control_event);
EVENT_SUBSCRIBE(MODULE, config_event);
