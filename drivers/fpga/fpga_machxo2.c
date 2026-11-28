/*
 * Copyright (c) 2026 FIRST RF
 * SPDX-License-Identifier: Apache-2.0
 *
 * Lattice MachXO2 CPLD/FPGA sysCONFIG slave SPI driver.
 *
 * The programming sequence follows the sysCONFIG command set described in the
 * MachXO2 Family Data Sheet and mirrors the Linux machxo2-spi driver.
 */

#define DT_DRV_COMPAT lattice_machxo2_fpga

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "fpga_machxo2.h"

LOG_MODULE_REGISTER(fpga_machxo2, CONFIG_FPGA_LOG_LEVEL);

struct fpga_machxo2_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec done;
	struct gpio_dt_spec initn;
	uint32_t reset_delay_us;
};

struct fpga_machxo2_data {
	struct k_mutex lock;
	uint32_t crc;
	uint32_t image_size;
	bool loaded;
	bool on;
	char info[160];
};

#define FPGA_MACHXO2_MAX_SPI_HZ    66000000U
#define MACHXO2_LOW_DELAY_US       5U
#define MACHXO2_HIGH_DELAY_US      200U
#define MACHXO2_REFRESH_US         4800U
#define MACHXO2_INIT_TIMEOUT_MS    100U
#define MACHXO2_REFRESH_TIMEOUT_MS 1000U
#define MACHXO2_ERASE_TIMEOUT_MS   60000U

static bool machxo2_status_ready(uint32_t status)
{
	return ((status & FPGA_MACHXO2_STATUS_BUSY) == 0U) &&
	       ((status & FPGA_MACHXO2_STATUS_DONE) != 0U) &&
	       ((status & FPGA_MACHXO2_STATUS_FAIL) == 0U);
}

static bool machxo2_status_fail(uint32_t status)
{
	return ((status & FPGA_MACHXO2_STATUS_FAIL) != 0U);
}

int fpga_machxo2_lock(const struct device *dev, k_timeout_t timeout)
{
	struct fpga_machxo2_data *data = dev->data;

	return k_mutex_lock(&data->lock, timeout);
}

void fpga_machxo2_unlock(const struct device *dev)
{
	struct fpga_machxo2_data *data = dev->data;

	k_mutex_unlock(&data->lock);
}

int fpga_machxo2_transceive(const struct device *dev, struct fpga_machxo2_cmd *cmd,
			    const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len)
{

	/**
	 *  Transaction Format:
	 * 	TX: [CMD][OP][OP][OP]
	 * 	RX: [   ][  ][  ][  ][P(n)][P(n)][P(n+1)]
	 *  For multiple receives the first page will be repeated.
	 */

	const struct fpga_machxo2_config *config = dev->config;
	struct spi_buf tx_bufs[3];
	struct spi_buf rx_bufs[3];
	struct spi_buf_set tx;
	struct spi_buf_set rx;
	int ret;
	int nops;

	if (tx_data != NULL && rx_data != NULL) {
		return -EINVAL;
	}

	if ((tx_data == NULL) && (tx_len != 0U)) {
		return -EINVAL;
	}

	if ((rx_data == NULL) && (rx_len != 0U)) {
		return -EINVAL;
	}

	tx_bufs[0].buf = &cmd->cmd;
	tx_bufs[0].len = sizeof(cmd->cmd);
	tx_bufs[1].buf = &cmd->op;
	tx_bufs[1].len = cmd->op_len;
	tx_bufs[2].buf = (void *)tx_data;
	tx_bufs[2].len = tx_len;

	rx_bufs[0].buf = NULL;
	rx_bufs[0].len = sizeof(cmd->cmd);
	rx_bufs[1].buf = NULL;
	rx_bufs[1].len = cmd->op_len;
	rx_bufs[2].buf = (void *)rx_data;
	rx_bufs[2].len = rx_len;

	int nbuf = (rx_data != NULL || tx_data != NULL) ? 3 : 2;
	tx.buffers = tx_bufs;
	tx.count = nbuf;
	rx.buffers = rx_bufs;
	rx.count = nbuf;

	/* Specific commands requires deadtime to be inserted; otherwise,
	 * cmd, op, and data can be one spi_buf_set. See figure 10.7,
	 * MACHXO2-Programming-and-Configuration-User-Guide
	 */

	if (cmd == FPGA_MACHXO2_CMD_LSC_READ_INCR_NV) {

		struct spi_dt_spec hold_cs_bus = config->bus;
		hold_cs_bus.config.operation |= SPI_HOLD_ON_CS;

		tx.buffers = tx_bufs;
		tx.count = 1;
		rx.buffers = rx_bufs;
		rx.count = 1;
		ret = spi_transceive_dt(&config->bus, &tx, &rx);
		if (ret != 0) {
			goto error_exit;
		}

		k_busy_wait(1);

		tx.buffers = &tx_bufs[1];
		tx.count = nbuf - 1;
		rx.buffers = &rx_bufs[1];
		rx.count = nbuf - 1;
		ret = spi_transceive_dt(&config->bus, &tx, &rx);
		if (ret != 0) {
			goto error_exit;
		}
		spi_release_dt(&config->bus);

	} else {
		ret = spi_transceive_dt(&config->bus, &tx, &rx);
		if (ret != 0) {
			goto error_exit;
		}
	}

	return ret;

error_exit:
	LOG_ERR("SPI command 0x%02x failed: %d", cmd, ret);
	return ret;
}

