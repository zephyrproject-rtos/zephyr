/* ST Microelectronics LIS2DS12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2ds12.pdf
 */

#define DT_DRV_COMPAT st_lis2ds12


#include <string.h>
#include <drivers/spi.h>
#include "lis2ds12.h"
#include <logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define LIS2DS12_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(LIS2DS12, CONFIG_SENSOR_LOG_LEVEL);

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
static struct spi_cs_control lis2ds12_cs_ctrl;
#endif

static struct spi_config lis2ds12_spi_conf = {
	.frequency = DT_INST_PROP(0, spi_max_frequency),
	.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
		      SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE),
	.slave     = DT_INST_REG_ADDR(0),
	.cs        = NULL,
};

static int lis2ds12_spi_read(struct lis2ds12_data *data, uint8_t reg,
			     uint8_t *value, uint16_t len)
{
	struct spi_config *spi_cfg = &lis2ds12_spi_conf;
	uint8_t buffer_tx[2] = { reg | LIS2DS12_SPI_READ, 0 };
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


	if (spi_transceive(data->comm_master, spi_cfg, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int lis2ds12_spi_write(struct lis2ds12_data *data, uint8_t reg,
			      uint8_t *value, uint16_t len)
{
	struct spi_config *spi_cfg = &lis2ds12_spi_conf;
	uint8_t buffer_tx[1] = { reg & ~LIS2DS12_SPI_READ };
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

	if (spi_write(data->comm_master, spi_cfg, &tx)) {
		return -EIO;
	}

	return 0;
}

stmdev_ctx_t lis2ds12_spi_ctx = {
	.read_reg = (stmdev_read_ptr)lis2ds12_spi_read,
	.write_reg = (stmdev_write_ptr)lis2ds12_spi_write,
};

int lis2ds12_spi_init(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;

	data->ctx = &lis2ds12_spi_ctx;
	data->ctx->handle = data;

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	/* handle SPI CS thru GPIO if it is the case */
	lis2ds12_cs_ctrl.gpio_dev = device_get_binding(
		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!lis2ds12_cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	lis2ds12_cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	lis2ds12_cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	lis2ds12_cs_ctrl.delay = 0U;

	lis2ds12_spi_conf.cs = &lis2ds12_cs_ctrl;

	LOG_DBG("SPI GPIO CS configured on %s:%u",
		    DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
		    DT_INST_SPI_DEV_CS_GPIOS_PIN(0));
#endif

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
