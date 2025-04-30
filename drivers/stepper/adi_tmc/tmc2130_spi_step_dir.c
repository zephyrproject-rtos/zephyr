/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Navimatix GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc2130_spi_step_dir

#include "adi_tmc_reg.h"
#include "adi_tmc_spi.h"
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include "zephyr/devicetree.h"
#include "zephyr/device.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/stepper.h>
#include "../step_dir/step_dir_stepper_common.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tmc2130, CONFIG_STEPPER_LOG_LEVEL);

#define REG_INIT_NUMBER      6
#define TPWMTHRS_MAX_VALUE   ((1 << 20) - 1)
#define TPOWERDOWN_MAX_VALUE ((1 << 8) - 1)
#define IRUN_MAX_VALUE       ((1 << 5) - 1)
#define IHOLD_MAX_VALUE      ((1 << 5) - 1)
#define IHOLDDELAY_MAX_VALUE ((1 << 4) - 1)

/* Initial register values derived from the "Getting Started" Chapter of the TMC2130 datasheet*/
#define TMC2130_CHOPCONF_INIT                                                                      \
	0x000100C3 /* ms_res=256, TOFF=3, HSTRT=4, HEND=1, TBL=2, CHM=0 (SpreadCycle) */
#define TMC2130_PWMCONF_INIT                                                                       \
	0x000504C8 /* AUTO=1, 2/683 Fclk, Switch amplitude limit=200, Grad=4                       \
		    */

struct tmc2130_spi_sd_config {
	struct step_dir_stepper_common_config common;
	struct gpio_dt_spec en_pin;
	struct spi_dt_spec spi;
	bool stealth_chop_enabled;
	uint32_t tpwmthrs;
	uint8_t irun;
	uint8_t ihold;
	uint8_t iholddelay;
	uint8_t tpowerdown;
};

struct tmc2130_spi_sd_data {
	struct step_dir_stepper_common_data common;
	bool enabled;
	enum stepper_micro_step_resolution ustep_res;
};

STEP_DIR_STEPPER_STRUCT_CHECK(struct tmc2130_spi_sd_config, struct tmc2130_spi_sd_data);

static int tmc2130_spi_sd_stepper_enable(const struct device *dev)
{
	const struct tmc2130_spi_sd_config *config = dev->config;
	struct tmc2130_spi_sd_data *data = dev->data;
	int ret;

	/* Check availability of the enable pin, as it might be hardwired. */
	if (config->en_pin.port == NULL) {
		LOG_WRN_ONCE("%s: Enable pin is not available. The device is always enabled.",
			     dev->name);
		return 0;
	}

	K_SPINLOCK(&data->common.lock) {

		ret = gpio_pin_set_dt(&config->en_pin, 1);
		if (ret != 0) {
			LOG_ERR("%s: Failed to set en_pin (error: %d)", dev->name, ret);
			K_SPINLOCK_BREAK;
		}

		data->enabled = true;
	}

	return ret;
}

static int tmc2130_spi_sd_stepper_disable(const struct device *dev)
{
	const struct tmc2130_spi_sd_config *config = dev->config;
	struct tmc2130_spi_sd_data *data = dev->data;
	int ret;

	/* Check availability of the enable pin, as it might be hardwired. */
	if (config->en_pin.port == NULL) {
		LOG_ERR("%s: Failed to disable device, enable pin is not "
			"available. The device is always enabled.",
			dev->name);
		return -ENOTSUP;
	}

	K_SPINLOCK(&data->common.lock) {
		ret = gpio_pin_set_dt(&config->en_pin, 0);
		if (ret != 0) {
			LOG_ERR("%s: Failed to set en_pin (error: %d)", dev->name, ret);
			K_SPINLOCK_BREAK;
		}
		ret = step_dir_stepper_common_stop(dev);
		if (ret != 0) {
			LOG_ERR("%s: Failed to stop stepper (error: %d)", dev->name, ret);
			K_SPINLOCK_BREAK;
		}
		if (!config->common.dual_edge) {
			ret = gpio_pin_set_dt(&config->common.step_pin, 0);
			if (ret != 0) {
				LOG_ERR("%s: Failed to set step_pin (error: %d)", dev->name, ret);
				K_SPINLOCK_BREAK;
			}
		}
		data->enabled = false;
	}

	return ret;
}

