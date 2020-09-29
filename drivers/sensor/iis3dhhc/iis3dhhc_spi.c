/* ST Microelectronics IIS3DHHC accelerometer sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dhhc.pdf
 */

#define DT_DRV_COMPAT st_iis3dhhc

#include <string.h>
#include "iis3dhhc.h"
#include <logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define IIS3DHHC_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(IIS3DHHC, CONFIG_SENSOR_LOG_LEVEL);

static struct spi_config iis3dhhc_spi_conf = {
	.frequency = DT_INST_PROP(0, spi_max_frequency),
	.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
		      SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE),
	.slave     = DT_INST_REG_ADDR(0),
	.cs        = NULL,
};

static int iis3dhhc_spi_read(struct iis3dhhc_data *ctx, uint8_t reg,
			    uint8_t *data, uint16_t len)
{
	struct spi_config *spi_cfg = &iis3dhhc_spi_conf;
	uint8_t buffer_tx[2] = { reg | IIS3DHHC_SPI_READ, 0 };
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

static int iis3dhhc_spi_write(struct iis3dhhc_data *ctx, uint8_t reg,
			     uint8_t *data, uint16_t len)
{
	struct spi_config *spi_cfg = &iis3dhhc_spi_conf;
	uint8_t buffer_tx[1] = { reg & ~IIS3DHHC_SPI_READ };
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

stmdev_ctx_t iis3dhhc_spi_ctx = {
	.read_reg = (stmdev_read_ptr) iis3dhhc_spi_read,
	.write_reg = (stmdev_write_ptr) iis3dhhc_spi_write,
};

int iis3dhhc_spi_init(const struct device *dev)
{
	struct iis3dhhc_data *data = dev->data;

	data->ctx = &iis3dhhc_spi_ctx;
	data->ctx->handle = data;

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	/* handle SPI CS thru GPIO if it is the case */
	data->cs_ctrl.gpio_dev = device_get_binding(
		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!data->cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	data->cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	data->cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	data->cs_ctrl.delay = 0U;

	iis3dhhc_spi_conf.cs = &data->cs_ctrl;

	LOG_DBG("SPI GPIO CS configured on %s:%u",
		    DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
		    DT_INST_SPI_DEV_CS_GPIOS_PIN(0));
#endif

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
