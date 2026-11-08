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

#define FPGA_MACHXO2_MAX_SPI_HZ 66000000U
#define MACHXO2_LOW_DELAY_US       5U
#define MACHXO2_HIGH_DELAY_US      200U
#define MACHXO2_REFRESH_US         4800U
#define MACHXO2_INIT_TIMEOUT_MS    100U
#define MACHXO2_REFRESH_TIMEOUT_MS 1000U
#define MACHXO2_ERASE_TIMEOUT_MS   60000U

static void machxo2_build_cmd(uint8_t cmd[4], uint8_t opcode, uint32_t operand)
{
	cmd[0] = opcode;
	cmd[1] = (uint8_t)(operand >> 16);
	cmd[2] = (uint8_t)(operand >> 8);
	cmd[3] = (uint8_t)operand;
}

static bool machxo2_status_ready(uint32_t status)
{
	return ((status & FPGA_MACHXO2_STATUS_BUSY) == 0U) &&
	       ((status & FPGA_MACHXO2_STATUS_DONE) != 0U) &&
	       (((status >> FPGA_MACHXO2_STATUS_ERR_POS) & FPGA_MACHXO2_STATUS_ERR_MASK) == 0U);
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

int fpga_machxo2_transceive(const struct device *dev, uint8_t opcode, uint32_t operand,
			    const void *tx_data, size_t tx_len, void *rx_data, size_t rx_len)
{
	const struct fpga_machxo2_config *config = dev->config;
	uint8_t cmd[4];
	struct spi_buf tx_bufs[2];
	struct spi_buf rx_bufs[2];
	struct spi_buf_set tx;
	struct spi_buf_set rx;
	int ret;

	if ((tx_data == NULL) && (tx_len != 0U)) {
		return -EINVAL;
	}

	if ((rx_data == NULL) && (rx_len != 0U)) {
		return -EINVAL;
	}

	machxo2_build_cmd(cmd, opcode, operand);

	tx_bufs[0].buf = cmd;
	tx_bufs[0].len = sizeof(cmd);
	tx_bufs[1].buf = (void *)tx_data;
	tx_bufs[1].len = tx_len;
	tx.buffers = tx_bufs;
	tx.count = (tx_len == 0U) ? 1U : 2U;

	/* The first four RX bytes echo the command and are discarded; a NULL
	 * buffer makes the controller clock them out without storing them.
	 */
	rx_bufs[0].buf = NULL;
	rx_bufs[0].len = sizeof(cmd);
	rx_bufs[1].buf = rx_data;
	rx_bufs[1].len = rx_len;
	rx.buffers = rx_bufs;
	rx.count = (rx_len == 0U) ? 1U : 2U;

	ret = spi_transceive_dt(&config->bus, &tx, &rx);
	if (ret != 0) {
		LOG_ERR("SPI command 0x%02x failed: %d", opcode, ret);
	}

	return ret;
}

static int fpga_machxo2_read_status(const struct device *dev, uint32_t *status)
{
	uint8_t buf[4];
	int ret;

	ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_LSC_READ_STATUS, 0U, NULL, 0U, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	*status = sys_get_be32(buf);

	return 0;
}

static int fpga_machxo2_read_idcode(const struct device *dev, uint32_t *idcode)
{
	uint8_t buf[4];
	int ret;

	ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_IDCODE_PUB, 0U, NULL, 0U, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	*idcode = sys_get_be32(buf);

	return 0;
}

int fpga_machxo2_wait_not_busy(const struct device *dev, uint32_t timeout_ms)
{
	uint32_t start = k_uptime_get_32();
	uint32_t status;
	int ret;

	for (;;) {
		ret = fpga_machxo2_read_status(dev, &status);
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

static int fpga_machxo2_reset_pulse(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	int ret;

	if (config->reset.port == NULL) {
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret != 0) {
		return ret;
	}

	k_busy_wait(config->reset_delay_us);

	return gpio_pin_set_dt(&config->reset, 0);
}

static int fpga_machxo2_done_get(const struct device *dev, int *value)
{
	const struct fpga_machxo2_config *config = dev->config;
	int ret;

	if (config->done.port == NULL) {
		return -ENOTSUP;
	}

	ret = gpio_pin_get_dt(&config->done);
	if (ret < 0) {
		return ret;
	}

	*value = ret;

	return 0;
}

static int fpga_machxo2_initn_get(const struct device *dev, int *value)
{
	const struct fpga_machxo2_config *config = dev->config;
	int ret;

	if (config->initn.port == NULL) {
		return -ENOTSUP;
	}

	ret = gpio_pin_get_dt(&config->initn);
	if (ret < 0) {
		return ret;
	}

	*value = ret;

	return 0;
}

static int machxo2_wait_initn_deassert(const struct device *dev, uint32_t timeout_ms)
{
	const struct fpga_machxo2_config *config = dev->config;
	uint32_t start = k_uptime_get_32();
	int value;

	for (;;) {
		value = gpio_pin_get_dt(&config->initn);
		if (value < 0) {
			return value;
		}

		if (value == 0) {
			return 0;
		}

		if ((k_uptime_get_32() - start) >= timeout_ms) {
			return -ETIMEDOUT;
		}

		k_msleep(1);
	}
}

static int machxo2_reset_and_wait(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	int ret;

	if (config->reset.port != NULL) {
		ret = fpga_machxo2_reset_pulse(dev);
		if (ret != 0) {
			return ret;
		}

		if (config->initn.port != NULL) {
			return machxo2_wait_initn_deassert(dev, MACHXO2_INIT_TIMEOUT_MS);
		}

		k_busy_wait(config->reset_delay_us);

		return 0;
	}

	if (config->initn.port != NULL) {
		return machxo2_wait_initn_deassert(dev, MACHXO2_INIT_TIMEOUT_MS);
	}

	return 0;
}

static int machxo2_refresh(const struct device *dev)
{
	uint32_t start = k_uptime_get_32();
	uint32_t status;
	int ret;

	for (;;) {
		ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_LSC_REFRESH, 0U, NULL, 0U, NULL, 0U);
		if (ret != 0) {
			return ret;
		}

		k_busy_wait(MACHXO2_REFRESH_US);

		ret = fpga_machxo2_read_status(dev, &status);
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

static void machxo2_cleanup(const struct device *dev)
{
	int ret;

	ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_ISC_ERASE,
				      FPGA_MACHXO2_ERASE_CONFIG, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		LOG_ERR("Cleanup erase failed: %d", ret);
		return;
	}

	ret = fpga_machxo2_wait_not_busy(dev, MACHXO2_ERASE_TIMEOUT_MS);
	if (ret != 0) {
		LOG_ERR("Cleanup erase timed out: %d", ret);
		return;
	}

	ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_LSC_REFRESH, 0U, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		LOG_ERR("Cleanup refresh failed: %d", ret);
	}
}

static int machxo2_verify_active(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	uint32_t status;
	int value;
	int ret;

	if (config->done.port != NULL) {
		ret = fpga_machxo2_done_get(dev, &value);
		if (ret != 0) {
			return ret;
		}

		if (value == 0) {
			return -EIO;
		}

		if (config->initn.port != NULL) {
			ret = fpga_machxo2_initn_get(dev, &value);
			if (ret != 0) {
				return ret;
			}

			if (value != 0) {
				return -EIO;
			}
		}

		return 0;
	}

	if (config->initn.port != NULL) {
		ret = fpga_machxo2_initn_get(dev, &value);
		if (ret != 0) {
			return ret;
		}

		if (value != 0) {
			return -EIO;
		}
	}

	ret = fpga_machxo2_read_status(dev, &status);
	if (ret != 0) {
		return ret;
	}

	if (!machxo2_status_ready(status)) {
		return -EIO;
	}

	return 0;
}

static enum FPGA_status machxo2_status_locked(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	uint32_t status;
	int value;
	int ret;

	if (config->initn.port != NULL) {
		value = gpio_pin_get_dt(&config->initn);
		if (value > 0) {
			return FPGA_STATUS_INACTIVE;
		}
	}

	if (config->done.port != NULL) {
		value = gpio_pin_get_dt(&config->done);
		if (value >= 0) {
			return (value > 0) ? FPGA_STATUS_ACTIVE : FPGA_STATUS_INACTIVE;
		}
	}

	ret = fpga_machxo2_read_status(dev, &status);
	if (ret == 0) {
		if (machxo2_status_ready(status)) {
			return FPGA_STATUS_ACTIVE;
		}

		return FPGA_STATUS_INACTIVE;
	}

	return (data->loaded && data->on) ? FPGA_STATUS_ACTIVE : FPGA_STATUS_INACTIVE;
}

static enum FPGA_status fpga_machxo2_get_status(const struct device *dev)
{
	struct fpga_machxo2_data *data = dev->data;
	enum FPGA_status status;

	k_mutex_lock(&data->lock, K_FOREVER);
	status = machxo2_status_locked(dev);
	k_mutex_unlock(&data->lock);

	return status;
}

static const char *fpga_machxo2_get_info(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	uint32_t idcode;
	uint32_t status;
	int done_value = -1;
	int initn_value = -1;
	int value;
	int ret;

	if (k_mutex_lock(&data->lock, K_NO_WAIT) != 0) {
		return FPGA_GET_INFO_DEFAULT;
	}

	ret = fpga_machxo2_read_idcode(dev, &idcode);
	if (ret != 0) {
		goto unavailable;
	}

	ret = fpga_machxo2_read_status(dev, &status);
	if (ret != 0) {
		goto unavailable;
	}

	if ((idcode == 0U) || (idcode == UINT32_MAX) || (status == 0U) || (status == UINT32_MAX)) {
		goto unavailable;
	}

	if (config->done.port != NULL) {
		value = gpio_pin_get_dt(&config->done);
		if (value >= 0) {
			done_value = value;
		}
	}

	if (config->initn.port != NULL) {
		value = gpio_pin_get_dt(&config->initn);
		if (value >= 0) {
			initn_value = value;
		}
	}

	if (data->loaded) {
		snprintf(data->info, sizeof(data->info),
			 "idcode=0x%08x status=0x%08x done=%u busy=%u err=%u done_gpio=%d "
			 "initn_gpio=%d crc32=0x%08x size=%u",
			 idcode, status, (status & FPGA_MACHXO2_STATUS_DONE) != 0U,
			 (status & FPGA_MACHXO2_STATUS_BUSY) != 0U,
			 (status >> FPGA_MACHXO2_STATUS_ERR_POS) & FPGA_MACHXO2_STATUS_ERR_MASK,
			 done_value, initn_value, data->crc, data->image_size);
	} else {
		snprintf(data->info, sizeof(data->info),
			 "idcode=0x%08x status=0x%08x done=%u busy=%u err=%u done_gpio=%d "
			 "initn_gpio=%d",
			 idcode, status, (status & FPGA_MACHXO2_STATUS_DONE) != 0U,
			 (status & FPGA_MACHXO2_STATUS_BUSY) != 0U,
			 (status >> FPGA_MACHXO2_STATUS_ERR_POS) & FPGA_MACHXO2_STATUS_ERR_MASK,
			 done_value, initn_value);
	}

	k_mutex_unlock(&data->lock);

	return data->info;

unavailable:
	k_mutex_unlock(&data->lock);

	return FPGA_GET_INFO_DEFAULT;
}

static int fpga_machxo2_on(const struct device *dev)
{
	const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	int ret;

	if (config->reset.port == NULL) {
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
		ret = machxo2_reset_and_wait(dev);
	} else {
		ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_LSC_REFRESH, 0U, NULL, 0U, NULL, 0U);
		if (ret == 0) {
			ret = fpga_machxo2_wait_not_busy(dev, MACHXO2_REFRESH_TIMEOUT_MS);
		}
	}

	if (ret == 0) {
		data->on = true;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int fpga_machxo2_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	const struct fpga_machxo2_config *config = dev->config;
	struct fpga_machxo2_data *data = dev->data;
	const uint8_t *image = (const uint8_t *)image_ptr;
	uint8_t page[4 + FPGA_MACHXO2_PAGE_SIZE];
	uint32_t status;
	uint32_t offset;
	bool entered = false;
	int ret;

	if ((image_ptr == NULL) || (img_size == 0U) ||
	    ((img_size % FPGA_MACHXO2_PAGE_SIZE) != 0U)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = machxo2_reset_and_wait(dev);
	if (ret != 0) {
		goto unlock;
	}

	ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_ISC_ENABLE,
				      FPGA_MACHXO2_ENABLE_FLASH, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		goto unlock;
	}

	entered = true;
	k_busy_wait(MACHXO2_LOW_DELAY_US);

	ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_ISC_ERASE,
				      FPGA_MACHXO2_ERASE_CONFIG, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		goto unlock;
	}

	ret = fpga_machxo2_wait_not_busy(dev, MACHXO2_ERASE_TIMEOUT_MS);
	if (ret != 0) {
		goto unlock;
	}

	ret = fpga_machxo2_read_status(dev, &status);
	if (ret != 0) {
		goto unlock;
	}

	if ((status & FPGA_MACHXO2_STATUS_FAIL) != 0U) {
		LOG_ERR("Configuration erase failed");
		ret = -EIO;
		goto unlock;
	}

	ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_LSC_INIT_ADDRESS, 0U, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		goto unlock;
	}

	for (offset = 0U; offset < img_size; offset += FPGA_MACHXO2_PAGE_SIZE) {
		struct spi_buf tx_buf = {
			.buf = page,
			.len = sizeof(page),
		};
		const struct spi_buf_set tx = {
			.buffers = &tx_buf,
			.count = 1,
		};

		machxo2_build_cmd(page, FPGA_MACHXO2_CMD_LSC_PROG_INC_RNV,
				  FPGA_MACHXO2_PROG_INCR);
		memcpy(&page[4], &image[offset], FPGA_MACHXO2_PAGE_SIZE);

		ret = spi_write_dt(&config->bus, &tx);
		if (ret != 0) {
			LOG_ERR("Failed to write page at offset %u: %d", offset, ret);
			goto unlock;
		}

		k_busy_wait(MACHXO2_HIGH_DELAY_US);
	}

	ret = fpga_machxo2_wait_not_busy(dev, MACHXO2_ERASE_TIMEOUT_MS);
	if (ret != 0) {
		goto unlock;
	}

	ret = fpga_machxo2_transceive(dev, FPGA_MACHXO2_CMD_ISC_PROGRAM_DONE, 0U, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		goto unlock;
	}

	ret = fpga_machxo2_wait_not_busy(dev, MACHXO2_ERASE_TIMEOUT_MS);
	if (ret != 0) {
		goto unlock;
	}

	ret = fpga_machxo2_read_status(dev, &status);
	if (ret != 0) {
		goto unlock;
	}

	if ((status & FPGA_MACHXO2_STATUS_DONE) == 0U) {
		LOG_ERR("DONE did not assert after programming");
		ret = -EIO;
		goto unlock;
	}

	ret = machxo2_refresh(dev);
	if (ret != 0) {
		goto unlock;
	}

	ret = machxo2_verify_active(dev);
	if (ret != 0) {
		LOG_ERR("Device did not become active after refresh: %d", ret);
		goto unlock;
	}

	data->crc = crc32_ieee(image, img_size);
	data->image_size = img_size;
	data->loaded = true;
	data->on = true;

	LOG_INF("Loaded %u bytes, CRC32 0x%08x", img_size, data->crc);

	ret = 0;

unlock:
	if ((ret != 0) && entered) {
		machxo2_cleanup(dev);
	}

	k_mutex_unlock(&data->lock);

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
			LOG_ERR("Failed to configure PROGRAMN GPIO: %d", ret);
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
