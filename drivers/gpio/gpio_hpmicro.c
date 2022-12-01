/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT hpmicro_hpm_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <hpm_gpio_drv.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

struct gpio_hpm_config {
	struct gpio_driver_config common;
	GPIO_Type *gpio_base;
	uint32_t  port_base;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif /* CONFIG_PINCTRL */
};

struct gpio_hpm_data {
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

static int gpio_hpm_configure(const struct device *dev,
				   gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_hpm_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port_base = config->port_base;

	if ((flags & GPIO_INPUT) && (flags & GPIO_OUTPUT)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_SINGLE_ENDED) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_UP) || (flags & GPIO_PULL_DOWN)) {
		return -ENOTSUP;
	}

	/* The flags contain options that require touching registers in the
	 * GPIO module and the corresponding PORT module.
	 *
	 * Start with the GPIO module and set up the pin direction register.
	 * 0 - pin is input, 1 - pin is output
	 */

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_DISCONNECTED:
	case GPIO_INPUT:
		gpio_disable_pin_output(gpio_base, port_base, pin);
		break;
	case GPIO_OUTPUT:
		gpio_enable_pin_output(gpio_base, port_base, pin);
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_write_pin(gpio_base, port_base, pin, 1);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_write_pin(gpio_base, port_base, pin, 0);
		}
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int gpio_hpm_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_hpm_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port_base = config->port_base;

	*value = gpio_read_port(gpio_base, port_base);

	return 0;
}

static int gpio_hpm_port_set_masked_raw(const struct device *dev,
					 uint32_t mask,
					 uint32_t value)
{
	const struct gpio_hpm_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port_base = config->port_base;
	uint32_t port_val;

	port_val = gpio_read_port(gpio_base, port_base);
	mask &= gpio_base->OE[port_base].CLEAR;
	if (mask != 0) {
		gpio_write_port(gpio_base, port_base, (port_val & ~mask) | (mask & value));
	}

	return 0;
}

static int gpio_hpm_port_set_bits_raw(const struct device *dev,
					   uint32_t mask)
{
	const struct gpio_hpm_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port_base = config->port_base;

	gpio_set_port_high_with_mask(gpio_base, port_base, mask);

	return 0;
}

static int gpio_hpm_port_clear_bits_raw(const struct device *dev,
					 uint32_t mask)
{
	const struct gpio_hpm_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port_base = config->port_base;

	gpio_set_port_low_with_mask(gpio_base, port_base, mask);

	return 0;
}

static int gpio_hpm_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_hpm_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port_base = config->port_base;

	gpio_toggle_port_with_mask(gpio_base, port_base, mask);

	return 0;
}

static int gpio_hpm_pin_interrupt_configure(const struct device *dev,
						 gpio_pin_t pin, enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	const struct gpio_hpm_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port_base = config->port_base;
	gpio_interrupt_trigger_t trigger = 0;

	if (mode == GPIO_INT_MODE_LEVEL) {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			trigger = gpio_interrupt_trigger_level_low;
			break;
		case GPIO_INT_TRIG_HIGH:
			trigger = gpio_interrupt_trigger_level_high;
			break;
		default:
			return -ENOTSUP;
		}
	} else if (mode == GPIO_INT_MODE_EDGE) {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			trigger = gpio_interrupt_trigger_edge_falling;
			break;
		case GPIO_INT_TRIG_HIGH:
			trigger = gpio_interrupt_trigger_edge_rising;
			break;
		default:
			return -ENOTSUP;
		}
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		gpio_disable_pin_interrupt(gpio_base, port_base, pin);
	} else {
		gpio_config_pin_interrupt(gpio_base, port_base,
						   pin, trigger);
		gpio_enable_pin_interrupt(gpio_base, port_base,
							pin);
	}
	return 0;
}

static int gpio_hpm_manage_callback(const struct device *dev,
					 struct gpio_callback *callback, bool set)
{
	struct gpio_hpm_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_hpm_port_isr(const struct device *dev)
{
	const struct gpio_hpm_config *config = dev->config;
	struct gpio_hpm_data *data = dev->data;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port_base = config->port_base;
	uint32_t int_status;

	int_status = gpio_base->IF[port_base].VALUE;

	/* Clear the port interrupts */
	gpio_base->IF[port_base].VALUE = int_status;

	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}


static const struct gpio_driver_api gpio_hpm_driver_api = {
	.pin_configure = gpio_hpm_configure,
	.port_get_raw = gpio_hpm_port_get_raw,
	.port_set_masked_raw = gpio_hpm_port_set_masked_raw,
	.port_set_bits_raw = gpio_hpm_port_set_bits_raw,
	.port_clear_bits_raw = gpio_hpm_port_clear_bits_raw,
	.port_toggle_bits = gpio_hpm_port_toggle_bits,
	.pin_interrupt_configure = gpio_hpm_pin_interrupt_configure,
	.manage_callback = gpio_hpm_manage_callback,
};

#define GPIO_HPMICRO_IRQ_INIT(n)						\
	do {								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
				DT_INST_IRQ(n, priority),			\
				gpio_hpm_port_isr,				\
				DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	} while (0)

#define GPIO_PORT_BASE_ADDR(n) DT_INST_PROP(n, hpmicro_gpio_port)

#ifdef CONFIG_PINCTRL
#define PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#define PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#else
#define PINCTRL_DEFINE(n)
#define PINCTRL_INIT(n)
#endif

#define GPIO_DEVICE_INIT_HPMICRO(n)					\
	static int gpio_hpm_port## n ## _init(const struct device *dev); \
	PINCTRL_DEFINE(n)								\
	static const struct gpio_hpm_config gpio_hpm_port## n ## _config = {\
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),\
		},							\
		.gpio_base = (GPIO_Type *)DT_INST_REG_ADDR(n),		\
		.port_base = GPIO_PORT_BASE_ADDR(n),	\
		PINCTRL_INIT(n)		\
	};								\
									\
	static struct gpio_hpm_data gpio_hpm_port## n ##_data;	\
									\
	DEVICE_DT_INST_DEFINE(n,					\
		gpio_hpm_port## n ##_init,			\
		NULL,					\
		&gpio_hpm_port## n ##_data,		\
		&gpio_hpm_port## n##_config,		\
		POST_KERNEL,				\
		CONFIG_GPIO_INIT_PRIORITY,			\
		&gpio_hpm_driver_api);			\
							\
	static int gpio_hpm_port## n ##_init(const struct device *dev)	\
	{								\
		GPIO_HPMICRO_IRQ_INIT(n);\
		return 0;						\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_DEVICE_INIT_HPMICRO)
