/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Stefano Cottafavi
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc51xx

#include <stdlib.h>

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include "adi_tmc_reg.h"
#include "adi_tmc_spi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc51xx, CONFIG_STEPPER_LOG_LEVEL);

struct tmc51xx_data {
	struct k_sem sem;
	struct k_work_delayable stallguard_dwork;
	/* Work item to run the callback in a thread context. */
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL
	struct k_work_delayable rampstat_callback_dwork;
#endif
	/* device pointer required to access config in k_work */
	const struct device *stepper;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
};

struct tmc51xx_config {
	const uint32_t gconf;
	struct spi_dt_spec spi;
	const uint32_t clock_frequency;
	const uint16_t default_micro_step_res;
	/* StallGuard */
	const int8_t sg_threshold;
	const bool is_sg_enabled;
	const uint32_t sg_velocity_check_interval_ms;
	const uint32_t sg_threshold_velocity;
	/* Ramp */
#ifdef CONFIG_STEPPER_ADI_TMC_RAMP_GEN
	const struct tmc_ramp_generator_data default_ramp_config;
#endif
};

static int tmc51xx_write(const struct device *dev, const uint8_t reg_addr, const uint32_t reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	struct tmc51xx_data *data = dev->data;
	const struct spi_dt_spec bus = config->spi;
	int err;

	k_sem_take(&data->sem, K_FOREVER);

	err = tmc_spi_write_register(&bus, TMC51XX_WRITE_BIT, reg_addr, reg_val);

	k_sem_give(&data->sem);

	if (err) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
		return err;
	}
	return 0;
}

static int tmc51xx_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	struct tmc51xx_data *data = dev->data;
	const struct spi_dt_spec bus = config->spi;
	int err;

	k_sem_take(&data->sem, K_FOREVER);

	err = tmc_spi_read_register(&bus, TMC51XX_ADDRESS_MASK, reg_addr, reg_val);

	k_sem_give(&data->sem);

	if (err) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
		return err;
	}
	return 0;
}

static void calculate_velocity_from_hz_to_fclk(const struct device *dev, const uint32_t velocity_hz,
					       uint32_t *const velocity_fclk)
{
	const struct tmc51xx_config *config = dev->config;

	*velocity_fclk =
		((uint64_t)(velocity_hz) << TMC51XX_CLOCK_FREQ_SHIFT) / config->clock_frequency;
	LOG_DBG("Stepper motor controller %s velocity: %d Hz, velocity_fclk: %d", dev->name,
		velocity_hz, *velocity_fclk);
}

static int tmc51xx_stepper_set_event_callback(const struct device *dev,
					      stepper_event_callback_t callback, void *user_data)
{
	struct tmc51xx_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;
	return 0;
}

static int stallguard_enable(const struct device *dev, const bool enable)
{
	const struct tmc51xx_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_SWMODE, &reg_value);
	if (err) {
		LOG_ERR("Failed to read SWMODE register");
		return -EIO;
	}

	if (enable) {
		reg_value |= TMC51XX_SW_MODE_SG_STOP_ENABLE;

		int32_t actual_velocity;

		err = tmc51xx_read(dev, TMC51XX_VACTUAL, &actual_velocity);
		if (err) {
			LOG_ERR("Failed to read VACTUAL register");
			return -EIO;
		}

		actual_velocity = (actual_velocity << (31 - TMC_RAMP_VACTUAL_SHIFT)) >>
				  (31 - TMC_RAMP_VACTUAL_SHIFT);
		LOG_DBG("actual velocity: %d", actual_velocity);

		if (abs(actual_velocity) < config->sg_threshold_velocity) {
			return -EAGAIN;
		}
	} else {
		reg_value &= ~TMC51XX_SW_MODE_SG_STOP_ENABLE;
	}
	err = tmc51xx_write(dev, TMC51XX_SWMODE, reg_value);
	if (err) {
		LOG_ERR("Failed to write SWMODE register");
		return -EIO;
	}
	return 0;
}

static void stallguard_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmc51xx_data *stepper_data =
		CONTAINER_OF(dwork, struct tmc51xx_data, stallguard_dwork);
	int err;

	const struct tmc51xx_config *stepper_config = stepper_data->stepper->config;

	err = stallguard_enable(stepper_data->stepper, true);
	if (err == -EAGAIN) {
		LOG_ERR("retrying stallguard activation");
		k_work_reschedule(dwork, K_MSEC(stepper_config->sg_velocity_check_interval_ms));
	}
	if (err == -EIO) {
		LOG_ERR("Failed to enable stallguard because of I/O error");
		return;
	}
}

