/* Bosch BMA4xx 3-axis accelerometer driver
 *
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma4xx

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#include "bma4xx.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

static int bma4xx_spi_read_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	return -ENOTSUP;
}

static int bma4xx_spi_write_data(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	return -ENOTSUP;
}

static int bma4xx_spi_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	return -ENOTSUP;
}

static int bma4xx_spi_write_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t value)
{
	return -ENOTSUP;
}

static int bma4xx_spi_update_reg(const struct device *dev, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	return -ENOTSUP;
}

static const struct bma4xx_hw_operations spi_ops = {
	.read_data = bma4xx_spi_read_data,
	.write_data = bma4xx_spi_write_data,
	.read_reg  = bma4xx_spi_read_reg,
	.write_reg  = bma4xx_spi_write_reg,
	.update_reg = bma4xx_spi_update_reg,
};

int bma4xx_spi_init(const struct device *dev)
{
	struct bma4xx_data *data = dev->data;
	const struct bma4xx_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus_cfg.spi.bus)) {
		LOG_ERR("SPI bus device is not ready");
		return -ENODEV;
	}

	data->hw_ops = &spi_ops;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
