/*
 * Copyright (c) 2023 Efinix Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT efinix_sapphire_gpio

#include <zephyr/drivers/gpio/gpio_utils.h>

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

LOG_MODULE_REGISTER(gpio_efinix_sapphire);

#define SUPPORTED_FLAGS                                                                            \
	(GPIO_INPUT | GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH |                 \
	 GPIO_ACTIVE_LOW | GPIO_ACTIVE_HIGH)

#define GPIO_LOW  0
#define GPIO_HIGH 1

#define BSP_GPIO_INPUT                 0x00
#define BSP_GPIO_OUTPUT                0x04
#define BSP_GPIO_OUTPUT_ENABLE         0x08
#define BSP_GPIO_INTERRUPT_RISE_ENABLE 0x20
#define BSP_GPIO_INTERRUPT_FALL_ENABLE 0x24
#define BSP_GPIO_INTERRUPT_HIGH_ENABLE 0x28
#define BSP_GPIO_INTERRUPT_LOW_ENABLE  0x2c

/* efinix sapphire specefic gpio config struct */
struct gpio_efinix_sapphire_cfg {
	uint32_t base_addr;
	int n_gpios;
	struct gpio_driver_config common;
};

/* efinix sapphire specefic gpio data struct */
struct gpio_efinix_sapphire_data {
	struct gpio_driver_data common;
	const struct device *dev;
	sys_slist_t cb;
};

/* Device access pointer helpers */
#define DEV_GPIO_CFG(dev) ((const struct gpio_efinix_sapphire_cfg *)(dev)->config)
#define GPIO_OUTPUT_ADDR  config->base_addr + BSP_GPIO_OUTPUT

static inline void cfg_output_enable_bit(const struct gpio_efinix_sapphire_cfg *config,
					 gpio_pin_t pin, uint32_t type)
{

#define GPIO_OUTPUT_ENABLE_ADDR config->base_addr + BSP_GPIO_OUTPUT_ENABLE
	uint32_t c_reg_val = sys_read32(GPIO_OUTPUT_ENABLE_ADDR);

	if (type == GPIO_INPUT) {
		sys_write32(c_reg_val &= ~pin, GPIO_OUTPUT_ENABLE_ADDR);
	} else if (type == GPIO_OUTPUT) {
		sys_write32(c_reg_val |= pin, GPIO_OUTPUT_ENABLE_ADDR);
	}
}

static inline void cfg_output_bit(const struct gpio_efinix_sapphire_cfg *config, gpio_pin_t pin,
				  uint32_t value)
{

	uint32_t c_reg_val = sys_read32(GPIO_OUTPUT_ADDR);

	if (value == GPIO_LOW) {
		sys_write32(c_reg_val &= ~pin, GPIO_OUTPUT_ADDR);
	} else if (value == GPIO_HIGH) {
		sys_write32(c_reg_val |= pin, GPIO_OUTPUT_ADDR);
	}
}

/* To use the controller bare minimun as IO, Peripheral has to configure, */
/* the Output enable register, b0 : Input, b1 : Output */

static int gpio_efinix_sapphire_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_efinix_sapphire_cfg *config = DEV_GPIO_CFG(dev);
	/* Check if the controller supports the requested GPIO configuration.  */
	if (flags & ~SUPPORTED_FLAGS) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_MASK) {
		/* Pin cannot be configured as input and output */
		return -ENOTSUP;
	} else if ((flags & GPIO_DIR_MASK) == GPIO_DISCONNECTED) {
		/* Pin has to be configured as input or output */
		return -ENOTSUP;
	}

	/* Configure the output register based on the direction flag */
	if (flags & GPIO_OUTPUT) {
		/* Set the pin as output */
		cfg_output_enable_bit(config, BIT(pin), GPIO_OUTPUT);
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			/* Set the pin to high */
			cfg_output_bit(config, BIT(pin), GPIO_HIGH);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			/* Set the pin to low */
			cfg_output_bit(config, BIT(pin), GPIO_LOW);
		}
	} else {
		/* Set the pin as input */
		cfg_output_enable_bit(config, BIT(pin), GPIO_INPUT);
	}

	return 0;
}