#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL

static void execute_callback(const struct device *dev, const enum stepper_event event)
{
	struct tmc51xx_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback registered");
		return;
	}
	data->callback(dev, event, data->event_cb_user_data);
}

static void rampstat_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	struct tmc51xx_data *stepper_data =
		CONTAINER_OF(dwork, struct tmc51xx_data, rampstat_callback_dwork);

	uint32_t drv_status;
	int err;

	tmc51xx_read(stepper_data->stepper, TMC51XX_DRVSTATUS, &drv_status);

	LOG_DBG("ramp workhandler");

	if (FIELD_GET(TMC51XX_DRV_STATUS_SG_STATUS_MASK, drv_status) == 1U) {
		LOG_INF("%s: Stall detected", stepper_data->stepper->name);
		err = tmc51xx_write(stepper_data->stepper, TMC51XX_RAMPMODE,
				    TMC51XX_RAMPMODE_HOLD_MODE);
		if (err != 0) {
			LOG_ERR("%s: Failed to stop motor", stepper_data->stepper->name);
			return;
		}
	}

	uint32_t rampstat_value;

	err = tmc51xx_read(stepper_data->stepper, TMC51XX_RAMPSTAT, &rampstat_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to read RAMPSTAT register", stepper_data->stepper->name);
		return;
	}

	const uint8_t ramp_stat_values = FIELD_GET(TMC51XX_RAMPSTAT_INT_MASK, rampstat_value);

	if (ramp_stat_values > 0) {
		switch (ramp_stat_values) {

		case TMC51XX_STOP_LEFT_EVENT:
			LOG_DBG("RAMPSTAT %s:Left end-stop detected", stepper_data->stepper->name);
			execute_callback(stepper_data->stepper,
					 STEPPER_EVENT_LEFT_END_STOP_DETECTED);
			break;

		case TMC51XX_STOP_RIGHT_EVENT:
			LOG_DBG("RAMPSTAT %s:Right end-stop detected", stepper_data->stepper->name);
			execute_callback(stepper_data->stepper,
					 STEPPER_EVENT_RIGHT_END_STOP_DETECTED);
			break;

		case TMC51XX_POS_REACHED_EVENT:
			LOG_DBG("RAMPSTAT %s:Position reached", stepper_data->stepper->name);
			execute_callback(stepper_data->stepper, STEPPER_EVENT_STEPS_COMPLETED);
			break;

		case TMC51XX_STOP_SG_EVENT:
			LOG_DBG("RAMPSTAT %s:Stall detected", stepper_data->stepper->name);
			stallguard_enable(stepper_data->stepper, false);
			execute_callback(stepper_data->stepper, STEPPER_EVENT_STALL_DETECTED);
			break;
		default:
			LOG_ERR("Illegal ramp stat bit field");
			break;
		}
	} else {
		k_work_reschedule(
			&stepper_data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
	}
}

#endif

static int tmc51xx_stepper_enable(const struct device *dev, const bool enable)
{
	LOG_DBG("Stepper motor controller %s %s", dev->name, enable ? "enabled" : "disabled");
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	if (enable) {
		reg_value |= TMC51XX_CHOPCONF_DRV_ENABLE_MASK;
	} else {
		reg_value &= ~TMC51XX_CHOPCONF_DRV_ENABLE_MASK;
	}

	err = tmc51xx_write(dev, TMC51XX_CHOPCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

static int tmc51xx_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_DRVSTATUS, &reg_value);

	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", dev->name);
		return -EIO;
	}

	*is_moving = (FIELD_GET(TMC51XX_DRV_STATUS_STST_BIT, reg_value) != 1U);
	LOG_DBG("Stepper motor controller %s is moving: %d", dev->name, *is_moving);
	return 0;
}

