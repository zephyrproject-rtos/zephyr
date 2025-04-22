/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc50xx

#include <stdlib.h>

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include "adi_tmc_spi.h"
#include "adi_tmc5xxx_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc50xx, CONFIG_STEPPER_LOG_LEVEL);

struct tmc50xx_data {
	struct k_sem sem;
};

struct tmc50xx_config {
	const uint32_t gconf;
	struct spi_dt_spec spi;
	const uint32_t clock_frequency;
};

struct tmc50xx_stepper_data {
	struct k_work_delayable stallguard_dwork;
	/* Work item to run the callback in a thread context. */
#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL
	struct k_work_delayable rampstat_callback_dwork;
#endif
	/* device pointer required to access config in k_work */
	const struct device *stepper;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
};

struct tmc50xx_stepper_config {
	const uint8_t index;
	const uint16_t default_micro_step_res;
	const int8_t sg_threshold;
	const bool is_sg_enabled;
	const uint32_t sg_velocity_check_interval_ms;
	const uint32_t sg_threshold_velocity;
	/* parent controller required for bus communication */
	const struct device *controller;
#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN
	const struct tmc_ramp_generator_data default_ramp_config;
#endif
};

static int read_actual_position(const struct tmc50xx_stepper_config *config, int32_t *position);

static int tmc50xx_write(const struct device *dev, const uint8_t reg_addr, const uint32_t reg_val)
{
	const struct tmc50xx_config *config = dev->config;
	struct tmc50xx_data *data = dev->data;
	const struct spi_dt_spec bus = config->spi;
	int err;

	k_sem_take(&data->sem, K_FOREVER);

	err = tmc_spi_write_register(&bus, TMC5XXX_WRITE_BIT, reg_addr, reg_val);

	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
		return err;
	}
	return 0;
}

static int tmc50xx_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc50xx_config *config = dev->config;
	struct tmc50xx_data *data = dev->data;
	const struct spi_dt_spec bus = config->spi;
	int err;

	k_sem_take(&data->sem, K_FOREVER);

	err = tmc_spi_read_register(&bus, TMC5XXX_ADDRESS_MASK, reg_addr, reg_val);

	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
		return err;
	}
	return 0;
}

static int tmc50xx_stepper_set_event_callback(const struct device *dev,
					      stepper_event_callback_t callback, void *user_data)
{
	struct tmc50xx_stepper_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;
	return 0;
}

static int read_vactual(const struct tmc50xx_stepper_config *config, int32_t *actual_velocity)
{
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_VACTUAL(config->index), actual_velocity);
	if (err) {
		LOG_ERR("Failed to read VACTUAL register");
		return err;
	}

	*actual_velocity = sign_extend(*actual_velocity, TMC_RAMP_VACTUAL_SHIFT);
	if (actual_velocity) {
		LOG_DBG("actual velocity: %d", *actual_velocity);
	}
	return 0;
}

static int stallguard_enable(const struct device *dev, const bool enable)
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

		err = read_vactual(config, &actual_velocity);
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

static void stallguard_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmc50xx_stepper_data *stepper_data =
		CONTAINER_OF(dwork, struct tmc50xx_stepper_data, stallguard_dwork);
	int err;
	const struct tmc50xx_stepper_config *stepper_config = stepper_data->stepper->config;

	err = stallguard_enable(stepper_data->stepper, true);
	if (err == -EAGAIN) {
		k_work_reschedule(dwork, K_MSEC(stepper_config->sg_velocity_check_interval_ms));
	}
	if (err == -EIO) {
		LOG_ERR("Failed to enable stallguard because of I/O error");
		return;
	}
}

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL

static void execute_callback(const struct device *dev, const enum stepper_event event)
{
	struct tmc50xx_stepper_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback registered");
		return;
	}
	data->callback(dev, event, data->event_cb_user_data);
}

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_STALLGUARD_LOG

