/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmcm3216_stepper_ctrl

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <adi_tmcm_rs485.h>
#include "tmcm3216.h"
#include "tmcm3216_reg.h"

/* Polling interval in milliseconds for motion status check */
#define TMCM3216_POLL_INTERVAL_MS 100

LOG_MODULE_DECLARE(tmcm3216, CONFIG_STEPPER_LOG_LEVEL);

/* Vendor-specific functions */

int tmcm3216_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;

	return tmcm3216_sap(dev, config->motor_index, TMCL_AP_MAX_VELOCITY, velocity);
}

int tmcm3216_get_max_velocity(const struct device *dev, uint32_t *velocity)
{
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;

	return tmcm3216_gap(dev, config->motor_index, TMCL_AP_MAX_VELOCITY, velocity);
}

int tmcm3216_set_max_acceleration(const struct device *dev, uint32_t acceleration)
{
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;

	return tmcm3216_sap(dev, config->motor_index, TMCL_AP_MAX_ACCELERATION, acceleration);
}

int tmcm3216_get_actual_velocity(const struct device *dev, int32_t *velocity)
{
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;
	uint32_t raw;
	int err;

	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_ACTUAL_VELOCITY, &raw);
	if (err != 0) {
		return err;
	}

	*velocity = sign_extend(raw, TMCM3216_DATA_BITS);
	return 0;
}

int tmcm3216_get_status(const struct device *dev, struct tmcm3216_status *status)
{
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;
	uint32_t raw;
	int err;

	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_ACTUAL_POSITION, &raw);
	if (err != 0) {
		return err;
	}
	status->actual_position = sign_extend(raw, TMCM3216_DATA_BITS);

	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_ACTUAL_VELOCITY, &raw);
	if (err != 0) {
		return err;
	}
	status->actual_velocity = sign_extend(raw, TMCM3216_DATA_BITS);

	status->is_moving = (status->actual_velocity != 0);

	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_POSITION_REACHED_FLAG, &raw);
	if (err != 0) {
		return err;
	}
	status->position_reached = (raw != 0);

	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_RIGHT_ENDSTOP, &raw);
	if (err != 0) {
		return err;
	}
	status->right_endstop = (raw != 0);

	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_LEFT_ENDSTOP, &raw);
	if (err != 0) {
		return err;
	}
	status->left_endstop = (raw != 0);

	return 0;
}

static int tmcm3216_stepper_ctrl_set_reference_position(const struct device *dev,
							const int32_t position)
{
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;

	LOG_DBG("%s set reference position to %d", dev->name, position);
	return tmcm3216_sap(dev, config->motor_index, TMCL_AP_ACTUAL_POSITION, position);
}

static int tmcm3216_stepper_ctrl_get_actual_position(const struct device *dev, int32_t *position)
{
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;
	uint32_t raw_value;
	int err;

	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_ACTUAL_POSITION, &raw_value);
	if (err != 0) {
		return err;
	}

	*position = sign_extend(raw_value, TMCM3216_DATA_BITS);

	LOG_DBG("%s actual position: %d", dev->name, *position);
	return 0;
}

static int tmcm3216_stepper_ctrl_move_by(const struct device *dev, const int32_t micro_steps)
{
	LOG_DBG("%s move by %d steps", dev->name, micro_steps);
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;
	struct tmcm3216_stepper_ctrl_data *data = dev->data;
	struct tmcm_rs485_cmd cmd;
	uint32_t reply;
	int err;

	cmd.module_addr = 0;
	cmd.command_number = TMCL_CMD_MVP;
	cmd.type_number = TMCL_MVP_REL;
	cmd.bank_number = config->motor_index;
	cmd.data = micro_steps;

	err = tmcm3216_send_command(dev, &cmd, &reply);
	if (err != 0) {
		LOG_ERR("Failed to move by %d steps", micro_steps);
		return -EIO;
	}

	if (data->callback != NULL) {
		k_work_reschedule(&data->callback_dwork, K_MSEC(TMCM3216_POLL_INTERVAL_MS));
	}

	return 0;
}

