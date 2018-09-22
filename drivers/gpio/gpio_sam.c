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
#include <gpio.h>

#include "gpio_utils.h"

typedef void (*config_func_t)(struct device *dev);

struct gpio_sam_config {
	Pio *regs;
	config_func_t config_func;
	u32_t periph_id;
};

struct gpio_sam_runtime {
	sys_slist_t cb;
};

#define DEV_CFG(dev) \
	((const struct gpio_sam_config *const)(dev)->config->config_info)

static int gpio_sam_config(struct device *dev, int access_op, u32_t pin,
			   int flags)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio *const pio = cfg->regs;
	u32_t mask = 1 << pin;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	/* Setup the pin direcion. */
	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_OUT) {
		pio->PIO_OER = mask;
	} else {
		pio->PIO_ODR = mask;
	}

	/* Setup interrupt configuration. */
	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_DOUBLE_EDGE) {
			return -ENOTSUP;
		}

		/* Enable the interrupt. */
		pio->PIO_IER = mask;

		/* Enable the additional interrupt modes. */
		pio->PIO_AIMER = mask;

		if (flags & GPIO_INT_EDGE) {
			pio->PIO_ESR = mask;
		} else {
			pio->PIO_LSR = mask;
		}

		if (flags & GPIO_INT_ACTIVE_HIGH) {
			/* Set to high-level or rising edge. */
			pio->PIO_REHLSR = mask;
		} else {
			/* Set to low-level or falling edge. */
			pio->PIO_FELLSR = mask;
		}
	} else {
		/* Disable the interrupt. */
		pio->PIO_IDR = mask;

		/* Disable ther additional interrupt modes. */
		pio->PIO_AIMDR = mask;
	}

	/* Setup Pull-up resistor. */
	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
		/* Enable pull-up. */
		pio->PIO_PUER = mask;
	} else {
		pio->PIO_PUDR = mask;
	}

	/* Setup Pull-down resistor. */
	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN) {
		pio->PIO_PPDER = mask;
	} else {
		pio->PIO_PPDDR = mask;
	}

#if defined(SOC_SERIES_SAM3X)
	/* Setup debounce. */
	if (flags & GPIO_INT_DEBOUNCE) {
		pio->PIO_DIFSR = mask;
	} else {
		pio->PIO_SCIFSR = mask;
	}
#elif defined(SOC_SERIES_SAM4S) || defined(SOC_SERIES_SAME70)
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

static int gpio_sam_write(struct device *dev, int access_op, u32_t pin,
			  u32_t value)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio *const pio = cfg->regs;
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
		if (value) {
			/* Set all pins. */
			pio->PIO_SODR = 0xffffffff;
		} else {
			/* Clear all pins. */
			pio->PIO_CODR = 0xffffffff;
		}
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
	Pio *const pio = cfg->regs;

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

static void gpio_sam_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_sam_config * const cfg = DEV_CFG(dev);
	Pio *const pio = cfg->regs;
	struct gpio_sam_runtime *context = dev->driver_data;
	u32_t int_stat;

	int_stat = pio->PIO_ISR;

	_gpio_fire_callbacks(&context->cb, dev, int_stat);
}

static int gpio_sam_manage_callback(struct device *port,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_sam_runtime *context = port->driver_data;

	_gpio_manage_callback(&context->cb, callback, set);

	return 0;
}

static int gpio_sam_enable_callback(struct device *port,
				    int access_op, u32_t pin)
{
	const struct gpio_sam_config * const cfg = DEV_CFG(port);
	Pio *const pio = cfg->regs;
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
	Pio *const pio = cfg->regs;
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
#ifdef CONFIG_GPIO_SAM_PORTA_BASE_ADDRESS
static void port_a_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_a_sam_config = {
	.regs = (Pio *)CONFIG_GPIO_SAM_PORTA_BASE_ADDRESS,
	.periph_id = CONFIG_GPIO_SAM_PORTA_PERIPHERAL_ID,
	.config_func = port_a_sam_config_func,
};

static struct gpio_sam_runtime port_a_sam_runtime;

DEVICE_AND_API_INIT(port_a_sam, CONFIG_GPIO_SAM_PORTA_LABEL, gpio_sam_init,
		    &port_a_sam_runtime, &port_a_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_a_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(CONFIG_GPIO_SAM_PORTA_IRQ, CONFIG_GPIO_SAM_PORTA_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_a_sam), 0);
	irq_enable(CONFIG_GPIO_SAM_PORTA_IRQ);
}

