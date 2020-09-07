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


#define LIS2DH_SPI_READ_BIT		BIT(7)
#define LIS2DH_SPI_AUTOINC		BIT(6)
#define LIS2DH_SPI_ADDR_MASK		BIT_MASK(6)

/* LIS2DH supports only SPI mode 0, word size 8 bits, MSB first */
#define LIS2DH_SPI_CFG			SPI_WORD_SET(8)

static int lis2dh_raw_read(const struct device *dev, uint8_t reg_addr,
			    uint8_t *value, uint8_t len)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
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

static int lis2dh_raw_write(const struct device *dev, uint8_t reg_addr,
			     uint8_t *value, uint8_t len)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
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

static int lis2dh_spi_read_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	return lis2dh_raw_read(dev, reg_addr, value, len);
}

static int lis2dh_spi_write_data(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	return lis2dh_raw_write(dev, reg_addr, value, len);
}

static int lis2dh_spi_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	return lis2dh_raw_read(dev, reg_addr, value, 1);
}

static int lis2dh_spi_write_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t value)
{
	uint8_t tmp_val = value;

	return lis2dh_raw_write(dev, reg_addr, &tmp_val, 1);
}

static int lis2dh_spi_update_reg(const struct device *dev, uint8_t reg_addr,
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

int lis2dh_spi_init(const struct device *dev)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	const struct lis2dh_spi_cfg *spi_cfg = cfg->bus_cfg.spi_cfg;

	data->hw_tf = &lis2dh_spi_transfer_fn;

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
