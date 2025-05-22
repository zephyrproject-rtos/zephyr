/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "fpga_ice40_common.h"

LOG_MODULE_DECLARE(fpga_ice40);

static int fpga_ice40_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	int ret;
	uint32_t crc;
	k_spinlock_key_t key;
	struct spi_buf tx_buf;
	const struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct fpga_ice40_data *data = dev->data;
	uint8_t clock_buf[(UINT8_MAX + 1) / BITS_PER_BYTE];
	const struct fpga_ice40_config *config = dev->config;
	struct spi_dt_spec bus;

	memcpy(&bus, &config->bus, sizeof(bus));
	/*
	 * Disable the automatism for chip select within the SPI driver,
	 * as the configuration sequence requires this signal to be inactive
	 * during the leading and trailing clock phase.
	 */
	bus.config.cs.gpio.port = NULL;

	/* crc check */
	crc = crc32_ieee((uint8_t *)image_ptr, img_size);
	if (data->loaded && crc == data->crc) {
		LOG_WRN("already loaded with image CRC32c: 0x%08x", data->crc);
	}

	key = k_spin_lock(&data->lock);

	/* clear crc */
	data->crc = 0;
	data->loaded = false;
	fpga_ice40_crc_to_str(0, data->info);

	LOG_DBG("Initializing GPIO");
	ret = gpio_pin_configure_dt(&config->cdone, GPIO_INPUT) ||
	      gpio_pin_configure_dt(&config->creset, GPIO_OUTPUT_HIGH) ||
	      gpio_pin_configure_dt(&config->bus.config.cs.gpio, GPIO_OUTPUT_HIGH);
	__ASSERT(ret == 0, "Failed to initialize GPIO: %d", ret);

	LOG_DBG("Set CRESET low");
	ret = gpio_pin_configure_dt(&config->creset, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("failed to set CRESET low: %d", ret);
		goto unlock;
	}

	LOG_DBG("Set SPI_CS low");
	ret = gpio_pin_configure_dt(&config->bus.config.cs.gpio, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("failed to set SPI_CS low: %d", ret);
		goto unlock;
	}

	/* Wait a minimum of 200ns */
	LOG_DBG("Delay %u us", config->creset_delay_us);
	k_usleep(config->creset_delay_us);

	if (gpio_pin_get_dt(&config->cdone) != 0) {
		LOG_ERR("CDONE should be low after the reset");
		ret = -EIO;
		goto unlock;
	}

	LOG_DBG("Set CRESET high");
	ret = gpio_pin_configure_dt(&config->creset, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("failed to set CRESET high: %d", ret);
		goto unlock;
	}

	LOG_DBG("Delay %u us", config->config_delay_us);
	k_busy_wait(config->config_delay_us);

	LOG_DBG("Set SPI_CS high");
	ret = gpio_pin_configure_dt(&config->bus.config.cs.gpio, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("failed to set SPI_CS high: %d", ret);
		goto unlock;
	}

	LOG_DBG("Send %u clocks", config->leading_clocks);
	tx_buf.buf = clock_buf;
	tx_buf.len = DIV_ROUND_UP(config->leading_clocks, BITS_PER_BYTE);
	ret = spi_write_dt(&bus, &tx_bufs);
	if (ret < 0) {
		LOG_ERR("Failed to send leading %u clocks: %d", config->leading_clocks, ret);
		goto unlock;
	}

	LOG_DBG("Set SPI_CS low");
	ret = gpio_pin_configure_dt(&config->bus.config.cs.gpio, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("failed to set SPI_CS low: %d", ret);
		goto unlock;
	}

	LOG_DBG("Send bin file");
	tx_buf.buf = image_ptr;
	tx_buf.len = img_size;
	ret = spi_write_dt(&bus, &tx_bufs);
	if (ret < 0) {
		LOG_ERR("Failed to send bin file: %d", ret);
		goto unlock;
	}

	LOG_DBG("Set SPI_CS high");
	ret = gpio_pin_configure_dt(&config->bus.config.cs.gpio, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("failed to set SPI_CS high: %d", ret);
		goto unlock;
	}

	LOG_DBG("Send %u clocks", config->trailing_clocks);
	tx_buf.buf = clock_buf;
	tx_buf.len = DIV_ROUND_UP(config->trailing_clocks, BITS_PER_BYTE);
	ret = spi_write_dt(&bus, &tx_bufs);
	if (ret < 0) {
		LOG_ERR("Failed to send trailing %u clocks: %d", config->trailing_clocks, ret);
		goto unlock;
	}

	LOG_DBG("checking CDONE");
	ret = gpio_pin_get_dt(&config->cdone);
	if (ret < 0) {
		LOG_ERR("failed to read CDONE: %d", ret);
		goto unlock;
	} else if (ret != 1) {
		ret = -EIO;
		LOG_ERR("CDONE did not go high");
		goto unlock;
	}

	ret = 0;
	data->loaded = true;
	fpga_ice40_crc_to_str(crc, data->info);
	LOG_INF("Loaded image with CRC32 0x%08x", crc);

unlock:
	(void)gpio_pin_configure_dt(&config->creset, GPIO_OUTPUT_HIGH);
	(void)gpio_pin_configure_dt(&config->bus.config.cs.gpio, GPIO_OUTPUT_HIGH);

	k_spin_unlock(&data->lock, key);

	return ret;
}

static DEVICE_API(fpga, fpga_ice40_api) = {
	.get_status = fpga_ice40_get_status,
	.reset = fpga_ice40_reset,
	.load = fpga_ice40_load,
	.on = fpga_ice40_on,
	.off = fpga_ice40_off,
	.get_info = fpga_ice40_get_info,
};

#define FPGA_ICE40_DEFINE(inst)                                                                    \
	static struct fpga_ice40_data fpga_ice40_data_##inst;                                      \
                                                                                                   \
	FPGA_ICE40_CONFIG_DEFINE(inst, NULL);                                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fpga_ice40_init, NULL, &fpga_ice40_data_##inst,                \
			      &fpga_ice40_config_##inst, POST_KERNEL, CONFIG_FPGA_INIT_PRIORITY,   \
			      &fpga_ice40_api);

#define DT_DRV_COMPAT lattice_ice40_fpga
DT_INST_FOREACH_STATUS_OKAY(FPGA_ICE40_DEFINE)