static int tmc51xx_stepper_move(const struct device *dev, const int32_t steps)
{
	const struct tmc51xx_config *config = dev->config;
	struct tmc51xx_data *data = dev->data;
	int err;

	if (config->is_sg_enabled) {
		err = stallguard_enable(dev, false);
		if (err != 0) {
			return -EIO;
		}
	}

	int32_t position;

	err = stepper_get_actual_position(dev, &position);
	if (err != 0) {
		return -EIO;
	}
	int32_t target_position = position + steps;

	err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC51XX_RAMPMODE_POSITIONING_MODE);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("Stepper motor controller %s moved to %d by steps: %d", dev->name, target_position,
		steps);
	err = tmc51xx_write(dev, TMC51XX_XTARGET, target_position);
	if (err != 0) {
		return -EIO;
	}

	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL
	if (data->callback) {
		k_work_reschedule(
			&data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
	}
#endif
	return 0;
}

static int tmc51xx_stepper_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	uint32_t velocity_fclk;
	int err;

	calculate_velocity_from_hz_to_fclk(dev, velocity, &velocity_fclk);
	err = tmc51xx_write(dev, TMC51XX_VMAX, velocity_fclk);
	if (err != 0) {
		LOG_ERR("%s: Failed to set max velocity", dev->name);
		return -EIO;
	}
	return 0;
}

static int tmc51xx_stepper_set_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution res)
{
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC51XX_CHOPCONF_MRES_MASK;
	reg_value |= ((MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - LOG2(res))
		      << TMC51XX_CHOPCONF_MRES_SHIFT);

	err = tmc51xx_write(dev, TMC51XX_CHOPCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Stepper motor controller %s set micro step resolution to 0x%x", dev->name,
		reg_value);
	return 0;
}

static int tmc51xx_stepper_get_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution *res)
{
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}
	reg_value &= TMC51XX_CHOPCONF_MRES_MASK;
	reg_value >>= TMC51XX_CHOPCONF_MRES_SHIFT;
	*res = (1 << (MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - reg_value));
	LOG_DBG("Stepper motor controller %s get micro step resolution: %d", dev->name, *res);
	return 0;
}

static int tmc51xx_stepper_set_actual_position(const struct device *dev, const int32_t position)
{
	int err;

	err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC51XX_RAMPMODE_HOLD_MODE);
	if (err != 0) {
		return -EIO;
	}

	err = tmc51xx_write(dev, TMC51XX_XACTUAL, position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("Stepper motor controller %s set actual position to %d", dev->name, position);
	return 0;
}

static int tmc51xx_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	int err;

	err = tmc51xx_read(dev, TMC51XX_XACTUAL, position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("%s actual position: %d", dev->name, *position);
	return 0;
}

static int tmc51xx_stepper_set_target_position(const struct device *dev, const int32_t position)
{
	LOG_DBG("Stepper motor controller %s set target position to %d", dev->name, position);
	const struct tmc51xx_config *config = dev->config;
	struct tmc51xx_data *data = dev->data;
	int err;

	if (config->is_sg_enabled) {
		stallguard_enable(dev, false);
	}

	err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC51XX_RAMPMODE_POSITIONING_MODE);
	if (err != 0) {
		return -EIO;
	}
	tmc51xx_write(dev, TMC51XX_XTARGET, position);

	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL
	if (data->callback) {
		k_work_reschedule(
			&data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
	}
#endif
	return 0;
}

static int tmc51xx_stepper_enable_constant_velocity_mode(const struct device *dev,
							 const enum stepper_direction direction,
							 const uint32_t velocity)
{
	LOG_DBG("Stepper motor controller %s enable constant velocity mode", dev->name);
	const struct tmc51xx_config *config = dev->config;
	struct tmc51xx_data *data = dev->data;
	uint32_t velocity_fclk;
	int err;

	calculate_velocity_from_hz_to_fclk(dev, velocity, &velocity_fclk);

	if (config->is_sg_enabled) {
		err = stallguard_enable(dev, false);
		if (err != 0) {
			return -EIO;
		}
	}

	switch (direction) {
	case STEPPER_DIRECTION_POSITIVE:
		err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC51XX_RAMPMODE_POSITIVE_VELOCITY_MODE);
		if (err != 0) {
			return -EIO;
		}
		err = tmc51xx_write(dev, TMC51XX_VMAX, velocity_fclk);
		if (err != 0) {
			return -EIO;
		}
		break;

	case STEPPER_DIRECTION_NEGATIVE:
		err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC51XX_RAMPMODE_NEGATIVE_VELOCITY_MODE);
		if (err != 0) {
			return -EIO;
		}
		err = tmc51xx_write(dev, TMC51XX_VMAX, velocity_fclk);
		if (err != 0) {
			return -EIO;
		}
		break;
	}

	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL
	if (data->callback) {
		k_work_reschedule(
			&data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
	}
#endif
	return 0;
}

