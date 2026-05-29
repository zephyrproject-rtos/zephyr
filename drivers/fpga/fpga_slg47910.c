/*
 * Copyright (c) 2026 Sameer Srivastava <l1zard78@proton.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for Renesas SLG47910 FPGA configured via SPI.
 */

#define DT_DRV_COMPAT renesas_slg47910

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fpga_slg47910, CONFIG_FPGA_LOG_LEVEL);

#define SLG47910_T_RESET_MS      500   /* initial power-off reset hold    */
#define SLG47910_T_PRE_POWER_MS  100   /* settle before powering on       */
#define SLG47910_T_POST_POWER_MS 100   /* settle after PWR+EN assert      */
#define SLG47910_T_SS_PULSE_US   2000  /* SS high pulse entering cfg mode */
#define SLG47910_T_TX_DRAIN_US   1000  /* wait for SPI FIFO to drain      */

struct fpga_slg47910_data {
	bool loaded;
	struct k_spinlock lock;
};

struct fpga_slg47910_config {
	struct spi_dt_spec bus;  /* CS unused, SS driven manually */
	struct gpio_dt_spec pwr; /* PWR enable GPIO */
	struct gpio_dt_spec en;  /* EN GPIO */
	struct gpio_dt_spec ss;  /* SS / config-mode GPIO (manual) */
};

static void slg47910_reset_hw(const struct fpga_slg47910_config *config)
{
	gpio_pin_set_dt(&config->pwr, 0);
	gpio_pin_set_dt(&config->en, 0);
}

static enum FPGA_status fpga_slg47910_get_status(const struct device *dev)
{
	struct fpga_slg47910_data *data = dev->data;
	enum FPGA_status status;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	status = data->loaded ? FPGA_STATUS_ACTIVE : FPGA_STATUS_INACTIVE;
	k_spin_unlock(&data->lock, key);

	return status;
}

static int fpga_slg47910_reset(const struct device *dev)
{
	const struct fpga_slg47910_config *config = dev->config;
	struct fpga_slg47910_data *data = dev->data;
	k_spinlock_key_t key;

	slg47910_reset_hw(config);

	key = k_spin_lock(&data->lock);
	data->loaded = false;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int fpga_slg47910_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	const struct fpga_slg47910_config *config = dev->config;
	struct fpga_slg47910_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	if (image_ptr == NULL || img_size == 0) {
		return -EINVAL;
	}

	/* Create spi_buf_set */
	struct spi_buf tx_buf = {
		.buf = (void *)image_ptr,
		.len = img_size,
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	/* Reset */
	slg47910_reset_hw(config);
	k_msleep(SLG47910_T_RESET_MS);

	/* All lines low before power */
	gpio_pin_set_dt(&config->ss, 0);
	gpio_pin_set_dt(&config->en, 0);
	gpio_pin_set_dt(&config->pwr, 0);
	k_msleep(SLG47910_T_PRE_POWER_MS);

	/* Power on */
	gpio_pin_set_dt(&config->en, 1);
	gpio_pin_set_dt(&config->pwr, 1);
	k_msleep(SLG47910_T_POST_POWER_MS);

	/* SS: device enters configuration mode */
	gpio_pin_set_dt(&config->ss, 1);
	k_busy_wait(SLG47910_T_SS_PULSE_US);

	/* Assert SS and write bitstream */
	gpio_pin_set_dt(&config->ss, 0);

	ret = spi_write_dt(&config->bus, &tx_set);

	/* Hold SS low until FIFO drains, then deassert */
	k_busy_wait(SLG47910_T_TX_DRAIN_US);
	gpio_pin_set_dt(&config->ss, 1);

	if (ret < 0) {
		LOG_ERR("SPI write failed: %d", ret);
		return ret;
	}

	key = k_spin_lock(&data->lock);
	data->loaded = true;
	k_spin_unlock(&data->lock, key);

	LOG_INF("Bitstream loaded (%u bytes)", img_size);
	return 0;
}

static DEVICE_API(fpga, fpga_slg47910_api) = {
	.get_status = fpga_slg47910_get_status,
	.reset = fpga_slg47910_reset,
	.load = fpga_slg47910_load,
};

static int fpga_slg47910_init(const struct device *dev)
{
	const struct fpga_slg47910_config *config = dev->config;
	int ret;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->pwr)) {
		LOG_ERR("PWR GPIO not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->en)) {
		LOG_ERR("EN GPIO not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->ss)) {
		LOG_ERR("SS GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->pwr, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure PWR GPIO: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->en, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure EN GPIO: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->ss, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure SS GPIO: %d", ret);
		return ret;
	}

	slg47910_reset_hw(config);

	return 0;
}

#define SLG47910_SPI_OPERATION                                                                     \
	(SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER | SPI_LINES_SINGLE)

#define SLG47910_INIT(inst)                                                                        \
	static struct fpga_slg47910_data fpga_slg47910_data_##inst;                                \
                                                                                                   \
	static const struct fpga_slg47910_config fpga_slg47910_config_##inst = {                   \
		.bus = SPI_DT_SPEC_INST_GET(inst, SLG47910_SPI_OPERATION),                      \
		.pwr = GPIO_DT_SPEC_INST_GET(inst, pwr_gpios),                                     \
		.en = GPIO_DT_SPEC_INST_GET(inst, en_gpios),                                       \
		.ss = GPIO_DT_SPEC_INST_GET(inst, ss_gpios),                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fpga_slg47910_init, NULL, &fpga_slg47910_data_##inst,          \
			      &fpga_slg47910_config_##inst, POST_KERNEL,                           \
			      CONFIG_FPGA_INIT_PRIORITY, &fpga_slg47910_api)

DT_INST_FOREACH_STATUS_OKAY(SLG47910_INIT)
