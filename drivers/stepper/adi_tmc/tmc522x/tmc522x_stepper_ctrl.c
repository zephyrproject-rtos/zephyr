/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <Cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc522x_stepper_ctrl

#include <stdlib.h>

#include <zephyr/drivers/stepper/stepper_ctrl.h>

#include "tmc522x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tmc522x, CONFIG_STEPPER_LOG_LEVEL);

struct tmc522x_stepper_data {
	const struct device *dev;
	stepper_ctrl_event_callback_t callback;
	void *event_cb_user_data;

	/* Work items for event handling */
	struct k_work stallguard_work;
	struct k_work pos_reached_work;

	/* Event enable flags */
	bool stallguard_event_enabled;
	bool pos_reached_event_enabled;

	/* Cached motion parameters */
	uint32_t vmax;
	uint32_t xtarget;
};

struct tmc522x_stepper_config {
	const uint8_t index;

	/* Motion parameters from devicetree */
	const uint32_t default_vstart;
	const uint32_t default_a1;
	const uint32_t default_v1;
	const uint32_t default_amax;
	const uint32_t default_vmax;
	const uint32_t default_dmax;
	const uint32_t default_d1;
	const uint32_t default_vstop;
	const uint32_t default_a2;
	const uint32_t default_v2;
	const uint32_t default_d2;

	/* Parent controller required for bus communication */
	const struct device *controller;
};

/* Get stepper index for parent MFD iteration */
int tmc522x_stepper_index(const struct device *dev)
{
	const struct tmc522x_stepper_config *config = dev->config;

	return config->index;
}

/* Trigger application callback (called by parent MFD on RAMP_STAT events) */
void tmc522x_stepper_trigger_cb(const struct device *dev, const enum stepper_ctrl_event event)
{
	if (dev == NULL) {
		return;
	}

	struct tmc522x_stepper_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No motion controller callback registered");
		return;
	}

	data->callback(dev, event, data->event_cb_user_data);
}

/* Clear stall event (internal helper — routes through parent MFD device) */
static int tmc522x_stepper_clear_stall(const struct device *dev)
{
	const struct tmc522x_stepper_config *config = dev->config;

	return tmc522x_clear_stall(config->controller);
}

/* Clear position reached event */
static int __maybe_unused tmc522x_clear_pos_reached(const struct device *dev)
{
	const struct tmc522x_stepper_config *config = dev->config;

	/* Write-1-to-clear the position reached event bit */
	return tmc522x_write(config->controller, TMC522X_REG_RAMP_STAT,
			     TMC522X_EVENT_POS_REACH_MASK);
}

/* Set absolute position (move_to) */
static int tmc522x_set_position(const struct device *dev, int32_t position)
{
	const struct tmc522x_stepper_config *config = dev->config;
	struct tmc522x_stepper_data *data = dev->data;
	int ret;

	ret = tmc522x_stepper_clear_stall(dev);
	if (ret) {
		return ret;
	}

	/* Restore VMAX to configured value (in case it was set to 0 by stop) */
	ret = tmc522x_write(config->controller, TMC522X_REG_VMAX, data->vmax);
	if (ret) {
		LOG_ERR("Failed to restore VMAX: %d", ret);
		return ret;
	}

	/* Set to POSITION mode */
	ret = tmc522x_write(config->controller, TMC522X_REG_RAMPMODE, TMC522X_RAMPMODE_POSITION);
	if (ret) {
		return ret;
	}

	/* Write the TARGET position - this starts the movement */
	ret = tmc522x_write(config->controller, TMC522X_REG_XTARGET, position);
	if (ret) {
		return ret;
	}

	/* Cache the target position */
	data->xtarget = position;

	LOG_DBG("Moving to absolute position: %d", position);
	return 0;
}

