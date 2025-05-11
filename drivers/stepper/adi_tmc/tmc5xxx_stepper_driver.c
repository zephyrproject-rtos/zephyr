/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc5xxx_stepper_driver

#include <stdlib.h>

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>
#include <zephyr/drivers/stepper_control/adi_tmc_spi.h>
#include <zephyr/drivers/stepper/adi_tmc_reg.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc50xx_stepper_driver, CONFIG_STEPPER_LOG_LEVEL);

struct tmc5xxx_stepper_data {
	struct k_work_delayable stallguard_dwork;
	/* device pointer required to access config in k_work */
	const struct device *dev;
	void *event_cb_user_data;
};

struct tmc5xxx_stepper_config {
	const uint8_t index;
	const uint16_t default_micro_step_res;
	/* parent controller required for bus communication, device pointer to tmc50xx */
	const struct device *controller;
	const int8_t sg_threshold;
	const bool is_sg_enabled;
	const uint32_t sg_velocity_check_interval_ms;
	const uint32_t sg_threshold_velocity;
};

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_STALLGUARD_LOG

void tmc5xxx_log_stallguard(const struct device *dev, const uint32_t drv_status)
{
	const uint8_t sg_result = FIELD_GET(TMC5XXX_DRV_STATUS_SG_RESULT_MASK, drv_status);
	const bool sg_status = FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status);

	LOG_DBG("%s: | sg result: %3d status: %d", dev->name, sg_result, sg_status);
}

#endif

static int read_vactual(const struct tmc5xxx_stepper_config *config, int32_t *actual_velocity)
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

int tmc5xxx_stallguard_enable(const struct device *dev, const bool enable)
{
	const struct tmc5xxx_stepper_config *config = dev->config;
	struct tmc5xxx_stepper_data *data = dev->data;

	LOG_DBG("%s stallguard via %s", enable ? "Enabling" : "Disabling",
		config->controller->name);
	uint32_t reg_value;
	int err;

	if (config->is_sg_enabled) {
		return -ENOTSUP;
	}

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
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
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
	struct tmc5xxx_stepper_data *stepper_data =
		CONTAINER_OF(dwork, struct tmc5xxx_stepper_data, stallguard_dwork);
	int err;
	const struct tmc5xxx_stepper_config *stepper_config = stepper_data->dev->config;

	err = tmc5xxx_stallguard_enable(stepper_data->dev, true);
	if (err == -EAGAIN) {
		k_work_reschedule(dwork, K_MSEC(stepper_config->sg_velocity_check_interval_ms));
	}
	if (err == -EIO) {
		LOG_ERR("Failed to enable stallguard because of I/O error");
		return;
	}
}

static int tmc50xx_stepper_enable(const struct device *dev)
{
	const struct tmc5xxx_stepper_config *config = dev->config;
	uint32_t reg_value;
	int err;

	LOG_DBG("Enabling Stepper driver %s for controller %s", dev->name,
		config->controller->name);

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
	const struct tmc5xxx_stepper_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_CHOPCONF(config->index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	return tmc50xx_write(config->controller, TMC50XX_CHOPCONF(config->index), reg_value);
}

static int tmc50xx_stepper_set_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution res)
{
	if (!VALID_MICRO_STEP_RES(res)) {
		LOG_ERR("Invalid micro step resolution %d", res);
		return -ENOTSUP;
	}

	const struct tmc5xxx_stepper_config *config = dev->config;
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
	const struct tmc5xxx_stepper_config *config = dev->config;
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

static int tmc50xx_stepper_init(const struct device *dev)
{
	const struct tmc5xxx_stepper_config *stepper_config = dev->config;
	struct tmc5xxx_stepper_data *data = dev->data;
	int err;

	LOG_DBG("Controller: %s, Stepper: %s", stepper_config->controller->name, dev->name);
	data->dev = dev;
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
	};
	err = tmc50xx_stepper_set_micro_step_res(dev, stepper_config->default_micro_step_res);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

static DEVICE_API(stepper, tmc50xx_stepper_api) = {
	.enable = tmc50xx_stepper_enable,
	.disable = tmc50xx_stepper_disable,
	.set_micro_step_res = tmc50xx_stepper_set_micro_step_res,
	.get_micro_step_res = tmc50xx_stepper_get_micro_step_res,
};

#define TMC5XXX_STEPPER_DRIVER_DEFINE(inst)                                                        \
	COND_CODE_1(DT_PROP_EXISTS(DT_DRV_INST(inst), stallguard_threshold_velocity),              \
	BUILD_ASSERT(DT_INST_PROP(inst, stallguard_threshold_velocity),                            \
	"stallguard threshold velocity must be a positive value"), ());                            \
	static const struct tmc5xxx_stepper_config tmc5xxx_stepper_config_##inst = {               \
		.controller = DEVICE_DT_GET(DT_GPARENT(DT_DRV_INST(inst))),                        \
		.default_micro_step_res = DT_INST_PROP(inst, micro_step_res),                      \
		.index = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(inst))),                                \
		.sg_threshold = DT_INST_PROP(inst, stallguard2_threshold),                         \
		.sg_threshold_velocity = DT_INST_PROP(inst, stallguard_threshold_velocity),        \
		.sg_velocity_check_interval_ms =                                                   \
			DT_INST_PROP(inst, stallguard_velocity_check_interval_ms),                 \
		.is_sg_enabled = DT_INST_PROP(inst, activate_stallguard2),                         \
	};                                                                                         \
	static struct tmc5xxx_stepper_data tmc5xxx_stepper_data_##inst;                            \
	DEVICE_DT_INST_DEFINE(inst, tmc50xx_stepper_init, NULL, &tmc5xxx_stepper_data_##inst,      \
			      &tmc5xxx_stepper_config_##inst, POST_KERNEL,                         \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmc50xx_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(TMC5XXX_STEPPER_DRIVER_DEFINE)
