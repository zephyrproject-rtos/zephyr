/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for the Atmel SAM3 PIO Controller.
 */

#include <errno.h>

#include <kernel.h>

#include <device.h>
#include <init.h>

#include <soc.h>

#include <gpio.h>
#include "gpio_utils.h"

typedef void (*config_func_t)(struct device *dev);

/* Configuration data */
struct gpio_sam3_config {
	Pio *port;
	config_func_t		config_func;
};

struct gpio_sam3_runtime {
	/* callbacks */
	sys_slist_t		cb;
};

static void _config(struct device *dev, u32_t mask, int flags)
{
	const struct gpio_sam3_config *cfg = dev->config->config_info;

	/* Setup the pin direction */
	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_OUT) {
		cfg->port->PIO_OER = mask;
	} else {
		cfg->port->PIO_ODR = mask;
	}

	/* Setup interrupt config */
	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_DOUBLE_EDGE) {
			cfg->port->PIO_AIMDR = mask;
		} else {
			cfg->port->PIO_AIMER = mask;

			if (flags & GPIO_INT_EDGE) {
				cfg->port->PIO_ESR = mask;
			} else {
				cfg->port->PIO_LSR = mask;
			}

			if (flags & GPIO_INT_ACTIVE_HIGH) {
				/* Trigger in high level or rising edge */
				cfg->port->PIO_REHLSR = mask;
			} else {
				/* Trigger in low level or falling edge */
				cfg->port->PIO_FELLSR = mask;
			}
		}
	}

	/* Pull-up? */
	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
		/* Enable pull-up */
		cfg->port->PIO_PUER = mask;
	} else {
		/* Disable pull-up */
		cfg->port->PIO_PUDR = mask;
	}

	/* Debounce */
	if (flags & GPIO_INT_DEBOUNCE) {
		cfg->port->PIO_DIFSR = mask;
	} else {
		cfg->port->PIO_SCIFSR = mask;
	}

	cfg->port->PIO_PER = mask;
}

