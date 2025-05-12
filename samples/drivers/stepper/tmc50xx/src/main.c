/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper_control.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc50xx_sample, CONFIG_STEPPER_LOG_LEVEL);

const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));
const struct device *stepper_control = DEVICE_DT_GET(DT_ALIAS(stepper_controller));

int32_t ping_pong_target_position =
	CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV * DT_PROP(DT_ALIAS(stepper), micro_step_res);

K_SEM_DEFINE(steps_completed_sem, 0, 1);

void stepper_callback(const struct device *dev, const enum stepper_control_event event,
		      void *user_data)
{
	switch (event) {
	case STEPPER_CONTROL_EVENT_STEPS_COMPLETED:
		k_sem_give(&steps_completed_sem);
		break;
	default:
		break;
	}
}

int main(void)
{
	printf("Starting tmc50xx stepper sample\n");
	if (!device_is_ready(stepper)) {
		printf("Device %s is not ready\n", stepper->name);
		return -ENODEV;
	}
	LOG_DBG("stepper is %p, name is %s\n", stepper, stepper->name);

	if (!device_is_ready(stepper_control)) {
		printf("Device %s is not ready\n", stepper_control->name);
		return -ENODEV;
	}

	LOG_DBG("Stepper controller is %p, name is %s\n", stepper_control, stepper_control->name);
	stepper_control_set_event_callback(stepper_control, stepper_callback, NULL);

	LOG_DBG("Enabling stepper driver %s", stepper->name);
	stepper_enable(stepper);

	LOG_DBG("Setting reference position to 0 for %s", stepper_control->name);
	stepper_control_set_reference_position(stepper_control, 0);

	LOG_DBG("Moving to %d", ping_pong_target_position);
	stepper_control_move_by(stepper_control, ping_pong_target_position);

	/* Change Max Velocity during runtime */
	int32_t tmc_velocity =
		DT_PROP(DT_ALIAS(stepper_controller), vmax) * CONFIG_MAX_VELOCITY_MULTIPLIER;
	(void)tmc50xx_stepper_control_set_max_velocity(stepper_control, tmc_velocity);

	for (;;) {
		if (k_sem_take(&steps_completed_sem, K_FOREVER) == 0) {
			ping_pong_target_position *= -1;
			stepper_control_move_by(stepper_control, ping_pong_target_position);
		}
	}
	return 0;
}
