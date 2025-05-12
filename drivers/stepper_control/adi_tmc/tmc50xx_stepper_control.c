/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc50xx_stepper_control

#include <zephyr/drivers/stepper_control.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>
#include <zephyr/drivers/stepper_control/adi_tmc_spi.h>
#include <zephyr/drivers/stepper/adi_tmc_reg.h>

#include "adi_tmc5xxx_common.h"

#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc50xx_stepper_control, CONFIG_STEPPER_CONTROL_LOG_LEVEL);

struct tmc50xx_stepper_control_data {
	const struct device *dev;
	/* Work item to run the callback in a thread context. */
	struct k_work_delayable rampstat_callback_dwork;
	stepper_control_event_callback_t callback;
	void *event_cb_user_data;
};

struct tmc50xx_stepper_control_config {
	/* parent controller required for bus communication, device pointer to tmc50xx */
	const struct device *controller;
	/* child stepper device */
	const struct device *stepper;
	const uint32_t clock_frequency;
	const uint8_t index;
#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN
	const struct tmc_ramp_generator_data default_ramp_config;
#endif
};

static int read_actual_position(const struct tmc50xx_stepper_control_config *config,
				int32_t *position)
{
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_XACTUAL(config->index), position);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

static void rampstat_work_reschedule(struct k_work_delayable *rampstat_callback_dwork)
{
	k_work_reschedule(rampstat_callback_dwork,
			  K_MSEC(CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
}

static void execute_callback(const struct device *dev, const enum stepper_control_event event)
{
	struct tmc50xx_stepper_control_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback registered");
		return;
	}
	data->callback(dev, event, data->event_cb_user_data);
}

static void rampstat_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	struct tmc50xx_stepper_control_data *stepper_control_data =
		CONTAINER_OF(dwork, struct tmc50xx_stepper_control_data, rampstat_callback_dwork);
	const struct tmc50xx_stepper_control_config *stepper_control_config =
		stepper_control_data->dev->config;

	__ASSERT_NO_MSG(stepper_control_config->controller != NULL);

	uint32_t drv_status;
	int err;

	err = tmc50xx_read(stepper_control_config->controller,
			   TMC50XX_DRVSTATUS(stepper_control_config->index), &drv_status);
	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", stepper_control_data->dev->name);
		return;
	}
#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_STALLGUARD_LOG
	tmc5xxx_log_stallguard(stepper_control_config->stepper, drv_status);
#endif
	if (FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status) == 1U) {
		LOG_INF("%s: Stall detected", stepper_control_data->dev->name);
		err = tmc50xx_write(stepper_control_config->controller,
				    TMC50XX_RAMPMODE(stepper_control_config->index),
				    TMC5XXX_RAMPMODE_HOLD_MODE);
		if (err != 0) {
			LOG_ERR("%s: Failed to stop motor", stepper_control_data->dev->name);
			return;
		}
	}

	uint32_t rampstat_value;

	err = tmc50xx_read(stepper_control_config->controller,
			   TMC50XX_RAMPSTAT(stepper_control_config->index), &rampstat_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to read RAMPSTAT register", stepper_control_data->dev->name);
		return;
	}
	const uint8_t ramp_stat_values = FIELD_GET(TMC5XXX_RAMPSTAT_INT_MASK, rampstat_value);

	if (ramp_stat_values > 0) {
		switch (ramp_stat_values) {

		case TMC5XXX_STOP_LEFT_EVENT:
			LOG_DBG("RAMPSTAT %s:Left end-stop detected",
				stepper_control_data->dev->name);
			execute_callback(stepper_control_data->dev,
					 STEPPER_CONTROL_EVENT_LEFT_END_STOP_DETECTED);
			break;

		case TMC5XXX_STOP_RIGHT_EVENT:
			LOG_DBG("RAMPSTAT %s:Right end-stop detected",
				stepper_control_data->dev->name);
			execute_callback(stepper_control_data->dev,
					 STEPPER_CONTROL_EVENT_RIGHT_END_STOP_DETECTED);
			break;

		case TMC5XXX_POS_REACHED_EVENT:
			LOG_DBG("RAMPSTAT %s:Position reached", stepper_control_data->dev->name);
			execute_callback(stepper_control_data->dev,
					 STEPPER_CONTROL_EVENT_STEPS_COMPLETED);
			break;

		case TMC5XXX_STOP_SG_EVENT:
			LOG_DBG("RAMPSTAT %s:Stall detected", stepper_control_data->dev->name);
			tmc5xxx_stallguard_enable(stepper_control_data->dev, false);
			execute_callback(stepper_control_data->dev,
					 STEPPER_CONTROL_EVENT_STALL_DETECTED);
			break;
		default:
			LOG_ERR("Illegal ramp stat bit field");
			break;
		}
	} else {
		rampstat_work_reschedule(&stepper_control_data->rampstat_callback_dwork);
	}
}