#ifdef CONFIG_STEPPER_ADI_TMC_RAMP_GEN

int tmc51xx_stepper_set_ramp(const struct device *dev,
			     const struct tmc_ramp_generator_data *ramp_data)
{
	LOG_DBG("Stepper motor controller %s set ramp", dev->name);
	int err;

	err = tmc51xx_write(dev, TMC51XX_VSTART, ramp_data->vstart);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_A1, ramp_data->a1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_AMAX, ramp_data->amax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_D1, ramp_data->d1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_DMAX, ramp_data->dmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_V1, ramp_data->v1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_VMAX, ramp_data->vmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_VSTOP, ramp_data->vstop);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_TZEROWAIT, ramp_data->tzerowait);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_THIGH, ramp_data->vhigh);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_TCOOLTHRS, ramp_data->vcoolthrs);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_IHOLD_IRUN, ramp_data->iholdrun);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

#endif

static int tmc51xx_init(const struct device *dev)
{
	LOG_DBG("TMC51XX stepper motor controller %s initialized", dev->name);
	struct tmc51xx_data *data = dev->data;
	const struct tmc51xx_config *config = dev->config;
	int err;

	k_sem_init(&data->sem, 1, 1);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	/* Init configuration register */
	err = tmc51xx_write(dev, TMC51XX_GCONF, config->gconf);
	if (err != 0) {
		return -EIO;
	}

	/* Re-write GSTAT to clear (error) status flags */
	err = tmc51xx_write(dev, TMC51XX_GSTAT, 0x07);
	if (err != 0) {
		return -EIO;
	}

	err = tmc51xx_stepper_set_micro_step_res(dev, config->default_micro_step_res);
	if (err != 0) {
		return -EIO;
	}

	if (config->is_sg_enabled) {
		k_work_init_delayable(&data->stallguard_dwork, stallguard_work_handler);

		err = tmc51xx_write(dev, TMC51XX_SWMODE, BIT(10));
		if (err != 0) {
			return -EIO;
		}

		LOG_DBG("Setting stall guard to %d with delay %d ms", config->sg_threshold,
			config->sg_velocity_check_interval_ms);
		if (!IN_RANGE(config->sg_threshold, TMC51XX_SG_MIN_VALUE, TMC51XX_SG_MAX_VALUE)) {
			LOG_ERR("Stallguard threshold out of range");
			return -EINVAL;
		}

		int32_t stall_guard_threshold = (int32_t)config->sg_threshold;

		err = tmc51xx_write(dev, TMC51XX_COOLCONF,
				    stall_guard_threshold
					    << TMC51XX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT);
		if (err != 0) {
			return -EIO;
		}
		err = stallguard_enable(dev, true);
		if (err == -EAGAIN) {
			LOG_ERR("retrying stallguard activation");
			k_work_reschedule(&data->stallguard_dwork,
					  K_MSEC(config->sg_velocity_check_interval_ms));
		}
	}

#ifdef CONFIG_STEPPER_ADI_TMC_RAMP_GEN
	err = tmc51xx_stepper_set_ramp(dev, &config->default_ramp_config);
	if (err != 0) {
		return -EIO;
	}
#endif

#if CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL
	k_work_init_delayable(&data->rampstat_callback_dwork, rampstat_work_handler);
	k_work_reschedule(&data->rampstat_callback_dwork,
			  K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
#endif

	LOG_DBG("Device %s initialized", dev->name);

	return 0;
}

#define TMC_RAMP_DT_SPEC_GET(inst)                                                                 \
	{                                                                                          \
		.vstart = DT_INST_PROP(inst, vstart), .v1 = DT_INST_PROP(inst, v1),                \
		.vmax = DT_INST_PROP(inst, vmax), .a1 = DT_INST_PROP(inst, a1),                    \
		.amax = DT_INST_PROP(inst, amax), .d1 = DT_INST_PROP(inst, d1),                    \
		.dmax = DT_INST_PROP(inst, dmax), .vstop = DT_INST_PROP(inst, vstop),              \
		.tzerowait = DT_INST_PROP(inst, tzerowait),                                        \
		.vcoolthrs = DT_INST_PROP(inst, vcoolthrs), .vhigh = DT_INST_PROP(inst, vhigh),    \
		.iholdrun = (TMC51XX_IRUN(DT_INST_PROP(inst, irun)) |                              \
			     TMC51XX_IHOLD(DT_INST_PROP(inst, ihold)) |                            \
			     TMC51XX_IHOLDDELAY(DT_INST_PROP(inst, iholddelay))),                  \
	}

#define TMC51XX_STEPPER_API_DEFINE(inst)                                                           \
	static const struct stepper_driver_api tmc51xx_api_##inst = {                              \
		.enable = tmc51xx_stepper_enable,                                                  \
		.is_moving = tmc51xx_stepper_is_moving,                                            \
		.move = tmc51xx_stepper_move,                                                      \
		.set_max_velocity = tmc51xx_stepper_set_max_velocity,                              \
		.set_micro_step_res = tmc51xx_stepper_set_micro_step_res,                          \
		.get_micro_step_res = tmc51xx_stepper_get_micro_step_res,                          \
		.set_actual_position = tmc51xx_stepper_set_actual_position,                        \
		.get_actual_position = tmc51xx_stepper_get_actual_position,                        \
		.set_target_position = tmc51xx_stepper_set_target_position,                        \
		.enable_constant_velocity_mode = tmc51xx_stepper_enable_constant_velocity_mode,    \
		.set_event_callback = tmc51xx_stepper_set_event_callback,                          \
	};

