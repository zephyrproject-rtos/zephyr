/*
 * Copyright (c) 2018 Justin Watson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_gpio

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/gpio.h>

#include "gpio_utils.h"

typedef void (*config_func_t)(const struct device *dev);

struct gpio_sam_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	Pio *regs;
	config_func_t config_func;
	uint32_t periph_id;
};

struct gpio_sam_runtime {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t cb;
};

#define DEV_CFG(dev) \
	((const struct gpio_sam_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct gpio_sam_runtime * const)(dev)->data)

#define GPIO_SAM_ALL_PINS    0xFFFFFFFF

static int gpio_sam_port_configure(const struct device *dev, uint32_t mask,
				   gpio_flags_t flags)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	if (flags & GPIO_SINGLE_ENDED) {
		/* TODO: Add support for Open Source, Open Drain mode */
		return -ENOTSUP;
	}

	if (!(flags & (GPIO_OUTPUT | GPIO_INPUT))) {
		/* Neither input nor output mode is selected */

		/* Disable the interrupt. */
		pio->PIO_IDR = mask;
		/* Disable pull-up. */
		pio->PIO_PUDR = mask;
#if defined(CONFIG_SOC_SERIES_SAM4S) || \
	defined(CONFIG_SOC_SERIES_SAM4E) || \
	defined(CONFIG_SOC_SERIES_SAME70) || \
	defined(CONFIG_SOC_SERIES_SAMV71)
		/* Disable pull-down. */
		pio->PIO_PPDDR = mask;
#endif
		/* Let the PIO control the pin (instead of a peripheral). */
		pio->PIO_PER = mask;
		/* Disable output. */
		pio->PIO_ODR = mask;

		return 0;
	}

	/* Setup the pin direcion. */
	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			/* Set the pin. */
			pio->PIO_SODR = mask;
		}
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			/* Clear the pin. */
			pio->PIO_CODR = mask;
		}
		/* Enable the output */
		pio->PIO_OER = mask;
		/* Enable direct control of output level via PIO_ODSR */
		pio->PIO_OWER = mask;
	} else {
		/* Disable the output */
		pio->PIO_ODR = mask;
	}

	/* Note: Input is always enabled. */

	/* Setup selected Pull resistor.
	 *
	 * A pull cannot be enabled if the opposite pull is enabled.
	 * Clear both pulls, then enable the one we need.
	 */
	pio->PIO_PUDR = mask;
#if defined(CONFIG_SOC_SERIES_SAM4S) || \
	defined(CONFIG_SOC_SERIES_SAM4E) || \
	defined(CONFIG_SOC_SERIES_SAME70) || \
	defined(CONFIG_SOC_SERIES_SAMV71)
	pio->PIO_PPDDR = mask;
#endif
	if (flags & GPIO_PULL_UP) {
		/* Enable pull-up. */
		pio->PIO_PUER = mask;
#if defined(CONFIG_SOC_SERIES_SAM4S) || \
	defined(CONFIG_SOC_SERIES_SAM4E) || \
	defined(CONFIG_SOC_SERIES_SAME70) || \
	defined(CONFIG_SOC_SERIES_SAMV71)

	/* Setup Pull-down resistor. */
	} else if (flags & GPIO_PULL_DOWN) {
		/* Enable pull-down. */
		pio->PIO_PPDER = mask;
#endif
	}

#if defined(CONFIG_SOC_SERIES_SAM3X)
	/* Setup debounce. */
	if (flags & GPIO_INT_DEBOUNCE) {
		pio->PIO_DIFSR = mask;
	} else {
		pio->PIO_SCIFSR = mask;
	}
#elif defined(CONFIG_SOC_SERIES_SAM4S) || \
	defined(CONFIG_SOC_SERIES_SAM4E) || \
	defined(CONFIG_SOC_SERIES_SAME70) || \
	defined(CONFIG_SOC_SERIES_SAMV71)

	/* Setup debounce. */
	if (flags & GPIO_INT_DEBOUNCE) {
		pio->PIO_IFSCER = mask;
	} else {
		pio->PIO_IFSCDR = mask;
	}
