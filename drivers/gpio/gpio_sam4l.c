/*
 * Copyright (c) 2018 Justin Watson
 * Copyright (c) 2020-2023 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam4l_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/irq.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

typedef void (*config_func_t)(const struct device *dev);

struct gpio_sam_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	Gpio *regs;
	config_func_t config_func;

	const struct atmel_sam_pmc_config clock_cfg;
};

struct gpio_sam_runtime {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t cb;
};

#define GPIO_SAM_ALL_PINS    0xFFFFFFFF

static int gpio_sam_port_configure(const struct device *dev,
				   uint32_t mask,
				   gpio_flags_t flags)
{
	const struct gpio_sam_config * const cfg = dev->config;
	Gpio * const gpio = cfg->regs;

	/* No hardware support */
	if (flags & GPIO_SINGLE_ENDED) {
		return -ENOTSUP;
	}

	if (!(flags & (GPIO_OUTPUT | GPIO_INPUT))) {
		gpio->IERC = mask;
		gpio->PUERC = mask;
		gpio->PDERC = mask;
		gpio->GPERS = mask;
		gpio->ODERC = mask;
		gpio->STERC = mask;

		return 0;
	}

	/*
	 * Always enable schmitt-trigger because SAM4L GPIO Ctrl
	 * is Input only or Input/Output.
	 */
	gpio->STERS = mask;

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio->OVRS = mask;
		}
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio->OVRC = mask;
		}
		gpio->ODERS = mask;
	} else {
		gpio->ODERC = mask;
	}

	gpio->PUERC = mask;
	gpio->PDERC = mask;
	if (flags & GPIO_PULL_UP) {
		gpio->PUERS = mask;
	} else if (flags & GPIO_PULL_DOWN) {
		gpio->PDERS = mask;
	}

	/* Enable the GPIO to control the pin (instead of a peripheral). */
	gpio->GPERS = mask;

	return 0;
}

static int gpio_sam_config(const struct device *dev,
			   gpio_pin_t pin,
			   gpio_flags_t flags)
{
	return gpio_sam_port_configure(dev, BIT(pin), flags);
}

static int gpio_sam_port_get_raw(const struct device *dev,
				 uint32_t *value)
{
	const struct gpio_sam_config * const cfg = dev->config;
	Gpio * const gpio = cfg->regs;

	*value = gpio->PVR;

	return 0;
}

static int gpio_sam_port_set_masked_raw(const struct device *dev,
					uint32_t mask,
					uint32_t value)
{
	const struct gpio_sam_config * const cfg = dev->config;
	Gpio * const gpio = cfg->regs;

	gpio->OVR = (gpio->PVR & ~mask) | (mask & value);

	return 0;
}

static int gpio_sam_port_set_bits_raw(const struct device *dev,
				      uint32_t mask)
{
	const struct gpio_sam_config * const cfg = dev->config;
	Gpio * const gpio = cfg->regs;

	gpio->OVRS = mask;

	return 0;
}

static int gpio_sam_port_clear_bits_raw(const struct device *dev,
					uint32_t mask)
{
	const struct gpio_sam_config * const cfg = dev->config;
	Gpio * const gpio = cfg->regs;

	gpio->OVRC = mask;

	return 0;
}

static int gpio_sam_port_toggle_bits(const struct device *dev,
				     uint32_t mask)
{
	const struct gpio_sam_config * const cfg = dev->config;
	Gpio * const gpio = cfg->regs;

	gpio->OVRT = mask;

	return 0;
}

static int gpio_sam_port_interrupt_configure(const struct device *dev,
					     uint32_t mask,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_sam_config * const cfg = dev->config;
	Gpio * const gpio = cfg->regs;

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	gpio->IERC = mask;
	gpio->IMR0C = mask;
	gpio->IMR1C = mask;

	if (trig != GPIO_INT_TRIG_BOTH) {
		if (trig == GPIO_INT_TRIG_HIGH) {
			gpio->IMR0S = mask;
		} else {
			gpio->IMR1S = mask;
		}
	}

	if (mode != GPIO_INT_MODE_DISABLED) {
		gpio->IFRC = mask;
		gpio->IERS = mask;
	}

	return 0;
}

static int gpio_sam_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	return gpio_sam_port_interrupt_configure(dev, BIT(pin), mode, trig);
}

static void gpio_sam_isr(const struct device *dev)
{
	const struct gpio_sam_config * const cfg = dev->config;
	Gpio * const gpio = cfg->regs;
	struct gpio_sam_runtime *context = dev->data;
	uint32_t int_stat;

	int_stat = gpio->IFR;
	gpio->IFRC = int_stat;

	gpio_fire_callbacks(&context->cb, dev, int_stat);
}

static int gpio_sam_manage_callback(const struct device *port,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_sam_runtime *context = port->data;

	return gpio_manage_callback(&context->cb, callback, set);
}

static DEVICE_API(gpio, gpio_sam_api) = {
	.pin_configure = gpio_sam_config,
	.port_get_raw = gpio_sam_port_get_raw,
	.port_set_masked_raw = gpio_sam_port_set_masked_raw,
	.port_set_bits_raw = gpio_sam_port_set_bits_raw,
	.port_clear_bits_raw = gpio_sam_port_clear_bits_raw,
	.port_toggle_bits = gpio_sam_port_toggle_bits,
	.pin_interrupt_configure = gpio_sam_pin_interrupt_configure,
	.manage_callback = gpio_sam_manage_callback,
};

int gpio_sam_init(const struct device *dev)
{
	const struct gpio_sam_config * const cfg = dev->config;

	/* Enable GPIO clock in PM */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clock_cfg);

	cfg->config_func(dev);

	return 0;
}

#define GPIO_SAM_IRQ_CONNECT(n, m)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq),		\
			    DT_INST_IRQ_BY_IDX(n, m, priority),		\
			    gpio_sam_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));		\
	} while (false)

#define GPIO_SAM_INIT(n)						\
	static void port_##n##_sam_config_func(const struct device *dev);\
									\
	static const struct gpio_sam_config port_##n##_sam_config = {	\
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),\
		},							\
		.regs = (Gpio *)DT_INST_REG_ADDR(n),			\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),		\
		.config_func = port_##n##_sam_config_func,		\
	};								\
									\
	static struct gpio_sam_runtime port_##n##_sam_runtime;		\
									\
	DEVICE_DT_INST_DEFINE(n, gpio_sam_init, NULL,			\
			    &port_##n##_sam_runtime,			\
			    &port_##n##_sam_config, PRE_KERNEL_1,	\
			    CONFIG_GPIO_INIT_PRIORITY,			\
			    &gpio_sam_api);				\
									\
	static void port_##n##_sam_config_func(const struct device *dev)\
	{								\
		GPIO_SAM_IRQ_CONNECT(n, 0);				\
		GPIO_SAM_IRQ_CONNECT(n, 1);				\
		GPIO_SAM_IRQ_CONNECT(n, 2);				\
		GPIO_SAM_IRQ_CONNECT(n, 3);				\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_SAM_INIT)