#define TMC51XX_DEFINE(inst)                                                                       \
	BUILD_ASSERT((DT_INST_PROP(inst, clock_frequency) > 0),                                    \
		     "clock frequency must be non-zero positive value");                           \
	COND_CODE_1(DT_INST_PROP_EXISTS(inst, stallguard_threshold_velocity), \
	BUILD_ASSERT(DT_INST_PROP(inst, stallguard_threshold_velocity),	\
			"stallguard threshold velocity must be a positive value"), ());                                                                  \
	IF_ENABLED(CONFIG_STEPPER_ADI_TMC_RAMP_GEN, (CHECK_RAMP_DT_DATA(inst)));                    \
	static struct tmc51xx_data tmc51xx_data_##inst = {                                         \
		.stepper = DEVICE_DT_INST_GET(inst),                                               \
	};                                                                                         \
	static const struct tmc51xx_config tmc51xx_config_##inst = {                               \
		.gconf = ((DT_INST_PROP(inst, invert_direction) << TMC51XX_GCONF_SHAFT_SHIFT) |    \
			  (DT_INST_PROP(inst, test_mode) << TMC51XX_GCONF_TEST_MODE_SHIFT)),       \
		.default_micro_step_res = DT_INST_PROP(inst, micro_step_res),                      \
		.spi = SPI_DT_SPEC_INST_GET(inst,                                                  \
					    (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |               \
					     SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8)),     \
					    0),                                                    \
		.clock_frequency = DT_INST_PROP(inst, clock_frequency),                            \
		.is_sg_enabled = DT_INST_PROP(inst, activate_stallguard2),                         \
		COND_CODE_1(DT_INST_PROP(inst, activate_stallguard2), ( \
			.sg_threshold = DT_INST_PROP(inst, stallguard2_threshold), \
			.sg_threshold_velocity = DT_INST_PROP(inst, stallguard_threshold_velocity), \
			.sg_velocity_check_interval_ms = DT_INST_PROP(inst, \
						stallguard_velocity_check_interval_ms),), ()) IF_ENABLED(CONFIG_STEPPER_ADI_TMC_RAMP_GEN,	\
		(.default_ramp_config = TMC_RAMP_DT_SPEC_GET(inst))) };   \
	TMC51XX_STEPPER_API_DEFINE(inst)                                                           \
	DEVICE_DT_INST_DEFINE(inst, tmc51xx_init, NULL, &tmc51xx_data_##inst,                      \
			      &tmc51xx_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,   \
			      &tmc51xx_api_##inst);

DT_INST_FOREACH_STATUS_OKAY(TMC51XX_DEFINE)