static void log_stallguard(struct tmc50xx_stepper_data *stepper_data, const uint32_t drv_status)
{
	const struct tmc50xx_stepper_config *stepper_config = stepper_data->stepper->config;
	int32_t position;
	int err;

	err = read_actual_position(stepper_config, &position);
	if (err != 0) {
		LOG_ERR("%s: Failed to read XACTUAL register", stepper_data->stepper->name);
		return;
	}

	const uint8_t sg_result = FIELD_GET(TMC5XXX_DRV_STATUS_SG_RESULT_MASK, drv_status);
	const bool sg_status = FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status);

	LOG_DBG("%s position: %d | sg result: %3d status: %d",
		stepper_data->stepper->name, position, sg_result, sg_status);
}

#endif

static void rampstat_work_reschedule(struct k_work_delayable *rampstat_callback_dwork)
{
	k_work_reschedule(rampstat_callback_dwork,
		K_MSEC(CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
}

static void rampstat_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	struct tmc50xx_stepper_data *stepper_data =
		CONTAINER_OF(dwork, struct tmc50xx_stepper_data, rampstat_callback_dwork);
	const struct tmc50xx_stepper_config *stepper_config = stepper_data->stepper->config;

	__ASSERT_NO_MSG(stepper_config->controller != NULL);

	uint32_t drv_status;
	int err;

	err = tmc50xx_read(stepper_config->controller, TMC50XX_DRVSTATUS(stepper_config->index),
			   &drv_status);
	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", stepper_data->stepper->name);
		return;
	}
#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_STALLGUARD_LOG
	log_stallguard(stepper_data, drv_status);
#endif
	if (FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status) == 1U) {
		LOG_INF("%s: Stall detected", stepper_data->stepper->name);
		err = tmc50xx_write(stepper_config->controller,
				    TMC50XX_RAMPMODE(stepper_config->index),
				    TMC5XXX_RAMPMODE_HOLD_MODE);
		if (err != 0) {
			LOG_ERR("%s: Failed to stop motor", stepper_data->stepper->name);
			return;
		}
	}

	uint32_t rampstat_value;

	err = tmc50xx_read(stepper_config->controller, TMC50XX_RAMPSTAT(stepper_config->index),
			   &rampstat_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to read RAMPSTAT register", stepper_data->stepper->name);
		return;
	}

	const uint8_t ramp_stat_values = FIELD_GET(TMC5XXX_RAMPSTAT_INT_MASK, rampstat_value);

	if (ramp_stat_values > 0) {
		switch (ramp_stat_values) {

		case TMC5XXX_STOP_LEFT_EVENT:
			LOG_DBG("RAMPSTAT %s:Left end-stop detected", stepper_data->stepper->name);
			execute_callback(stepper_data->stepper,
					 STEPPER_EVENT_LEFT_END_STOP_DETECTED);
			break;

		case TMC5XXX_STOP_RIGHT_EVENT:
			LOG_DBG("RAMPSTAT %s:Right end-stop detected", stepper_data->stepper->name);
			execute_callback(stepper_data->stepper,
					 STEPPER_EVENT_RIGHT_END_STOP_DETECTED);
			break;

		case TMC5XXX_POS_REACHED_EVENT:
			LOG_DBG("RAMPSTAT %s:Position reached", stepper_data->stepper->name);
			execute_callback(stepper_data->stepper, STEPPER_EVENT_STEPS_COMPLETED);
			break;

		case TMC5XXX_STOP_SG_EVENT:
			LOG_DBG("RAMPSTAT %s:Stall detected", stepper_data->stepper->name);
			stallguard_enable(stepper_data->stepper, false);
			execute_callback(stepper_data->stepper, STEPPER_EVENT_STALL_DETECTED);
			break;
		default:
			LOG_ERR("Illegal ramp stat bit field");
			break;
		}
	} else {
		rampstat_work_reschedule(&stepper_data->rampstat_callback_dwork);
	}
}

#endif

