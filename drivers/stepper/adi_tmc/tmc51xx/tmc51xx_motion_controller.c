/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/drivers/stepper.h>
#include "tmc51xx.h"
#include <adi_tmc5xxx_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tmc51xx, CONFIG_STEPPER_LOG_LEVEL);

#define DT_DRV_COMPAT adi_tmc51xx_motion_controller

struct tmc51xx_motion_controller_config {
	const bool is_sg_enabled;
	const uint32_t sg_velocity_check_interval_ms;
	const uint32_t sg_threshold_velocity;
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN
	const struct tmc_ramp_generator_data default_ramp_config;
#endif
	/* parent controller required for bus communication */
	const struct device *controller;
};

struct tmc51xx_motion_controller_data {
	struct k_work_delayable stallguard_dwork;
	/* Work item to run the callback in a thread context. */
	const struct device *dev;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
};

void tmc51xx_motion_controller_trigger_cb(const struct device *dev, const enum stepper_event event)
{
	struct tmc51xx_motion_controller_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No %s callback registered", "motion controller");
		return;
	}
	data->callback(dev, event, data->event_cb_user_data);
}

static int read_vactual(const struct device *dev, int32_t *actual_velocity)
{
	__ASSERT(actual_velocity != NULL, "actual_velocity pointer must not be NULL");
	const struct tmc51xx_motion_controller_config *config = dev->config;
	const struct device *controller = config->controller;
	int err;
	uint32_t raw_value;

	err = tmc51xx_read(controller, TMC51XX_VACTUAL, &raw_value);
	if (err) {
		LOG_ERR("Failed to read VACTUAL register");
		return err;
	}

	*actual_velocity = sign_extend(raw_value, TMC_RAMP_VACTUAL_SHIFT);
	if (*actual_velocity) {
		LOG_DBG("actual velocity: %d", *actual_velocity);
	}
	return 0;
}

int tmc51xx_stallguard_enable(const struct device *dev, const bool enable)
{
	const struct tmc51xx_motion_controller_config *config = dev->config;
	const struct device *controller = config->controller;
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(controller, TMC51XX_SWMODE, &reg_value);
	if (err) {
		LOG_ERR("Failed to read SWMODE register");
		return -EIO;
	}

	if (enable) {
		reg_value |= TMC5XXX_SW_MODE_SG_STOP_ENABLE;

		int32_t actual_velocity;

		err = read_vactual(dev, &actual_velocity);
		if (err) {
			return -EIO;
		}
		if (abs(actual_velocity) < config->sg_threshold_velocity) {
			return -EAGAIN;
		}
	} else {
		reg_value &= ~TMC5XXX_SW_MODE_SG_STOP_ENABLE;
	}
	err = tmc51xx_write(controller, TMC51XX_SWMODE, reg_value);
	if (err) {
		LOG_ERR("Failed to write SWMODE register");
		return -EIO;
	}

	LOG_DBG("Stallguard %s", enable ? "enabled" : "disabled");
	return 0;
}

static void stallguard_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmc51xx_motion_controller_data const *data =
		CONTAINER_OF(dwork, struct tmc51xx_motion_controller_data, stallguard_dwork);
	const struct device *dev = data->dev;
	const struct tmc51xx_motion_controller_config *config = dev->config;
	int err;

	err = tmc51xx_stallguard_enable(dev, true);
	if (err == -EAGAIN) {
		k_work_reschedule(dwork, K_MSEC(config->sg_velocity_check_interval_ms));
	}
	if (err == -EIO) {
		LOG_ERR("Failed to enable stallguard because of I/O error");
	}
}

static int tmc51xx_stepper_set_event_callback(const struct device *dev,
					      stepper_event_callback_t callback, void *user_data)
{
	struct tmc51xx_motion_controller_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;

	return 0;
}

static int tmc51xx_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	const struct tmc51xx_motion_controller_config *config = dev->config;
	const struct device *controller = config->controller;
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(controller, TMC51XX_DRVSTATUS, &reg_value);

	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", dev->name);
		return -EIO;
	}

	*is_moving = (FIELD_GET(TMC5XXX_DRV_STATUS_STST_BIT, reg_value) != 1U);
	LOG_DBG("Stepper motor controller %s is moving: %d", dev->name, *is_moving);
	return 0;
}