static int machxo2_read_status(const struct device *dev, uint32_t *status)
{
	uint8_t buf[4];
	int ret;
	struct fpga_machxo2_cmd cmd = {
		.cmd = FPGA_MACHXO2_CMD_LSC_READ_STATUS,
		.op = FPGA_MACHXO2_OP_NONE,
		.op_len = 2,
	};

	ret = fpga_machxo2_transceive(dev, &cmd, NULL, 0, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	*status = sys_get_be32(buf);

	return 0;
}

static int machxo2_read_idcode(const struct device *dev, uint32_t *idcode)
{
	uint8_t buf[4];
	int ret;
	struct fpga_machxo2_cmd cmd = {
		.cmd = FPGA_MACHXO2_CMD_IDCODE_PUB,
		.op = FPGA_MACHXO2_OP_NONE,
		.op_len = 3,
	};

	ret = fpga_machxo2_transceive(dev, &cmd, NULL, 0, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	*idcode = sys_get_be32(buf);

	return 0;
}

int machxo2_wait_not_busy(const struct device *dev, uint32_t timeout_ms)
{
	uint32_t start = k_uptime_get_32();
	uint32_t status;
	int ret;

	for (;;) {
		ret = machxo2_read_status(dev, &status);
		if (ret != 0) {
			return ret;
		}

		if ((status & FPGA_MACHXO2_STATUS_BUSY) == 0U) {
			return 0;
		}

		if ((k_uptime_get_32() - start) >= timeout_ms) {
			return -ETIMEDOUT;
		}

		k_msleep(1);
	}
}

static int machxo2_refresh(const struct device *dev)
{
	uint32_t start = k_uptime_get_32();
	uint32_t status;
	int ret;
	struct fpga_machxo2_cmd cmd = {
		.cmd = FPGA_MACHXO2_CMD_LSC_REFRESH,
		.op = FPGA_MACHXO2_OP_NONE,
		.op_len = 2,
	};

	ret = fpga_machxo2_transceive(dev, &cmd, NULL, 0, NULL, 0);
	if (ret != 0) {
		return ret;
	}

	/* FIXME: Do we need this? */
	k_busy_wait(MACHXO2_REFRESH_US);

	for (;;) {

		ret = machxo2_read_status(dev, &status);
		if (ret != 0) {
			return ret;
		}

		if (machxo2_status_ready(status)) {
			return 0;
		}

		if ((k_uptime_get_32() - start) >= MACHXO2_REFRESH_TIMEOUT_MS) {
			return -ETIMEDOUT;
		}
	}
}

static int machxo2_done(const struct device *dev)
{
}

static int machxo2_config(const struct device *dev, bool enable, bool transparent)
{
}

static int machxo2_erase(const struct device *dev, uint8_t sector)
{
}

static int machxo2_write(const struct device *dev, uint8_t sector, uint8_t tx_data, size_t tx_len)
{
}

/* FPGA API */

static enum FPGA_status fpga_machxo2_get_status(const struct device *dev)
{
	struct fpga_machxo2_data *data = dev->data;
	enum FPGA_status status;

	k_mutex_lock(&data->lock, K_FOREVER);
	LOG_ERR("Not implemented");
	k_mutex_unlock(&data->lock);

	return FPGA_STATUS_INACTIVE;
}

static const char *fpga_machxo2_get_info(const struct device *dev)
{
	// const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	// uint32_t idcode;
	// uint32_t status;
	// int done_value = -1;
	// int initn_value = -1;
	// int value;
	// int ret;

	if (k_mutex_lock(&data->lock, K_NO_WAIT) != 0) {
		return FPGA_GET_INFO_DEFAULT;
	}

	// ret = fpga_machxo2_read_idcode(dev, &idcode);
	// if (ret != 0) {
	// 	goto unavailable;
	// }

	// ret = fpga_machxo2_read_status(dev, &status);
	// if (ret != 0) {
	// 	goto unavailable;
	// }

	// if ((idcode == 0U) || (idcode == UINT32_MAX) || (status == 0U) || (status == UINT32_MAX))
	// { 	goto unavailable;
	// }

	// if (config->done.port != NULL) {
	// 	value = gpio_pin_get_dt(&config->done);
	// 	if (value >= 0) {
	// 		done_value = value;
	// 	}
	// }

	// if (config->initn.port != NULL) {
	// 	value = gpio_pin_get_dt(&config->initn);
	// 	if (value >= 0) {
	// 		initn_value = value;
	// 	}
	// }

	// if (data->loaded) {
	// 	snprintf(data->info, sizeof(data->info),
	// 		 "idcode=0x%08x status=0x%08x done=%u busy=%u err=%u done_gpio=%d "
	// 		 "initn_gpio=%d crc32=0x%08x size=%u",
	// 		 idcode, status, (status & FPGA_MACHXO2_STATUS_DONE) != 0U,
	// 		 (status & FPGA_MACHXO2_STATUS_BUSY) != 0U,
	// 		 (status >> FPGA_MACHXO2_STATUS_ERR_POS) & FPGA_MACHXO2_STATUS_ERR_MASK,
	// 		 done_value, initn_value, data->crc, data->image_size);
	// } else {
	// 	snprintf(data->info, sizeof(data->info),
	// 		 "idcode=0x%08x status=0x%08x done=%u busy=%u err=%u done_gpio=%d "
	// 		 "initn_gpio=%d",
	// 		 idcode, status, (status & FPGA_MACHXO2_STATUS_DONE) != 0U,
	// 		 (status & FPGA_MACHXO2_STATUS_BUSY) != 0U,
	// 		 (status >> FPGA_MACHXO2_STATUS_ERR_POS) & FPGA_MACHXO2_STATUS_ERR_MASK,
	// 		 done_value, initn_value);
	// }

	k_mutex_unlock(&data->lock);

	// return data->info;

	// unavailable:
	// 	k_mutex_unlock(&data->lock);

	return FPGA_GET_INFO_DEFAULT;
}

static int fpga_machxo2_on(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	int ret;

	if (config->reset.port == NULL) {
		LOG_ERR("PROGRAMN (reset) pin is required for on/off.");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gpio_pin_set_dt(&config->reset, 0);
	if (ret == 0) {
		data->on = true;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int fpga_machxo2_off(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	int ret;

	if (config->reset.port == NULL) {
		LOG_ERR("PROGRAMN (reset) pin is required for on/off.");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret == 0) {
		data->on = false;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int fpga_machxo2_reset(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (config->reset.port != NULL) {
		/* TODO: Implement */
		LOG_ERR("GPIO reset is not implemented");
	} else {
		ret = machxo2_refresh(dev);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int fpga_machxo2_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	const uint8_t *image = (const uint8_t *)image_ptr;
	uint32_t status;
	int ret;

	if ((image_ptr == NULL) || (img_size == 0U) ||
	    ((img_size % FPGA_MACHXO2_PAGE_SIZE) != 0U)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Enter Configuration Mode */
	machxo2_config(dev, true, true);

	// Why not do all this in config command?
	machxo2_wait_not_busy(dev, 1);
	machxo2_read_status(dev, &status);
	if (machxo2_status_fail(status)) {
		/* clean up and return */
	}

	/* FIXME: Check for failed? */

	/* Erase Flash */
	ret = machxo2_erase(dev, 0);

	/* Write Flash */

	/* Valid Flash */

	/* Set Done */

	return ret;
}

static DEVICE_API(fpga, fpga_machxo2_api) = {
	.get_status = fpga_machxo2_get_status,
	.reset = fpga_machxo2_reset,
	.load = fpga_machxo2_load,
	.on = fpga_machxo2_on,
	.off = fpga_machxo2_off,
	.get_info = fpga_machxo2_get_info,
};

static int fpga_machxo2_init(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("PROGRAMN GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to configure PROGRAMN (reset) GPIO: %d", ret);
			return ret;
		}
	}

	if (config->done.port != NULL) {
		if (!gpio_is_ready_dt(&config->done)) {
			LOG_ERR("DONE GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->done, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("Failed to configure DONE GPIO: %d", ret);
			return ret;
		}
	}

	if (config->initn.port != NULL) {
		if (!gpio_is_ready_dt(&config->initn)) {
			LOG_ERR("INITN GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->initn, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("Failed to configure INITN GPIO: %d", ret);
			return ret;
		}
	}

	data->on = true;

	return 0;
}

#define FPGA_MACHXO2_SPI_OPERATION                                                                 \
	(SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_LINES_SINGLE)

#define FPGA_MACHXO2_INIT(inst)                                                                    \
	BUILD_ASSERT(DT_INST_PROP(inst, spi_max_frequency) <= FPGA_MACHXO2_MAX_SPI_HZ);            \
                                                                                                   \
	static struct fpga_machxo2_data fpga_machxo2_data_##inst;                                  \
                                                                                                   \
	static const struct fpga_machxo2_config fpga_machxo2_config_##inst = {                     \
		.bus = SPI_DT_SPEC_INST_GET(inst, FPGA_MACHXO2_SPI_OPERATION),                     \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		.done = GPIO_DT_SPEC_INST_GET_OR(inst, done_gpios, {0}),                           \
		.initn = GPIO_DT_SPEC_INST_GET_OR(inst, initn_gpios, {0}),                         \
		.reset_delay_us = DT_INST_PROP(inst, reset_delay_us),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fpga_machxo2_init, NULL, &fpga_machxo2_data_##inst,            \
			      &fpga_machxo2_config_##inst, POST_KERNEL, CONFIG_FPGA_INIT_PRIORITY, \
			      &fpga_machxo2_api)

DT_INST_FOREACH_STATUS_OKAY(FPGA_MACHXO2_INIT)