static int tmc50xx_stepper_enable(const struct device *dev)
{
	LOG_DBG("Enabling Stepper motor controller %s", dev->name);
	const struct tmc50xx_stepper_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_CHOPCONF(config->index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value |= TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	return tmc50xx_write(config->controller, TMC50XX_CHOPCONF(config->index), reg_value);
}

static int tmc50xx_stepper_disable(const struct device *dev)
{
	LOG_DBG("Disabling Stepper motor controller %s", dev->name);
	const struct tmc50xx_stepper_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_CHOPCONF(config->index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	return tmc50xx_write(config->controller, TMC50XX_CHOPCONF(config->index), reg_value);
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

	*is_moving = (FIELD_GET(TMC5XXX_DRV_STATUS_STST_BIT, reg_value) == 1U);
	LOG_DBG("Stepper motor controller %s is moving: %d", dev->name, *is_moving);
	return 0;
}

int tmc50xx_stepper_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	const struct tmc50xx_config *tmc50xx_config = config->controller->config;
	const uint32_t clock_frequency = tmc50xx_config->clock_frequency;
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

static int tmc50xx_stepper_set_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution res)
{
	if (!VALID_MICRO_STEP_RES(res)) {
		LOG_ERR("Invalid micro step resolution %d", res);
		return -ENOTSUP;
	}

	const struct tmc50xx_stepper_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_CHOPCONF(config->index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value |= ((MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - LOG2(res))
		      << TMC5XXX_CHOPCONF_MRES_SHIFT);

	err = tmc50xx_write(config->controller, TMC50XX_CHOPCONF(config->index), reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Stepper motor controller %s set micro step resolution to 0x%x", dev->name,
		reg_value);
	return 0;
}

static int tmc50xx_stepper_get_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution *res)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_CHOPCONF(config->index), &reg_value);
	if (err != 0) {
		return -EIO;
	}
	reg_value &= TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value >>= TMC5XXX_CHOPCONF_MRES_SHIFT;
	*res = (1 << (MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - reg_value));
	LOG_DBG("Stepper motor controller %s get micro step resolution: %d", dev->name, *res);
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

static int read_actual_position(const struct tmc50xx_stepper_config *config, int32_t *position)
{
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_XACTUAL(config->index), position);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

static int tmc50xx_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	const struct tmc50xx_stepper_config *config = dev->config;
	int err;

	err = read_actual_position(config, position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("%s actual position: %d", dev->name, *position);
	return 0;
}

