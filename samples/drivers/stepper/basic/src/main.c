/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>

const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));

int32_t ping_pong_target_position = CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV *
				    DT_PROP(DT_ALIAS(stepper), micro_step_res);

atomic_t is_position_reached = ATOMIC_INIT(0);

void stepper_callback(const struct device *dev, const enum stepper_event event, void *user_data)
{
	switch (event) {
	case STEPPER_EVENT_STEPS_COMPLETED:
		printf("%s steps completed changing direction\n", __func__);
		ping_pong_target_position *= -1;
		atomic_set(&is_position_reached, 1);
		break;
	default:
		break;
	}
}

int main(void)
{
	printf("Starting basic stepper sample\n");
	if (!device_is_ready(stepper)) {
		printf("Device %s is not ready\n", stepper->name);
		return -ENODEV;
	}
	printf("stepper is %p, name is %s\n", stepper, stepper->name);

	stepper_set_event_callback(stepper, stepper_callback, NULL);
	stepper_enable(stepper, true);
	stepper_set_reference_position(stepper, 0);
	stepper_move_by(stepper, ping_pong_target_position);

	for (;;) {
		if (atomic_get(&is_position_reached)) {
			atomic_clear(&is_position_reached);
			stepper_move_by(stepper, ping_pong_target_position);
		}
	}
	return 0;
}
