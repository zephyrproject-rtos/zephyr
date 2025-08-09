/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));
const struct device *stepper_controller = DEVICE_DT_GET(DT_ALIAS(stepper_motion_control));
const uint8_t stepper_index = DT_REG_ADDR(DT_ALIAS(stepper));

int32_t ping_pong_target_position =
	CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV * DT_PROP(DT_ALIAS(stepper), micro_step_res);

K_SEM_DEFINE(steps_completed_sem, 0, 1);

void stepper_callback(const struct device *dev, const uint8_t stepper_idx,
		      const enum stepper_event event, void *user_data)
{
	ARG_UNUSED(stepper_idx);
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
	printf("Starting tmc50xx stepper sample\n");
	if (!device_is_ready(stepper)) {
		printf("Device %s is not ready\n", stepper->name);
		return -ENODEV;
	}
	printf("stepper is %p, name is %s\n", stepper, stepper->name);

	stepper_set_event_callback(stepper_controller, stepper_index, stepper_callback, NULL);
	stepper_drv_enable(stepper);

	enum stepper_drv_micro_step_resolution micro_step_res;

	stepper_drv_get_micro_step_res(stepper, &micro_step_res);
	printf("Microstep resolution is %d\n", micro_step_res);

	stepper_set_reference_position(stepper_controller, stepper_index, 0);
	stepper_move_by(stepper_controller, stepper_index, ping_pong_target_position);

	/* Change Max Velocity during runtime */
	int32_t tmc_velocity = DT_PROP(DT_ALIAS(stepper), vmax) * CONFIG_MAX_VELOCITY_MULTIPLIER;
	(void)tmc50xx_stepper_set_max_velocity(stepper_controller, stepper_index, tmc_velocity);

	for (;;) {
		if (k_sem_take(&steps_completed_sem, K_FOREVER) == 0) {
			ping_pong_target_position *= -1;
			stepper_move_by(stepper_controller, stepper_index,
					ping_pong_target_position);

			int32_t actual_position;

			if (stepper_get_actual_position(stepper_controller, stepper_index,
							&actual_position) == 0) {
				printf("Actual position: %d\n", actual_position);
			} else {
				printf("Failed to get actual position\n");
			}
		}
	}
	return 0;
}