static int tmc50xx_stepper_control_set_event_callback(const struct device *dev,
						      stepper_control_event_callback_t callback,
						      void *user_data)
{
	struct tmc50xx_stepper_control_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;
	return 0;
}

static int tmc50xx_stepper_control_move_to(const struct device *dev, const int32_t micro_steps)
{
	LOG_DBG("%s set target position to %d", dev->name, micro_steps);
	const struct tmc50xx_stepper_control_config *config = dev->config;
	struct tmc50xx_stepper_control_data *data = dev->data;
	int err;

	LOG_DBG("Enabling Stallguard for %s", config->stepper->name);
	tmc5xxx_stallguard_enable(config->stepper, false);

	err = tmc50xx_write(config->controller, TMC50XX_RAMPMODE(config->index),
			    TMC5XXX_RAMPMODE_POSITIONING_MODE);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_XTARGET(config->index), micro_steps);
	if (err != 0) {
		return -EIO;
	}

	tmc5xxx_stallguard_enable(config->stepper, true);
	if (data->callback) {
		rampstat_work_reschedule(&data->rampstat_callback_dwork);
	}
	return 0;
}

static int tmc50xx_stepper_control_move_by(const struct device *dev, const int32_t micro_steps)
{
	int err;
	int32_t position;

	err = stepper_control_get_actual_position(dev, &position);
	if (err != 0) {
		return -EIO;
	}
	int32_t target_position = position + micro_steps;

	LOG_DBG("%s moved to %d by steps: %d", dev->name, target_position, micro_steps);

	return tmc50xx_stepper_control_move_to(dev, target_position);
}

static int tmc50xx_stepper_control_run(const struct device *dev,
				       const enum stepper_direction direction)
{
	LOG_DBG("Stepper motor controller %s run", dev->name);
	const struct tmc50xx_stepper_control_config *config = dev->config;
	struct tmc50xx_stepper_control_data *data = dev->data;
	int err;

	tmc5xxx_stallguard_enable(dev, false);

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

	tmc5xxx_stallguard_enable(dev, true);

	if (data->callback) {
		rampstat_work_reschedule(&data->rampstat_callback_dwork);
	}
	return 0;
}

static int tmc50xx_stepper_control_set_reference_position(const struct device *dev,
							  const int32_t position)
{
	const struct tmc50xx_stepper_control_config *config = dev->config;
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

static int tmc50xx_stepper_control_get_actual_position(const struct device *dev, int32_t *position)
{
	const struct tmc50xx_stepper_control_config *config = dev->config;
	int err;

	err = read_actual_position(config, position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("%s actual position: %d", dev->name, *position);
	return 0;
}

static int tmc50xx_stepper_control_is_moving(const struct device *dev, bool *is_moving)
{
	const struct tmc50xx_stepper_control_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_DRVSTATUS(config->index), &reg_value);

	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", dev->name);
		return -EIO;
	}

	*is_moving = (FIELD_GET(TMC5XXX_DRV_STATUS_STST_BIT, reg_value) == 1U);
	LOG_DBG("Stepper motor controller %s is moving: %d", dev->name, *is_moving);
	return 0;
}