static int tmcm3216_stepper_ctrl_move_to(const struct device *dev, const int32_t micro_steps)
{
	LOG_DBG("%s move to position %d", dev->name, micro_steps);
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;
	struct tmcm3216_stepper_ctrl_data *data = dev->data;
	struct tmcm_rs485_cmd cmd;
	uint32_t reply;
	int err;

	cmd.module_addr = 0;
	cmd.command_number = TMCL_CMD_MVP;
	cmd.type_number = TMCL_MVP_ABS;
	cmd.bank_number = config->motor_index;
	cmd.data = micro_steps;

	err = tmcm3216_send_command(dev, &cmd, &reply);
	if (err != 0) {
		LOG_ERR("Failed to move to position %d", micro_steps);
		return -EIO;
	}

	if (data->callback != NULL) {
		k_work_reschedule(&data->callback_dwork, K_MSEC(TMCM3216_POLL_INTERVAL_MS));
	}

	return 0;
}

static int tmcm3216_stepper_ctrl_run(const struct device *dev,
				     const enum stepper_ctrl_direction direction)
{
	LOG_DBG("%s run in direction %d", dev->name, direction);
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;
	struct tmcm_rs485_cmd cmd;
	uint32_t velocity;
	uint32_t reply;
	int err;

	/* Get max velocity */
	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_MAX_VELOCITY, &velocity);
	if (err != 0) {
		LOG_ERR("Failed to read max velocity");
		return err;
	}

	cmd.module_addr = 0;
	/* Use ROR (Rotate Right) or ROL (Rotate Left) */
	if (direction == STEPPER_CTRL_DIRECTION_POSITIVE) {
		cmd.command_number = TMCL_CMD_ROR; /* Rotate Right */
	} else {
		cmd.command_number = TMCL_CMD_ROL; /* Rotate Left */
	}
	cmd.type_number = 0;
	cmd.bank_number = config->motor_index;
	cmd.data = velocity;

	err = tmcm3216_send_command(dev, &cmd, &reply);
	if (err != 0) {
		LOG_ERR("Failed to run motor");
		return -EIO;
	}

	return 0;
}

static int tmcm3216_stepper_ctrl_stop(const struct device *dev)
{
	LOG_DBG("%s stop", dev->name);
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;
	struct tmcm_rs485_cmd cmd;
	uint32_t reply;
	int err;

	cmd.module_addr = 0;
	cmd.command_number = TMCL_CMD_MST; /* Motor Stop */
	cmd.type_number = 0;
	cmd.bank_number = config->motor_index;
	cmd.data = 0;

	err = tmcm3216_send_command(dev, &cmd, &reply);
	if (err != 0) {
		LOG_ERR("Failed to stop motor");
		return -EIO;
	}

	return 0;
}

static int tmcm3216_stepper_ctrl_is_moving(const struct device *dev, bool *is_moving)
{
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;
	uint32_t position_reached;
	uint32_t velocity;
	int err;

	/* Check position reached flag for MVP commands */
	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_POSITION_REACHED_FLAG,
			   &position_reached);
	if (err != 0) {
		return err;
	}

	/* Also check velocity for ROR/ROL commands where position flag doesn't apply */
	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_ACTUAL_VELOCITY, &velocity);
	if (err != 0) {
		return err;
	}

	/* Motor is moving if position not reached AND velocity is non-zero */
	*is_moving = (position_reached == 0) && ((int32_t)velocity != 0);

	LOG_DBG("%s is_moving: %d (pos_reached: %d, velocity: %d)", dev->name, *is_moving,
		position_reached, (int32_t)velocity);
	return 0;
}