static int
tmc2130_spi_sd_stepper_set_micro_step_res(const struct device *dev,
					  enum stepper_micro_step_resolution micro_step_res)
{
	const struct tmc2130_spi_sd_config *config = dev->config;
	struct tmc2130_spi_sd_data *data = dev->data;
	int ret;
	uint32_t reg_value;

	if (!VALID_MICRO_STEP_RES(micro_step_res)) {
		LOG_ERR("Invalid micro step resolution %d", micro_step_res);
		return -EINVAL;
	}

	K_SPINLOCK(&data->common.lock) {
		ret = tmc_spi_read_register(&config->spi, TMC2130_ADDRESS_MASK, TMC2130_CHOPCONF,
					    &reg_value);
		if (ret != 0) {
			LOG_ERR("%s: Failed to read register 0x%x (error code: %d)", dev->name,
				TMC2130_CHOPCONF, ret);
			K_SPINLOCK_BREAK;
		}
		reg_value &= ~TMC2130_CHOPCONF_MRES_MASK;
		reg_value |= ((MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - LOG2(micro_step_res))
			      << TMC2130_CHOPCONF_MRES_SHIFT);

		ret = tmc_spi_write_register(&config->spi, TMC2130_WRITE_BIT, TMC2130_CHOPCONF,
					     reg_value);
		if (ret != 0) {
			LOG_ERR("%s: Failed to write register 0x%x (error code: %d)", dev->name,
				TMC2130_CHOPCONF, ret);
			K_SPINLOCK_BREAK;
		}

		data->ustep_res = micro_step_res;
	}
	return ret;
}

static int
tmc2130_spi_sd_stepper_get_micro_step_res(const struct device *dev,
					  enum stepper_micro_step_resolution *micro_step_res)
{
	struct tmc2130_spi_sd_data *data = dev->data;
	*micro_step_res = data->ustep_res;
	return 0;
}

static int tmc2130_spi_sd_stepper_move_to(const struct device *dev, int32_t target)
{
	struct tmc2130_spi_sd_data *data = dev->data;

	if (!data->enabled) {
		LOG_ERR("Failed to move to target position, device is not enabled");
		return -ECANCELED;
	}

	return step_dir_stepper_common_move_to(dev, target);
}

static int tmc2130_spi_sd_stepper_move_by(const struct device *dev, int32_t steps)
{
	struct tmc2130_spi_sd_data *data = dev->data;

	if (!data->enabled) {
		LOG_ERR("Failed to move by delta, device is not enabled");
		return -ECANCELED;
	}

	return step_dir_stepper_common_move_by(dev, steps);
}

static int tmc2130_spi_sd_stepper_run(const struct device *dev, enum stepper_direction direction)
{
	struct tmc2130_spi_sd_data *data = dev->data;

	if (!data->enabled) {
		LOG_ERR("Failed to run stepper, device is not enabled");
		return -ECANCELED;
	}

	return step_dir_stepper_common_run(dev, direction);
}

