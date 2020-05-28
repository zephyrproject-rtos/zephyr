/* ST Microelectronics LIS2DH 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dh.pdf
 */

#define DT_DRV_COMPAT st_lis2dh

#include <string.h>
#include "lis2dh.h"
#include <logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

LOG_MODULE_DECLARE(lis2dh, CONFIG_SENSOR_LOG_LEVEL);

static int lis2dh_raw_read(struct device *dev, uint8_t reg_addr,
			    uint8_t *value, uint8_t len)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->spi_conf;
	uint8_t buffer_tx[2] = { reg_addr | LIS2DH_SPI_READ_BIT, 0 };
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

	if (len > 1) {
		buffer_tx[0] |= LIS2DH_SPI_AUTOINC;
	}

	if (spi_transceive(data->bus, spi_cfg, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int lis2dh_raw_write(struct device *dev, uint8_t reg_addr,
			     uint8_t *value, uint8_t len)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->spi_conf;
	uint8_t buffer_tx[1] = { reg_addr & ~LIS2DH_SPI_READ_BIT };
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

	if (len > 1) {
		buffer_tx[0] |= LIS2DH_SPI_AUTOINC;
	}

	if (spi_write(data->bus, spi_cfg, &tx)) {
		return -EIO;
	}

	return 0;
}

static int lis2dh_spi_read_data(struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	return lis2dh_raw_read(dev, reg_addr, value, len);
}

static int lis2dh_spi_write_data(struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	return lis2dh_raw_write(dev, reg_addr, value, len);
}

static int lis2dh_spi_read_reg(struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	return lis2dh_raw_read(dev, reg_addr, value, 1);
}

static int lis2dh_spi_write_reg(struct device *dev, uint8_t reg_addr,
				uint8_t value)
{
	uint8_t tmp_val = value;

	return lis2dh_raw_write(dev, reg_addr, &tmp_val, 1);
}

static int lis2dh_spi_update_reg(struct device *dev, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	uint8_t tmp_val;

	lis2dh_raw_read(dev, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | (value & mask);

	return lis2dh_raw_write(dev, reg_addr, &tmp_val, 1);
}

static const struct lis2dh_transfer_function lis2dh_spi_transfer_fn = {
	.read_data = lis2dh_spi_read_data,
	.write_data = lis2dh_spi_write_data,
	.read_reg  = lis2dh_spi_read_reg,
	.write_reg  = lis2dh_spi_write_reg,
	.update_reg = lis2dh_spi_update_reg,
};

int lis2dh_spi_init(struct device *dev)
{
	struct lis2dh_data *data = dev->data;

	data->hw_tf = &lis2dh_spi_transfer_fn;

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	const struct lis2dh_config *cfg = dev->config;

	/* handle SPI CS thru GPIO if it is the case */
	data->cs_ctrl.gpio_dev = device_get_binding(cfg->gpio_cs_port);
	if (!data->cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	data->cs_ctrl.gpio_pin = cfg->cs_gpio;
	data->cs_ctrl.gpio_dt_flags = cfg->cs_gpio_flags;
	data->cs_ctrl.delay = 0;

	LOG_DBG("SPI GPIO CS configured on %s:%u",
		cfg->gpio_cs_port, cfg->cs_gpio);
#endif

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
