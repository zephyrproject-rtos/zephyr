/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <gpio.h>
#include <soc.h>

struct gpio_sam0_config {
	PortGroup *regs;
};

#define DEV_CFG(dev) \
	((const struct gpio_sam0_config *const)(dev)->config->config_info)

static int gpio_sam0_config(struct device *dev, int access_op, u32_t pin,
			    int flags)
{
	const struct gpio_sam0_config *config = DEV_CFG(dev);
	PortGroup *regs = config->regs;
	u32_t mask = 1 << pin;
	bool is_out = (flags & GPIO_DIR_MASK) == GPIO_DIR_OUT;
	int pud = flags & GPIO_PUD_MASK;
	PORT_PINCFG_Type pincfg;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	/* Builds the configuration and writes it in one go */
	pincfg.reg = 0;
	pincfg.bit.INEN = 1;

	/* Direction */
	if (is_out) {
		regs->DIRSET.bit.DIRSET = mask;
	} else {
		regs->DIRCLR.bit.DIRCLR = mask;
	}

	/* Pull up / pull down */
	if (is_out && pud != GPIO_PUD_NORMAL) {
		return -ENOTSUP;
	}

	switch (pud) {
	case GPIO_PUD_NORMAL:
		break;
	case GPIO_PUD_PULL_UP:
		pincfg.bit.PULLEN = 1;
		regs->OUTSET.reg = mask;
		break;
	case GPIO_PUD_PULL_DOWN:
		pincfg.bit.PULLEN = 1;
		regs->OUTCLR.reg = mask;
		break;
	default:
		return -ENOTSUP;
	}

	/* Write the now-built pin configuration */
	regs->PINCFG[pin] = pincfg;

	if ((flags & GPIO_INT) != 0) {
		/* TODO(mlhx): implement. */
		return -ENOTSUP;
	}

	if ((flags & GPIO_POL_MASK) != GPIO_POL_NORMAL) {
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_sam0_write(struct device *dev, int access_op, u32_t pin,
			   u32_t value)
{
	const struct gpio_sam0_config *config = DEV_CFG(dev);
	u32_t mask = 1 << pin;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		/* TODO(mlhx): support GPIO_ACCESS_BY_PORT */
		return -ENOTSUP;
	}

	if (value != 0) {
		config->regs->OUTSET.bit.OUTSET = mask;
	} else {
		config->regs->OUTCLR.bit.OUTCLR = mask;
	}

	return 0;
}

static int gpio_sam0_read(struct device *dev, int access_op, u32_t pin,
			  u32_t *value)
{
	const struct gpio_sam0_config *config = DEV_CFG(dev);
	u32_t bits;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		/* TODO(mlhx): support GPIO_ACCESS_BY_PORT */
		return -ENOTSUP;
	}

	bits = config->regs->IN.bit.IN;
	*value = (bits >> pin) & 1;

	return 0;
}

static const struct gpio_driver_api gpio_sam0_api = {
	.config = gpio_sam0_config,
	.write = gpio_sam0_write,
	.read = gpio_sam0_read,
};

static int gpio_sam0_init(struct device *dev) { return 0; }

/* Port A */
#ifdef CONFIG_GPIO_SAM0_PORTA_BASE_ADDRESS

static const struct gpio_sam0_config gpio_sam0_config_0 = {
	.regs = (PortGroup *)CONFIG_GPIO_SAM0_PORTA_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(gpio_sam0_0, CONFIG_GPIO_SAM0_PORTA_LABEL, gpio_sam0_init,
		    NULL, &gpio_sam0_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam0_api);
#endif

/* Port B */
#ifdef CONFIG_GPIO_SAM0_PORTB_BASE_ADDRESS

static const struct gpio_sam0_config gpio_sam0_config_1 = {
	.regs = (PortGroup *)CONFIG_GPIO_SAM0_PORTB_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(gpio_sam0_1, CONFIG_GPIO_SAM0_PORTB_LABEL, gpio_sam0_init,
		    NULL, &gpio_sam0_config_1, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam0_api);
#endif
