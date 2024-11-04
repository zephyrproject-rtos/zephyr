/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lattice_ice40_fpga

#include <stdbool.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

/*
 * Note: When loading a bitstream, the iCE40 has a 'quirk' in that the CS
 * polarity must be inverted during the 'leading clocks' phase and
 * 'trailing clocks' phase. While the bitstream is being transmitted, the
 * CS polarity is normal (active low). Zephyr's SPI driver model currently
 * does not handle these types of quirks (in contrast to e.g. Linux).
 *
 * The logical alternative would be to put the CS into GPIO mode, perform 3
 * separate SPI transfers (inverting CS polarity as necessary) and then
 * restore the default pinctrl settings. On some higher-end microcontrollers
 * and microprocessors, it's possible to do that without breaking the iCE40
 * timing requirements.
 *
 * However, on lower-end microcontrollers, the amount of time that elapses
 * between SPI transfers does break the iCE40 timing requirements. That
 * leaves us with the bitbanging option. Of course, on lower-end
 * microcontrollers, the amount of time required to execute something
 * like gpio_pin_configure_dt() dwarfs the 2*500 nanoseconds needed to
 * achieve the minimum 1 MHz clock rate for loading the iCE40 bistream. So
 * in order to bitbang on lower-end microcontrollers, we actually require
 * direct register access to the set and clear registers.
 */

/*
 * Values in Hz, intentionally to be comparable with the spi-max-frequency
 * property from DT bindings in spi-device.yaml.
 */
#define FPGA_ICE40_SPI_HZ_MIN 1000000
#define FPGA_ICE40_SPI_HZ_MAX 25000000

#define FPGA_ICE40_CRESET_DELAY_US_MIN 1 /* 200ns absolute minimum */
#define FPGA_ICE40_CONFIG_DELAY_US_MIN 1200
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
	struct gpio_dt_spec cdone;
	struct gpio_dt_spec creset;
	struct gpio_dt_spec clk;
	struct gpio_dt_spec pico;
	volatile gpio_port_pins_t *set;
	volatile gpio_port_pins_t *clear;
	uint16_t mhz_delay_count;
	uint16_t creset_delay_us;
	uint16_t config_delay_us;
	uint8_t leading_clocks;
	uint8_t trailing_clocks;
	fpga_api_load load;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
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

/*
 * This is a calibrated delay loop used to achieve a 1 MHz SPI_CLK frequency
 * with the bitbang mode. It is used both in fpga_ice40_send_clocks()
 * and fpga_ice40_spi_send_data().
 *
 * Calibration is achieved via the mhz_delay_count device tree parameter. See
 * lattice,ice40-fpga.yaml for details.
 */
