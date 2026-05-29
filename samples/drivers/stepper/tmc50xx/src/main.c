/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_tmc50xx, CONFIG_STEPPER_LOG_LEVEL);

static const struct device *stepper_ctrl = DEVICE_DT_GET(DT_ALIAS(stepper_ctrl));
static const struct device *stepper_driver = DEVICE_DT_GET(DT_ALIAS(stepper_driver));

int32_t ping_pong_target_position = CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV *
				    DT_PROP(DT_ALIAS(stepper_driver), micro_step_res);

K_SEM_DEFINE(steps_completed_sem, 0, 1);

void stepper_callback(const struct device *dev, const enum stepper_ctrl_event event,
		      void *user_data)
{
	ARG_UNUSED(user_data);
	switch (event) {
	case STEPPER_CTRL_EVENT_STEPS_COMPLETED:
		k_sem_give(&steps_completed_sem);
		break;
	default:
		break;
	}
}

int main(void)
{
	LOG_INF("Starting tmc50xx stepper sample");
	if (!device_is_ready(stepper_ctrl)) {
		LOG_ERR("Device %s is not ready", stepper_ctrl->name);
		return -ENODEV;
	}
	LOG_DBG("stepper is %p, name is %s", stepper_ctrl, stepper_ctrl->name);

	stepper_ctrl_set_event_cb(stepper_ctrl, stepper_callback, NULL);
	stepper_enable(stepper_driver);

	enum stepper_micro_step_resolution micro_step_res;

	stepper_get_micro_step_res(stepper_driver, &micro_step_res);
	LOG_DBG("Microstep resolution is %d", micro_step_res);

	stepper_ctrl_set_reference_position(stepper_ctrl, 0);
	stepper_ctrl_move_by(stepper_ctrl, ping_pong_target_position);

	/* Change Max Velocity during runtime */
	int32_t tmc_velocity;

	tmc_velocity = DT_PROP(DT_ALIAS(stepper_ctrl), vmax) * CONFIG_MAX_VELOCITY_MULTIPLIER;
	(void)tmc50xx_stepper_ctrl_set_max_velocity(stepper_ctrl, tmc_velocity);

	for (;;) {
		if (k_sem_take(&steps_completed_sem, K_FOREVER) == 0) {
			ping_pong_target_position *= -1;
			stepper_ctrl_move_by(stepper_ctrl, ping_pong_target_position);

			int32_t actual_position;
			int ret;

			ret = stepper_ctrl_get_actual_position(stepper_ctrl, &actual_position);
			if (ret == 0) {
				LOG_INF("Actual position: %d", actual_position);
			} else {
				LOG_ERR("Failed to get actual position");
			}
		}
	}
	return 0;
}
