/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dw12.pdf
 */


#include <string.h>
#include "lis2dw12.h"
#include <logging/log.h>

#ifdef DT_ST_LIS2DW12_BUS_SPI

#define LIS2DW12_SPI_READ		(1 << 7)

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(LIS2DW12);

static struct spi_config lis2dw12_spi_conf = {
	.frequency = DT_ST_LIS2DW12_0_SPI_MAX_FREQUENCY,
	.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
		      SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE),
	.slave     = DT_ST_LIS2DW12_0_BASE_ADDRESS,
	.cs        = NULL,
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

static int lis2dw12_spi_write_data(struct lis2dw12_data *data, u8_t reg_addr,
				  u8_t *value, u8_t len)
{
	return lis2dw12_raw_write(data, reg_addr, value, len);
}

static int lis2dw12_spi_read_reg(struct lis2dw12_data *data, u8_t reg_addr,
				u8_t *value)
{
	return lis2dw12_raw_read(data, reg_addr, value, 1);
}

static int lis2dw12_spi_write_reg(struct lis2dw12_data *data, u8_t reg_addr,
				u8_t value)
{
	u8_t tmp_val = value;

	return lis2dw12_raw_write(data, reg_addr, &tmp_val, 1);
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
	.read_data = lis2dw12_spi_read_data,
	.write_data = lis2dw12_spi_write_data,
	.read_reg  = lis2dw12_spi_read_reg,
	.write_reg  = lis2dw12_spi_write_reg,
	.update_reg = lis2dw12_spi_update_reg,
};

int lis2dw12_spi_init(struct device *dev)
{
	struct lis2dw12_data *data = dev->driver_data;

	data->hw_tf = &lis2dw12_spi_transfer_fn;

#if defined(DT_ST_LIS2DW12_0_CS_GPIO_CONTROLLER)
	/* handle SPI CS thru GPIO if it is the case */
	data->cs_ctrl.gpio_dev = device_get_binding(
		DT_ST_LIS2DW12_0_CS_GPIO_CONTROLLER);
	if (!data->cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	data->cs_ctrl.gpio_pin = DT_ST_LIS2DW12_0_CS_GPIO_PIN;
	data->cs_ctrl.delay = 0;

	lis2dw12_spi_conf.cs = &data->cs_ctrl;

	LOG_DBG("SPI GPIO CS configured on %s:%u",
		    DT_ST_LIS2DW12_0_CS_GPIO_CONTROLLER,
		    DT_ST_LIS2DW12_0_CS_GPIO_PIN);
#endif

	return 0;
}
#endif /* DT_ST_LIS2DW12_BUS_SPI */
