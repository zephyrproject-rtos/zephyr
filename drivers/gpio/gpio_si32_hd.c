/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/silabs-si32-hd-gpio.h>
#include <zephyr/kernel.h>

#include <SI32_PBCFG_A_Type.h>
#include <SI32_PBHD_A_Type.h>
#include <si32_device.h>

#include <errno.h>

struct gpio_si32_hd_config {
	struct gpio_driver_config common;
	SI32_PBHD_A_Type *base;

	uint32_t nchannel_current_limit;
	uint32_t pchannel_current_limit;

	bool has_nchannel_current_limit;
	bool has_pchannel_current_limit;

	bool enable_bias;
	bool low_power_port_mode;
	bool enable_drivers;
};

struct gpio_si32_hd_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

static int gpio_si32_hd_configure(const struct device *dev, const gpio_pin_t pin,
				  const gpio_flags_t flags)
{
	const struct gpio_si32_hd_config *config = dev->config;

	/* Simultaneous input & output mode not supported */
	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	/* Set pin function to GPIO. This is the only supported mode for now. */
	switch (pin) {
	case 0:
		SI32_PBHD_A_select_pin0_function(SI32_PBHD_4, 0);
		break;
	case 1:
		SI32_PBHD_A_select_pin1_function(SI32_PBHD_4, 0);
		break;
	case 2:
		SI32_PBHD_A_select_pin2_function(SI32_PBHD_4, 0);
		break;
	case 3:
		SI32_PBHD_A_select_pin3_function(SI32_PBHD_4, 0);
		break;
	case 4:
		SI32_PBHD_A_select_pin4_function(SI32_PBHD_4, 0);
		break;
	case 5:
		SI32_PBHD_A_select_pin5_function(SI32_PBHD_4, 0);
		break;
	}

	if (flags & SI32_GPIO_DS_ENABLE_CURRENT_LIMIT) {
		SI32_PBHD_A_enable_pin_current_limit(config->base, BIT(pin));
	}

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			SI32_PBHD_A_write_pins_high(config->base, BIT(pin));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			SI32_PBHD_A_write_pins_low(config->base, BIT(pin));
		}

		SI32_PBHD_A_set_pins_push_pull_output(config->base, BIT(pin));
	} else if (flags & GPIO_INPUT) {
		SI32_PBHD_A_set_pins_digital_input(config->base, BIT(pin));
	} else {
		SI32_PBHD_A_set_pins_analog(config->base, BIT(pin));
	}

	return 0;
}

static int gpio_si32_hd_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_si32_hd_config *config = dev->config;

	*value = SI32_PBHD_A_read_pins(config->base);

	return 0;
}

static int gpio_si32_hd_port_set_masked_raw(const struct device *dev, const gpio_port_pins_t mask,
					    const gpio_port_value_t value)
{
	const struct gpio_si32_hd_config *config = dev->config;

	SI32_PBHD_A_write_pins_masked(config->base, value, mask);

	return 0;
}

static int gpio_si32_hd_port_set_bits_raw(const struct device *dev, const gpio_port_pins_t pins)
{
	const struct gpio_si32_hd_config *config = dev->config;

	SI32_PBHD_A_write_pins_high(config->base, pins);

	return 0;
}

static int gpio_si32_hd_port_clear_bits_raw(const struct device *dev, const gpio_port_pins_t pins)
{
	const struct gpio_si32_hd_config *config = dev->config;

	SI32_PBHD_A_write_pins_low(config->base, pins);

	return 0;
}

static int gpio_si32_hd_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_si32_hd_config *config = dev->config;

	SI32_PBHD_A_toggle_pins(config->base, pins);

	return 0;
}

static int gpio_si32_hd_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

static const struct gpio_driver_api gpio_si32_hd_driver = {
	.pin_configure = gpio_si32_hd_configure,
	.port_get_raw = gpio_si32_hd_port_get_raw,
	.port_set_masked_raw = gpio_si32_hd_port_set_masked_raw,
	.port_set_bits_raw = gpio_si32_hd_port_set_bits_raw,
	.port_clear_bits_raw = gpio_si32_hd_port_clear_bits_raw,
	.port_toggle_bits = gpio_si32_hd_port_toggle_bits,
	.pin_interrupt_configure = gpio_si32_hd_pin_interrupt_configure,
};

static int gpio_si32_hd_init(const struct device *dev)
{
	const struct gpio_si32_hd_config *config = dev->config;

	SI32_PBCFG_A_unlock_ports(SI32_PBCFG_0);
	SI32_PBHD_A_write_pblock(config->base, 0x0000);

	if (config->has_nchannel_current_limit) {
		SI32_PBHD_A_select_nchannel_current_limit(config->base,
							  config->nchannel_current_limit);
	}
	if (config->has_pchannel_current_limit) {
		SI32_PBHD_A_select_pchannel_current_limit(config->base,
							  config->pchannel_current_limit);
	}
	if (config->enable_bias) {
		SI32_PBHD_A_enable_bias(config->base);
	}
	if (config->low_power_port_mode) {
		SI32_PBHD_A_select_low_power_port_mode(config->base);
	}
	if (config->enable_drivers) {
		SI32_PBHD_A_enable_drivers(config->base);
	}

	return 0;
}

/* clang-format off */
#define OPT_INT_PROP(__node, name)                                                                 \
	COND_CODE_1(DT_NODE_HAS_PROP(__node, name),                                                \
		    (.name = DT_PROP(__node, name), .has_##name = true,), ())
/* clang-format on */

#define GPIO_DEVICE_INIT(__node, __suffix, __base_addr)                                            \
	static const struct gpio_si32_hd_config gpio_si32_hd_cfg_##__suffix = {                    \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(6U),               \
			},                                                                         \
		.base = (SI32_PBHD_A_Type *)__base_addr,                                           \
		OPT_INT_PROP(__node, nchannel_current_limit)                                       \
			OPT_INT_PROP(__node, nchannel_current_limit)                               \
				.enable_bias = DT_PROP(__node, enable_bias),                       \
		.low_power_port_mode = DT_PROP(__node, low_power_port_mode),                       \
		.enable_drivers = DT_PROP(__node, enable_drivers),                                 \
	};                                                                                         \
	static struct gpio_si32_hd_data gpio_si32_hd_data_##__suffix;                              \
	DEVICE_DT_DEFINE(__node, gpio_si32_hd_init, NULL, &gpio_si32_hd_data_##__suffix,           \
			 &gpio_si32_hd_cfg_##__suffix, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,    \
			 &gpio_si32_hd_driver)

#define GPIO_DEVICE_INIT_SI32_HD(__suffix)                                                         \
	GPIO_DEVICE_INIT(DT_NODELABEL(gpio##__suffix), __suffix,                                   \
			 DT_REG_ADDR(DT_NODELABEL(gpio##__suffix)))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio4), okay)
GPIO_DEVICE_INIT_SI32_HD(4);
#endif