#endif /* CONFIG_GPIO_SAM_PORTA_BASE_ADDRESS */

/* PORT B */
#ifdef CONFIG_GPIO_SAM_PORTB_BASE_ADDRESS
static void port_b_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_b_sam_config = {
	.regs = (Pio *)CONFIG_GPIO_SAM_PORTB_BASE_ADDRESS,
	.periph_id = CONFIG_GPIO_SAM_PORTB_PERIPHERAL_ID,
	.config_func = port_b_sam_config_func,
};

static struct gpio_sam_runtime port_b_sam_runtime;

DEVICE_AND_API_INIT(port_b_sam, CONFIG_GPIO_SAM_PORTB_LABEL, gpio_sam_init,
		    &port_b_sam_runtime, &port_b_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_b_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(CONFIG_GPIO_SAM_PORTB_IRQ, CONFIG_GPIO_SAM_PORTB_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_b_sam), 0);
	irq_enable(CONFIG_GPIO_SAM_PORTB_IRQ);
}

#endif /* CONFIG_GPIO_SAM_PORTB_BASE_ADDRESS */

/* PORT C */
#ifdef CONFIG_GPIO_SAM_PORTC_BASE_ADDRESS
static void port_c_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_c_sam_config = {
	.regs = (Pio *)CONFIG_GPIO_SAM_PORTC_BASE_ADDRESS,
	.periph_id = CONFIG_GPIO_SAM_PORTC_PERIPHERAL_ID,
	.config_func = port_c_sam_config_func,
};

static struct gpio_sam_runtime port_c_sam_runtime;

DEVICE_AND_API_INIT(port_c_sam, CONFIG_GPIO_SAM_PORTC_LABEL, gpio_sam_init,
		    &port_c_sam_runtime, &port_c_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_c_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(CONFIG_GPIO_SAM_PORTC_IRQ, CONFIG_GPIO_SAM_PORTC_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_c_sam), 0);
	irq_enable(CONFIG_GPIO_SAM_PORTC_IRQ);
}

#endif /* CONFIG_GPIO_SAM_PORTC_BASE_ADDRESS */

/* PORT D */
#ifdef CONFIG_GPIO_SAM_PORTD_BASE_ADDRESS
static void port_d_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_d_sam_config = {
	.regs = (Pio *)CONFIG_GPIO_SAM_PORTD_BASE_ADDRESS,
	.periph_id = CONFIG_GPIO_SAM_PORTD_PERIPHERAL_ID,
	.config_func = port_d_sam_config_func,
};

static struct gpio_sam_runtime port_d_sam_runtime;

DEVICE_AND_API_INIT(port_d_sam, CONFIG_GPIO_SAM_PORTD_LABEL, gpio_sam_init,
		    &port_d_sam_runtime, &port_d_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_d_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(CONFIG_GPIO_SAM_PORTD_IRQ, CONFIG_GPIO_SAM_PORTD_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_d_sam), 0);
	irq_enable(CONFIG_GPIO_SAM_PORTD_IRQ);
}

#endif /* CONFIG_GPIO_SAM_PORTD_BASE_ADDRESS */

/* PORT E */
#ifdef CONFIG_GPIO_SAM_PORTE_BASE_ADDRESS
static void port_e_sam_config_func(struct device *dev);

static const struct gpio_sam_config port_e_sam_config = {
	.regs = (Pio *)CONFIG_GPIO_SAM_PORTE_BASE_ADDRESS,
	.periph_id = CONFIG_GPIO_SAM_PORTE_PERIPHERAL_ID,
	.config_func = port_e_sam_config_func,
};

static struct gpio_sam_runtime port_e_sam_runtime;

DEVICE_AND_API_INIT(port_e_sam, CONFIG_GPIO_SAM_PORTE_LABEL, gpio_sam_init,
		    &port_e_sam_runtime, &port_e_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam_api);

static void port_e_sam_config_func(struct device *dev)
{
	IRQ_CONNECT(CONFIG_GPIO_SAM_PORTE_IRQ, CONFIG_GPIO_SAM_PORTE_IRQ_PRIO,
		    gpio_sam_isr, DEVICE_GET(port_e_sam), 0);
	irq_enable(CONFIG_GPIO_SAM_PORTE_IRQ);
}

#endif /* CONFIG_GPIO_SAM_PORTE_BASE_ADDRESS */
