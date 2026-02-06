/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc50xx_stepper

#include <stdlib.h>

#include "tmc50xx.h"
#include <adi_tmc5xxx_common.h>

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tmc50xx, CONFIG_STEPPER_LOG_LEVEL);

struct tmc50xx_stepper_data {
	struct k_work_delayable stallguard_dwork;
	const struct device *dev;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
};

struct tmc50xx_stepper_config {
	const uint8_t index;
	const bool is_sg_enabled;
	const uint32_t sg_velocity_check_interval_ms;
	const uint32_t sg_threshold_velocity;
#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN
	const struct tmc_ramp_generator_data default_ramp_config;
#endif
	/* parent controller required for bus communication */
	const struct device *controller;
};

int tmc50xx_stepper_index(const struct device *dev)
{
	const struct tmc50xx_stepper_config *config = dev->config;

	return config->index;
}

int tmc50xx_stepper_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	const uint32_t clock_frequency = tmc50xx_get_clock_frequency(dev);
	uint32_t velocity_fclk;
	int err;

	velocity_fclk = tmc5xxx_calculate_velocity_from_hz_to_fclk(velocity, clock_frequency);

	err = tmc50xx_write(config->controller, TMC50XX_VMAX(config->index), velocity_fclk);
	if (err != 0) {
		LOG_ERR("%s: Failed to set max velocity", dev->name);
		return -EIO;
	}
	return 0;
}

static int read_vactual(const struct device *dev, int32_t *actual_velocity)
{
	__ASSERT(actual_velocity != NULL, "actual_velocity pointer must not be NULL");
	const struct tmc50xx_stepper_config *config = dev->config;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_VACTUAL(config->index), actual_velocity);
	if (err) {
		LOG_ERR("Failed to read VACTUAL register");
		return err;
	}

	*actual_velocity = sign_extend(*actual_velocity, TMC_RAMP_VACTUAL_SHIFT);

	LOG_DBG("actual velocity: %d", *actual_velocity);

	return 0;
}

static void stallguard_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmc50xx_stepper_data *data =
		CONTAINER_OF(dwork, struct tmc50xx_stepper_data, stallguard_dwork);
	const struct tmc50xx_stepper_config *config = data->dev->config;
	int err;

	err = tmc50xx_stepper_stallguard_enable(data->dev, true);
	if (err == -EAGAIN) {
		k_work_reschedule(dwork, K_MSEC(config->sg_velocity_check_interval_ms));
	}
	if (err == -EIO) {
		LOG_ERR("Failed to enable stallguard because of I/O error");
		return;
	}
}

int tmc50xx_stepper_stallguard_enable(const struct device *dev, const bool enable)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_SWMODE(config->index), &reg_value);
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
	err = tmc50xx_write(config->controller, TMC50XX_SWMODE(config->index), reg_value);
	if (err) {
		LOG_ERR("Failed to write SWMODE register");
		return -EIO;
	}

	LOG_DBG("Stallguard %s", enable ? "enabled" : "disabled");
	return 0;
}

void tmc50xx_stepper_trigger_cb(const struct device *dev, const enum stepper_event event)
{
	if (dev == NULL) {
		return;
	}
	struct tmc50xx_stepper_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No motion controller callback registered");
		return;
	}
	data->callback(dev, event, data->event_cb_user_data);
}

static int tmc50xx_stepper_set_event_callback(const struct device *dev,
							stepper_event_callback_t callback,
							void *user_data)
{
	struct tmc50xx_stepper_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;

	return 0;
}

static int tmc50xx_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_DRVSTATUS(config->index), &reg_value);

	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", dev->name);
		return -EIO;
	}

	*is_moving = (FIELD_GET(TMC5XXX_DRV_STATUS_STST_BIT, reg_value) != 1U);
	LOG_DBG("Stepper motor controller %s is moving: %d", dev->name, *is_moving);
	return 0;
}

static int tmc50xx_stepper_set_reference_position(const struct device *dev, const int32_t position)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	int err;

	err = tmc50xx_write(config->controller, TMC50XX_RAMPMODE(config->index),
			    TMC5XXX_RAMPMODE_HOLD_MODE);
	if (err != 0) {
		return -EIO;
	}

	err = tmc50xx_write(config->controller, TMC50XX_XACTUAL(config->index), position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("Stepper motor controller %s set actual position to %d", dev->name, position);
	return 0;
}

static int tmc50xx_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	int err;

	err = tmc50xx_read_actual_position(config->controller, config->index, position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("%s actual position: %d", dev->name, *position);
	return 0;
}

static int tmc50xx_stepper_move_to(const struct device *dev, const int32_t micro_steps)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	struct tmc50xx_stepper_data *data = dev->data;
	int err;

	LOG_DBG("%s set target position to %d", dev->name, micro_steps);

	if (config->is_sg_enabled) {
		tmc50xx_stepper_stallguard_enable(dev, false);
	}

	err = tmc50xx_write(config->controller, TMC50XX_RAMPMODE(config->index),
			    TMC5XXX_RAMPMODE_POSITIONING_MODE);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_XTARGET(config->index), micro_steps);
	if (err != 0) {
		return -EIO;
	}

	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}

	if (data->callback) {
		tmc50xx_rampstat_work_reschedule(config->controller);
	}
	return 0;
}