static int tmc51xx_stepper_set_reference_position(const struct device *dev, const int32_t position)
{
	const struct tmc51xx_motion_controller_config *config = dev->config;
	const struct device *controller = config->controller;
	int err;

	err = tmc51xx_write(controller, TMC51XX_RAMPMODE, TMC5XXX_RAMPMODE_HOLD_MODE);
	if (err != 0) {
		return -EIO;
	}

	err = tmc51xx_write(controller, TMC51XX_XACTUAL, position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("Stepper motor controller %s set actual position to %d", dev->name, position);
	return 0;
}

static int tmc51xx_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	const struct tmc51xx_motion_controller_config *config = dev->config;
	int err;

	err = tmc51xx_read_actual_position(config->controller, position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("%s actual position: %d", dev->name, *position);
	return 0;
}

static int tmc51xx_stepper_move_to(const struct device *dev, const int32_t micro_steps)
{
	LOG_DBG("%s set target position to %d", dev->name, micro_steps);
	const struct tmc51xx_motion_controller_config *config = dev->config;
	struct tmc51xx_motion_controller_data *data = dev->data;
	const struct device *comm_device = config->controller;
	int err;

	if (config->is_sg_enabled) {
		tmc51xx_stallguard_enable(dev, false);
	}

	err = tmc51xx_write(comm_device, TMC51XX_RAMPMODE, TMC5XXX_RAMPMODE_POSITIONING_MODE);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(comm_device, TMC51XX_XTARGET, micro_steps);
	if (err != 0) {
		return -EIO;
	}

	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}
	if (data->callback) {
		/* For SPI with DIAG0 pin, we use interrupt-driven approach */

		if (tmc51xx_is_interrupt_driven(config->controller)) {
			return 0;
		}
		/* For UART or SPI without DIAG0, reschedule RAMPSTAT polling */
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC
		tmc51xx_reschedule_rampstat_callback(config->controller);
#endif /* CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC */
	}
	return 0;
}

static int tmc51xx_stepper_move_by(const struct device *dev, const int32_t micro_steps)
{
	int err;
	int32_t position;

	err = tmc51xx_stepper_get_actual_position(dev, &position);
	if (err != 0) {
		return -EIO;
	}
	int32_t target_position = position + micro_steps;

	LOG_DBG("%s moved to %d by steps: %d", dev->name, target_position, micro_steps);

	return tmc51xx_stepper_move_to(dev, target_position);
}

static int tmc51xx_stepper_run(const struct device *dev, const enum stepper_direction direction)
{
	LOG_DBG("Stepper motor controller %s run", dev->name);
	const struct tmc51xx_motion_controller_config *config = dev->config;
	struct tmc51xx_motion_controller_data *data = dev->data;
	const struct device *controller = config->controller;
	int err;

	if (config->is_sg_enabled) {
		err = tmc51xx_stallguard_enable(dev, false);
		if (err != 0) {
			return -EIO;
		}
	}

	switch (direction) {
	case STEPPER_DIRECTION_POSITIVE:
		err = tmc51xx_write(controller, TMC51XX_RAMPMODE,
			TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE);
		if (err != 0) {
			return -EIO;
		}
		break;

	case STEPPER_DIRECTION_NEGATIVE:
		err = tmc51xx_write(controller, TMC51XX_RAMPMODE,
			TMC5XXX_RAMPMODE_NEGATIVE_VELOCITY_MODE);
		if (err != 0) {
			return -EIO;
		}
		break;
	}

	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}
	if (data->callback) {
		/* For SPI with DIAG0 pin, we use interrupt-driven approach */
		if (tmc51xx_is_interrupt_driven(controller)) {
			return 0;
		}

		/* For UART or SPI without DIAG0, reschedule RAMPSTAT polling */
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC
		tmc51xx_reschedule_rampstat_callback(config->controller);
#endif /* CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC */
	}
	return 0;
}

#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN

