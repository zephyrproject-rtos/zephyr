/* lsm6dsl_spi.c - SPI routines for LSM6DSL driver
 */

/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm6dsl

#include <string.h>
#include <drivers/spi.h>
#include "lsm6dsl.h"
#include <logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define LSM6DSL_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(LSM6DSL, CONFIG_SENSOR_LOG_LEVEL);

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
static struct spi_cs_control lsm6dsl_cs_ctrl;
#endif

#define SPI_CS NULL

static struct spi_config lsm6dsl_spi_conf = {
	.frequency = DT_INST_PROP(0, spi_max_frequency),
	.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
		      SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE),
	.slave     = DT_INST_REG_ADDR(0),
	.cs        = SPI_CS,
};

static int lsm6dsl_raw_read(struct lsm6dsl_data *data, uint8_t reg_addr,
			    uint8_t *value, uint8_t len)
{
	struct spi_config *spi_cfg = &lsm6dsl_spi_conf;
	uint8_t buffer_tx[2] = { reg_addr | LSM6DSL_SPI_READ, 0 };
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

	if (spi_transceive(data->comm_master, spi_cfg, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_raw_write(struct lsm6dsl_data *data, uint8_t reg_addr,
			     uint8_t *value, uint8_t len)
{
	struct spi_config *spi_cfg = &lsm6dsl_spi_conf;
	uint8_t buffer_tx[1] = { reg_addr & ~LSM6DSL_SPI_READ };
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

	if (spi_write(data->comm_master, spi_cfg, &tx)) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_spi_read_data(struct lsm6dsl_data *data, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	return lsm6dsl_raw_read(data, reg_addr, value, len);
}

static int lsm6dsl_spi_write_data(struct lsm6dsl_data *data, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	return lsm6dsl_raw_write(data, reg_addr, value, len);
}

static int lsm6dsl_spi_read_reg(struct lsm6dsl_data *data, uint8_t reg_addr,
				uint8_t *value)
{
	return lsm6dsl_raw_read(data, reg_addr, value, 1);
}

static int lsm6dsl_spi_update_reg(struct lsm6dsl_data *data, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	uint8_t tmp_val;

	lsm6dsl_raw_read(data, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | (value & mask);

	return lsm6dsl_raw_write(data, reg_addr, &tmp_val, 1);
}

static const struct lsm6dsl_transfer_function lsm6dsl_spi_transfer_fn = {
	.read_data = lsm6dsl_spi_read_data,
	.write_data = lsm6dsl_spi_write_data,
	.read_reg  = lsm6dsl_spi_read_reg,
	.update_reg = lsm6dsl_spi_update_reg,
};

int lsm6dsl_spi_init(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;

	data->hw_tf = &lsm6dsl_spi_transfer_fn;

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	/* handle SPI CS thru GPIO if it is the case */
	lsm6dsl_cs_ctrl.gpio_dev = device_get_binding(
		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!lsm6dsl_cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	lsm6dsl_cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	lsm6dsl_cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	lsm6dsl_cs_ctrl.delay = 0U;

	lsm6dsl_spi_conf.cs = &lsm6dsl_cs_ctrl;

	LOG_DBG("SPI GPIO CS configured on %s:%u",
		    DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
		    DT_INST_SPI_DEV_CS_GPIOS_PIN(0));
#endif

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
