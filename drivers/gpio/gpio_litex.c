/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_gpio

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <zephyr/types.h>
#include <sys/util.h>
#include <string.h>
#include <logging/log.h>

#define SUPPORTED_FLAGS (GPIO_INPUT | GPIO_OUTPUT | \
			GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH | \
			GPIO_ACTIVE_LOW | GPIO_ACTIVE_HIGH)

#define GPIO_LOW        0
#define GPIO_HIGH       1

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
LOG_MODULE_REGISTER(gpio_litex);

static const char *LITEX_LOG_REG_SIZE_NGPIOS_MISMATCH =
	"Cannot handle all of the gpios with the register of given size\n";
static const char *LITEX_LOG_CANNOT_CHANGE_DIR =
	"Cannot change port direction selected in device tree\n";

struct gpio_litex_cfg {
	volatile uint32_t *reg_addr;
	int reg_size;
	int nr_gpios;
	bool port_is_output;
};

struct gpio_litex_data {
	struct gpio_driver_data common;
};

/* Helper macros for GPIO */

#define DEV_GPIO_CFG(dev)						\
	((const struct gpio_litex_cfg *)(dev)->config)

/* Helper functions for bit / port access */

static inline void set_bit(const struct gpio_litex_cfg *config,
			   uint32_t bit, bool val)
{
	int regv, new_regv;

	regv = litex_read(config->reg_addr, config->reg_size);
	new_regv = (regv & ~BIT(bit)) | (val << bit);
	litex_write(config->reg_addr, config->reg_size, new_regv);
}

static inline uint32_t get_bit(const struct gpio_litex_cfg *config, uint32_t bit)
{
	int regv = litex_read(config->reg_addr, config->reg_size);

	return !!(regv & BIT(bit));
}

static inline void set_port(const struct gpio_litex_cfg *config, uint32_t value)
{
	litex_write(config->reg_addr, config->reg_size, value);
}

static inline uint32_t get_port(const struct gpio_litex_cfg *config)
{
	int regv = litex_read(config->reg_addr, config->reg_size);

	return (regv & BIT_MASK(config->nr_gpios));
}

/* Driver functions */

static int gpio_litex_init(const struct device *dev)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);

	/* each 4-byte register is able to handle 8 GPIO pins */
	if (gpio_config->nr_gpios > (gpio_config->reg_size * 8)) {
		LOG_ERR("%s", LITEX_LOG_REG_SIZE_NGPIOS_MISMATCH);
		return -EINVAL;
	}

	return 0;
}

static int gpio_litex_configure(const struct device *dev,
				gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);

	if (flags & ~SUPPORTED_FLAGS) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) && (flags & GPIO_INPUT)) {
		/* Pin cannot be configured as input and output */
		return -ENOTSUP;
	} else if (!(flags & (GPIO_INPUT | GPIO_OUTPUT))) {
		/* Pin has to be configuread as input or output */
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT) {
		if (!gpio_config->port_is_output) {
			LOG_ERR("%s", LITEX_LOG_CANNOT_CHANGE_DIR);
			return -EINVAL;
		}

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			set_bit(gpio_config, pin, GPIO_HIGH);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			set_bit(gpio_config, pin, GPIO_LOW);
		}
	} else {
		if (gpio_config->port_is_output) {
			LOG_ERR("%s", LITEX_LOG_CANNOT_CHANGE_DIR);
			return -EINVAL;
		}
	}

	return 0;
}

static int gpio_litex_port_get_raw(const struct device *dev,
				   gpio_port_value_t *value)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);

	*value = get_port(gpio_config);
	return 0;
}

static int gpio_litex_port_set_masked_raw(const struct device *dev,
					  gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	port_val = get_port(gpio_config);
	port_val = (port_val & ~mask) | (value & mask);
	set_port(gpio_config, port_val);

	return 0;
}

static int gpio_litex_port_set_bits_raw(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	port_val = get_port(gpio_config);
	port_val |= pins;
	set_port(gpio_config, port_val);

	return 0;
}

static int gpio_litex_port_clear_bits_raw(const struct device *dev,
					  gpio_port_pins_t pins)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	port_val = get_port(gpio_config);
	port_val &= ~pins;
	set_port(gpio_config, port_val);

	return 0;
}

static int gpio_litex_port_toggle_bits(const struct device *dev,
				       gpio_port_pins_t pins)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	port_val = get_port(gpio_config);
	port_val ^= pins;
	set_port(gpio_config, port_val);

	return 0;
}

static int gpio_litex_pin_interrupt_configure(const struct device *dev,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	int ret = 0;

	if (mode != GPIO_INT_MODE_DISABLED) {
		ret = -ENOTSUP;
	}
	return ret;
}

static const struct gpio_driver_api gpio_litex_driver_api = {
	.pin_configure = gpio_litex_configure,
	.port_get_raw = gpio_litex_port_get_raw,
	.port_set_masked_raw = gpio_litex_port_set_masked_raw,
	.port_set_bits_raw = gpio_litex_port_set_bits_raw,
	.port_clear_bits_raw = gpio_litex_port_clear_bits_raw,
	.port_toggle_bits = gpio_litex_port_toggle_bits,
	.pin_interrupt_configure = gpio_litex_pin_interrupt_configure,
};

/* Device Instantiation */

#define GPIO_LITEX_INIT(n) \
	BUILD_ASSERT(DT_INST_REG_SIZE(n) != 0 \
		     && DT_INST_REG_SIZE(n) % 4 == 0, \
		     "Register size must be a multiple of 4"); \
\
	static const struct gpio_litex_cfg gpio_litex_cfg_##n = { \
		.reg_addr = \
		(volatile uint32_t *) DT_INST_REG_ADDR(n), \
		.reg_size = DT_INST_REG_SIZE(n) / 4, \
		.nr_gpios = DT_INST_PROP(n, ngpios), \
		.port_is_output = DT_INST_PROP(n, port_is_output), \
	}; \
	static struct gpio_litex_data gpio_litex_data_##n; \
\
	DEVICE_DT_INST_DEFINE(n, \
			    gpio_litex_init, \
			    device_pm_control_nop, \
			    &gpio_litex_data_##n, \
			    &gpio_litex_cfg_##n, \
			    POST_KERNEL, \
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			    &gpio_litex_driver_api \
			   );

DT_INST_FOREACH_STATUS_OKAY(GPIO_LITEX_INIT)