/**
 * @brief Configure pin or port
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_sam3_config(struct device *dev, int access_op,
			    u32_t pin, int flags)
{
	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		_config(dev, BIT(pin), flags);
		break;
	case GPIO_ACCESS_BY_PORT:
		_config(dev, (0xFFFFFFFF), flags);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Set the pin or port output
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value to set (0 or 1)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_sam3_write(struct device *dev, int access_op,
			   u32_t pin, u32_t value)
{
	const struct gpio_sam3_config *cfg = dev->config->config_info;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		if (value) {
			/* set the pin */
			cfg->port->PIO_SODR = BIT(pin);
		} else {
			/* clear the pin */
			cfg->port->PIO_CODR = BIT(pin);
		}
		break;
	case GPIO_ACCESS_BY_PORT:
		if (value) {
			/* set all pins */
			cfg->port->PIO_SODR = 0xFFFFFFFF;
		} else {
			/* clear all pins */
			cfg->port->PIO_CODR = 0xFFFFFFFF;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Read the pin or port status
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value of input pin(s)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_sam3_read(struct device *dev, int access_op,
				       u32_t pin, u32_t *value)
{
	const struct gpio_sam3_config *cfg = dev->config->config_info;

	*value = cfg->port->PIO_PDSR;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		*value = (*value >> pin) & 0x01;
		break;
	case GPIO_ACCESS_BY_PORT:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void gpio_sam3_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_sam3_config *cfg = dev->config->config_info;
	struct gpio_sam3_runtime *context = dev->driver_data;
	u32_t int_stat;

	int_stat = cfg->port->PIO_ISR;

	_gpio_fire_callbacks(&context->cb, dev, int_stat);
}

static int gpio_sam3_manage_callback(struct device *dev,
				     struct gpio_callback *callback,
				     bool set)
{
	struct gpio_sam3_runtime *context = dev->driver_data;

	_gpio_manage_callback(&context->cb, callback, set);

	return 0;
}

static int gpio_sam3_enable_callback(struct device *dev,
				     int access_op, u32_t pin)
{
	const struct gpio_sam3_config *cfg = dev->config->config_info;
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

	cfg->port->PIO_IER |= mask;

	return 0;
}

static int gpio_sam3_disable_callback(struct device *dev,
				      int access_op, u32_t pin)
{
	const struct gpio_sam3_config *cfg = dev->config->config_info;
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

	cfg->port->PIO_IDR |= mask;

	return 0;
}

static const struct gpio_driver_api gpio_sam3_drv_api_funcs = {
	.config = gpio_sam3_config,
	.write = gpio_sam3_write,
	.read = gpio_sam3_read,
	.manage_callback = gpio_sam3_manage_callback,
	.enable_callback = gpio_sam3_enable_callback,
	.disable_callback = gpio_sam3_disable_callback,
};

/**
 * @brief Initialization function of MMIO
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_sam3_init(struct device *dev)
{
	const struct gpio_sam3_config *cfg = dev->config->config_info;

	cfg->config_func(dev);

	return 0;
}

/* Port A */
#ifdef CONFIG_GPIO_ATMEL_SAM3_PORTA
static void gpio_sam3_config_a(struct device *dev);

static const struct gpio_sam3_config gpio_sam3_a_cfg = {
	.port = PIOA,

	.config_func = gpio_sam3_config_a,
};

static struct gpio_sam3_runtime gpio_sam3_a_runtime;

DEVICE_AND_API_INIT(gpio_sam3_a, CONFIG_GPIO_ATMEL_SAM3_PORTA_DEV_NAME,
		    gpio_sam3_init, &gpio_sam3_a_runtime, &gpio_sam3_a_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_sam3_drv_api_funcs);

static void gpio_sam3_config_a(struct device *dev)
{
	/* Enable clock for PIO controller */
	PMC->PMC_PCER0 = BIT(ID_PIOA);

	IRQ_CONNECT(PIOA_IRQn, CONFIG_GPIO_ATMEL_SAM3_PORTA_IRQ_PRI,
		    gpio_sam3_isr, DEVICE_GET(gpio_sam3_a), 0);
	irq_enable(PIOA_IRQn);
}
#endif /* CONFIG_GPIO_ATMEL_SAM3_PORTA */

/* Port B */
#ifdef CONFIG_GPIO_ATMEL_SAM3_PORTB
static void gpio_sam3_config_b(struct device *dev);

static const struct gpio_sam3_config gpio_sam3_b_cfg = {
	.port = PIOB,

	.config_func = gpio_sam3_config_b,
};

static struct gpio_sam3_runtime gpio_sam3_b_runtime;

DEVICE_AND_API_INIT(gpio_sam3_b, CONFIG_GPIO_ATMEL_SAM3_PORTB_DEV_NAME,
		    gpio_sam3_init, &gpio_sam3_b_runtime, &gpio_sam3_b_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_sam3_drv_api_funcs);

static void gpio_sam3_config_b(struct device *dev)
{
	/* Enable clock for PIO controller */
	PMC->PMC_PCER0 = BIT(ID_PIOB);

	IRQ_CONNECT(PIOB_IRQn, CONFIG_GPIO_ATMEL_SAM3_PORTB_IRQ_PRI,
		    gpio_sam3_isr, DEVICE_GET(gpio_sam3_b), 0);
	irq_enable(PIOB_IRQn);
}
#endif /* CONFIG_GPIO_ATMEL_SAM3_PORTB */

/* Port C */
#ifdef CONFIG_GPIO_ATMEL_SAM3_PORTC
static void gpio_sam3_config_c(struct device *dev);

static const struct gpio_sam3_config gpio_sam3_c_cfg = {
	.port = PIOC,

	.config_func = gpio_sam3_config_c,
};

static struct gpio_sam3_runtime gpio_sam3_c_runtime;

DEVICE_AND_API_INIT(gpio_sam3_c, CONFIG_GPIO_ATMEL_SAM3_PORTC_DEV_NAME,
		    gpio_sam3_init, &gpio_sam3_c_runtime, &gpio_sam3_c_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_sam3_drv_api_funcs);

static void gpio_sam3_config_c(struct device *dev)
{
	/* Enable clock for PIO controller */
	PMC->PMC_PCER0 = BIT(ID_PIOC);

	IRQ_CONNECT(PIOC_IRQn, CONFIG_GPIO_ATMEL_SAM3_PORTC_IRQ_PRI,
		    gpio_sam3_isr, DEVICE_GET(gpio_sam3_c), 0);
	irq_enable(PIOC_IRQn);
}
#endif /* CONFIG_GPIO_ATMEL_SAM3_PORTA */

/* Port D */
#ifdef CONFIG_GPIO_ATMEL_SAM3_PORTD
static void gpio_sam3_config_d(struct device *dev);

static const struct gpio_sam3_config gpio_sam3_d_cfg = {
	.port = PIOD,

	.config_func = gpio_sam3_config_d,
};

static struct gpio_sam3_runtime gpio_sam3_d_runtime;

DEVICE_AND_API_INIT(gpio_sam3_d, CONFIG_GPIO_ATMEL_SAM3_PORTD_DEV_NAME,
		    gpio_sam3_init, &gpio_sam3_d_runtime, &gpio_sam3_d_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_sam3_drv_api_funcs);

static void gpio_sam3_config_d(struct device *dev)
{
	/* Enable clock for PIO controller */
	PMC->PMC_PCER0 = BIT(ID_PIOD);

	IRQ_CONNECT(PIOD_IRQn, CONFIG_GPIO_ATMEL_SAM3_PORTD_IRQ_PRI,
		    gpio_sam3_isr, DEVICE_GET(gpio_sam3_d), 0);
	irq_enable(PIOD_IRQn);
}
#endif /* CONFIG_GPIO_ATMEL_SAM3_PORTD */