/* Set velocity (run mode) */
static int tmc522x_set_velocity(const struct device *dev, int32_t velocity)
{
	const struct tmc522x_stepper_config *config = dev->config;
	struct tmc522x_stepper_data *data = dev->data;
	uint32_t vmax_abs;
	enum tmc522x_rampmode rampmode;
	int ret;

	if (velocity == 0) {
		/* Stop motor - set VMAX to 0 to decelerate using ramps */
		ret = tmc522x_write(config->controller, TMC522X_REG_VMAX, 0);
		if (ret) {
			LOG_ERR("Failed to set VMAX=0: %d", ret);
			return ret;
		}
		LOG_DBG("Set VMAX=0 to decelerate motor to stop");
		return 0;
	}

	/* Determine direction and set ramp mode */
	if (velocity < 0) {
		vmax_abs = (uint32_t)(-velocity);
		rampmode = TMC522X_RAMPMODE_NEGATIVE_VELOCITY;
	} else {
		vmax_abs = (uint32_t)velocity;
		rampmode = TMC522X_RAMPMODE_POSITIVE_VELOCITY;
	}

	/* Write VMAX register with absolute velocity */
	ret = tmc522x_write(config->controller, TMC522X_REG_VMAX, vmax_abs);
	if (ret) {
		LOG_ERR("Failed to write VMAX: %d", ret);
		return ret;
	}

	/* Set ramp mode to velocity mode */
	ret = tmc522x_write(config->controller, TMC522X_REG_RAMPMODE, rampmode);
	if (ret) {
		LOG_ERR("Failed to set velocity mode: %d", ret);
		return ret;
	}

	data->vmax = vmax_abs;

	LOG_DBG("Velocity mode set: velocity=%d, mode=%d", velocity, rampmode);
	return 0;
}

static int tmc522x_stepper_move_to(const struct device *dev, int32_t micro_steps)
{
	const struct tmc522x_stepper_config *config = dev->config;
	struct tmc522x_stepper_data *data = dev->data;
	int ret;

	LOG_DBG("Moving to absolute position: %d micro-steps", micro_steps);
	ret = tmc522x_set_position(dev, micro_steps);
	if (ret != 0) {
		return ret;
	}

	/* Start autonomous event polling if callback is registered */
	if (data->callback) {
		tmc522x_reschedule_rampstat_work(config->controller);
	}

	return 0;
}

static int tmc522x_stepper_move_by(const struct device *dev, int32_t micro_steps)
{
	const struct tmc522x_stepper_config *config = dev->config;
	struct tmc522x_stepper_data *data = dev->data;
	int32_t current_pos;
	uint32_t xactual;
	int ret;

	/* Read current position from XACTUAL register */
	ret = tmc522x_read(config->controller, TMC522X_REG_XACTUAL, &xactual);
	if (ret) {
		LOG_ERR("Failed to read current position: %d", ret);
		return ret;
	}

	current_pos = (int32_t)xactual;
	LOG_DBG("Moving by %d micro-steps from position %d", micro_steps, current_pos);

	/* Move to new position */
	ret = tmc522x_set_position(dev, current_pos + micro_steps);
	if (ret != 0) {
		return ret;
	}

	/* Start autonomous event polling if callback is registered */
	if (data->callback) {
		tmc522x_reschedule_rampstat_work(config->controller);
	}

	return 0;
}

static int tmc522x_stepper_run(const struct device *dev, enum stepper_ctrl_direction direction)
{
	const struct tmc522x_stepper_config *config = dev->config;
	struct tmc522x_stepper_data *data = dev->data;
	int32_t velocity;
	int ret;

	/* Use VMAX for continuous velocity mode */
	velocity = (direction == STEPPER_CTRL_DIRECTION_POSITIVE) ? (int32_t)data->vmax
							     : -(int32_t)data->vmax;

	LOG_DBG("Running continuously at velocity: %d", velocity);
	ret = tmc522x_set_velocity(dev, velocity);
	if (ret != 0) {
		return ret;
	}

	/* Start autonomous event polling if callback is registered */
	if (data->callback) {
		tmc522x_reschedule_rampstat_work(config->controller);
	}

	return 0;
}

static int tmc522x_stepper_stop(const struct device *dev)
{
	LOG_DBG("Stopping motor");
	/* Set velocity to 0 to decelerate and stop */
	return tmc522x_set_velocity(dev, 0);
}

static int tmc522x_stepper_set_reference_position(const struct device *dev, int32_t value)
{
	const struct tmc522x_stepper_config *config = dev->config;

	LOG_DBG("Setting reference position to: %d", value);
	return tmc522x_write(config->controller, TMC522X_REG_XACTUAL, (uint32_t)value);
}

static int tmc522x_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	const struct tmc522x_stepper_config *config = dev->config;
	uint32_t xactual;
	int ret;

	if (position == NULL) {
		return -EINVAL;
	}

	ret = tmc522x_read(config->controller, TMC522X_REG_XACTUAL, &xactual);
	if (ret) {
		LOG_ERR("Failed to read XACTUAL register: %d", ret);
		return ret;
	}

	*position = (int32_t)xactual;
	LOG_DBG("Actual position: %d", *position);
	return 0;
}