static inline void fpga_ice40_delay(size_t n)
{
	for (; n > 0; --n) {
		__asm__ __volatile__("");
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

	if (data->loaded && data->on) {
		st = FPGA_STATUS_ACTIVE;
	} else {
		st = FPGA_STATUS_INACTIVE;
	}

	k_spin_unlock(&data->lock, key);

	return st;
}

/*
 * See iCE40 Family Handbook, Appendix A. SPI Slave Configuration Procedure,
 * pp 15-21.
 *
 * https://www.latticesemi.com/~/media/LatticeSemi/Documents/Handbooks/iCE40FamilyHandbook.pdf
 */
static int fpga_ice40_load_gpio(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	int ret;
	uint32_t crc;
	gpio_port_pins_t cs;
	gpio_port_pins_t clk;
	k_spinlock_key_t key;
	gpio_port_pins_t pico;
	gpio_port_pins_t creset;
	struct fpga_ice40_data *data = dev->data;
	const struct fpga_ice40_config *config = dev->config;

	if (!device_is_ready(config->clk.port)) {
		LOG_ERR("%s: GPIO for clk is not ready", dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->pico.port)) {
		LOG_ERR("%s: GPIO for pico is not ready", dev->name);
		return -ENODEV;
	}

	if (config->set == NULL) {
		LOG_ERR("%s: set register was not specified", dev->name);
		return -EFAULT;
	}

	if (config->clear == NULL) {
		LOG_ERR("%s: clear register was not specified", dev->name);
		return -EFAULT;
	}

	/* prepare masks */
	cs = BIT(config->bus.config.cs.gpio.pin);
	clk = BIT(config->clk.pin);
	pico = BIT(config->pico.pin);
	creset = BIT(config->creset.pin);

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
	      gpio_pin_configure_dt(&config->bus.config.cs.gpio, GPIO_OUTPUT_HIGH) ||
	      gpio_pin_configure_dt(&config->clk, GPIO_OUTPUT_HIGH) ||
	      gpio_pin_configure_dt(&config->pico, GPIO_OUTPUT_HIGH);
	__ASSERT(ret == 0, "Failed to initialize GPIO: %d", ret);

	LOG_DBG("Set CRESET low");
	LOG_DBG("Set SPI_CS low");
	*config->clear |= (creset | cs);

	/* Wait a minimum of 200ns */
	LOG_DBG("Delay %u us", config->creset_delay_us);
	fpga_ice40_delay(2 * config->mhz_delay_count * config->creset_delay_us);

	if (gpio_pin_get_dt(&config->cdone) != 0) {
		LOG_ERR("CDONE should be low after the reset");
		ret = -EIO;
		goto unlock;
	}

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
	(void)gpio_pin_configure_dt(&config->bus.config.cs.gpio, GPIO_OUTPUT_HIGH);
	(void)gpio_pin_configure_dt(&config->clk, GPIO_DISCONNECTED);
	(void)gpio_pin_configure_dt(&config->pico, GPIO_DISCONNECTED);
#ifdef CONFIG_PINCTRL
	(void)pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
#endif

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int fpga_ice40_load_spi(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
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
	ret = spi_write_dt(&config->bus, &tx_bufs);
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
	ret = spi_write_dt(&config->bus, &tx_bufs);
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
	ret = spi_write_dt(&config->bus, &tx_bufs);
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
#ifdef CONFIG_PINCTRL
	(void)pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
#endif

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int fpga_ice40_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
	const struct fpga_ice40_config *config = dev->config;

	return config->load(dev, image_ptr, img_size);
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

	if (!device_is_ready(config->creset.port)) {
		LOG_ERR("%s: GPIO for creset is not ready", dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->cdone.port)) {
		LOG_ERR("%s: GPIO for cdone is not ready", dev->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->creset, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("failed to configure CRESET: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->cdone, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to initialize CDONE: %d", ret);
		return ret;
	}

	return 0;
}

#define FPGA_ICE40_BUS_FREQ(inst) DT_INST_PROP(inst, spi_max_frequency)

#define FPGA_ICE40_CONFIG_DELAY_US(inst)                                                           \
	DT_INST_PROP_OR(inst, config_delay_us, FPGA_ICE40_CONFIG_DELAY_US_MIN)

#define FPGA_ICE40_CRESET_DELAY_US(inst)                                                           \
	DT_INST_PROP_OR(inst, creset_delay_us, FPGA_ICE40_CRESET_DELAY_US_MIN)

#define FPGA_ICE40_LEADING_CLOCKS(inst)                                                            \
	DT_INST_PROP_OR(inst, leading_clocks, FPGA_ICE40_LEADING_CLOCKS_MIN)

#define FPGA_ICE40_TRAILING_CLOCKS(inst)                                                           \
	DT_INST_PROP_OR(inst, trailing_clocks, FPGA_ICE40_TRAILING_CLOCKS_MIN)

#define FPGA_ICE40_MHZ_DELAY_COUNT(inst) DT_INST_PROP_OR(inst, mhz_delay_count, 0)

#define FPGA_ICE40_GPIO_PINS(inst, name) (volatile gpio_port_pins_t *)DT_INST_PROP_OR(inst, name, 0)

#define FPGA_ICE40_LOAD_FUNC(inst)                                                                 \
	(DT_INST_PROP(inst, load_mode_bitbang) ? fpga_ice40_load_gpio : fpga_ice40_load_spi)

#ifdef CONFIG_PINCTRL
#define FPGA_ICE40_PINCTRL_CONFIG(inst) .pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(inst)),
#define FPGA_ICE40_PINCTRL_DEFINE(inst) PINCTRL_DT_DEFINE(DT_INST_PARENT(inst))
#else
#define FPGA_ICE40_PINCTRL_CONFIG(inst)
#define FPGA_ICE40_PINCTRL_DEFINE(inst)
#endif

#define FPGA_ICE40_DEFINE(inst)                                                                    \
	BUILD_ASSERT(FPGA_ICE40_BUS_FREQ(inst) >= FPGA_ICE40_SPI_HZ_MIN);                          \
	BUILD_ASSERT(FPGA_ICE40_BUS_FREQ(inst) <= FPGA_ICE40_SPI_HZ_MAX);                          \
	BUILD_ASSERT(FPGA_ICE40_CONFIG_DELAY_US(inst) >= FPGA_ICE40_CONFIG_DELAY_US_MIN);          \
	BUILD_ASSERT(FPGA_ICE40_CONFIG_DELAY_US(inst) <= UINT16_MAX);                              \
	BUILD_ASSERT(FPGA_ICE40_CRESET_DELAY_US(inst) >= FPGA_ICE40_CRESET_DELAY_US_MIN);          \
	BUILD_ASSERT(FPGA_ICE40_CRESET_DELAY_US(inst) <= UINT16_MAX);                              \
	BUILD_ASSERT(FPGA_ICE40_LEADING_CLOCKS(inst) >= FPGA_ICE40_LEADING_CLOCKS_MIN);            \
	BUILD_ASSERT(FPGA_ICE40_LEADING_CLOCKS(inst) <= UINT8_MAX);                                \
	BUILD_ASSERT(FPGA_ICE40_TRAILING_CLOCKS(inst) >= FPGA_ICE40_TRAILING_CLOCKS_MIN);          \
	BUILD_ASSERT(FPGA_ICE40_TRAILING_CLOCKS(inst) <= UINT8_MAX);                               \
	BUILD_ASSERT(FPGA_ICE40_MHZ_DELAY_COUNT(inst) >= 0);                                       \
                                                                                                   \
	FPGA_ICE40_PINCTRL_DEFINE(inst);                                                           \
	static struct fpga_ice40_data fpga_ice40_data_##inst;                                      \
                                                                                                   \
	static const struct fpga_ice40_config fpga_ice40_config_##inst = {                         \
		.bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),          \
		.creset = GPIO_DT_SPEC_INST_GET(inst, creset_gpios),                               \
		.cdone = GPIO_DT_SPEC_INST_GET(inst, cdone_gpios),                                 \
		.clk = GPIO_DT_SPEC_INST_GET_OR(inst, clk_gpios, {0}),                             \
		.pico = GPIO_DT_SPEC_INST_GET_OR(inst, pico_gpios, {0}),                           \
		.set = FPGA_ICE40_GPIO_PINS(inst, gpios_set_reg),                                  \
		.clear = FPGA_ICE40_GPIO_PINS(inst, gpios_clear_reg),                              \
		.mhz_delay_count = FPGA_ICE40_MHZ_DELAY_COUNT(inst),                               \
		.config_delay_us = FPGA_ICE40_CONFIG_DELAY_US(inst),                               \
		.creset_delay_us = FPGA_ICE40_CRESET_DELAY_US(inst),                               \
		.leading_clocks = FPGA_ICE40_LEADING_CLOCKS(inst),                                 \
		.trailing_clocks = FPGA_ICE40_TRAILING_CLOCKS(inst),                               \
		.load = FPGA_ICE40_LOAD_FUNC(inst),                                                \
		FPGA_ICE40_PINCTRL_CONFIG(inst)};                                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fpga_ice40_init, NULL, &fpga_ice40_data_##inst,                \
			      &fpga_ice40_config_##inst, POST_KERNEL, CONFIG_FPGA_INIT_PRIORITY,   \
			      &fpga_ice40_api);

DT_INST_FOREACH_STATUS_OKAY(FPGA_ICE40_DEFINE)
