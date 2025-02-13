/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

static const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));

enum stepper_mode {
	STEPPER_MODE_PING_PONG_RELATIVE,
	STEPPER_MODE_PING_PONG_ABSOLUTE,
	STEPPER_MODE_ROTATE_CW,
	STEPPER_MODE_ROTATE_CCW,
};

static atomic_t stepper_mode = ATOMIC_INIT(STEPPER_MODE_PING_PONG_RELATIVE);

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

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	enum stepper_mode mode = atomic_get(&stepper_mode);

	if (mode == STEPPER_MODE_ROTATE_CCW) {
		atomic_set(&stepper_mode, STEPPER_MODE_PING_PONG_RELATIVE);
	} else {
		atomic_inc(&stepper_mode);
	}
	k_sem_give(&stepper_generic_sem);
}

int main(void)
{
	printf("Starting generic stepper sample\n");
	if (!device_is_ready(stepper)) {
		printf("Device %s is not ready\n", stepper->name);
		return -ENODEV;
	}
	printf("stepper is %p, name is %s\n", stepper, stepper->name);

	if (!gpio_is_ready_dt(&button)) {
		printf("Error: button device %s is not ready\n", button.port->name);
		return 0;
	}

	if (gpio_pin_configure_dt(&button, GPIO_INPUT) != 0) {
		printf("Failed to configure %s pin %d\n", button.port->name, button.pin);
		return 0;
	}

	if (gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE) != 0) {
		printf("Failed to configure interrupt on %s pin %d\n", button.port->name,
		       button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	stepper_set_event_callback(stepper, stepper_callback, NULL);
	stepper_enable(stepper, true);
	stepper_set_reference_position(stepper, 0);
	stepper_set_microstep_interval(stepper, 1000000);
	stepper_move_by(stepper, ping_pong_target_position);

	for (;;) {
		k_sem_take(&stepper_generic_sem, K_FOREVER);
		switch (atomic_get(&stepper_mode)) {
		case STEPPER_MODE_ROTATE_CW:
			stepper_run(stepper, STEPPER_DIRECTION_POSITIVE);
			printf("mode: rotate cw\n");
			break;
		case STEPPER_MODE_ROTATE_CCW:
			stepper_run(stepper, STEPPER_DIRECTION_NEGATIVE);
			printf("mode: rotate ccw\n");
			break;
		case STEPPER_MODE_PING_PONG_RELATIVE:
			ping_pong_target_position *= -1;
			stepper_move_by(stepper, ping_pong_target_position);
			printf("mode: ping pong relative\n");
			break;
		case STEPPER_MODE_PING_PONG_ABSOLUTE:
			ping_pong_target_position *= -1;
			stepper_move_to(stepper, ping_pong_target_position);
			printf("mode: ping pong absolute\n");
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
		printf("Actual position: %d\n", actual_position);
		k_sleep(K_MSEC(CONFIG_MONITOR_THREAD_TIMEOUT_MS));
	}
}

K_THREAD_DEFINE(monitor_tid, CONFIG_MONITOR_THREAD_STACK_SIZE, monitor_thread, NULL, NULL, NULL, 5,
		0, 0);
