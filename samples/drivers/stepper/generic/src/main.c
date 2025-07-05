/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper, CONFIG_STEPPER_LOG_LEVEL);

#define HZ_TO_NS(hz) DIV_ROUND_CLOSEST(1000000000, hz)

static const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));

struct stepper_ramp_profile ramp_constant_profile = {
	.type = STEPPER_RAMP_TYPE_SQUARE,
	.square =
		{
			.interval_ns = HZ_TO_NS(5000),
		},
};

struct stepper_ramp_profile ramp_trapezoidal_profile = {
	.type = STEPPER_RAMP_TYPE_TRAPEZOIDAL,
	.trapezoidal =
		{
			.interval_ns = HZ_TO_NS(100000),
			.acceleration_rate = 5000,
			.deceleration_rate = 5000,
		},
};

static struct stepper_ramp_profile *current_ramp_profile = &ramp_trapezoidal_profile;

enum stepper_mode {
	STEPPER_MODE_ENABLE,
	STEPPER_MODE_PING_PONG_RELATIVE,
	STEPPER_MODE_PING_PONG_ABSOLUTE,
	STEPPER_MODE_ROTATE_CW,
	STEPPER_MODE_ROTATE_CCW,
	STEPPER_MODE_STOP,
	STEPPER_MODE_DISABLE,

	STEPPER_MODE_COUNT,
};

static enum stepper_mode current_mode = STEPPER_MODE_DISABLE;

static int32_t ping_pong_target_position =
	CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV * DT_PROP(DT_ALIAS(stepper), micro_step_res);

static K_SEM_DEFINE(stepper_generic_sem, 0, 1);

static void stepper_callback(const struct device *dev, const enum stepper_event event,
			     void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (event) {
	case STEPPER_EVENT_STEPS_COMPLETED:
		LOG_DBG("Steps completed");
		k_sem_give(&stepper_generic_sem);
		break;
	default:
		break;
	}
}

static void button_pressed(struct input_event *event, void *user_data)
{
	ARG_UNUSED(user_data);

	if (event->value == 0 && event->type == INPUT_EV_KEY) {
		return;
	}

	current_mode = (current_mode + 1) % STEPPER_MODE_COUNT;

	if (current_mode == STEPPER_MODE_ENABLE) {
		if (current_ramp_profile->type == STEPPER_RAMP_TYPE_SQUARE) {
			LOG_INF("Ramp type: square");
			current_ramp_profile = &ramp_trapezoidal_profile;
		} else {
			LOG_INF("Ramp type: trapezoidal");
			current_ramp_profile = &ramp_constant_profile;
		}
		stepper_set_ramp(stepper, current_ramp_profile);
	}

	k_sem_give(&stepper_generic_sem);
}

INPUT_CALLBACK_DEFINE(NULL, button_pressed, NULL);

int main(void)
{
	LOG_INF("Starting generic stepper sample");
	if (!device_is_ready(stepper)) {
		LOG_ERR("Device %s is not ready", stepper->name);
		return -ENODEV;
	}
	LOG_DBG("stepper is %p, name is %s", stepper, stepper->name);

	stepper_set_event_callback(stepper, stepper_callback, NULL);
	stepper_set_reference_position(stepper, 0);

	do {
		switch (current_mode) {
		case STEPPER_MODE_ENABLE:
			LOG_INF("mode: enable");
			stepper_enable(stepper);
			break;
		case STEPPER_MODE_STOP:
			LOG_INF("mode: stop");
			stepper_stop(stepper);
			break;
		case STEPPER_MODE_ROTATE_CW:
			LOG_INF("mode: rotate cw");
			stepper_run(stepper, STEPPER_DIRECTION_POSITIVE);
			break;
		case STEPPER_MODE_ROTATE_CCW:
			LOG_INF("mode: rotate ccw");
			stepper_run(stepper, STEPPER_DIRECTION_NEGATIVE);
			break;
		case STEPPER_MODE_PING_PONG_RELATIVE:
			LOG_INF("mode: ping pong relative");
			ping_pong_target_position *= -1;
			stepper_move_by(stepper, ping_pong_target_position);
			break;
		case STEPPER_MODE_PING_PONG_ABSOLUTE:
			LOG_INF("mode: ping pong absolute");
			ping_pong_target_position *= -1;
			stepper_move_to(stepper, ping_pong_target_position);
			break;
		case STEPPER_MODE_DISABLE:
			LOG_INF("mode: disable");
			stepper_disable(stepper);
			break;
		default:
			LOG_ERR("Unknown mode");
			break;
		}
	} while (k_sem_take(&stepper_generic_sem, K_FOREVER) == 0);

	return 0;
}

static void monitor_thread(void)
{
	for (;;) {
		int32_t actual_position;

		stepper_get_actual_position(stepper, &actual_position);
		LOG_DBG("Actual position: %d", actual_position);
		k_sleep(K_MSEC(CONFIG_MONITOR_THREAD_TIMEOUT_MS));
	}
}

K_THREAD_DEFINE(monitor_tid, CONFIG_MONITOR_THREAD_STACK_SIZE, monitor_thread, NULL, NULL, NULL, 5,
		0, 0);
