/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <interrupt_controller/sam0_eic.h>

#include "gpio_utils.h"

struct gpio_sam0_config {
	PortGroup *regs;
#ifdef CONFIG_SAM0_EIC
	u8_t id;
#endif
};

struct gpio_sam0_data {
#ifdef CONFIG_SAM0_EIC
	sys_slist_t callbacks;
#endif
};

#define DEV_CFG(dev) \
	((const struct gpio_sam0_config *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct gpio_sam0_data *const)(dev)->driver_data)

#ifdef CONFIG_SAM0_EIC
static void gpio_sam0_isr(u32_t pins, void *arg)
{
	struct device *const dev = (struct device *) arg;
	struct gpio_sam0_data *const data = DEV_DATA(dev);

	gpio_fire_callbacks(&data->callbacks, dev, pins);
}
#endif

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

#ifdef CONFIG_SAM0_EIC
	if ((flags & GPIO_INT) != 0) {
		/*
		 * The EIC requires setting the pin to function A, so do
		 * that as part of the interrupt enable, so that the API
		 * is consistent.
		 */
		pincfg.bit.PMUXEN = 1;
		if (pin & 1) {
			regs->PMUX[pin / 2].bit.PMUXO = PORT_PMUX_PMUXE_A_Val;
		} else {
			regs->PMUX[pin / 2].bit.PMUXE = PORT_PMUX_PMUXE_A_Val;
		}

		enum sam0_eic_trigger trigger;

		if ((flags & GPIO_INT_DOUBLE_EDGE) != 0) {
			trigger = SAM0_EIC_BOTH;
		} else if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				trigger = SAM0_EIC_RISING;
			} else {
				trigger = SAM0_EIC_FALLING;
			}
		} else {
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				trigger = SAM0_EIC_HIGH;
			} else {
				trigger = SAM0_EIC_LOW;
			}
		}

		int retval = sam0_eic_acquire(config->id, pin, trigger,
					      (flags & GPIO_INT_DEBOUNCE) != 0,
					      gpio_sam0_isr, dev);
		if (retval != 0) {
			return retval;
		}
	} else {
		sam0_eic_release(config->id, pin);
		/*
		 * Pinmux disabled by the config set, so this
		 * correctly inverts the EIC enable part above.
		 */
	}
#else
	if ((flags & GPIO_INT) != 0) {
		return -ENOTSUP;
	}
#endif

	/* Write the now-built pin configuration */
	regs->PINCFG[pin] = pincfg;

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

	if (value != 0U) {
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

#ifdef CONFIG_SAM0_EIC

int gpio_sam0_manage_callback(struct device *dev,
			      struct gpio_callback *callback, bool set)
{
	struct gpio_sam0_data *const data = DEV_DATA(dev);

	return gpio_manage_callback(&data->callbacks, callback, set);
}

int gpio_sam0_enable_callback(struct device *dev, int access_op, u32_t pin)
{
	const struct gpio_sam0_config *config = DEV_CFG(dev);

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	return sam0_eic_enable_interrupt(config->id, pin);
}

int gpio_sam0_disable_callback(struct device *dev, int access_op, u32_t pin)
{
	const struct gpio_sam0_config *config = DEV_CFG(dev);

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	return sam0_eic_disable_interrupt(config->id, pin);
}

u32_t gpio_sam0_get_pending_int(struct device *dev)
{
	const struct gpio_sam0_config *config = DEV_CFG(dev);

	return sam0_eic_interrupt_pending(config->id);
}

#endif

static const struct gpio_driver_api gpio_sam0_api = {
	.config = gpio_sam0_config,
	.write = gpio_sam0_write,
	.read = gpio_sam0_read,
#ifdef CONFIG_SAM0_EIC
	.manage_callback = gpio_sam0_manage_callback,
	.enable_callback = gpio_sam0_enable_callback,
	.disable_callback = gpio_sam0_disable_callback,
	.get_pending_int = gpio_sam0_get_pending_int,
#endif
};

static int gpio_sam0_init(struct device *dev) { return 0; }

/* Port A */
#if DT_ATMEL_SAM0_GPIO_PORT_A_BASE_ADDRESS

static const struct gpio_sam0_config gpio_sam0_config_0 = {
	.regs = (PortGroup *)DT_ATMEL_SAM0_GPIO_PORT_A_BASE_ADDRESS,
#ifdef CONFIG_SAM0_EIC
	.id = 0,
#endif
};

static struct gpio_sam0_data gpio_sam0_data_0;

DEVICE_AND_API_INIT(gpio_sam0_0, DT_ATMEL_SAM0_GPIO_PORT_A_LABEL,
		    gpio_sam0_init, &gpio_sam0_data_0, &gpio_sam0_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_sam0_api);
#endif

/* Port B */
#if DT_ATMEL_SAM0_GPIO_PORT_B_BASE_ADDRESS

static const struct gpio_sam0_config gpio_sam0_config_1 = {
	.regs = (PortGroup *)DT_ATMEL_SAM0_GPIO_PORT_B_BASE_ADDRESS,
#ifdef CONFIG_SAM0_EIC
	.id = 1,
#endif
};

static struct gpio_sam0_data gpio_sam0_data_1;

DEVICE_AND_API_INIT(gpio_sam0_1, DT_ATMEL_SAM0_GPIO_PORT_B_LABEL,
		    gpio_sam0_init, &gpio_sam0_data_1, &gpio_sam0_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_sam0_api);
#endif

/* Port C */
#if DT_ATMEL_SAM0_GPIO_PORT_C_BASE_ADDRESS

static const struct gpio_sam0_config gpio_sam0_config_2 = {
	.regs = (PortGroup *)DT_ATMEL_SAM0_GPIO_PORT_C_BASE_ADDRESS,
#ifdef CONFIG_SAM0_EIC
	.id = 2,
#endif
};

static struct gpio_sam0_data gpio_sam0_data_2;

DEVICE_AND_API_INIT(gpio_sam0_2, DT_ATMEL_SAM0_GPIO_PORT_C_LABEL,
		    gpio_sam0_init, &gpio_sam0_data_2, &gpio_sam0_config_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_sam0_api);
#endif