static inline uint32_t get_port(const struct gpio_efinix_sapphire_cfg *config)
{
	uint32_t c_reg_val = sys_read32(GPIO_OUTPUT_ADDR);

	return (c_reg_val & BIT_MASK(config->n_gpios));
}

static inline void set_port(const struct gpio_efinix_sapphire_cfg *config, uint32_t value)
{
	sys_write32(value, GPIO_OUTPUT_ADDR);
}

static int gpio_efinix_sapphire_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_efinix_sapphire_cfg *config = DEV_GPIO_CFG(dev);

	*value = get_port(config);
	return 0;
}

static int gpio_efinix_sapphire_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
						    gpio_port_value_t value)
{
	const struct gpio_efinix_sapphire_cfg *config = DEV_GPIO_CFG(dev);

	uint32_t c_reg_val = get_port(config);

	/* Sets ports value at one go */
	c_reg_val &= ~mask;
	c_reg_val |= (value & mask);

	set_port(config, c_reg_val);

	return 0;
}

static int gpio_efinix_sapphire_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_efinix_sapphire_cfg *config = DEV_GPIO_CFG(dev);

	uint32_t c_reg_val = get_port(config);

	/* Sets ports value at one go */
	c_reg_val |= pins;

	set_port(config, c_reg_val);

	return 0;
}

static int gpio_efinix_sapphire_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_efinix_sapphire_cfg *config = DEV_GPIO_CFG(dev);

	uint32_t c_reg_val = get_port(config);

	/* Sets ports value at one go */
	c_reg_val &= ~pins;

	set_port(config, c_reg_val);

	return 0;
}

static int gpio_efinix_sapphire_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_efinix_sapphire_cfg *config = DEV_GPIO_CFG(dev);

	uint32_t c_reg_val = get_port(config);

	/* Sets ports value at one go */
	c_reg_val ^= pins;

	set_port(config, c_reg_val);

	return 0;
}

static int gpio_efinix_sapphire_init(const struct device *dev)
{
	const struct gpio_efinix_sapphire_cfg *config = DEV_GPIO_CFG(dev);

	if (config->n_gpios > 4) {
		return -EINVAL;
	}
	return 0;
}

/* API map */
static const struct gpio_driver_api gpio_efinix_sapphire_api = {
	.pin_configure = gpio_efinix_sapphire_config,
	.port_get_raw = gpio_efinix_sapphire_port_get_raw,
	.port_set_masked_raw = gpio_efinix_sapphire_port_set_masked_raw,
	.port_set_bits_raw = gpio_efinix_sapphire_port_set_bits_raw,
	.port_clear_bits_raw = gpio_efinix_sapphire_port_clear_bits_raw,
	.port_toggle_bits = gpio_efinix_sapphire_port_toggle_bits,
};

#define GPIO_EFINIX_SAPPHIRE_INIT(n) \
	static struct gpio_efinix_sapphire_cfg gpio_efinix_sapphire_cfg_##n = { \
		.base_addr = DT_INST_REG_ADDR(n), \
		.n_gpios = DT_INST_PROP(n, ngpios), \
}; \
static struct gpio_efinix_sapphire_data gpio_efinix_sapphire_data_##n; \
	DEVICE_DT_INST_DEFINE(n, \
			    gpio_efinix_sapphire_init, \
			    NULL, \
			    &gpio_efinix_sapphire_data_##n, \
			    &gpio_efinix_sapphire_cfg_##n, \
			    POST_KERNEL, \
			    CONFIG_GPIO_INIT_PRIORITY, \
			    &gpio_efinix_sapphire_api \
				); \

DT_INST_FOREACH_STATUS_OKAY(GPIO_EFINIX_SAPPHIRE_INIT)