static int tmc50xx_stepper_move_by(const struct device *dev, const int32_t micro_steps)
{
	int32_t position;
	int err;

	err = tmc50xx_stepper_get_actual_position(dev, &position);
	if (err != 0) {
		return -EIO;
	}
	int32_t target_position = position + micro_steps;

	LOG_DBG("%s moved to %d by steps: %d", dev->name, target_position, micro_steps);

	return tmc50xx_stepper_move_to(dev, target_position);
}

static int tmc50xx_stepper_run(const struct device *dev, const enum stepper_direction direction)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	struct tmc50xx_stepper_data *data = dev->data;
	int err;

	LOG_DBG("Stepper motor controller %s run", dev->name);

	if (config->is_sg_enabled) {
		err = tmc50xx_stepper_stallguard_enable(dev, false);
		if (err != 0) {
			return -EIO;
		}
	}

	switch (direction) {
	case STEPPER_DIRECTION_POSITIVE:
		err = tmc50xx_write(config->controller, TMC50XX_RAMPMODE(config->index),
				    TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE);
		if (err != 0) {
			return -EIO;
		}
		break;

	case STEPPER_DIRECTION_NEGATIVE:
		err = tmc50xx_write(config->controller, TMC50XX_RAMPMODE(config->index),
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
		tmc50xx_rampstat_work_reschedule(config->controller);
	}
	return 0;
}

static int tmc50xx_stepper_stop(const struct device *dev)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	int err;

	err = tmc50xx_write(config->controller, TMC50XX_RAMPMODE(config->index),
			    TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE);
	if (err != 0) {
		return -EIO;
	}

	err = tmc50xx_write(config->controller, TMC50XX_VMAX(config->index), 0);
	if (err != 0) {
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN

int tmc50xx_stepper_set_ramp(const struct device *dev,
			     const struct tmc_ramp_generator_data *ramp_data)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	const struct device *controller = config->controller;
	int err;

	LOG_DBG("Stepper motor controller %s set ramp", dev->name);

	err = tmc50xx_write(controller, TMC50XX_VSTART(config->index), ramp_data->vstart);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_A1(config->index), ramp_data->a1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_AMAX(config->index), ramp_data->amax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_D1(config->index), ramp_data->d1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_DMAX(config->index), ramp_data->dmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_V1(config->index), ramp_data->v1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_VMAX(config->index), ramp_data->vmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_VSTOP(config->index), ramp_data->vstop);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_TZEROWAIT(config->index), ramp_data->tzerowait);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_VHIGH(config->index), ramp_data->vhigh);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_VCOOLTHRS(config->index), ramp_data->vcoolthrs);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(controller, TMC50XX_IHOLD_IRUN(config->index), ramp_data->iholdrun);
	if (err != 0) {
		return -EIO;
	}

	return 0;
}

#endif

static int tmc50xx_stepper_init(const struct device *dev)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	struct tmc50xx_stepper_data *data = dev->data;
	int err;

	LOG_DBG("Controller: %s, Motion Controller: %s", config->controller->name, dev->name);
	data->dev = dev;

	if (config->is_sg_enabled) {
		k_work_init_delayable(&data->stallguard_dwork, stallguard_work_handler);

		err = tmc50xx_write(config->controller, TMC50XX_SWMODE(config->index), BIT(10));
		if (err != 0) {
			return -EIO;
		}

		LOG_DBG("stallguard delay %d ms", config->sg_velocity_check_interval_ms);
		k_work_reschedule(&data->stallguard_dwork, K_NO_WAIT);
	}

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN
	err = tmc50xx_stepper_set_ramp(dev, &config->default_ramp_config);
	if (err != 0) {
		return -EIO;
	}
#endif
	return 0;
}

static DEVICE_API(stepper, tmc50xx_stepper_api) = {
	.is_moving = tmc50xx_stepper_is_moving,
	.move_by = tmc50xx_stepper_move_by,
	.set_reference_position = tmc50xx_stepper_set_reference_position,
	.get_actual_position = tmc50xx_stepper_get_actual_position,
	.move_to = tmc50xx_stepper_move_to,
	.run = tmc50xx_stepper_run,
	.stop = tmc50xx_stepper_stop,
	.set_event_callback = tmc50xx_stepper_set_event_callback,
};

#define TMC50XX_STEPPER_DEFINE(inst)                                                               \
	IF_ENABLED(CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN, (CHECK_RAMP_DT_DATA(DT_DRV_INST(inst))));  \
	static const struct tmc50xx_stepper_config tmc50xx_stepper_cfg_##inst = {                  \
		.controller = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.index = DT_INST_PROP(inst, idx),                                                  \
		.sg_threshold_velocity = DT_INST_PROP(inst, stallguard_threshold_velocity),        \
		.sg_velocity_check_interval_ms =                                                   \
			DT_INST_PROP(inst, stallguard_velocity_check_interval_ms),                 \
		.is_sg_enabled = DT_INST_PROP(inst, activate_stallguard2),                         \
		IF_ENABLED(CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN,                                    \
		(.default_ramp_config = TMC_RAMP_DT_SPEC_GET_TMC50XX(DT_DRV_INST(inst)))) };       \
	static struct tmc50xx_stepper_data tmc50xx_stepper_data_##inst;                            \
	DEVICE_DT_INST_DEFINE(inst, tmc50xx_stepper_init, NULL,                                    \
			      &tmc50xx_stepper_data_##inst,                                        \
			      &tmc50xx_stepper_cfg_##inst, POST_KERNEL,                            \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmc50xx_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(TMC50XX_STEPPER_DEFINE)