static int tmc522x_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	const struct tmc522x_stepper_config *config = dev->config;
	uint32_t vactual;
	int ret;

	if (is_moving == NULL) {
		return -EINVAL;
	}

	/* Read actual velocity from VACTUAL register */
	ret = tmc522x_read(config->controller, TMC522X_REG_VACTUAL, &vactual);
	if (ret) {
		LOG_ERR("Failed to read VACTUAL register: %d", ret);
		return ret;
	}

	/* Motor is moving if velocity is non-zero */
	*is_moving = (vactual != 0);
	LOG_DBG("Motor %s", *is_moving ? "is moving" : "is stopped");
	return 0;
}

static int tmc522x_stepper_set_event_callback(const struct device *dev,
					      stepper_ctrl_event_callback_t callback,
					      void *user_data)
{
	struct tmc522x_stepper_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;
	LOG_DBG("Event callback %s", callback ? "registered" : "cleared");
	return 0;
}

static void tmc522x_stallguard_work_handler(struct k_work *work)
{
	struct tmc522x_stepper_data *data =
		CONTAINER_OF(work, struct tmc522x_stepper_data, stallguard_work);
	const struct device *dev = data->dev;
	const struct tmc522x_stepper_config *config = dev->config;
	uint32_t ramp_stat;
	int ret;

	/* Read RAMP_STAT to confirm stall event */
	ret = tmc522x_read(config->controller, TMC522X_REG_RAMP_STAT, &ramp_stat);
	if (ret) {
		LOG_ERR("Failed to read RAMP_STAT in stallguard handler: %d", ret);
		return;
	}

	if (ramp_stat & TMC522X_EVENT_STOP_SG_MASK) {
		LOG_DBG("StallGuard event detected! RAMP_STAT: 0x%08x", ramp_stat);

		/* Clear the event by writing 1 to the bit */
		ret = tmc522x_write(config->controller, TMC522X_REG_RAMP_STAT,
				    TMC522X_EVENT_STOP_SG_MASK);
		if (ret) {
			LOG_ERR("Failed to clear stallguard event: %d", ret);
		}

		/* Notify application via callback if registered */
		if (data->callback && data->stallguard_event_enabled) {
			data->callback(dev, STEPPER_CTRL_EVENT_LEFT_END_STOP_DETECTED,
				       data->event_cb_user_data);
		}
	}
}

static void tmc522x_pos_reached_work_handler(struct k_work *work)
{
	struct tmc522x_stepper_data *data =
		CONTAINER_OF(work, struct tmc522x_stepper_data, pos_reached_work);
	const struct device *dev = data->dev;
	const struct tmc522x_stepper_config *config = dev->config;
	uint32_t ramp_stat;
	int ret;

	/* Read RAMP_STAT to confirm position reached event */
	ret = tmc522x_read(config->controller, TMC522X_REG_RAMP_STAT, &ramp_stat);
	if (ret) {
		LOG_ERR("Failed to read RAMP_STAT in pos_reached handler: %d", ret);
		return;
	}

	if (ramp_stat & TMC522X_EVENT_POS_REACH_MASK) {
		LOG_DBG("Position reached event detected! RAMP_STAT: 0x%08x", ramp_stat);

		/* Clear the event by writing 1 to the bit */
		ret = tmc522x_write(config->controller, TMC522X_REG_RAMP_STAT,
				    TMC522X_EVENT_POS_REACH_MASK);
		if (ret) {
			LOG_ERR("Failed to clear position reached event: %d", ret);
		}

		/* Notify application via callback if registered */
		if (data->callback && data->pos_reached_event_enabled) {
			data->callback(dev, STEPPER_CTRL_EVENT_STEPS_COMPLETED,
				       data->event_cb_user_data);
		}
	}
}