#endif

	/* Enable the PIO to control the pin (instead of a peripheral). */
	pio->PIO_PER = mask;

	return 0;
}

static int gpio_sam_config(const struct device *dev, gpio_pin_t pin,
			   gpio_flags_t flags)
{
	return gpio_sam_port_configure(dev, BIT(pin), flags);
}

static int gpio_sam_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	*value = pio->PIO_PDSR;

	return 0;
}

static int gpio_sam_port_set_masked_raw(const struct device *dev,
					uint32_t mask,
					uint32_t value)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	pio->PIO_ODSR = (pio->PIO_ODSR & ~mask) | (mask & value);

	return 0;
}

static int gpio_sam_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	/* Set pins. */
	pio->PIO_SODR = mask;

	return 0;
}

static int gpio_sam_port_clear_bits_raw(const struct device *dev,
					uint32_t mask)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	/* Clear pins. */
	pio->PIO_CODR = mask;

	return 0;
}

static int gpio_sam_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	/* Toggle pins. */
	pio->PIO_ODSR ^= mask;

	return 0;
}

static int gpio_sam_port_interrupt_configure(const struct device *dev,
					     uint32_t mask,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	/* Disable the interrupt. */
	pio->PIO_IDR = mask;
	/* Disable additional interrupt modes. */
	pio->PIO_AIMDR = mask;

	if (trig != GPIO_INT_TRIG_BOTH) {
		/* Enable additional interrupt modes to support single
		 * edge/level detection.
		 */
		pio->PIO_AIMER = mask;

		if (mode == GPIO_INT_MODE_EDGE) {
			pio->PIO_ESR = mask;
		} else {
			pio->PIO_LSR = mask;
		}

		uint32_t rising_edge;

		if (trig == GPIO_INT_TRIG_HIGH) {
			rising_edge = mask;
		} else {
			rising_edge = ~mask;
		}

		/* Set to high-level or rising edge. */
		pio->PIO_REHLSR = rising_edge & mask;
		/* Set to low-level or falling edge. */
		pio->PIO_FELLSR = ~rising_edge & mask;
	}

	if (mode != GPIO_INT_MODE_DISABLED) {
		/* Clear any pending interrupts */
		(void)pio->PIO_ISR;
		/* Enable the interrupt. */
		pio->PIO_IER = mask;
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
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;
	struct gpio_sam_runtime *context = dev->data;
	uint32_t int_stat;

	int_stat = pio->PIO_ISR;

	gpio_fire_callbacks(&context->cb, dev, int_stat);
}

static int gpio_sam_manage_callback(const struct device *port,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_sam_runtime *context = port->data;

	return gpio_manage_callback(&context->cb, callback, set);
}

static const struct gpio_driver_api gpio_sam_api = {
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
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);

	/* The peripheral clock must be enabled for the interrupts to work. */
	soc_pmc_peripheral_enable(cfg->periph_id);

	cfg->config_func(dev);

	return 0;
}

#define GPIO_SAM_INIT(n)						\
	static void port_##n##_sam_config_func(const struct device *dev);	\
									\
	static const struct gpio_sam_config port_##n##_sam_config = {	\
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),\
		},							\
		.regs = (Pio *)DT_INST_REG_ADDR(n),			\
		.periph_id = DT_INST_PROP(n, peripheral_id),		\
		.config_func = port_##n##_sam_config_func,		\
	};								\
									\
	static struct gpio_sam_runtime port_##n##_sam_runtime;		\
									\
	DEVICE_DT_INST_DEFINE(n, gpio_sam_init, device_pm_control_nop,	\
			    &port_##n##_sam_runtime,			\
			    &port_##n##_sam_config, POST_KERNEL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &gpio_sam_api);				\
									\
	static void port_##n##_sam_config_func(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    gpio_sam_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_SAM_INIT)
