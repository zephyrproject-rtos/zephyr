/* lsm6dsl_spi.c - SPI routines for LSM6DSL driver
 */

/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <spi.h>
#include "lsm6dsl.h"

#define LSM6DSL_SPI_READ		(1 << 7)

#if defined(CONFIG_LSM6DSL_SPI_GPIO_CS)
static struct spi_cs_control lsm6dsl_cs_ctrl;
#endif

#define SPI_CS NULL

static struct spi_config lsm6dsl_spi_conf = {
	.frequency = CONFIG_LSM6DSL_SPI_BUS_FREQ,
	.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
		      SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE),
	.slave     = CONFIG_LSM6DSL_SPI_SELECT_SLAVE,
	.cs        = SPI_CS,
};

static int lsm6dsl_raw_read(struct spi_config *spi_cfg, u8_t reg_addr,
			    u8_t *value, u8_t len)
{
	u8_t buffer_tx[2] = {reg_addr | LSM6DSL_SPI_READ, 0};
	const struct spi_buf tx_buf = {
			.buf = buffer_tx,
			.len = 2,
	};
	struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = value,
			.len = len,
		}
	};

	if (len > 64) {
		return -EIO;
	}

	if (spi_transceive(spi_cfg, &tx_buf, 1, rx_buf, 2)) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_raw_write(struct spi_config *spi_cfg, u8_t reg_addr,
			     u8_t *value, u8_t len)
{
	u8_t buffer_tx[1] = {reg_addr & ~LSM6DSL_SPI_READ};
	struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 1,
		},
		{
			.buf = value,
			.len = len,
		}
	};

	if (len > 64) {
		return -EIO;
	}

	if (spi_transceive(spi_cfg, tx_buf, 2, NULL, 0)) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_spi_read_data(struct lsm6dsl_data *data, u8_t reg_addr,
				 u8_t *value, u8_t len)
{
	struct spi_config *spi_cfg = &lsm6dsl_spi_conf;

	return lsm6dsl_raw_read(spi_cfg, reg_addr, value, len);
}

static int lsm6dsl_spi_write_data(struct lsm6dsl_data *data, u8_t reg_addr,
				  u8_t *value, u8_t len)
{
	struct spi_config *spi_cfg = &lsm6dsl_spi_conf;

	return lsm6dsl_raw_write(spi_cfg, reg_addr, value, len);
}

static int lsm6dsl_spi_read_reg(struct lsm6dsl_data *data, u8_t reg_addr,
				u8_t *value)
{
	struct spi_config *spi_cfg = &lsm6dsl_spi_conf;

	return lsm6dsl_raw_read(spi_cfg, reg_addr, value, 1);
}

static int lsm6dsl_spi_update_reg(struct lsm6dsl_data *data, u8_t reg_addr,
				  u8_t mask, u8_t value)
{
	struct spi_config *spi_cfg = &lsm6dsl_spi_conf;
	u8_t tmp_val;

	lsm6dsl_raw_read(spi_cfg, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | (value & mask);
	return lsm6dsl_raw_write(spi_cfg, reg_addr, &tmp_val, 1);
}

static const struct lsm6dsl_transfer_function lsm6dsl_spi_transfer_fn = {
	.read_data = lsm6dsl_spi_read_data,
	.write_data = lsm6dsl_spi_write_data,
	.read_reg  = lsm6dsl_spi_read_reg,
	.update_reg = lsm6dsl_spi_update_reg,
};

int lsm6dsl_spi_init(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;

	lsm6dsl_spi_conf.dev = data->comm_master;
	data->hw_tf = &lsm6dsl_spi_transfer_fn;

#if defined(CONFIG_LSM6DSL_SPI_GPIO_CS)
	/* handle SPI CS thru GPIO if it is the case */
	if (IS_ENABLED(CONFIG_LSM6DSL_SPI_GPIO_CS)) {
		lsm6dsl_cs_ctrl.gpio_dev = device_get_binding(
			CONFIG_LSM6DSL_SPI_GPIO_CS_DRV_NAME);
		if (!lsm6dsl_cs_ctrl.gpio_dev) {
			SYS_LOG_ERR("Unable to get GPIO SPI CS device");
			return -ENODEV;
		}

		lsm6dsl_cs_ctrl.gpio_pin = CONFIG_LSM6DSL_SPI_GPIO_CS_PIN;
		lsm6dsl_cs_ctrl.delay = 0;

		lsm6dsl_spi_conf.cs = &lsm6dsl_cs_ctrl;

		SYS_LOG_DBG("SPI GPIO CS configured on %s:%u",
			    CONFIG_LSM6DSL_SPI_GPIO_CS_DRV_NAME,
			    CONFIG_LSM6DSL_SPI_GPIO_CS_PIN);
	}
#endif

	return 0;
}
