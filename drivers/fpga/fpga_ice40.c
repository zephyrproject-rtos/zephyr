/*
 * Copyright (c) 2022 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lattice_ice40_fpga

#include <stdbool.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/posix/time.h>
#include <zephyr/sys/crc.h>
#include <zephyr/zephyr.h>

#ifndef BITS_PER_NIBBLE
#define BITS_PER_NIBBLE 4
#endif

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE 8
#endif

#ifndef NIBBLES_PER_BYTE
#define NIBBLES_PER_BYTE (BITS_PER_BYTE / BITS_PER_NIBBLE)
#endif

#define FPGA_ICE40_DELAY_CLOCKS 8

#define FPGA_ICE40_SPI_FREQ_MIN 1000000
#define FPGA_ICE40_SPI_FREQ_MAX 25000000

#define FPGA_ICE40_CRESET_DELAY_NS_MIN	 200
#define FPGA_ICE40_CONFIG_DELAY_US_MIN	 300
#define FPGA_ICE40_ADDITIONAL_CLOCKS_MIN 49

LOG_MODULE_REGISTER(fpga_ice40, LOG_LEVEL_DBG);

struct fpga_ice40_data {
	uint32_t crc;
	/* simply use crc32 as info */
	char info[2 * sizeof(uint32_t) + 1];
	bool on;
	bool loaded;
	struct k_spinlock lock;
};

enum fpga_ice40_pin {
	FPGA_ICE40_CDONE,
	FPGA_ICE40_CRESET,
};

struct fpga_ice40_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec gpio[2];
	const struct pinctrl_dev_config *pincfg;
	uint16_t config_delay_us;
	uint8_t creset_delay_ns;
	uint8_t additional_clocks;
};

static void fpga_ice40_crc_to_str(uint32_t crc, char *s)
{
	char ch;
	uint8_t i;
	uint8_t nibble;
	const char *table = "0123456789abcdef";

	for (i = 0; i < sizeof(crc) * NIBBLES_PER_BYTE; ++i, crc >>= BITS_PER_NIBBLE) {
		nibble = crc & GENMASK(BITS_PER_NIBBLE, 0);
		ch = table[nibble];
		s[sizeof(crc) * NIBBLES_PER_BYTE - i - 1] = ch;
	}

	s[sizeof(crc) * NIBBLES_PER_BYTE] = '\0';
}

static int fpga_ice40_send_clocks(const struct spi_dt_spec *bus, uint8_t n)
{
	uint8_t buf[(UINT8_MAX + 1) / BITS_PER_BYTE];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = ceiling_fraction(n, BITS_PER_BYTE),
	};
	struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1,
	};

	return spi_write_dt(bus, &tx_bufs);
}

static enum FPGA_status fpga_ice40_get_status(const struct device *dev)
{
	enum FPGA_status st;
	k_spinlock_key_t key;
	struct fpga_ice40_data *data = dev->data;

	key = k_spin_lock(&data->lock);
	/* TODO: make 'on' stateless: i.e. direction == out && CRESET_B == 1 */
	if (data->loaded && data->on) {
		st = FPGA_STATUS_ACTIVE;
	} else {
		st = FPGA_STATUS_INACTIVE;
	}

	k_spin_unlock(&data->lock, key);

	return st;
}

/*
 * See iCE40 Family Handbook, Appendix A. SPI Slave Configuration Procedure, pp 15-21
 *
 * https://www.latticesemi.com/~/media/LatticeSemi/Documents/Handbooks/iCE40FamilyHandbook.pdf
 */