static int tmc522x_stepper_init(const struct device *dev)
{
	const struct tmc522x_stepper_config *config = dev->config;
	struct tmc522x_stepper_data *data = dev->data;
	int ret;

	LOG_DBG("Controller: %s, Stepper: %s", config->controller->name, dev->name);

	/* Store device pointer for work context */
	data->dev = dev;

	/* Initialize cached parameters from devicetree defaults */
	data->vmax = config->default_vmax;
	data->xtarget = 0;

	/* Initialize event flags */
	data->stallguard_event_enabled = false;
	data->pos_reached_event_enabled = false;

	/* Initialize work items */
	k_work_init(&data->stallguard_work, tmc522x_stallguard_work_handler);
	k_work_init(&data->pos_reached_work, tmc522x_pos_reached_work_handler);

	/* Initialize motion parameters */
	ret = tmc522x_write(config->controller, TMC522X_REG_VSTART, config->default_vstart);
	if (ret) {
		LOG_ERR("Failed to write VSTART");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_A1, config->default_a1);
	if (ret) {
		LOG_ERR("Failed to write A1");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_V1, config->default_v1);
	if (ret) {
		LOG_ERR("Failed to write V1");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_A2, config->default_a2);
	if (ret) {
		LOG_ERR("Failed to write A2");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_V2, config->default_v2);
	if (ret) {
		LOG_ERR("Failed to write V2");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_AMAX, config->default_amax);
	if (ret) {
		LOG_ERR("Failed to write AMAX");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_VMAX, config->default_vmax);
	if (ret) {
		LOG_ERR("Failed to write VMAX");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_DMAX, config->default_dmax);
	if (ret) {
		LOG_ERR("Failed to write DMAX");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_D2, config->default_d2);
	if (ret) {
		LOG_ERR("Failed to write D2");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_D1, config->default_d1);
	if (ret) {
		LOG_ERR("Failed to write D1");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_VSTOP, config->default_vstop);
	if (ret) {
		LOG_ERR("Failed to write VSTOP");
		return ret;
	}

	/*
	 * Ensure motor is stopped and position is zeroed before the application
	 * starts. Without this, stale register values from a previous session
	 * (RAMPMODE, XACTUAL, XTARGET) can cause unintended motor movement
	 * immediately after the driver enables the motor outputs.
	 *
	 * Sequence: HOLD mode (stop motion) → zero positions → POSITION mode
	 */
	ret = tmc522x_write(config->controller, TMC522X_REG_RAMPMODE, TMC522X_RAMPMODE_HOLD);
	if (ret) {
		LOG_ERR("Failed to write RAMPMODE (hold)");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_XACTUAL, 0);
	if (ret) {
		LOG_ERR("Failed to zero XACTUAL");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_XTARGET, 0);
	if (ret) {
		LOG_ERR("Failed to zero XTARGET");
		return ret;
	}

	ret = tmc522x_write(config->controller, TMC522X_REG_RAMPMODE, TMC522X_RAMPMODE_POSITION);
	if (ret) {
		LOG_ERR("Failed to write RAMPMODE (position)");
		return ret;
	}

	/* Set TCOOLTHRS to 0 for maximum compatibility */
	ret = tmc522x_write(config->controller, TMC522X_REG_TCOOLTHRS, 0);
	if (ret) {
		LOG_ERR("Failed to write TCOOLTHRS");
		return ret;
	}

	LOG_DBG("Motion controller initialized (VMAX=%u, AMAX=%u, XACTUAL=0, XTARGET=0)",
		config->default_vmax, config->default_amax);

	return 0;
}

/* Stepper API */
static DEVICE_API(stepper_ctrl, tmc522x_stepper_ctrl_api) = {
	.move_by = tmc522x_stepper_move_by,
	.move_to = tmc522x_stepper_move_to,
	.run = tmc522x_stepper_run,
	.stop = tmc522x_stepper_stop,
	.set_reference_position = tmc522x_stepper_set_reference_position,
	.get_actual_position = tmc522x_stepper_get_actual_position,
	.is_moving = tmc522x_stepper_is_moving,
	.set_event_cb = tmc522x_stepper_set_event_callback,
};

#define TMC522X_STEPPER_DEFINE(inst)                                                               \
	static const struct tmc522x_stepper_config tmc522x_stepper_config_##inst = {               \
		.controller = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.index = DT_INST_PROP(inst, idx),                                                  \
		.default_vstart = DT_INST_PROP(inst, vstart),                                      \
		.default_a1 = DT_INST_PROP(inst, a1),                                              \
		.default_v1 = DT_INST_PROP(inst, v1),                                              \
		.default_amax = DT_INST_PROP(inst, amax),                                          \
		.default_vmax = DT_INST_PROP(inst, vmax),                                          \
		.default_dmax = DT_INST_PROP(inst, dmax),                                          \
		.default_d1 = DT_INST_PROP(inst, d1),                                              \
		.default_vstop = DT_INST_PROP(inst, vstop),                                        \
		.default_a2 = DT_INST_PROP(inst, a2),                                              \
		.default_v2 = DT_INST_PROP(inst, v2),                                              \
		.default_d2 = DT_INST_PROP(inst, d2),                                              \
	};                                                                                         \
	static struct tmc522x_stepper_data tmc522x_stepper_data_##inst;                            \
	DEVICE_DT_INST_DEFINE(inst, tmc522x_stepper_init, NULL, &tmc522x_stepper_data_##inst,      \
			      &tmc522x_stepper_config_##inst, POST_KERNEL,                         \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmc522x_stepper_ctrl_api);

DT_INST_FOREACH_STATUS_OKAY(TMC522X_STEPPER_DEFINE)
