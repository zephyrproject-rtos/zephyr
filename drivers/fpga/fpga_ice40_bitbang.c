/*
 * Copyright (c) 2022 Meta
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif /* CONFIG_PINCTRL */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "fpga_ice40_common.h"

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

LOG_MODULE_DECLARE(fpga_ice40);

struct fpga_ice40_config_bitbang {
	struct gpio_dt_spec clk;
	struct gpio_dt_spec pico;
	volatile gpio_port_pins_t *set;
	volatile gpio_port_pins_t *clear;
	uint16_t mhz_delay_count;
	const struct pinctrl_dev_config *pincfg;
};

/*
 * This is a calibrated delay loop used to achieve a 1 MHz SPI_CLK frequency
 * with the GPIO bitbang mode. It is used both in fpga_ice40_send_clocks()
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

/*
 * See iCE40 Family Handbook, Appendix A. SPI Slave Configuration Procedure,
 * pp 15-21.
 *
 * https://www.latticesemi.com/~/media/LatticeSemi/Documents/Handbooks/iCE40FamilyHandbook.pdf
 */
static int fpga_ice40_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
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
	const struct fpga_ice40_config_bitbang *config_bitbang = config->derived_config;

	if (!device_is_ready(config_bitbang->clk.port)) {
		LOG_ERR("%s: GPIO for clk is not ready", dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(config_bitbang->pico.port)) {
		LOG_ERR("%s: GPIO for pico is not ready", dev->name);
		return -ENODEV;
	}

	/* prepare masks */
	cs = BIT(config->bus.config.cs.gpio.pin);
	clk = BIT(config_bitbang->clk.pin);
	pico = BIT(config_bitbang->pico.pin);
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
	      gpio_pin_configure_dt(&config_bitbang->clk, GPIO_OUTPUT_HIGH) ||
	      gpio_pin_configure_dt(&config_bitbang->pico, GPIO_OUTPUT_HIGH);
	__ASSERT(ret == 0, "Failed to initialize GPIO: %d", ret);

	LOG_DBG("Set CRESET low");
	LOG_DBG("Set SPI_CS low");
	*config_bitbang->clear |= (creset | cs);

	/* Wait a minimum of 200ns */
	LOG_DBG("Delay %u us", config->creset_delay_us);
	fpga_ice40_delay(2 * config_bitbang->mhz_delay_count * config->creset_delay_us);

	if (gpio_pin_get_dt(&config->cdone) != 0) {
		LOG_ERR("CDONE should be low after the reset");
		ret = -EIO;
		goto unlock;
	}

	LOG_DBG("Set CRESET high");
	*config_bitbang->set |= creset;

	LOG_DBG("Delay %u us", config->config_delay_us);
	k_busy_wait(config->config_delay_us);

	LOG_DBG("Set SPI_CS high");
	*config_bitbang->set |= cs;

	LOG_DBG("Send %u clocks", config->leading_clocks);
	fpga_ice40_send_clocks(config_bitbang->mhz_delay_count, config_bitbang->set,
			       config_bitbang->clear, clk, config->leading_clocks);

	LOG_DBG("Set SPI_CS low");
	LOG_DBG("Send bin file");
	LOG_DBG("Set SPI_CS high");
	fpga_ice40_spi_send_data(config_bitbang->mhz_delay_count, config_bitbang->set,
				 config_bitbang->clear, cs, clk, pico, (uint8_t *)image_ptr,
				 img_size);

	LOG_DBG("Send %u clocks", config->trailing_clocks);
	fpga_ice40_send_clocks(config_bitbang->mhz_delay_count, config_bitbang->set,
			       config_bitbang->clear, clk, config->trailing_clocks);

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
	(void)gpio_pin_configure_dt(&config_bitbang->clk, GPIO_DISCONNECTED);
	(void)gpio_pin_configure_dt(&config_bitbang->pico, GPIO_DISCONNECTED);
#ifdef CONFIG_PINCTRL
	(void)pinctrl_apply_state(config_bitbang->pincfg, PINCTRL_STATE_DEFAULT);
#endif /* CONFIG_PINCTRL */

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

#ifdef CONFIG_PINCTRL
#define FPGA_ICE40_PINCTRL_DEFINE(inst) PINCTRL_DT_DEFINE(DT_INST_PARENT(inst))
#define FPGA_ICE40_PINCTRL_GET(inst)    .pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(inst)),
#else
#define FPGA_ICE40_PINCTRL_DEFINE(inst)
#define FPGA_ICE40_PINCTRL_GET(inst)
#endif /* CONFIG_PINCTRL */

#define FPGA_ICE40_DEFINE(inst)                                                                    \
	BUILD_ASSERT(DT_INST_PROP(inst, mhz_delay_count) >= 0);                                    \
                                                                                                   \
	FPGA_ICE40_PINCTRL_DEFINE(inst);                                                           \
	static struct fpga_ice40_data fpga_ice40_data_##inst;                                      \
                                                                                                   \
	static const struct fpga_ice40_config_bitbang fpga_ice40_config_bitbang_##inst = {         \
		.clk = GPIO_DT_SPEC_INST_GET(inst, clk_gpios),                                     \
		.pico = GPIO_DT_SPEC_INST_GET(inst, pico_gpios),                                   \
		.set = DT_INST_PROP(inst, gpios_set_reg),                                          \
		.clear = DT_INST_PROP(inst, gpios_clear_reg),                                      \
		.mhz_delay_count = DT_INST_PROP(inst, mhz_delay_count),                            \
		FPGA_ICE40_PINCTRL_GET(inst)};                                                     \
                                                                                                   \
	FPGA_ICE40_CONFIG_DEFINE(inst, &fpga_ice40_config_bitbang_##inst);                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fpga_ice40_init, NULL, &fpga_ice40_data_##inst,                \
			      &fpga_ice40_config_##inst, POST_KERNEL, CONFIG_FPGA_INIT_PRIORITY,   \
			      &fpga_ice40_api);

#define DT_DRV_COMPAT lattice_ice40_fpga_bitbang
DT_INST_FOREACH_STATUS_OKAY(FPGA_ICE40_DEFINE)
