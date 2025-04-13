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

static const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));

enum stepper_mode {
	STEPPER_MODE_ENABLE,
	STEPPER_MODE_PING_PONG_RELATIVE,
	STEPPER_MODE_PING_PONG_ABSOLUTE,
	STEPPER_MODE_ROTATE_CW,
	STEPPER_MODE_ROTATE_CCW,
	STEPPER_MODE_STOP,
	STEPPER_MODE_DISABLE,
};

static atomic_t stepper_mode = ATOMIC_INIT(STEPPER_MODE_DISABLE);

static int32_t ping_pong_target_position =
	CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV * DT_PROP(DT_ALIAS(stepper), micro_step_res);

static K_SEM_DEFINE(stepper_generic_sem, 0, 1);

static void stepper_callback(const struct device *dev, const enum stepper_event event,
			     void *user_data)
{
	switch (event) {
	case STEPPER_EVENT_STEPS_COMPLETED:
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
	enum stepper_mode mode = atomic_get(&stepper_mode);

	if (mode == STEPPER_MODE_DISABLE) {
		atomic_set(&stepper_mode, STEPPER_MODE_ENABLE);
	} else {
		atomic_inc(&stepper_mode);
	}
	k_sem_give(&stepper_generic_sem);
}

INPUT_CALLBACK_DEFINE(NULL, button_pressed, NULL);

int main(void)
{
	LOG_INF("Starting generic stepper sample\n");
	if (!device_is_ready(stepper)) {
		LOG_ERR("Device %s is not ready\n", stepper->name);
		return -ENODEV;
	}
	LOG_DBG("stepper is %p, name is %s\n", stepper, stepper->name);

	stepper_set_event_callback(stepper, stepper_callback, NULL);
	stepper_set_reference_position(stepper, 0);
	stepper_set_microstep_interval(stepper, CONFIG_STEP_INTERVAL_NS);

	for (;;) {
		k_sem_take(&stepper_generic_sem, K_FOREVER);
		switch (atomic_get(&stepper_mode)) {
		case STEPPER_MODE_ENABLE:
			stepper_enable(stepper);
			LOG_INF("mode: enable\n");
			break;
		case STEPPER_MODE_STOP:
			stepper_stop(stepper);
			LOG_INF("mode: stop\n");
			break;
		case STEPPER_MODE_ROTATE_CW:
			stepper_run(stepper, STEPPER_DIRECTION_POSITIVE);
			LOG_INF("mode: rotate cw\n");
			break;
		case STEPPER_MODE_ROTATE_CCW:
			stepper_run(stepper, STEPPER_DIRECTION_NEGATIVE);
			LOG_INF("mode: rotate ccw\n");
			break;
		case STEPPER_MODE_PING_PONG_RELATIVE:
			ping_pong_target_position *= -1;
			stepper_move_by(stepper, ping_pong_target_position);
			LOG_INF("mode: ping pong relative\n");
			break;
		case STEPPER_MODE_PING_PONG_ABSOLUTE:
			ping_pong_target_position *= -1;
			stepper_move_to(stepper, ping_pong_target_position);
			LOG_INF("mode: ping pong absolute\n");
			break;
		case STEPPER_MODE_DISABLE:
			stepper_disable(stepper);
			LOG_INF("mode: disable\n");
			break;
		}
	}
	return 0;
}

static void monitor_thread(void)
{
	for (;;) {
		int32_t actual_position;

		stepper_get_actual_position(stepper, &actual_position);
		LOG_DBG("Actual position: %d\n", actual_position);
		k_sleep(K_MSEC(CONFIG_MONITOR_THREAD_TIMEOUT_MS));
	}
}

K_THREAD_DEFINE(monitor_tid, CONFIG_MONITOR_THREAD_STACK_SIZE, monitor_thread, NULL, NULL, NULL, 5,
		0, 0);
