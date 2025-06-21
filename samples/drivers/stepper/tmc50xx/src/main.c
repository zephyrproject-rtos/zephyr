/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_tmc50xx, CONFIG_STEPPER_LOG_LEVEL);

const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper_driver));
const struct device *stepper_controller = DEVICE_DT_GET(DT_ALIAS(motion_controller));

int32_t ping_pong_target_position = CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV *
				    DT_PROP(DT_ALIAS(stepper_driver), micro_step_res);

K_SEM_DEFINE(steps_completed_sem, 0, 1);

void stepper_callback(const struct device *dev, const enum stepper_event event, void *user_data)
{
	ARG_UNUSED(user_data);
	switch (event) {
	case STEPPER_EVENT_STEPS_COMPLETED:
		k_sem_give(&steps_completed_sem);
		break;
	default:
		break;
	}
}

int main(void)
{
	LOG_INF("Starting tmc50xx stepper sample");
	if (!device_is_ready(stepper)) {
		LOG_ERR("Device %s is not ready", stepper->name);
		return -ENODEV;
	}
	LOG_DBG("stepper is %p, name is %s", stepper, stepper->name);

	stepper_set_event_callback(stepper_controller, stepper_callback, NULL);
	stepper_drv_enable(stepper);

	enum stepper_drv_micro_step_resolution micro_step_res;

	stepper_drv_get_micro_step_res(stepper, &micro_step_res);
	LOG_DBG("Microstep resolution is %d", micro_step_res);

	stepper_set_reference_position(stepper_controller, 0);
	stepper_move_by(stepper_controller, ping_pong_target_position);

	/* Change Max Velocity during runtime */
	int32_t tmc_velocity;

	tmc_velocity = DT_PROP(DT_ALIAS(motion_controller), vmax) * CONFIG_MAX_VELOCITY_MULTIPLIER;
	(void)tmc50xx_stepper_set_max_velocity(stepper_controller, tmc_velocity);

	for (;;) {
		if (k_sem_take(&steps_completed_sem, K_FOREVER) == 0) {
			ping_pong_target_position *= -1;
			stepper_move_by(stepper_controller, ping_pong_target_position);

			int32_t actual_position;
			int ret;

			ret = stepper_get_actual_position(stepper_controller, &actual_position);
			if (ret == 0) {
				LOG_INF("Actual position: %d", actual_position);
			} else {
				LOG_ERR("Failed to get actual position");
			}
		}
	}
	return 0;
}
