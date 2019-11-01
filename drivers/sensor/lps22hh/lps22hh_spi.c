/* ST Microelectronics LPS22HH pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hh.pdf
 */


#include <string.h>
#include "lps22hh.h"
#include <logging/log.h>

#ifdef DT_ST_LPS22HH_BUS_SPI

#define LPS22HH_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(LPS22HH, CONFIG_SENSOR_LOG_LEVEL);

static int lps22hh_spi_read(struct device *dev, u8_t reg_addr,
			    u8_t *value, u8_t len)
{
	struct lps22hh_data *data = dev->driver_data;
	const struct lps22hh_config *cfg = dev->config->config_info;
	const struct spi_config *spi_cfg = &cfg->spi_conf;
	u8_t buffer_tx[2] = { reg_addr | LPS22HH_SPI_READ, 0 };
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

static int lps22hh_spi_write(struct device *dev, u8_t reg_addr,
			     u8_t *value, u8_t len)
{
	struct lps22hh_data *data = dev->driver_data;
	const struct lps22hh_config *cfg = dev->config->config_info;
	const struct spi_config *spi_cfg = &cfg->spi_conf;
	u8_t buffer_tx[1] = { reg_addr & ~LPS22HH_SPI_READ };
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

int lps22hh_spi_init(struct device *dev)
{
	struct lps22hh_data *data = dev->driver_data;

	data->ctx_spi.read_reg = (stmdev_read_ptr) lps22hh_spi_read;
	data->ctx_spi.write_reg = (stmdev_write_ptr) lps22hh_spi_write;

	data->ctx = &data->ctx_spi;
	data->ctx->handle = dev;

#if defined(DT_INST_0_ST_LPS22HH_CS_GPIO_CONTROLLER)
	const struct lps22hh_config *cfg = dev->config->config_info;

	/* handle SPI CS thru GPIO if it is the case */
	data->cs_ctrl.gpio_dev = device_get_binding(cfg->gpio_cs_port);
	if (!data->cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	data->cs_ctrl.gpio_pin = cfg->cs_gpio;
	data->cs_ctrl.delay = 0;

	LOG_DBG("SPI GPIO CS configured on %s:%u",
		cfg->gpio_cs_port, cfg->cs_gpio);
#endif

	return 0;
}
#endif /* DT_ST_LPS22HH_BUS_SPI */
