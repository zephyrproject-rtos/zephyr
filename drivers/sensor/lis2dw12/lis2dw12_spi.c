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

LOG_MODULE_DECLARE(LIS2DW12, CONFIG_SENSOR_LOG_LEVEL);

static struct spi_config lis2dw12_spi_conf = {
	.frequency = DT_INST_0_ST_LIS2DW12_SPI_MAX_FREQUENCY,
	.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
		      SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE),
	.slave     = DT_INST_0_ST_LIS2DW12_BASE_ADDRESS,
	.cs        = NULL,
};

static int lis2dw12_spi_read(struct lis2dw12_data *ctx, u8_t reg,
			    u8_t *data, u16_t len)
{
	struct spi_config *spi_cfg = &lis2dw12_spi_conf;
	u8_t buffer_tx[2] = { reg | LIS2DW12_SPI_READ, 0 };
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
			.buf = data,
			.len = len,
		}
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};

	if (spi_transceive(ctx->bus, spi_cfg, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int lis2dw12_spi_write(struct lis2dw12_data *ctx, u8_t reg,
			     u8_t *data, u16_t len)
{
	struct spi_config *spi_cfg = &lis2dw12_spi_conf;
	u8_t buffer_tx[1] = { reg & ~LIS2DW12_SPI_READ };
	const struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};


	if (spi_write(ctx->bus, spi_cfg, &tx)) {
		return -EIO;
	}

	return 0;
}

stmdev_ctx_t lis2dw12_spi_ctx = {
	.read_reg = (stmdev_read_ptr) lis2dw12_spi_read,
	.write_reg = (stmdev_write_ptr) lis2dw12_spi_write,
};

int lis2dw12_spi_init(struct device *dev)
{
	struct lis2dw12_data *data = dev->driver_data;

	data->ctx = &lis2dw12_spi_ctx;
	data->ctx->handle = data;

#if defined(DT_INST_0_ST_LIS2DW12_CS_GPIOS_CONTROLLER)
	/* handle SPI CS thru GPIO if it is the case */
	data->cs_ctrl.gpio_dev = device_get_binding(
		DT_INST_0_ST_LIS2DW12_CS_GPIOS_CONTROLLER);
	if (!data->cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	data->cs_ctrl.gpio_pin = DT_INST_0_ST_LIS2DW12_CS_GPIOS_PIN;
	data->cs_ctrl.delay = 0U;

	lis2dw12_spi_conf.cs = &data->cs_ctrl;

	LOG_DBG("SPI GPIO CS configured on %s:%u",
		    DT_INST_0_ST_LIS2DW12_CS_GPIOS_CONTROLLER,
		    DT_INST_0_ST_LIS2DW12_CS_GPIOS_PIN);
#endif

	return 0;
}
#endif /* DT_ST_LIS2DW12_BUS_SPI */
