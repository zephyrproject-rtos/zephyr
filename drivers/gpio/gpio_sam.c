/*
 * Copyright (c) 2018 Justin Watson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/gpio.h>

#include "gpio_utils.h"

typedef void (*config_func_t)(struct device *dev);

struct gpio_sam_config {
	Pio *regs;
	config_func_t config_func;
	u32_t periph_id;
};

struct gpio_sam_runtime {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t cb;
};

#define DEV_CFG(dev) \
	((const struct gpio_sam_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct gpio_sam_runtime * const)(dev)->driver_data)

#define GPIO_SAM_ALL_PINS    0xFFFFFFFF

static int gpio_sam_port_configure(struct device *dev, u32_t mask, int flags)
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
#if defined(CONFIG_SOC_SERIES_SAM4S) || defined(CONFIG_SOC_SERIES_SAME70)
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

	/* Setup Pull-up resistor. */
	if (flags & GPIO_PULL_UP) {
		/* Enable pull-up. */
		pio->PIO_PUER = mask;
	} else {
		pio->PIO_PUDR = mask;
	}

#if defined(CONFIG_SOC_SERIES_SAM4S) || defined(CONFIG_SOC_SERIES_SAME70)
	/* Setup Pull-down resistor. */
	if (flags & GPIO_PULL_DOWN) {
		pio->PIO_PPDER = mask;
	} else {
		pio->PIO_PPDDR = mask;
	}
#endif

#if defined(CONFIG_SOC_SERIES_SAM3X)
	/* Setup debounce. */
	if (flags & GPIO_INT_DEBOUNCE) {
		pio->PIO_DIFSR = mask;
	} else {
		pio->PIO_SCIFSR = mask;
	}
#elif defined(CONFIG_SOC_SERIES_SAM4S) || defined(CONFIG_SOC_SERIES_SAME70)
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

static int gpio_sam_config(struct device *dev, int access_op, u32_t pin,
			   int flags)
{
	int ret;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		ret = gpio_sam_port_configure(dev, BIT(pin), flags);
		break;
	case GPIO_ACCESS_BY_PORT:
		ret = gpio_sam_port_configure(dev, GPIO_SAM_ALL_PINS, flags);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int gpio_sam_write(struct device *dev, int access_op, u32_t pin,
			  u32_t value)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;
	u32_t mask = 1 << pin;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		if (value) {
			/* Set the pin. */
			pio->PIO_SODR = mask;
		} else {
			/* Clear the pin. */
			pio->PIO_CODR = mask;
		}
		break;
	case GPIO_ACCESS_BY_PORT:
		pio->PIO_ODSR = value;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_sam_read(struct device *dev, int access_op, u32_t pin,
			 u32_t *value)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	*value = pio->PIO_PDSR;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		*value = (*value >> pin) & 1;
		break;
	case GPIO_ACCESS_BY_PORT:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_sam_port_get_raw(struct device *dev, u32_t *value)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	*value = pio->PIO_PDSR;

	return 0;
}

static int gpio_sam_port_set_masked_raw(struct device *dev, u32_t mask,
					u32_t value)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	pio->PIO_ODSR = (pio->PIO_ODSR & ~mask) | (mask & value);

	return 0;
}

static int gpio_sam_port_set_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	/* Set pins. */
	pio->PIO_SODR = mask;

	return 0;
}

static int gpio_sam_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	/* Clear pins. */
	pio->PIO_CODR = mask;

	return 0;
}

static int gpio_sam_port_toggle_bits(struct device *dev, u32_t mask)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;

	/* Toggle pins. */
	pio->PIO_ODSR ^= mask;

	return 0;
}

static int gpio_sam_port_interrupt_configure(struct device *dev, u32_t mask,
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

		u32_t rising_edge;

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

static int gpio_sam_pin_interrupt_configure(struct device *dev,
		unsigned int pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig)
{
	return gpio_sam_port_interrupt_configure(dev, BIT(pin), mode, trig);
}

static void gpio_sam_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio * const pio = cfg->regs;
	struct gpio_sam_runtime *context = dev->driver_data;
	u32_t int_stat;

	int_stat = pio->PIO_ISR;

	gpio_fire_callbacks(&context->cb, dev, int_stat);
}

static int gpio_sam_manage_callback(struct device *port,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_sam_runtime *context = port->driver_data;

	return gpio_manage_callback(&context->cb, callback, set);
}

static int gpio_sam_enable_callback(struct device *port,
				    int access_op, u32_t pin)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(port);
	Pio * const pio = cfg->regs;
	u32_t mask;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		mask = BIT(pin);
		break;
	case GPIO_ACCESS_BY_PORT:
		mask = 0xFFFFFFFF;
		break;
	default:
		return -ENOTSUP;
	}

	pio->PIO_IER |= mask;

	return 0;
}

static int gpio_sam_disable_callback(struct device *port,
				     int access_op, u32_t pin)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(port);
	Pio * const pio = cfg->regs;
	u32_t mask;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		mask = BIT(pin);
		break;
	case GPIO_ACCESS_BY_PORT:
		mask = 0xFFFFFFFF;
		break;
	default:
		return -ENOTSUP;
	}

	pio->PIO_IDR |= mask;

	return 0;
}