int fpga_ice40_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	int ret;
	uint32_t crc;
	uint32_t delay_us;
	k_spinlock_key_t key;
	const struct spi_buf tx_buf = {
		.buf = image_ptr,
		.len = img_size,
	};
	struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct fpga_ice40_data *data = dev->data;
	const struct fpga_ice40_config *config = dev->config;

	crc = crc32_c(0, (uint8_t *)image_ptr, img_size, true, true);
	if (data->loaded && crc == data->crc) {
		LOG_WRN("already loaded with image CRC32c: 0x%08x", data->crc);
	}

	key = k_spin_lock(&data->lock);

	data->crc = 0;
	data->loaded = false;
	fpga_ice40_crc_to_str(0, data->info);

	/* SPI_SS_B = 1, CRESET_B = 1, SPI_CLK = 1 */
	LOG_DBG("initializing SPI & GPIO");
	if ((ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT)) ||
	    (ret = gpio_pin_configure_dt(&config->gpio[FPGA_ICE40_CRESET], GPIO_OUTPUT_HIGH)) ||
	    (ret = gpio_pin_configure_dt(&config->gpio[FPGA_ICE40_CDONE], GPIO_INPUT))) {
		LOG_ERR("Failed to initialize SPI or GPIO: %d", ret);
		goto unlock;
	}

	LOG_DBG("Set SPI_SS_B low");
	ret = gpio_pin_configure_dt(&config->bus.config.cs->gpio, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("failed to set SPI_SS_B low: %d", ret);
		goto unlock;
	}

	LOG_DBG("Set CRESET low");
	ret = gpio_pin_configure_dt(&config->gpio[FPGA_ICE40_CRESET], GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("failed to set CRESET low: %d", ret);
		goto unlock;
	}

	/* Note SPI_SCK is pulled high by SPI peripheral */

	/* Wait a minimum of 200ns */
	LOG_DBG("Delay %u ns", config->creset_delay_ns);
	k_busy_wait(ceiling_fraction(config->creset_delay_ns, NSEC_PER_USEC));

	LOG_DBG("Set CRESET high");
	ret = gpio_pin_configure_dt(&config->gpio[FPGA_ICE40_CRESET], GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("failed to set CRESET high: %d", ret);
		goto unlock;
	}

	LOG_DBG("Delay %u us", config->config_delay_us);
	k_busy_wait(config->config_delay_us);

	LOG_DBG("Set SPI_SS_B high");
	ret = gpio_pin_configure_dt(&config->bus.config.cs->gpio, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("Failed to set SPI_SS_B high: %d", ret);
		goto unlock;
	}

	LOG_DBG("Send %u clocks", FPGA_ICE40_DELAY_CLOCKS);
	ret = fpga_ice40_send_clocks(&config->bus, FPGA_ICE40_DELAY_CLOCKS);
	if (ret < 0) {
		LOG_ERR("failed to send %u clocks: %d", FPGA_ICE40_DELAY_CLOCKS, ret);
		goto unlock;
	}

	LOG_DBG("Send bin file");
	ret = spi_write_dt(&config->bus, &tx_bufs);
	if (ret < 0) {
		LOG_ERR("Failed to send bin file: %d", ret);
		goto unlock;
	}

	LOG_DBG("Send %u clocks", config->additional_clocks);
	ret = fpga_ice40_send_clocks(&config->bus, config->additional_clocks);
	if (ret < 0) {
		LOG_ERR("failed to send %u clocks: %d", config->additional_clocks, ret);
		goto unlock;
	}

	LOG_DBG("checking CDONE");
	ret = gpio_pin_get_dt(&config->gpio[FPGA_ICE40_CDONE]);
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
	LOG_DBG("Loaded image with CRC32c 0x%08x", crc);

