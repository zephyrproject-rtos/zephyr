/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Navimatix GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc2130

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/stepper.h>
#include "../step_dir/step_dir_stepper_common.h"
#include "tmc2130_reg.h"
#include "adi_tmc_spi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc2130, CONFIG_STEPPER_LOG_LEVEL);

struct tmc2130_config {
	struct step_dir_stepper_common_config common;
	struct gpio_dt_spec en_pin;
	struct spi_dt_spec spi;
	bool stealth_chop_enabled;
	uint32_t tpwmthrs;
	uint8_t tpowerdown;
	uint32_t ihold_irun;
	enum stepper_micro_step_resolution default_ustep_res;
};

struct tmc2130_data {
	struct step_dir_stepper_common_data common;
	struct k_sem sem;
};

STEP_DIR_STEPPER_STRUCT_CHECK(struct tmc2130_config, struct tmc2130_data);

static int tmc2130_stepper_enable(const struct device *dev)
{
	const struct tmc2130_config *config = dev->config;
	int ret;

	/* Check availability of the enable pin, as it might be hardwired. */
	if (config->en_pin.port == NULL) {
		LOG_WRN_ONCE("%s: Enable pin undefined.", dev->name);
		return 0;
	}

	ret = gpio_pin_set_dt(&config->en_pin, 1);
	if (ret != 0) {
		LOG_ERR("%s: Failed to set en_pin (error: %d)", dev->name, ret);
	}

	return ret;
}

static int tmc2130_stepper_disable(const struct device *dev)
{
	const struct tmc2130_config *config = dev->config;
	int ret;

	/* Check availability of the enable pin, as it might be hardwired. */
	if (config->en_pin.port == NULL) {
		LOG_WRN_ONCE("%s: Enable pin undefined.", dev->name);
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->en_pin, 0);
	if (ret != 0) {
		LOG_ERR("%s: Failed to set en_pin (error: %d)", dev->name, ret);
	}

	return ret;
}

static int tmc2130_stepper_set_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution micro_step_res)
{
	const struct tmc2130_config *config = dev->config;
	struct tmc2130_data *data = dev->data;
	int ret;
	uint32_t reg_value;

	k_sem_take(&data->sem, K_FOREVER);

	ret = tmc_spi_read_register(&config->spi, TMC2130_ADDRESS_MASK, TMC2130_CHOPCONF,
				    &reg_value);
	if (ret != 0) {
		LOG_ERR("%s: Failed to read register 0x%x (error code: %d)", dev->name,
			TMC2130_CHOPCONF, ret);
		goto set_micro_step_res_end;
	}
	reg_value &= ~TMC2130_CHOPCONF_MRES_MASK;
	reg_value |= ((MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - LOG2(micro_step_res))
		      << TMC2130_CHOPCONF_MRES_SHIFT);

	ret = tmc_spi_write_register(&config->spi, TMC2130_WRITE_BIT, TMC2130_CHOPCONF, reg_value);
	if (ret != 0) {
		LOG_ERR("%s: Failed to write register 0x%x (error code: %d)", dev->name,
			TMC2130_CHOPCONF, ret);
	}

set_micro_step_res_end:
	k_sem_give(&data->sem);

	return ret;
}

static int tmc2130_stepper_get_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution *micro_step_res)
{
	const struct tmc2130_config *config = dev->config;
	struct tmc2130_data *data = dev->data;
	uint32_t reg_value;
	int err;

	k_sem_take(&data->sem, K_FOREVER);

	err = tmc_spi_read_register(&config->spi, TMC2130_ADDRESS_MASK, TMC2130_CHOPCONF,
				    &reg_value);
	if (err != 0) {
		return -EIO;
	}

	k_sem_give(&data->sem);

	reg_value &= TMC2130_CHOPCONF_MRES_MASK;
	reg_value >>= TMC2130_CHOPCONF_MRES_SHIFT;
	*micro_step_res = (1 << (MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - reg_value));

	return 0;
}

