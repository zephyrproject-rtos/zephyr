/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <Cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc522x_sample, CONFIG_STEPPER_LOG_LEVEL);

static const struct device *stepper_ctrl_dev = DEVICE_DT_GET(DT_ALIAS(stepper));
static const struct device *stepper_drv_dev = DEVICE_DT_GET(DT_ALIAS(stepper_drv));

int32_t ping_pong_target_position = CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV *
				    DT_PROP(DT_ALIAS(stepper_drv), micro_step_res);

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
	LOG_INF("Starting TMC522x stepper sample");

	if (!device_is_ready(stepper_ctrl_dev)) {
		LOG_ERR("Stepper controller %s is not ready", stepper_ctrl_dev->name);
		return -ENODEV;
	}
	if (!device_is_ready(stepper_drv_dev)) {
		LOG_ERR("Stepper driver %s is not ready", stepper_drv_dev->name);
		return -ENODEV;
	}

	stepper_ctrl_set_event_cb(stepper_ctrl_dev, stepper_callback, NULL);
	stepper_enable(stepper_drv_dev);

	enum stepper_micro_step_resolution micro_step_res;

	stepper_get_micro_step_res(stepper_drv_dev, &micro_step_res);
	LOG_INF("Microstep resolution: %d", micro_step_res);

	stepper_ctrl_set_reference_position(stepper_ctrl_dev, 0);
	stepper_ctrl_move_by(stepper_ctrl_dev, ping_pong_target_position);

	for (;;) {
		if (k_sem_take(&steps_completed_sem, K_FOREVER) == 0) {
			int32_t actual_position;
			int ret;

			ret = stepper_ctrl_get_actual_position(stepper_ctrl_dev, &actual_position);
			if (ret == 0) {
				LOG_INF("Actual position: %d", actual_position);
			} else {
				LOG_ERR("Failed to get actual position");
			}

			ping_pong_target_position *= -1;
			stepper_ctrl_move_by(stepper_ctrl_dev, ping_pong_target_position);
		}
	}
	return 0;
}
