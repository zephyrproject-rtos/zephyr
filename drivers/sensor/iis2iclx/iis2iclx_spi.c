/* ST Microelectronics IIS2ICLX 2-axis accelerometer sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2iclx.pdf
 */

#define DT_DRV_COMPAT st_iis2iclx

#include <string.h>
#include "iis2iclx.h"
#include <logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define IIS2ICLX_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(IIS2ICLX, CONFIG_SENSOR_LOG_LEVEL);

static int iis2iclx_spi_read(struct iis2iclx_data *data, uint8_t reg_addr,
			       uint8_t *value, uint8_t len)
{
	const struct device *dev = data->dev;
	const struct iis2iclx_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
	uint8_t buffer_tx[2] = { reg_addr | IIS2ICLX_SPI_READ, 0 };
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
			.buf = value,
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

	if (spi_transceive(data->bus, spi_cfg, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int iis2iclx_spi_write(struct iis2iclx_data *data, uint8_t reg_addr,
				uint8_t *value, uint8_t len)
{
	const struct device *dev = data->dev;
	const struct iis2iclx_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
	uint8_t buffer_tx[1] = { reg_addr & ~IIS2ICLX_SPI_READ };
	const struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 1,
		},
		{
			.buf = value,
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

	if (spi_write(data->bus, spi_cfg, &tx)) {
		return -EIO;
	}

	return 0;
}

int iis2iclx_spi_init(const struct device *dev)
{
	struct iis2iclx_data *data = dev->data;
	const struct iis2iclx_config *cfg = dev->config;
	const struct iis2iclx_spi_cfg *spi_cfg = cfg->bus_cfg.spi_cfg;

	data->ctx_spi.read_reg = (stmdev_read_ptr) iis2iclx_spi_read;
	data->ctx_spi.write_reg = (stmdev_write_ptr) iis2iclx_spi_write;

	data->ctx = &data->ctx_spi;
	data->ctx->handle = data;

	if (spi_cfg->cs_gpios_label != NULL) {
		/* handle SPI CS thru GPIO if it is the case */
		data->cs_ctrl.gpio_dev =
			    device_get_binding(spi_cfg->cs_gpios_label);
		if (!data->cs_ctrl.gpio_dev) {
			LOG_ERR("Unable to get GPIO SPI CS device");
			return -ENODEV;
		}

		LOG_DBG("SPI GPIO CS configured on %s:%u",
			spi_cfg->cs_gpios_label, data->cs_ctrl.gpio_pin);
	}

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