static const struct gpio_driver_api gpio_sam_api = {
	.config = gpio_sam_config,
	.write = gpio_sam_write,
	.read = gpio_sam_read,
	.port_get_raw = gpio_sam_port_get_raw,
	.port_set_masked_raw = gpio_sam_port_set_masked_raw,
	.port_set_bits_raw = gpio_sam_port_set_bits_raw,
	.port_clear_bits_raw = gpio_sam_port_clear_bits_raw,
	.port_toggle_bits = gpio_sam_port_toggle_bits,
	.pin_interrupt_configure = gpio_sam_pin_interrupt_configure,
	.manage_callback = gpio_sam_manage_callback,
	.enable_callback = gpio_sam_enable_callback,
	.disable_callback = gpio_sam_disable_callback,
};

int gpio_sam_init(struct device *dev)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);

	/* The peripheral clock must be enabled for the interrupts to work. */
	soc_pmc_peripheral_enable(cfg->periph_id);

	cfg->config_func(dev);

	return 0;
}

/* PORT A */
#ifdef DT_GPIO_SAM_PORTA_BASE_ADDRESS
static void port_a_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_a_sam_config = {
	.regs = (Pio *)DT_GPIO_SAM_PORTA_BASE_ADDRESS,
	.periph_id = DT_GPIO_SAM_PORTA_PERIPHERAL_ID,
	.config_func = port_a_sam_config_func,
};

static struct gpio_sam_runtime port_a_sam_runtime;

DEVICE_AND_API_INIT(port_a_sam, DT_GPIO_SAM_PORTA_LABEL, gpio_sam_init,
		    &port_a_sam_runtime, &port_a_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_a_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_SAM_PORTA_IRQ, DT_GPIO_SAM_PORTA_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_a_sam), 0);
	irq_enable(DT_GPIO_SAM_PORTA_IRQ);
}

#endif /* DT_GPIO_SAM_PORTA_BASE_ADDRESS */

/* PORT B */
#ifdef DT_GPIO_SAM_PORTB_BASE_ADDRESS
static void port_b_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_b_sam_config = {
	.regs = (Pio *)DT_GPIO_SAM_PORTB_BASE_ADDRESS,
	.periph_id = DT_GPIO_SAM_PORTB_PERIPHERAL_ID,
	.config_func = port_b_sam_config_func,
};

static struct gpio_sam_runtime port_b_sam_runtime;

DEVICE_AND_API_INIT(port_b_sam, DT_GPIO_SAM_PORTB_LABEL, gpio_sam_init,
		    &port_b_sam_runtime, &port_b_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_b_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_SAM_PORTB_IRQ, DT_GPIO_SAM_PORTB_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_b_sam), 0);
	irq_enable(DT_GPIO_SAM_PORTB_IRQ);
}

#endif /* DT_GPIO_SAM_PORTB_BASE_ADDRESS */

/* PORT C */
#ifdef DT_GPIO_SAM_PORTC_BASE_ADDRESS
static void port_c_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_c_sam_config = {
	.regs = (Pio *)DT_GPIO_SAM_PORTC_BASE_ADDRESS,
	.periph_id = DT_GPIO_SAM_PORTC_PERIPHERAL_ID,
	.config_func = port_c_sam_config_func,
};

static struct gpio_sam_runtime port_c_sam_runtime;

DEVICE_AND_API_INIT(port_c_sam, DT_GPIO_SAM_PORTC_LABEL, gpio_sam_init,
		    &port_c_sam_runtime, &port_c_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_c_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_SAM_PORTC_IRQ, DT_GPIO_SAM_PORTC_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_c_sam), 0);
	irq_enable(DT_GPIO_SAM_PORTC_IRQ);
}

#endif /* DT_GPIO_SAM_PORTC_BASE_ADDRESS */

/* PORT D */
#ifdef DT_GPIO_SAM_PORTD_BASE_ADDRESS
static void port_d_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_d_sam_config = {
	.regs = (Pio *)DT_GPIO_SAM_PORTD_BASE_ADDRESS,
	.periph_id = DT_GPIO_SAM_PORTD_PERIPHERAL_ID,
	.config_func = port_d_sam_config_func,
};

static struct gpio_sam_runtime port_d_sam_runtime;

DEVICE_AND_API_INIT(port_d_sam, DT_GPIO_SAM_PORTD_LABEL, gpio_sam_init,
		    &port_d_sam_runtime, &port_d_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_d_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_SAM_PORTD_IRQ, DT_GPIO_SAM_PORTD_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_d_sam), 0);
	irq_enable(DT_GPIO_SAM_PORTD_IRQ);
}

#endif /* DT_GPIO_SAM_PORTD_BASE_ADDRESS */

/* PORT E */
#ifdef DT_GPIO_SAM_PORTE_BASE_ADDRESS
static void port_e_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_e_sam_config = {
	.regs = (Pio *)DT_GPIO_SAM_PORTE_BASE_ADDRESS,
	.periph_id = DT_GPIO_SAM_PORTE_PERIPHERAL_ID,
	.config_func = port_e_sam_config_func,
};

static struct gpio_sam_runtime port_e_sam_runtime;

DEVICE_AND_API_INIT(port_e_sam, DT_GPIO_SAM_PORTE_LABEL, gpio_sam_init,
		    &port_e_sam_runtime, &port_e_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_e_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_SAM_PORTE_IRQ, DT_GPIO_SAM_PORTE_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_e_sam), 0);
	irq_enable(DT_GPIO_SAM_PORTE_IRQ);
}

#endif /* DT_GPIO_SAM_PORTE_BASE_ADDRESS */