unlock:
	(void)pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int fpga_ice40_on_off(const struct device *dev, bool on)
{
	int ret;
	k_spinlock_key_t key;
	struct fpga_ice40_data *data = dev->data;
	const struct fpga_ice40_config *config = dev->config;

	key = k_spin_lock(&data->lock);

	ret = gpio_pin_configure_dt(&config->gpio[FPGA_ICE40_CRESET],
				    on ? GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW);
	if (ret < 0) {
		goto unlock;
	}

	data->on = on;
	ret = 0;

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int fpga_ice40_on(const struct device *dev)
{
	return fpga_ice40_on_off(dev, true);
}

static int fpga_ice40_off(const struct device *dev)
{
	return fpga_ice40_on_off(dev, false);
}

static int fpga_ice40_reset(const struct device *dev)
{
	return fpga_ice40_off(dev) || fpga_ice40_on(dev);
}

static const char *fpga_ice40_get_info(const struct device *dev)
{
	struct fpga_ice40_data *data = dev->data;

	return data->info;
}

static const struct fpga_driver_api fpga_ice40_api = {
	.get_status = fpga_ice40_get_status,
	.reset = fpga_ice40_reset,
	.load = fpga_ice40_load,
	.on = fpga_ice40_on,
	.off = fpga_ice40_off,
	.get_info = fpga_ice40_get_info,
};

static int fpga_ice40_init(const struct device *dev)
{
	int ret;
	const struct fpga_ice40_config *config = dev->config;

	ret = gpio_pin_configure_dt(&config->gpio[FPGA_ICE40_CRESET], GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("failed to configure CRESET: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->gpio[FPGA_ICE40_CDONE], GPIO_INPUT);
	{
		LOG_ERR("Failed to initialize CDONE: %d", ret);
		return ret;
	}

	return 0;
}

#define FPGA_ICE40_BUS_FREQ(inst) DT_INST_PROP(inst, spi_max_frequency)

#define FPGA_ICE40_BUS_PERIOD_NS(inst) (NSEC_PER_SEC / FPGA_ICE40_BUS_FREQ(inst))

#define FPGA_ICE40_CONFIG_DELAY_US(inst)                                                           \
	DT_INST_PROP_OR(inst, config_delay_us, FPGA_ICE40_CONFIG_DELAY_US_MIN)

#define FPGA_ICE40_CRESET_DELAY_NS(inst)                                                           \
	DT_INST_PROP_OR(inst, creset_delay_ns, FPGA_ICE40_CRESET_DELAY_NS_MIN)

#define FPGA_ICE40_ADDITIONAL_CLOCKS(inst)                                                         \
	DT_INST_PROP_OR(inst, additional_clocks, FPGA_ICE40_ADDITIONAL_CLOCKS_MIN)

#define FPGA_ICE40_DEFINE(inst)                                                                    \
	BUILD_ASSERT(FPGA_ICE40_BUS_FREQ(inst) >= FPGA_ICE40_SPI_FREQ_MIN);                        \
	BUILD_ASSERT(FPGA_ICE40_BUS_FREQ(inst) <= FPGA_ICE40_SPI_FREQ_MAX);                        \
	BUILD_ASSERT(FPGA_ICE40_CONFIG_DELAY_US(inst) >= FPGA_ICE40_CONFIG_DELAY_US_MIN);          \
	BUILD_ASSERT(FPGA_ICE40_CONFIG_DELAY_US(inst) <= UINT16_MAX);                              \
	BUILD_ASSERT(FPGA_ICE40_CRESET_DELAY_NS(inst) >= FPGA_ICE40_CRESET_DELAY_NS_MIN);          \
	BUILD_ASSERT(FPGA_ICE40_CRESET_DELAY_NS(inst) <= UINT8_MAX);                               \
	BUILD_ASSERT(FPGA_ICE40_ADDITIONAL_CLOCKS(inst) >= FPGA_ICE40_ADDITIONAL_CLOCKS_MIN);      \
	BUILD_ASSERT(FPGA_ICE40_ADDITIONAL_CLOCKS(inst) <= UINT8_MAX);                             \
                                                                                                   \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(inst));                                                   \
                                                                                                   \
	static struct fpga_ice40_data fpga_ice40_data_##inst;                                      \
                                                                                                   \
	static const struct fpga_ice40_config fpga_ice40_config_##inst = {                         \
		.bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),          \
		.gpio =                                                                            \
			{                                                                          \
				[FPGA_ICE40_CDONE] = GPIO_DT_SPEC_INST_GET(inst, cdone_gpios),     \
				[FPGA_ICE40_CRESET] = GPIO_DT_SPEC_INST_GET(inst, creset_gpios),   \
			},                                                                         \
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(inst)),                         \
		.config_delay_us = FPGA_ICE40_CONFIG_DELAY_US(inst),                               \
		.creset_delay_ns = FPGA_ICE40_CRESET_DELAY_NS(inst),                               \
		.additional_clocks = FPGA_ICE40_ADDITIONAL_CLOCKS(inst),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fpga_ice40_init, NULL, &fpga_ice40_data_##inst,                \
			      &fpga_ice40_config_##inst, POST_KERNEL, 0, &fpga_ice40_api);

DT_INST_FOREACH_STATUS_OKAY(FPGA_ICE40_DEFINE)
