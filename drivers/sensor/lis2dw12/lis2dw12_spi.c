/*
 * STMicroelectronics LIS2DW12 SPI interface
 *
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spi.h>
#include <string.h>

#include "lis2dw12.h"

#define LIS2DW12_SPI_READ		BIT(7)
#define LIS2DW12_SPI_CS 		NULL

#if defined(CONFIG_LIS2DW12_SPI_GPIO_CS)
static struct spi_cs_control lis2dw12_cs_ctrl;
#endif

static struct spi_config lis2dw12_spi_conf = {
	.frequency = CONFIG_LIS2DW12_SPI_BUS_FREQ,
	.operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |
		      SPI_LINES_SINGLE | SPI_MODE_CPOL | SPI_MODE_CPHA),
	.slave     = CONFIG_LIS2DW12_SPI_SELECT_SLAVE,
	.cs        = LIS2DW12_SPI_CS,
};

static int lis2dw12_raw_read(struct lis2dw12_data *data, u8_t reg_addr,
			      u8_t *value, u8_t len)
{
	struct spi_config *spi_cfg = &lis2dw12_spi_conf;
	u8_t buffer_tx[2] = { reg_addr | LIS2DW12_SPI_READ, 0 };
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

static int lis2dw12_raw_write(struct lis2dw12_data *data, u8_t reg_addr,
			       u8_t *value, u8_t len)
{
	struct spi_config *spi_cfg = &lis2dw12_spi_conf;
	u8_t buffer_tx[1] = { reg_addr & ~LIS2DW12_SPI_READ };
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

static int lis2dw12_spi_read_data(struct lis2dw12_data *data, u8_t reg_addr,
				   u8_t *value, u8_t len)
{
	return lis2dw12_raw_read(data, reg_addr, value, len);
}

static int lis2dw12_spi_write_reg(struct lis2dw12_data *data, u8_t reg_addr,
				   u8_t value)
{
	return lis2dw12_raw_write(data, reg_addr, &value, 1);
}

static int lis2dw12_spi_read_reg(struct lis2dw12_data *data, u8_t reg_addr,
				  u8_t *value)
{
	return lis2dw12_raw_read(data, reg_addr, value, 1);
}

static int lis2dw12_spi_update_reg(struct lis2dw12_data *data, u8_t reg_addr,
				    u8_t mask, u8_t value)
{
	u8_t tmp_val;

	lis2dw12_raw_read(data, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | ((value  << __builtin_ctz(mask)) & mask);

	return lis2dw12_raw_write(data, reg_addr, &tmp_val, 1);
}

static const struct lis2dw12_tf lis2dw12_spi_transfer_fn = {
	.read_reg = lis2dw12_spi_read_reg,
	.write_reg = lis2dw12_spi_write_reg,
	.read_data  = lis2dw12_spi_read_data,
	.update_reg = lis2dw12_spi_update_reg,
};

int lis2dw12_spi_init(struct device *dev)
{
	struct lis2dw12_data *data = dev->driver_data;

	data->hw_tf = &lis2dw12_spi_transfer_fn;

#if defined(CONFIG_LIS2DW12_SPI_GPIO_CS)
	/* handle SPI CS thru GPIO if it is the case */
	if (IS_ENABLED(CONFIG_LIS2DW12_SPI_GPIO_CS)) {
		lis2dw12_cs_ctrl.gpio_dev = device_get_binding(
			CONFIG_LIS2DW12_SPI_GPIO_CS_DRV_NAME);

		if (!lis2dw12_cs_ctrl.gpio_dev) {
			SYS_LOG_ERR("Unable to get GPIO SPI CS device");
			return -ENODEV;
		}

		lis2dw12_cs_ctrl.gpio_pin = CONFIG_LIS2DW12_SPI_GPIO_CS_PIN;
		lis2dw12_cs_ctrl.delay = 0;
		lis2dw12_spi_conf.cs = &lis2dw12_cs_ctrl;

		SYS_LOG_DBG("SPI GPIO CS configured on %s:%u",
			    CONFIG_LIS2DW12_SPI_GPIO_CS_DRV_NAME,
			    CONFIG_LIS2DW12_SPI_GPIO_CS_PIN);
	}
#endif

	return 0;
}
