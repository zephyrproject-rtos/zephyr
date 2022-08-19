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

#define FPGA_ICE40_SPI_FREQ_MIN 1000000
#define FPGA_ICE40_SPI_FREQ_MAX 25000000

#define FPGA_ICE40_CRESET_DELAY_NS_MIN 200
#define FPGA_ICE40_CONFIG_DELAY_US_MIN 300
#define FPGA_ICE40_LEADING_CLOCKS_MIN  8
#define FPGA_ICE40_TRAILING_CLOCKS_MIN 49

LOG_MODULE_REGISTER(fpga_ice40);

struct fpga_ice40_data {
	uint32_t crc;
	/* simply use crc32 as info */
	char info[2 * sizeof(uint32_t) + 1];
	bool on;
	bool loaded;
	struct k_spinlock lock;
};

struct fpga_ice40_config {
	struct spi_dt_spec bus;
	const struct pinctrl_dev_config *pincfg;
	struct gpio_dt_spec cdone;
	struct gpio_dt_spec creset;
	struct gpio_dt_spec clk;
	struct gpio_dt_spec pico;
	volatile gpio_port_pins_t *set;
	volatile gpio_port_pins_t *clear;
	uint16_t mhz_delay_count;
	uint8_t creset_delay_ns;
	uint16_t config_delay_us;
	uint8_t leading_clocks;
	uint8_t trailing_clocks;
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

static inline void fpga_ice40_delay(size_t n)
{
	for (; n > 0; --n) {
	}
}

static void fpga_ice40_send_clocks(size_t delay, volatile gpio_port_pins_t *set,
				   volatile gpio_port_pins_t *clear, gpio_port_pins_t clk, size_t n)
{
	for (; n > 0; --n) {
		*clear |= clk;
		fpga_ice40_delay(delay);
		*set |= clk;
		fpga_ice40_delay(delay);
	}
}

static void fpga_ice40_spi_send_data(size_t delay, volatile gpio_port_pins_t *set,
				     volatile gpio_port_pins_t *clear, gpio_port_pins_t cs,
				     gpio_port_pins_t clk, gpio_port_pins_t pico, uint8_t *z,
				     size_t n)
{
	bool hi;

	/* assert chip-select (active low) */
	*clear |= cs;

	for (; n > 0; --n, ++z) {
		/* msb down to lsb */
		for (int b = 7; b >= 0; --b) {

			/* Data is shifted out on the falling edge (CPOL=0) */
			*clear |= clk;
			fpga_ice40_delay(delay);

			hi = !!(BIT(b) & *z);
			if (hi) {
				*set |= pico;
			} else {
				*clear |= pico;
			}

			/* Data is sampled on the rising edge (CPHA=0) */
			*set |= clk;
			fpga_ice40_delay(delay);
		}
	}

	/* de-assert chip-select (active low) */
	*set |= cs;
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
 *
 * This is a bit tricky.
 *
 * We want to use the SPI hardware and driver to deliver perfectly-timed clocks and manage
 * the chip-select, mainly to avoid calibrated delay loops and bit-banging. However, SPI_CS
 * must be pulled high to deliver 8 leading clocks and 49 trailing clocks. Normally, SPI_CS is
 * pulled low by the driver.
 *
 * However, with the current SPI API, there is no way to insert a callback between buffers
 * to e.g. change the SPI_CS polarity. The next logical approach would be to perform 3
 * consecutive SPI transfers, modifying the SPI_CS polarity each time. However, there is
 * some inconsistence with how that is done using the Zephyr SPI API. Additionally, some
 * SPI transceivers actually use a dedicated signal for CS and do not use a GPIO peripheral.
 *
 * In practice, it was not feasible to use the 3-transfer approach described above on an
 * 80 MHz microcontroller, as there was substantial overhead from one SPI transfer to the
 * next, which breaks iCE40 config timing.
 *
 * With that, we are left with bit-banging. This was also challenging, as the highest
 * clock rate that could be achieved on this platform using the Zephyr GPIO API was around
 * 275 kHz, which also breaks iCE40 timing.
 *
 * For that reason, it was not possible to rely on Zephyr's GPIO API for bit-banging, and
 * that is why this driver requires raw register access to set-bit and clear-bit registers.
 * With this approach, on an 80 MHz microcontroller, we were able to achieve switching times
 * of 220ns (thus a clock frequency of 2.2 MHz, which is within iCE40 config specifications).
 * DT bindings are available so that each platform may taylor a specific delay to achieve
 * a 1 MHz clock freqency.
 *
 * Outside of loading, the device may operate anywhere within the 1 MHz <= f <= 25 MHz
 * operating frequency.
 */
int fpga_ice40_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	int ret;
	uint32_t crc;
	uint32_t delay_us;
	gpio_port_pins_t cs;
	gpio_port_pins_t clk;
	k_spinlock_key_t key;
	gpio_port_pins_t pico;
	gpio_port_pins_t creset;
	struct fpga_ice40_data *data = dev->data;
	const struct fpga_ice40_config *config = dev->config;

	/* prepare masks */
	cs = BIT(config->bus.config.cs->gpio.pin);
	clk = BIT(config->clk.pin);
	pico = BIT(config->pico.pin);
	creset = BIT(config->creset.pin);

	/* crc check */
	crc = crc32_ieee((uint8_t *)image_ptr, img_size);
	if (data->loaded && crc == data->crc) {
		LOG_WRN("already loaded with image CRC32c: 0x%08x", data->crc);
	}

	/* precompute delay values */
	delay_us = ceiling_fraction(config->creset_delay_ns, NSEC_PER_USEC);

	key = k_spin_lock(&data->lock);

	/* clear crc */
	data->crc = 0;
	data->loaded = false;
	fpga_ice40_crc_to_str(0, data->info);

	LOG_DBG("Initializing GPIO");
	ret = gpio_pin_configure_dt(&config->cdone, GPIO_INPUT) ||
	      gpio_pin_configure_dt(&config->creset, GPIO_OUTPUT_HIGH) ||
	      gpio_pin_configure_dt(&config->bus.config.cs->gpio, GPIO_OUTPUT_HIGH) ||
	      gpio_pin_configure_dt(&config->clk, GPIO_OUTPUT_HIGH) ||
	      gpio_pin_configure_dt(&config->pico, GPIO_OUTPUT_HIGH) || 0;
	__ASSERT(ret == 0, "Failed to initialize GPIO: %d", ret);
	ARG_UNUSED(ret);

	LOG_DBG("Set CRESET low");
	LOG_DBG("Set SPI_CS low");
	*config->clear |= (creset | cs);

	/* Wait a minimum of 200ns */
	LOG_DBG("Delay %u ns (%u us)", config->creset_delay_ns, delay_us);
	fpga_ice40_delay(2 * config->mhz_delay_count * delay_us);

	__ASSERT(gpio_pin_get_dt(&config->cdone) == 0, "CDONE was not high");

	LOG_DBG("Set CRESET high");
	*config->set |= creset;

	LOG_DBG("Delay %u us", config->config_delay_us);
	k_busy_wait(config->config_delay_us);

	LOG_DBG("Set SPI_CS high");
	*config->set |= cs;

	LOG_DBG("Send %u clocks", config->leading_clocks);
	fpga_ice40_send_clocks(config->mhz_delay_count, config->set, config->clear, clk,
			       config->leading_clocks);

	LOG_DBG("Set SPI_CS low");
	LOG_DBG("Send bin file");
	LOG_DBG("Set SPI_CS high");
	fpga_ice40_spi_send_data(config->mhz_delay_count, config->set, config->clear, cs, clk, pico,
				 (uint8_t *)image_ptr, img_size);

	LOG_DBG("Send %u clocks", config->trailing_clocks);
	fpga_ice40_send_clocks(config->mhz_delay_count, config->set, config->clear, clk,
			       config->trailing_clocks);

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
	(void)gpio_pin_configure_dt(&config->bus.config.cs->gpio, GPIO_OUTPUT_HIGH);
	(void)gpio_pin_configure_dt(&config->clk, GPIO_DISCONNECTED);
	(void)gpio_pin_configure_dt(&config->pico, GPIO_DISCONNECTED);
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

	ret = gpio_pin_configure_dt(&config->creset, on ? GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW);
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

	ret = gpio_pin_configure_dt(&config->creset, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("failed to configure CRESET: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->cdone, 0);
	if (ret < 0) {
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

#define FPGA_ICE40_LEADING_CLOCKS(inst)                                                            \
	DT_INST_PROP_OR(inst, leading_clocks, FPGA_ICE40_LEADING_CLOCKS_MIN)

#define FPGA_ICE40_TRAILING_CLOCKS(inst)                                                           \
	DT_INST_PROP_OR(inst, trailing_clocks, FPGA_ICE40_TRAILING_CLOCKS_MIN)

#define FPGA_ICE40_MHZ_DELAY_COUNT(inst) DT_INST_PROP_OR(inst, mhz_delay_count, 0)

#define FPGA_ICE40_GPIO_PINS(inst, name) (volatile gpio_port_pins_t *)DT_INST_PROP(inst, name)

#define FPGA_ICE40_DEFINE(inst)                                                                    \
	BUILD_ASSERT(FPGA_ICE40_BUS_FREQ(inst) >= FPGA_ICE40_SPI_FREQ_MIN);                        \
	BUILD_ASSERT(FPGA_ICE40_BUS_FREQ(inst) <= FPGA_ICE40_SPI_FREQ_MAX);                        \
	BUILD_ASSERT(FPGA_ICE40_CONFIG_DELAY_US(inst) >= FPGA_ICE40_CONFIG_DELAY_US_MIN);          \
	BUILD_ASSERT(FPGA_ICE40_CONFIG_DELAY_US(inst) <= UINT16_MAX);                              \
	BUILD_ASSERT(FPGA_ICE40_CRESET_DELAY_NS(inst) >= FPGA_ICE40_CRESET_DELAY_NS_MIN);          \
	BUILD_ASSERT(FPGA_ICE40_CRESET_DELAY_NS(inst) <= UINT8_MAX);                               \
	BUILD_ASSERT(FPGA_ICE40_LEADING_CLOCKS(inst) >= FPGA_ICE40_LEADING_CLOCKS_MIN);            \
	BUILD_ASSERT(FPGA_ICE40_LEADING_CLOCKS(inst) <= UINT8_MAX);                                \
	BUILD_ASSERT(FPGA_ICE40_TRAILING_CLOCKS(inst) >= FPGA_ICE40_TRAILING_CLOCKS_MIN);          \
	BUILD_ASSERT(FPGA_ICE40_TRAILING_CLOCKS(inst) <= UINT8_MAX);                               \
	BUILD_ASSERT(FPGA_ICE40_MHZ_DELAY_COUNT(inst) >= 0);                                       \
                                                                                                   \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(inst));                                                   \
	static struct fpga_ice40_data fpga_ice40_data_##inst;                                      \
                                                                                                   \
	static const struct fpga_ice40_config fpga_ice40_config_##inst = {                         \
		.bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),          \
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(inst)),                         \
		.creset = GPIO_DT_SPEC_INST_GET(inst, creset_gpios),                               \
		.cdone = GPIO_DT_SPEC_INST_GET(inst, cdone_gpios),                                 \
		.clk = GPIO_DT_SPEC_INST_GET(inst, clk_gpios),                                     \
		.pico = GPIO_DT_SPEC_INST_GET(inst, pico_gpios),                                   \
		.set = FPGA_ICE40_GPIO_PINS(inst, gpios_set_reg),                                  \
		.clear = FPGA_ICE40_GPIO_PINS(inst, gpios_clear_reg),                              \
		.mhz_delay_count = FPGA_ICE40_MHZ_DELAY_COUNT(inst),                               \
		.config_delay_us = FPGA_ICE40_CONFIG_DELAY_US(inst),                               \
		.creset_delay_ns = FPGA_ICE40_CRESET_DELAY_NS(inst),                               \
		.leading_clocks = FPGA_ICE40_LEADING_CLOCKS(inst),                                 \
		.trailing_clocks = FPGA_ICE40_TRAILING_CLOCKS(inst),                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fpga_ice40_init, NULL, &fpga_ice40_data_##inst,                \
			      &fpga_ice40_config_##inst, POST_KERNEL, 0, &fpga_ice40_api);

DT_INST_FOREACH_STATUS_OKAY(FPGA_ICE40_DEFINE)
