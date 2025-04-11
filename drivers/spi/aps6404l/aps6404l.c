/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT apm_aps6404l

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#include "aps6404l.h"

LOG_MODULE_REGISTER(APS6404L, CONFIG_SPI_LOG_LEVEL);

const struct {
	const uint32_t MHz;
	const uint32_t MaxSize;
} g_SPI_SpeedMax[] = {
	{APS6404L_SPEED_48MHZ, APS6404L_48MHZ_MAX_BYTES},
	{APS6404L_SPEED_24MHZ, APS6404L_24MHZ_MAX_BYTES},
	{APS6404L_SPEED_16MHZ, APS6404L_16MHZ_MAX_BYTES},
	{APS6404L_SPEED_12MHZ, APS6404L_12MHZ_MAX_BYTES},
	{APS6404L_SPEED_8MHZ,
	 APS6404L_8MHZ_MAX_BYTES}
};

static uint32_t g_ui32MaxTransSize;

int aps6404l_command_read(const struct device *dev, uint8_t reg_addr, uint8_t *pData,
			  uint8_t ui32NumBytes)
{
	const struct aps6404l_config *cfg = dev->config;
	uint8_t cmd[1] = {reg_addr};

	struct spi_buf buf[] = {{.buf = cmd, .len = 1}, {.buf = pData, .len = ui32NumBytes}};

	struct spi_buf_set tx = {.buffers = buf, .count = 1};

	struct spi_buf_set rx = {.buffers = buf, .count = 2};

	return spi_transceive_dt(&cfg->spec, &tx, &rx);
}

int aps6404l_write(const struct device *dev, uint8_t *pui8TxBuffer, uint32_t ui32WriteAddress,
		   uint32_t ui32NumBytes)
{
	const struct aps6404l_config *cfg = dev->config;
	uint8_t cmd[] = {APS6404L_WRITE};
	uint8_t address[3] = {0};
	struct spi_buf tx_buf[2] = {
		{.buf = cmd, .len = 1},
		{.buf = pui8TxBuffer, .len = ui32NumBytes},
	};
	struct spi_buf_set tx = {.buffers = tx_buf, .count = 1};
	struct spi_dt_spec spec;

	memcpy(&spec, &cfg->spec, sizeof(spec));
	spec.config.operation |= SPI_HALF_DUPLEX;

	while (ui32NumBytes) {
		tx_buf[0].buf = cmd;
		tx_buf[0].len = 1;
		tx.count = 1;
		spec.config.operation |= SPI_HOLD_ON_CS;

		if (spi_transceive_dt(&spec, &tx, 0)) {
			return -1;
		}

		uint32_t maxSize =
			APS6404L_PAGE_SIZE - (ui32WriteAddress & (APS6404L_PAGE_SIZE - 1));
		uint32_t limit = (maxSize > g_ui32MaxTransSize) ? g_ui32MaxTransSize : maxSize;
		uint32_t size = (ui32NumBytes > limit) ? limit : ui32NumBytes;

		address[0] = (ui32WriteAddress & 0x00FF0000) >> 16;
		address[1] = (ui32WriteAddress & 0x0000FF00) >> 8;
		address[2] = (ui32WriteAddress & 0x000000FF);
		tx_buf->buf = address;
		tx_buf->len = 3;
		tx_buf[1].buf = pui8TxBuffer;
		tx.count = 2;

		spec.config.operation &= ~SPI_HOLD_ON_CS;
		if (spi_write_dt(&spec, &tx)) {
			return -1;
		}

		ui32NumBytes -= size;
		pui8TxBuffer += size;
		ui32WriteAddress += size;
	}
	return 0;
}