int tmc51xx_stepper_set_ramp(const struct device *dev,
			     const struct tmc_ramp_generator_data *ramp_data)
{
	const struct tmc51xx_motion_controller_config *config = dev->config;
	const struct device *controller = config->controller;
	int err;

	LOG_DBG("Stepper motor controller %s set ramp", dev->name);

	err = tmc51xx_write(controller, TMC51XX_VSTART, ramp_data->vstart);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_A1, ramp_data->a1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_AMAX, ramp_data->amax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_D1, ramp_data->d1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_DMAX, ramp_data->dmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_V1, ramp_data->v1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_VMAX, ramp_data->vmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_VSTOP, ramp_data->vstop);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_TZEROWAIT, ramp_data->tzerowait);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_THIGH, ramp_data->thigh);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_TCOOLTHRS, ramp_data->tcoolthrs);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_TPWMTHRS, ramp_data->tpwmthrs);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_TPOWER_DOWN, ramp_data->tpowerdown);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(controller, TMC51XX_IHOLD_IRUN, ramp_data->iholdrun);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

#endif

static int tmc51xx_motion_controller_init(const struct device *dev)
{
	const struct tmc51xx_motion_controller_config *config = dev->config;
	struct tmc51xx_motion_controller_data *data = dev->data;
	const struct device *controller = config->controller;
	int err;

	data->dev = dev;

	if (config->is_sg_enabled) {
		k_work_init_delayable(&data->stallguard_dwork, stallguard_work_handler);

		err = tmc51xx_write(controller, TMC51XX_SWMODE, BIT(10));
		if (err != 0) {
			return -EIO;
		}

		LOG_DBG("stallguard delay %d ms", config->sg_velocity_check_interval_ms);

		k_work_reschedule(&data->stallguard_dwork, K_NO_WAIT);
	}

#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN
	err = tmc51xx_stepper_set_ramp(dev, &config->default_ramp_config);
	if (err != 0) {
		return -EIO;
	}
#endif
	return 0;
}

static int tmc51xx_stepper_stop(const struct device *dev)
{
	const struct tmc51xx_motion_controller_config *config = dev->config;
	const struct device *controller = config->controller;
	int err;

	err = tmc51xx_write(controller, TMC51XX_RAMPMODE, TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE);
	if (err != 0) {
		return -EIO;
	}

	err = tmc51xx_write(controller, TMC51XX_VMAX, 0);
	if (err != 0) {
		return -EIO;
	}

	return 0;
}

static DEVICE_API(stepper, tmc51xx_stepper_api) = {
	.is_moving = tmc51xx_stepper_is_moving,
	.move_by = tmc51xx_stepper_move_by,
	.set_reference_position = tmc51xx_stepper_set_reference_position,
	.get_actual_position = tmc51xx_stepper_get_actual_position,
	.move_to = tmc51xx_stepper_move_to,
	.run = tmc51xx_stepper_run,
	.stop = tmc51xx_stepper_stop,
	.set_event_callback = tmc51xx_stepper_set_event_callback,
};

#define TMC51XX_MOTION_CONTROLLER_DEFINE(inst)                                                     \
	IF_ENABLED(CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN,	(CHECK_RAMP_DT_DATA(inst)));               \
	static const struct tmc51xx_motion_controller_config tmc51xx_motion_ctrl_cfg_##inst = {    \
		.controller = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.sg_threshold_velocity = DT_INST_PROP(inst, stallguard_threshold_velocity),        \
		.sg_velocity_check_interval_ms =                                                   \
			DT_INST_PROP(inst, stallguard_velocity_check_interval_ms),                 \
		.is_sg_enabled = DT_INST_PROP(inst, activate_stallguard2),                         \
		IF_ENABLED(CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN,                                    \
			(.default_ramp_config = TMC_RAMP_DT_SPEC_GET_TMC51XX(inst))) };            \
	static struct tmc51xx_motion_controller_data tmc51xx_motion_controller_data_##inst;        \
	DEVICE_DT_INST_DEFINE(inst, tmc51xx_motion_controller_init, NULL,                          \
			      &tmc51xx_motion_controller_data_##inst,                              \
			      &tmc51xx_motion_ctrl_cfg_##inst, POST_KERNEL,                        \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmc51xx_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(TMC51XX_MOTION_CONTROLLER_DEFINE)
