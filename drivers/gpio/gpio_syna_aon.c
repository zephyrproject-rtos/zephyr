/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Synaptics Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT syna_aon_gpio

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(syna_aon_gpio, CONFIG_GPIO_LOG_LEVEL);

struct syna_aon_config {
	struct gpio_driver_config common;
	mem_addr_t reg;
	uint32_t ngpios;
	uint32_t reg_width;
	uint32_t polarity_shift;
	const struct pinctrl_dev_config *pcfg;
};

static void syna_aon_gpio_configure(const struct syna_aon_config *config, uint8_t gpo,
				    uint32_t polarity)
{
	uint32_t shift;
	uint32_t value;

	shift = (gpo * config->reg_width) + config->polarity_shift;

	value = sys_read32(config->reg);
	value &= ~(0x1 << shift);
	value |= polarity << shift;
	sys_write32(value, config->reg);
}

static int syna_aon_gpio_get(const struct syna_aon_config *config, uint8_t gpo)
{
	uint32_t shift = (gpo * config->reg_width) + config->polarity_shift;

	return !!(sys_read32(config->reg) & BIT(shift));
}

static int syna_aon_gpio_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct syna_aon_config *config = dev->config;

	if ((config->common.port_pin_mask & BIT(pin)) == 0) {
		LOG_ERR("invalid pin number %i", pin);
		return -EINVAL;
	}

	if ((flags & GPIO_INPUT) != 0) {
		LOG_ERR("cannot configure pin as input");
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) == 0) {
		LOG_ERR("pin must be configured as an output");
		return -ENOTSUP;
	}

	syna_aon_gpio_configure(config, pin, !!(flags & GPIO_OUTPUT_INIT_HIGH));

	return 0;
}

static int syna_aon_gpio_port_get_raw(const struct device *dev, uint32_t *value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(value);

	LOG_ERR("input pins are not available");

	return -ENOTSUP;
}

static int syna_aon_gpio_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	const struct syna_aon_config *config = port->config;

	mask &= config->common.port_pin_mask;

	for (int i = 0; i < config->ngpios; i++) {
		if (mask & BIT(i)) {
			syna_aon_gpio_configure(config, i, !!(value & BIT(i)));
		}
	}

	return 0;
}

static int syna_aon_gpio_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	return syna_aon_gpio_port_set_masked_raw(port, pins, pins);
}

static int syna_aon_gpio_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	return syna_aon_gpio_port_set_masked_raw(port, pins, 0);
}

static int syna_aon_gpio_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct syna_aon_config *config = port->config;
	uint32_t value;

	pins &= config->common.port_pin_mask;

	for (int i = 0; i < config->ngpios; i++) {
		if (pins & BIT(i)) {
			value = syna_aon_gpio_get(config, i);
			syna_aon_gpio_configure(config, i, !value);
		}
	}

	return 0;
}

static DEVICE_API(gpio, syna_aon_gpio_driver_api) = {
	.pin_configure = syna_aon_gpio_pin_configure,
	.port_get_raw = syna_aon_gpio_port_get_raw,
	.port_set_masked_raw = syna_aon_gpio_port_set_masked_raw,
	.port_set_bits_raw = syna_aon_gpio_port_set_bits_raw,
	.port_clear_bits_raw = syna_aon_gpio_port_clear_bits_raw,
	.port_toggle_bits = syna_aon_gpio_port_toggle_bits,
};

static int syna_aon_gpio_initialize(const struct device *dev)
{
	const struct syna_aon_config *config = dev->config;
	int err;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	return 0;
}

#define SYNA_AON_GPIO_INIT(n)                                                                      \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct syna_aon_config aon_gpio_##n##_config = {                              \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.reg = DT_INST_REG_ADDR(n),                                                        \
		.reg_width = DT_INST_PROP_OR(n, reg_width, 9),                                     \
		.polarity_shift = DT_INST_PROP_OR(n, polarity_shift, 8),                           \
		.ngpios = DT_INST_PROP_OR(n, ngpios, 3),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, syna_aon_gpio_initialize, NULL, NULL, &aon_gpio_##n##_config,     \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &syna_aon_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SYNA_AON_GPIO_INIT)