static int tmc2130_spi_sd_stepper_init(const struct device *dev)
{
	const struct tmc2130_spi_sd_config *config = dev->config;
	struct tmc2130_spi_sd_data *data = dev->data;
	uint32_t gstat_data;
	int ret;

	if (!VALID_MICRO_STEP_RES(data->ustep_res)) {
		LOG_ERR("Invalid micro step resolution %d", data->ustep_res);
		return -EINVAL;
	}
	uint8_t ustep_res_reg_value =
		(MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - LOG2(data->ustep_res));
	/* Dual Edge is configured in the same byte as microstep resolution */
	if (config->common.dual_edge) {
		ustep_res_reg_value += TMC2130_DOUBLE_EDGE_OFFSET;
	}

	/* Read GSTAT register to clear any errors. */
	tmc_spi_read_register(&config->spi, TMC2130_ADDRESS_MASK, TMC2130_GSTAT, &gstat_data);
	LOG_DBG("GSTAT: %x", gstat_data);

	/* Configuration registers and their intended values. */
	uint32_t reg_combined[REG_INIT_NUMBER][2] = {
		{TMC2130_CHOPCONF, TMC2130_CHOPCONF_INIT + (ustep_res_reg_value << 24)},
		{TMC2130_IHOLD_IRUN,
		 0x00000000 + (config->iholddelay << 16) + (config->irun << 8) + config->ihold},
		{TMC2130_TPOWERDOWN, 0x00000000 + config->tpowerdown},
		{TMC2130_GCONF,
		 0x00000000 + (config->stealth_chop_enabled << TMC2130_STEALTH_CHOP_SHIFT)},
		{TMC2130_TPWMTHRS, 0x00000000 + config->tpwmthrs},
		{TMC2130_PWMCONF, TMC2130_PWMCONF_INIT}};

	/*Write configuration. */
	for (int i = 0; i < REG_INIT_NUMBER; i++) {
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
	} else {
		data->enabled = true;
	}

	ret = step_dir_stepper_common_init(dev);
	if (ret != 0) {
		LOG_ERR("%s: Failed to initialize common step direction stepper (error: %d)",
			dev->name, ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(stepper, tmc2130_spi_sd_stepper_api) = {
	.enable = tmc2130_spi_sd_stepper_enable,
	.disable = tmc2130_spi_sd_stepper_disable,
	.move_by = tmc2130_spi_sd_stepper_move_by,
	.is_moving = step_dir_stepper_common_is_moving,
	.set_reference_position = step_dir_stepper_common_set_reference_position,
	.get_actual_position = step_dir_stepper_common_get_actual_position,
	.move_to = tmc2130_spi_sd_stepper_move_to,
	.set_microstep_interval = step_dir_stepper_common_set_microstep_interval,
	.run = tmc2130_spi_sd_stepper_run,
	.stop = step_dir_stepper_common_stop,
	.set_event_callback = step_dir_stepper_common_set_event_callback,
	.set_micro_step_res = tmc2130_spi_sd_stepper_set_micro_step_res,
	.get_micro_step_res = tmc2130_spi_sd_stepper_get_micro_step_res,
};

#define TMC2130_SPI_SD_CHECK_CONFIGURATION(inst)                                                   \
	BUILD_ASSERT(DT_INST_PROP(inst, tpwmthrs) <= TPWMTHRS_MAX_VALUE, "tpwthrs is to large");   \
	BUILD_ASSERT(DT_INST_PROP(inst, iholddelay) <= IHOLDDELAY_MAX_VALUE,                       \
		     "iholddelay is to large");                                                    \
	BUILD_ASSERT(DT_INST_PROP(inst, tpowerdown) <= TPOWERDOWN_MAX_VALUE,                       \
		     "tpowerdown is to large");                                                    \
	BUILD_ASSERT(DT_INST_PROP(inst, ihold) <= IHOLD_MAX_VALUE, "ihold is to large");           \
	BUILD_ASSERT(DT_INST_PROP(inst, irun) <= IRUN_MAX_VALUE, "irun is to large");

#define TMC2130_SPI_SD_STEPPER_DEVICE(inst)                                                        \
	TMC2130_SPI_SD_CHECK_CONFIGURATION(inst);                                                  \
	static const struct tmc2130_spi_sd_config tmc2130_spi_sd_config_##inst = {                 \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                       \
		.en_pin = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),                           \
		.spi = SPI_DT_SPEC_INST_GET(inst,                                                  \
					    (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |               \
					     SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8)),     \
					    0),                                                    \
		.tpwmthrs = DT_INST_PROP(inst, tpwmthrs),                                          \
		.irun = DT_INST_PROP(inst, irun),                                                  \
		.ihold = DT_INST_PROP(inst, ihold),                                                \
		.iholddelay = DT_INST_PROP(inst, iholddelay),                                      \
		.tpowerdown = DT_INST_PROP(inst, tpowerdown),                                      \
		.stealth_chop_enabled = DT_INST_PROP(inst, stealth_chop_enabled),                  \
	};                                                                                         \
	static struct tmc2130_spi_sd_data tmc2130_spi_sd_data_##inst = {                           \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_DATA_INIT(inst),                         \
		.ustep_res = DT_INST_PROP(inst, micro_step_res),                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmc2130_spi_sd_stepper_init, NULL,                             \
			      &tmc2130_spi_sd_data_##inst, &tmc2130_spi_sd_config_##inst,          \
			      POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,                           \
			      &tmc2130_spi_sd_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(TMC2130_SPI_SD_STEPPER_DEVICE)