static int tmc50xx_stepper_move_to(const struct device *dev, const int32_t micro_steps)
{
	LOG_DBG("%s set target position to %d", dev->name, micro_steps);
	const struct tmc50xx_stepper_config *config = dev->config;
	struct tmc50xx_stepper_data *data = dev->data;
	int err;

	if (config->is_sg_enabled) {
		stallguard_enable(dev, false);
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
#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL
	if (data->callback) {
		rampstat_work_reschedule(&data->rampstat_callback_dwork);
	}
#endif
	return 0;
}

static int tmc50xx_stepper_move_by(const struct device *dev, const int32_t micro_steps)
{
	int err;
	int32_t position;

	err = stepper_get_actual_position(dev, &position);
	if (err != 0) {
		return -EIO;
	}
	int32_t target_position = position + micro_steps;

	LOG_DBG("%s moved to %d by steps: %d", dev->name, target_position, micro_steps);

	return tmc50xx_stepper_move_to(dev, target_position);
}

static int tmc50xx_stepper_run(const struct device *dev, const enum stepper_direction direction)
{
	LOG_DBG("Stepper motor controller %s run", dev->name);
	const struct tmc50xx_stepper_config *config = dev->config;
	struct tmc50xx_stepper_data *data = dev->data;
	int err;

	if (config->is_sg_enabled) {
		err = stallguard_enable(dev, false);
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
#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL
	if (data->callback) {
		rampstat_work_reschedule(&data->rampstat_callback_dwork);
	}
#endif
	return 0;
}

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN

int tmc50xx_stepper_set_ramp(const struct device *dev,
			     const struct tmc_ramp_generator_data *ramp_data)
{
	LOG_DBG("Stepper motor controller %s set ramp", dev->name);
	const struct tmc50xx_stepper_config *config = dev->config;
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

static int tmc50xx_init(const struct device *dev)
{
	LOG_DBG("TMC50XX stepper motor controller %s initialized", dev->name);
	struct tmc50xx_data *data = dev->data;
	const struct tmc50xx_config *config = dev->config;
	int err;

	k_sem_init(&data->sem, 1, 1);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	/* Init non motor-index specific registers here. */
	LOG_DBG("GCONF: %d", config->gconf);
	err = tmc50xx_write(dev, TMC5XXX_GCONF, config->gconf);
	if (err != 0) {
		return -EIO;
	}

	/* Read GSTAT register values to clear any errors SPI Datagram. */
	uint32_t gstat_value;

	err = tmc50xx_read(dev, TMC5XXX_GSTAT, &gstat_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Device %s initialized", dev->name);
	return 0;
}

static int tmc50xx_stepper_init(const struct device *dev)
{
	const struct tmc50xx_stepper_config *stepper_config = dev->config;
	struct tmc50xx_stepper_data *data = dev->data;
	int err;

	LOG_DBG("Controller: %s, Stepper: %s", stepper_config->controller->name, dev->name);

	if (stepper_config->is_sg_enabled) {
		k_work_init_delayable(&data->stallguard_dwork, stallguard_work_handler);

		err = tmc50xx_write(stepper_config->controller,
				    TMC50XX_SWMODE(stepper_config->index), BIT(10));
		if (err != 0) {
			return -EIO;
		}

		LOG_DBG("Setting stall guard to %d with delay %d ms", stepper_config->sg_threshold,
			stepper_config->sg_velocity_check_interval_ms);
		if (!IN_RANGE(stepper_config->sg_threshold, TMC5XXX_SG_MIN_VALUE,
			      TMC5XXX_SG_MAX_VALUE)) {
			LOG_ERR("Stallguard threshold out of range");
			return -EINVAL;
		}

		int32_t stall_guard_threshold = (int32_t)stepper_config->sg_threshold;

		err = tmc50xx_write(
			stepper_config->controller, TMC50XX_COOLCONF(stepper_config->index),
			stall_guard_threshold << TMC5XXX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT);
		if (err != 0) {
			return -EIO;
		}
		k_work_reschedule(&data->stallguard_dwork, K_NO_WAIT);
	}

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN
	err = tmc50xx_stepper_set_ramp(dev, &stepper_config->default_ramp_config);
	if (err != 0) {
		return -EIO;
	}
#endif

#if CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL
	k_work_init_delayable(&data->rampstat_callback_dwork, rampstat_work_handler);
	rampstat_work_reschedule(&data->rampstat_callback_dwork);
#endif
	err = tmc50xx_stepper_set_micro_step_res(dev, stepper_config->default_micro_step_res);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

static DEVICE_API(stepper, tmc50xx_stepper_api) = {
	.enable = tmc50xx_stepper_enable,
	.disable = tmc50xx_stepper_disable,
	.is_moving = tmc50xx_stepper_is_moving,
	.move_by = tmc50xx_stepper_move_by,
	.set_micro_step_res = tmc50xx_stepper_set_micro_step_res,
	.get_micro_step_res = tmc50xx_stepper_get_micro_step_res,
	.set_reference_position = tmc50xx_stepper_set_reference_position,
	.get_actual_position = tmc50xx_stepper_get_actual_position,
	.move_to = tmc50xx_stepper_move_to,
	.run = tmc50xx_stepper_run,
	.set_event_callback = tmc50xx_stepper_set_event_callback,
};

#define TMC50XX_SHAFT_CONFIG(child)								\
	(DT_PROP(child, invert_direction) << TMC50XX_GCONF_SHAFT_SHIFT(DT_REG_ADDR(child))) |

#define TMC50XX_STEPPER_CONFIG_DEFINE(child)							\
	COND_CODE_1(DT_PROP_EXISTS(child, stallguard_threshold_velocity),			\
	BUILD_ASSERT(DT_PROP(child, stallguard_threshold_velocity),				\
		     "stallguard threshold velocity must be a positive value"), ());		\
	IF_ENABLED(CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN, (CHECK_RAMP_DT_DATA(child)));		\
	static const struct tmc50xx_stepper_config tmc50xx_stepper_config_##child = {		\
		.controller = DEVICE_DT_GET(DT_PARENT(child)),					\
		.default_micro_step_res = DT_PROP(child, micro_step_res),			\
		.index = DT_REG_ADDR(child),							\
		.sg_threshold = DT_PROP(child, stallguard2_threshold),				\
		.sg_threshold_velocity = DT_PROP(child, stallguard_threshold_velocity),		\
		.sg_velocity_check_interval_ms = DT_PROP(child,					\
						stallguard_velocity_check_interval_ms),		\
		.is_sg_enabled = DT_PROP(child, activate_stallguard2),				\
		IF_ENABLED(CONFIG_STEPPER_ADI_TMC50XX_RAMP_GEN,					\
		(.default_ramp_config = TMC_RAMP_DT_SPEC_GET_TMC50XX(child))) };

#define TMC50XX_STEPPER_DATA_DEFINE(child)							\
	static struct tmc50xx_stepper_data tmc50xx_stepper_data_##child = {			\
		.stepper = DEVICE_DT_GET(child),};

#define TMC50XX_STEPPER_DEFINE(child)								\
	DEVICE_DT_DEFINE(child, tmc50xx_stepper_init, NULL, &tmc50xx_stepper_data_##child,	\
			 &tmc50xx_stepper_config_##child, POST_KERNEL,				\
			 CONFIG_STEPPER_INIT_PRIORITY, &tmc50xx_stepper_api);

#define TMC50XX_DEFINE(inst)									\
	BUILD_ASSERT(DT_INST_CHILD_NUM(inst) <= 2, "tmc50xx can drive two steppers at max");	\
	BUILD_ASSERT((DT_INST_PROP(inst, clock_frequency) > 0),					\
		     "clock frequency must be non-zero positive value");			\
	static struct tmc50xx_data tmc50xx_data_##inst;						\
	static const struct tmc50xx_config tmc50xx_config_##inst = {				\
		.gconf = (									\
		(DT_INST_PROP(inst, poscmp_enable) << TMC50XX_GCONF_POSCMP_ENABLE_SHIFT) |	\
		(DT_INST_PROP(inst, test_mode) << TMC50XX_GCONF_TEST_MODE_SHIFT) |		\
		DT_INST_FOREACH_CHILD(inst, TMC50XX_SHAFT_CONFIG)				\
		(DT_INST_PROP(inst, lock_gconf) << TMC50XX_LOCK_GCONF_SHIFT)),			\
		.spi = SPI_DT_SPEC_INST_GET(inst, (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |	\
					SPI_MODE_CPOL | SPI_MODE_CPHA |	SPI_WORD_SET(8)), 0),	\
		.clock_frequency = DT_INST_PROP(inst, clock_frequency),};			\
	DT_INST_FOREACH_CHILD(inst, TMC50XX_STEPPER_CONFIG_DEFINE);				\
	DT_INST_FOREACH_CHILD(inst, TMC50XX_STEPPER_DATA_DEFINE);				\
	DT_INST_FOREACH_CHILD(inst, TMC50XX_STEPPER_DEFINE);					\
	DEVICE_DT_INST_DEFINE(inst, tmc50xx_init, NULL, &tmc50xx_data_##inst,			\
			      &tmc50xx_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(TMC50XX_DEFINE)