static int tmcm3216_stepper_ctrl_set_event_callback(const struct device *dev,
						    stepper_ctrl_event_callback_t callback,
						    void *user_data)
{
	struct tmcm3216_stepper_ctrl_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;

	LOG_DBG("%s event callback %s", dev->name, callback != NULL ? "registered" : "cleared");
	return 0;
}

/* Callback handler */

static void tmcm3216_callback_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmcm3216_stepper_ctrl_data *data =
		CONTAINER_OF(dwork, struct tmcm3216_stepper_ctrl_data, callback_dwork);
	const struct device *dev = data->stepper;
	bool is_moving;
	int err;

	/* Check if motor is still moving */
	err = tmcm3216_stepper_ctrl_is_moving(dev, &is_moving);
	if (err != 0) {
		LOG_ERR("Failed to check motor status in callback");
		return;
	}

	if (is_moving) {
		/* Still moving, reschedule check */
		k_work_reschedule(dwork, K_MSEC(TMCM3216_POLL_INTERVAL_MS));
	} else {
		/* Motion complete, trigger callback */
		if (data->callback != NULL) {
			data->callback(dev, STEPPER_CTRL_EVENT_STEPS_COMPLETED,
				       data->event_cb_user_data);
		}
	}
}

/* Initialize individual stepper motor */
static int tmcm3216_stepper_ctrl_init(const struct device *dev)
{
	const struct tmcm3216_stepper_ctrl_config *config = dev->config;
	struct tmcm3216_stepper_ctrl_data *data = dev->data;
	int err;

	/* Verify parent controller is ready */
	if (!device_is_ready(config->controller)) {
		LOG_ERR("Parent controller not ready");
		return -ENODEV;
	}

	/* Initialize callback work handler */
	k_work_init_delayable(&data->callback_dwork, tmcm3216_callback_work_handler);

	/* Set default max velocity */
	err = tmcm3216_sap(dev, config->motor_index, TMCL_AP_MAX_VELOCITY, 2000);
	if (err != 0) {
		LOG_WRN("Failed to set default max velocity");
	}

	/* Set default acceleration */
	err = tmcm3216_sap(dev, config->motor_index, TMCL_AP_MAX_ACCELERATION, 500);
	if (err != 0) {
		LOG_WRN("Failed to set default acceleration");
	}

	LOG_INF("TMCM-3216 stepper ctrl %s initialized (motor: %d)", dev->name,
		config->motor_index);

	return 0;
}

static DEVICE_API(stepper_ctrl, tmcm3216_stepper_ctrl_api) = {
	.set_reference_position = tmcm3216_stepper_ctrl_set_reference_position,
	.get_actual_position = tmcm3216_stepper_ctrl_get_actual_position,
	.move_by = tmcm3216_stepper_ctrl_move_by,
	.move_to = tmcm3216_stepper_ctrl_move_to,
	.run = tmcm3216_stepper_ctrl_run,
	.stop = tmcm3216_stepper_ctrl_stop,
	.is_moving = tmcm3216_stepper_ctrl_is_moving,
	.set_event_cb = tmcm3216_stepper_ctrl_set_event_callback,
};

#define TMCM3216_STEPPER_CTRL_DEFINE(inst)                                                         \
	static struct tmcm3216_stepper_ctrl_data tmcm3216_stepper_ctrl_data_##inst = {             \
		.stepper = DEVICE_DT_GET(DT_DRV_INST(inst)),                                       \
	};                                                                                         \
	static const struct tmcm3216_stepper_ctrl_config tmcm3216_stepper_ctrl_config_##inst = {   \
		.controller = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                 \
		.motor_index = DT_INST_PROP(inst, idx),                                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmcm3216_stepper_ctrl_init, NULL,                              \
			      &tmcm3216_stepper_ctrl_data_##inst,                                  \
			      &tmcm3216_stepper_ctrl_config_##inst, POST_KERNEL,                   \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmcm3216_stepper_ctrl_api);

DT_INST_FOREACH_STATUS_OKAY(TMCM3216_STEPPER_CTRL_DEFINE)