int aps6404l_read(const struct device *dev, uint8_t *pui8RxBuffer, uint32_t ui32ReadAddress,
		  uint32_t ui32NumBytes)
{
	const struct aps6404l_config *cfg = dev->config;
	uint8_t cmd[] = {APS6404L_READ};
	uint8_t address[3] = {0};
	struct spi_buf tx_buf[2] = {{.buf = cmd, .len = 1},
				    {.buf = pui8RxBuffer, .len = ui32NumBytes}};
	struct spi_buf_set tx = {.buffers = tx_buf, .count = 1};
	struct spi_buf_set rx = {.buffers = tx_buf, .count = 2};
	struct spi_dt_spec spec;

	memcpy(&spec, &cfg->spec, sizeof(spec));
	spec.config.operation |= SPI_HALF_DUPLEX;

	while (ui32NumBytes) {
		tx_buf[0].buf = cmd;
		tx_buf[0].len = 1;
		tx.count = 1;

		spec.config.operation |= SPI_HOLD_ON_CS;
		if (spi_transceive_dt(&spec, &tx, 0)) {
			return -1;
		}

		uint32_t maxSize =
			APS6404L_PAGE_SIZE - (ui32ReadAddress & (APS6404L_PAGE_SIZE - 1));
		uint32_t limit = (maxSize > g_ui32MaxTransSize) ? g_ui32MaxTransSize : maxSize;
		uint32_t size = (ui32NumBytes > limit) ? limit : ui32NumBytes;

		address[0] = (ui32ReadAddress & 0x00FF0000) >> 16;
		address[1] = (ui32ReadAddress & 0x0000FF00) >> 8;
		address[2] = (ui32ReadAddress & 0x000000FF);
		tx_buf[0].buf = address;
		tx_buf[0].len = 3;
		tx_buf[1].len = size;
		tx_buf[1].buf = pui8RxBuffer;

		spec.config.operation &= ~SPI_HOLD_ON_CS;
		if (spi_transceive_dt(&spec, &tx, &rx)) {
			return -1;
		}

		ui32NumBytes -= size;
		pui8RxBuffer += size;
		ui32ReadAddress += size;
	}
	return 0;
}

/**
 * @brief Reads the ID of the external psram and returns the value.
 *
 * @return  0	  - the read was successful;
 *		  EINVAL - an error occurred.
 */
static int aps6404l_read_id(const struct device *dev, uint8_t *pDeviceID)
{
	/*
	 * Send the command sequence to read the Device ID.
	 */
	if (aps6404l_command_read(dev, APS6404L_READ_ID, pDeviceID, 5) != 0) {
		return -EINVAL;
	}
	return 0;
}

static int aps6404l_chip_init(const struct device *dev)
{
	return 0;
}

/**
 * @brief Initializes communication with the device and checks if the part is
 *			present by reading the device id.
 *
 * @return   0 - the initialization was successful and the device is present;
 *			-1 - an error occurred.
 */
static int aps6404l_init(const struct device *dev)
{
	const struct aps6404l_config *config = dev->config;
	uint8_t value[5] = {0, 0, 0, 0, 0};
	uint32_t ui32DeviceId = 0;
	int err;

	if (!spi_is_ready_dt(&config->spec)) {
		LOG_ERR("spi device not ready: %s", config->spec.bus->name);
		return -EINVAL;
	}

	k_sleep(K_MSEC(5));

	err = aps6404l_read_id(dev, value);

	ui32DeviceId = value[3] | ((value[4] & 0xFF) << 8);
	if (ui32DeviceId != APS6404L_PART_ID) {
		LOG_ERR("wrong part_id 0x%x", ui32DeviceId);
		return -ENODEV;
	}

	g_ui32MaxTransSize = 0;
	for (uint32_t i = 0; i < ARRAY_SIZE(g_SPI_SpeedMax); i++) {
		if (config->spec.config.frequency == g_SPI_SpeedMax[i].MHz) {
			g_ui32MaxTransSize = g_SPI_SpeedMax[i].MaxSize;
		}
	}

	if (aps6404l_chip_init(dev) < 0) {
		return -ENODEV;
	}

	return 0;
}

#define APS6404L_DEFINE(inst)                                                                      \
	static struct aps6404l_data aps6404l_data_##inst;                                          \
                                                                                                   \
	static const struct aps6404l_config aps6404l_config_##inst = {                             \
		.spec = SPI_DT_SPEC_INST_GET(inst,                                                 \
					     (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |              \
					      SPI_WORD_SET(8) | SPI_LINES_SINGLE),                 \
					     0),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, aps6404l_init, NULL, &aps6404l_data_##inst,                    \
			      &aps6404l_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,      \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(APS6404L_DEFINE)