static int tmc2130_stepper_init(const struct device *dev)
{
	const struct tmc2130_config *config = dev->config;
	struct tmc2130_data *data = dev->data;
	uint32_t gstat_data;
	int ret;

	uint8_t ustep_res_reg_value =
		(MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - LOG2(config->default_ustep_res));

	/* Read GSTAT register to clear any errors. */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}
	tmc_spi_read_register(&config->spi, TMC2130_ADDRESS_MASK, TMC2130_GSTAT, &gstat_data);
	LOG_DBG("GSTAT: %x", gstat_data);

	/* Configuration registers and their intended values. */
	uint32_t reg_combined[][2] = {
		{TMC2130_CHOPCONF,
		 TMC2130_CHOPCONF_INIT(ustep_res_reg_value, config->common.dual_edge)},
		{TMC2130_IHOLD_IRUN, config->ihold_irun},
		{TMC2130_TPOWERDOWN, TMC2130_TPOWERDOWN_INIT(config->tpowerdown)},
		{TMC2130_GCONF, TMC2130_GCONF_INIT(config->stealth_chop_enabled)},
		{TMC2130_TPWMTHRS, TMC2130_TPWMTHRS_INIT(config->tpwmthrs)},
		{TMC2130_PWMCONF, TMC2130_PWMCONF_INIT}};

	/*Write configuration. */
	for (int i = 0; i < ARRAY_SIZE(reg_combined); i++) {
		ret = tmc_spi_write_register(&config->spi, TMC2130_WRITE_BIT, reg_combined[i][0],
					     reg_combined[i][1]);
		if (ret != 0) {
			LOG_ERR("%s: Failed to write register 0x%x (error code: %d)", dev->name,
				reg_combined[i][0], ret);
			return ret;
		}
	}

	/* Configure enable pin if it is available */
	if (config->en_pin.port != NULL) {
		ret = gpio_pin_configure_dt(&config->en_pin, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("%s: Failed to configure en_pin (error: %d)", dev->name, ret);
			return ret;
		}
	}

	k_sem_init(&data->sem, 1, 1);

	ret = step_dir_stepper_common_init(dev);
	if (ret != 0) {
		LOG_ERR("%s: Failed to initialize common step direction stepper (error: %d)",
			dev->name, ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(stepper, tmc2130_stepper_api) = {
	.enable = tmc2130_stepper_enable,
	.disable = tmc2130_stepper_disable,
	.move_by = step_dir_stepper_common_move_by,
	.is_moving = step_dir_stepper_common_is_moving,
	.set_reference_position = step_dir_stepper_common_set_reference_position,
	.get_actual_position = step_dir_stepper_common_get_actual_position,
	.move_to = step_dir_stepper_common_move_to,
	.set_microstep_interval = step_dir_stepper_common_set_microstep_interval,
	.run = step_dir_stepper_common_run,
	.stop = step_dir_stepper_common_stop,
	.set_event_callback = step_dir_stepper_common_set_event_callback,
	.set_micro_step_res = tmc2130_stepper_set_micro_step_res,
	.get_micro_step_res = tmc2130_stepper_get_micro_step_res,
};

#define TMC2130_CHECK_CONFIGURATION(inst)                                                          \
	BUILD_ASSERT(DT_INST_PROP(inst, tpwmthrs) <= TMC2130_TPWMTHRS_MAX_VALUE,                   \
		     "tpwthrs is too large");                                                      \
	BUILD_ASSERT(DT_INST_PROP(inst, iholddelay) <= TMC2130_IHOLDDELAY_MAX_VALUE,               \
		     "iholddelay is too large");                                                   \
	BUILD_ASSERT(DT_INST_PROP(inst, tpowerdown) <= TMC2130_TPOWERDOWN_MAX_VALUE,               \
		     "tpowerdown is too large");                                                   \
	BUILD_ASSERT(DT_INST_PROP(inst, ihold) <= TMC2130_IHOLD_MAX_VALUE, "ihold is too large");  \
	BUILD_ASSERT(DT_INST_PROP(inst, irun) <= TMC2130_IRUN_MAX_VALUE, "irun is too large");

#define TMC2130_STEPPER_DEVICE(inst)                                                               \
	TMC2130_CHECK_CONFIGURATION(inst);                                                         \
	static const struct tmc2130_config tmc2130_config_##inst = {                               \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                       \
		.en_pin = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),                           \
		.spi = SPI_DT_SPEC_INST_GET(inst,                                                  \
					    (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |               \
					     SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8)),     \
					    0),                                                    \
		.tpwmthrs = DT_INST_PROP(inst, tpwmthrs),                                          \
		.tpowerdown = DT_INST_PROP(inst, tpowerdown),                                      \
		.stealth_chop_enabled = DT_INST_PROP(inst, en_pwm_mode),                           \
		.default_ustep_res = DT_INST_PROP(inst, micro_step_res),                           \
		.ihold_irun = TMC2130_IHOLD_IRUN_INIT(DT_INST_PROP(inst, iholddelay),              \
						      DT_INST_PROP(inst, irun),                    \
						      DT_INST_PROP(inst, ihold)),                  \
	};                                                                                         \
	static struct tmc2130_data tmc2130_data_##inst = {                                         \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_DATA_INIT(inst),                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmc2130_stepper_init, NULL, &tmc2130_data_##inst,              \
			      &tmc2130_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,   \
			      &tmc2130_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(TMC2130_STEPPER_DEVICE)