static int tmc50xx_stepper_control_set_mode(const struct device *dev,
					    const enum stepper_control_mode mode)
{
	if (mode == STEPPER_CONTROL_MODE_CONSTANT_SPEED) {
		return -ENOTSUP;
	}
	return 0;
}

int tmc50xx_stepper_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	const struct tmc50xx_stepper_control_config *config = dev->config;
	const uint32_t clock_frequency = config->clock_frequency;
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

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN

int tmc50xx_stepper_control_set_ramp(const struct device *dev,
				     const struct tmc_ramp_generator_data *ramp_data)
{
	LOG_DBG("Stepper motor controller %s set ramp", dev->name);
	const struct tmc50xx_stepper_control_config *config = dev->config;
	int err;

	err = tmc50xx_write(config->controller, TMC50XX_VSTART(config->index), ramp_data->vstart);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_A1(config->index), ramp_data->a1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_AMAX(config->index), ramp_data->amax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_D1(config->index), ramp_data->d1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_DMAX(config->index), ramp_data->dmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_V1(config->index), ramp_data->v1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_VMAX(config->index), ramp_data->vmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_VSTOP(config->index), ramp_data->vstop);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_TZEROWAIT(config->index),
			    ramp_data->tzerowait);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_VHIGH(config->index), ramp_data->vhigh);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_VCOOLTHRS(config->index),
			    ramp_data->vcoolthrs);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_write(config->controller, TMC50XX_IHOLD_IRUN(config->index),
			    ramp_data->iholdrun);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

#endif

static int tmc50xx_stepper_control_init(const struct device *dev)
{
	const struct tmc50xx_stepper_control_config *config = dev->config;
	struct tmc50xx_stepper_control_data *data = dev->data;
	int err;

	data->dev = dev;

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN
	err = tmc50xx_stepper_control_set_ramp(dev, &config->default_ramp_config);
	if (err != 0) {
		return -EIO;
	}
#endif

	k_work_init_delayable(&data->rampstat_callback_dwork, rampstat_work_handler);
	rampstat_work_reschedule(&data->rampstat_callback_dwork);
	return 0;
}

static DEVICE_API(stepper_control, tmc50xx_stepper_control_api) = {
	.move_to = tmc50xx_stepper_control_move_to,
	.move_by = tmc50xx_stepper_control_move_by,
	.run = tmc50xx_stepper_control_run,
	.get_actual_position = tmc50xx_stepper_control_get_actual_position,
	.set_reference_position = tmc50xx_stepper_control_set_reference_position,
	.set_mode = tmc50xx_stepper_control_set_mode,
	.is_moving = tmc50xx_stepper_control_is_moving,
	.set_event_callback = tmc50xx_stepper_control_set_event_callback,
};

#define TMC50XX_STEPPER_CONTROL_DEFINE(inst)                                                       \
	static const struct tmc50xx_stepper_control_config tmc50xx_stepper_control_conf_##inst = { \
		.clock_frequency = DT_PROP(DT_PARENT(DT_DRV_INST(inst)), clock_frequency),         \
		.controller = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.stepper = DEVICE_DT_GET(DT_CHILD(DT_DRV_INST(inst), tmc5xxx_stepper)),            \
		IF_ENABLED(CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN,                                    \
		(.default_ramp_config = TMC_RAMP_DT_SPEC_GET_TMC50XX(DT_DRV_INST(inst))))};        \
	static struct tmc50xx_stepper_control_data tmc50xx_stepper_control_data_##inst;            \
	DEVICE_DT_INST_DEFINE(inst, tmc50xx_stepper_control_init, NULL,                            \
			      &tmc50xx_stepper_control_data_##inst,                                \
			      &tmc50xx_stepper_control_conf_##inst, POST_KERNEL,                   \
			      CONFIG_STEPPER_CONTROL_INIT_PRIORITY, &tmc50xx_stepper_control_api);

DT_INST_FOREACH_STATUS_OKAY(TMC50XX_STEPPER_CONTROL_DEFINE)
