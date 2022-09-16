/* ST Microelectronics IIS2MDC 3-axis magnetometer sensor
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2mdc.pdf
 */

#define DT_DRV_COMPAT st_iis2mdc

#include <string.h>
#include "iis2mdc.h"
#include <zephyr/logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define IIS2MDC_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(IIS2MDC, CONFIG_SENSOR_LOG_LEVEL);

static int iis2mdc_spi_read(const struct device *dev, uint8_t reg,
			    uint8_t *val, uint16_t len)
{
	const struct iis2mdc_dev_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg;
	uint8_t buffer_tx[2] = { reg | IIS2MDC_SPI_READ, 0 };
	const struct spi_buf tx_buf = {
			.buf = buffer_tx,
			.len = 2,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	const struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = val,
			.len = len,
		}
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};

	if (len > 64) {
		return -EIO;
	}

	if (spi_transceive(cfg->bus, spi_cfg, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int iis2mdc_spi_write(const struct device *dev, uint8_t reg,
			     uint8_t *val, uint16_t len)
{
	const struct iis2mdc_dev_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg;
	uint8_t buffer_tx[1] = { reg & ~IIS2MDC_SPI_READ };
	const struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 1,
		},
		{
			.buf = val,
			.len = len,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};

	if (len > 64) {
		return -EIO;
	}

	if (spi_write(cfg->bus, spi_cfg, &tx)) {
		return -EIO;
	}

	return 0;
}

int iis2mdc_spi_init(const struct device *dev)
{
	struct iis2mdc_data *data = dev->data;
	const struct iis2mdc_dev_config *const cfg = dev->config;
	const struct spi_cs_control *cs_ctrl = cfg->bus_cfg.spi_cfg.cs;

	if (!device_is_ready(cs_ctrl->gpio_dev)) {
		LOG_ERR("Cannot get pointer to CS gpio device");
		return -ENODEV;
	}

	data->ctx_spi.read_reg = (stmdev_read_ptr) iis2mdc_spi_read;
	data->ctx_spi.write_reg = (stmdev_write_ptr) iis2mdc_spi_write;

	data->ctx = &data->ctx_spi;
	data->ctx->handle = (void *)dev;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
