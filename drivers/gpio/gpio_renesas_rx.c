/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_gpio

#include <string.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <soc.h>

struct gpio_rx_config {
	struct gpio_driver_config common;
	uint8_t port_num;
	volatile struct {
		volatile uint8_t *pdr;
		volatile uint8_t *podr;
		volatile uint8_t *pidr;
		volatile uint8_t *pmr;
		volatile uint8_t *odr0;
		volatile uint8_t *odr1;
		volatile uint8_t *pcr;
		volatile uint8_t *dscr;
		volatile uint8_t *dscr2;
	} reg;
};

struct gpio_rx_data {
	struct gpio_driver_data common;
};

static int gpio_rx_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_rx_config *cfg = dev->config;
	struct rx_pinctrl_soc_pin pincfg;

	memset(&pincfg, 0, sizeof(pincfg));

	if (((flags & GPIO_INPUT) != 0U) && ((flags & GPIO_OUTPUT) != 0U)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_DOWN) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_INT_ENABLE) != 0) {
		return -ENOTSUP;
	}

	pincfg.port_num = cfg->port_num;
	pincfg.pin_num = pin;

	/* Set pull-up if requested */
	if ((flags & GPIO_PULL_UP) != 0) {
		pincfg.cfg.bias_pull_up = 1;
	}

	/* Open drain (pins 0-3: odr0, pins 4-8: odr1) */
	if ((flags & GPIO_LINE_OPEN_DRAIN) != 0) {
		pincfg.cfg.drive_open_drain = 1;
	}

	/* Set to output */
	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			pincfg.cfg.output_high = 1;
		}
		pincfg.cfg.pin_mode = 0;
		pincfg.cfg.output_enable = 1;
	}

	return pinctrl_configure_pins(&pincfg, 1, PINCTRL_REG_NONE);
}

static int gpio_rx_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_rx_config *cfg = dev->config;

	*value = *(cfg->reg.pidr);
	return 0;
}

static int gpio_rx_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	const struct gpio_rx_config *cfg = dev->config;

	*(cfg->reg.podr) = ((*cfg->reg.podr) & ~mask) | (mask & value);
	return 0;
}

static int gpio_rx_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rx_config *cfg = dev->config;

	*(cfg->reg.podr) |= pins;
	return 0;
}

static int gpio_rx_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rx_config *cfg = dev->config;

	*(cfg->reg.podr) &= ~pins;
	return 0;
}

static int gpio_rx_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rx_config *cfg = dev->config;

	*(cfg->reg.podr) = *(cfg->reg.podr) ^ pins;
	return 0;
}

static DEVICE_API(gpio, gpio_rx_drv_api_funcs) = {
	.pin_configure = gpio_rx_pin_configure,
	.port_get_raw = gpio_rx_port_get_raw,
	.port_set_masked_raw = gpio_rx_port_set_masked_raw,
	.port_set_bits_raw = gpio_rx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rx_port_clear_bits_raw,
	.port_toggle_bits = gpio_rx_port_toggle_bits,
	.pin_interrupt_configure = NULL,
	.manage_callback = NULL,
};

#define GPIO_DEVICE_INIT(node, port_number, suffix, addr)                                          \
	static const struct gpio_rx_config gpio_rx_config_##suffix = {                             \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(8U)},                   \
		.port_num = port_number,                                                           \
		.reg = {.pdr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PDR),                          \
			.podr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PODR),                        \
			.pidr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PIDR),                        \
			.pmr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PMR),                          \
			.odr0 = (uint8_t *)DT_REG_ADDR_BY_NAME_OR(node, ODR0, NULL),               \
			.odr1 = (uint8_t *)DT_REG_ADDR_BY_NAME_OR(node, ODR1, NULL),               \
			.pcr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PCR)}};                        \
	static struct gpio_rx_data gpio_rx_data_##suffix;                                          \
	DEVICE_DT_DEFINE(node, NULL, NULL, &gpio_rx_data_##suffix, &gpio_rx_config_##suffix,       \
			 PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_rx_drv_api_funcs)

#define GPIO_DEVICE_INIT_RX(suffix)                                                                \
	GPIO_DEVICE_INIT(DT_NODELABEL(ioport##suffix),                                             \
			 DT_PROP(DT_NODELABEL(ioport##suffix), port), suffix,                      \
			 DT_REG_ADDR(DT_NODELABEL(ioport##suffix)))

#define GPIO_DEVICE_INIT_RX_IF_OKAY(suffix)                                                        \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(ioport##suffix), okay),                        \
		    (GPIO_DEVICE_INIT_RX(suffix)), ())

GPIO_DEVICE_INIT_RX_IF_OKAY(0);
GPIO_DEVICE_INIT_RX_IF_OKAY(1);
GPIO_DEVICE_INIT_RX_IF_OKAY(2);
GPIO_DEVICE_INIT_RX_IF_OKAY(3);
GPIO_DEVICE_INIT_RX_IF_OKAY(4);
GPIO_DEVICE_INIT_RX_IF_OKAY(5);
GPIO_DEVICE_INIT_RX_IF_OKAY(a);
GPIO_DEVICE_INIT_RX_IF_OKAY(b);
GPIO_DEVICE_INIT_RX_IF_OKAY(c);
GPIO_DEVICE_INIT_RX_IF_OKAY(d);
GPIO_DEVICE_INIT_RX_IF_OKAY(e);
GPIO_DEVICE_INIT_RX_IF_OKAY(h);
GPIO_DEVICE_INIT_RX_IF_OKAY(j);
