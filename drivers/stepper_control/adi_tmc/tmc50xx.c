/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc50xx

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/stepper/adi_tmc_reg.h>

#include <stdlib.h>

#include "adi_tmc_spi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc50xx, CONFIG_STEPPER_CONTROL_LOG_LEVEL);

struct tmc50xx_data {
	struct k_sem sem;
};

struct tmc50xx_config {
	const uint32_t gconf;
	struct spi_dt_spec spi;
};

int tmc50xx_write(const struct device *dev, const uint8_t reg_addr, const uint32_t reg_val)
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

int tmc50xx_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
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

static int tmc50xx_init(const struct device *dev)
{
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

#define TMC50XX_DEFINE(inst)                                                                       \
	BUILD_ASSERT(DT_INST_CHILD_NUM(inst) <= 2, "tmc50xx can drive two steppers at max");       \
	BUILD_ASSERT((DT_INST_PROP(inst, clock_frequency) > 0),                                    \
		     "clock frequency must be non-zero positive value");                           \
	static const struct tmc50xx_config tmc50xx_config_##inst = {                               \
		.gconf = ((DT_INST_PROP(inst, poscmp_enable)                                       \
			   << TMC50XX_GCONF_POSCMP_ENABLE_SHIFT) |                                 \
			  (DT_INST_PROP(inst, test_mode) << TMC50XX_GCONF_TEST_MODE_SHIFT) |       \
			  (DT_INST_PROP(inst, shaft1) << TMC50XX_GCONF_SHAFT_SHIFT(0)) |           \
			  (DT_INST_PROP(inst, shaft2) << TMC50XX_GCONF_SHAFT_SHIFT(1)) |           \
			  (DT_INST_PROP(inst, lock_gconf) << TMC50XX_LOCK_GCONF_SHIFT)),           \
		.spi = SPI_DT_SPEC_INST_GET(inst,                                                  \
					    (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |               \
					     SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8)),     \
					    0),                                                    \
	};                                                                                         \
	static struct tmc50xx_data tmc50xx_data_##inst;                                            \
	DEVICE_DT_INST_DEFINE(inst, tmc50xx_init, NULL, &tmc50xx_data_##inst,                      \
			      &tmc50xx_config_##inst, POST_KERNEL,                                 \
			      CONFIG_STEPPER_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TMC50XX_DEFINE)
