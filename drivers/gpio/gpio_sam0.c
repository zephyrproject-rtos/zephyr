/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2018 Madani Lainani
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <gpio.h>
#include <soc.h>

/* External Interrupt Controller support */
#if CONFIG_EIC_SAM0

#include "gpio_utils.h"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>
#include <pinmux.h>

static bool eic_peripheral_is_enabled;

struct extint {
	u32_t pin;
	u8_t num; /* external interrupt number */
};

#ifdef DT_GPIO_SAM0_PORTA_BASE_ADDRESS

DEVICE_DECLARE(gpio_sam0_0);

#define GPIO_SAM0_PORTA_EXTINT(n) \
	{ CONFIG_GPIO_SAM0_PORTA_EXTINT_##n##_PIN, n },

static struct extint porta_extints[] = {
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_0
	GPIO_SAM0_PORTA_EXTINT(0)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_1
	GPIO_SAM0_PORTA_EXTINT(1)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_2
	GPIO_SAM0_PORTA_EXTINT(2)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_3
	GPIO_SAM0_PORTA_EXTINT(3)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_4
	GPIO_SAM0_PORTA_EXTINT(4)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_5
	GPIO_SAM0_PORTA_EXTINT(5)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_6
	GPIO_SAM0_PORTA_EXTINT(6)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_7
	GPIO_SAM0_PORTA_EXTINT(7)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_8
	GPIO_SAM0_PORTA_EXTINT(8)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_9
	GPIO_SAM0_PORTA_EXTINT(9)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_10
	GPIO_SAM0_PORTA_EXTINT(10)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_11
	GPIO_SAM0_PORTA_EXTINT(11)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_12
	GPIO_SAM0_PORTA_EXTINT(12)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_13
	GPIO_SAM0_PORTA_EXTINT(13)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_14
	GPIO_SAM0_PORTA_EXTINT(14)
#endif
#if CONFIG_GPIO_SAM0_PORTA_EXTINT_15
	GPIO_SAM0_PORTA_EXTINT(15)
#endif
};

#endif /* DT_GPIO_SAM0_PORTA_BASE_ADDRESS */

#ifdef DT_GPIO_SAM0_PORTB_BASE_ADDRESS

DEVICE_DECLARE(gpio_sam0_1);

#define GPIO_SAM0_PORTB_EXTINT(n) \
	{ CONFIG_GPIO_SAM0_PORTB_EXTINT_##n##_PIN, n },

static struct extint portb_extints[] = {
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_0
	GPIO_SAM0_PORTB_EXTINT(0)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_1
	GPIO_SAM0_PORTB_EXTINT(1)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_2
	GPIO_SAM0_PORTB_EXTINT(2)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_3
	GPIO_SAM0_PORTB_EXTINT(3)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_4
	GPIO_SAM0_PORTB_EXTINT(4)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_5
	GPIO_SAM0_PORTB_EXTINT(5)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_6
	GPIO_SAM0_PORTB_EXTINT(6)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_7
	GPIO_SAM0_PORTB_EXTINT(7)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_8
	GPIO_SAM0_PORTB_EXTINT(8)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_9
	GPIO_SAM0_PORTB_EXTINT(9)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_10
	GPIO_SAM0_PORTB_EXTINT(10)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_11
	GPIO_SAM0_PORTB_EXTINT(11)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_12
	GPIO_SAM0_PORTB_EXTINT(12)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_13
	GPIO_SAM0_PORTB_EXTINT(13)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_14
	GPIO_SAM0_PORTB_EXTINT(14)
#endif
#if CONFIG_GPIO_SAM0_PORTB_EXTINT_15
	GPIO_SAM0_PORTB_EXTINT(15)
#endif
};

#endif /* DT_GPIO_SAM0_PORTB_BASE_ADDRESS */

struct gpio_sam0_config {
	PortGroup *regs;
	/* PORT group I/O pin-to-EXTINT number mapping table */
	struct extint *extints;
	/* Number of EXTINTs configured on PORT group */
	size_t num_extints;
};

struct gpio_sam0_data {
	/* list of registered callbacks */
	sys_slist_t callbacks;
	/* callback enable pin bitmask */
	u32_t pin_callback_enables;
};

#define DEV_DATA(dev) \
	((struct gpio_sam0_data *const)(dev)->driver_data)

#else /* CONFIG_EIC_SAM0 */

struct gpio_sam0_config {
	PortGroup *regs;
};

#endif /* CONFIG_EIC_SAM0 */

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

	if (flags & GPIO_INT) {
#if CONFIG_EIC_SAM0
		int extint_num = -1;
		int idx, pos;

		/* Cheap sanity check */
		if (is_out) {
			SYS_LOG_ERR("interrupt config on output pin %s[%02u]!",
				    dev->config->name, pin);
			return -EINVAL;
		}

		SYS_LOG_DBG("flags 0x%x", flags);

		/*
		 * Look for a matching entry in the interrupt -to- I/O pin
		 * mapping table.
		 */
		for (int i = 0; i < config->num_extints; i++) {
			if (pin == config->extints[i].pin) {
				extint_num = config->extints[i].num;
				SYS_LOG_DBG("%s[%02u] assigned to EXTINT[%u]",
					    dev->config->name, pin,
					    config->extints[i].num);
				break;
			}
		}

		if (extint_num < 0) {
			SYS_LOG_ERR("%s[%02u] not assigned to an EXTINT",
				    dev->config->name, pin);
			return -EINVAL;
		}

		/* Enable peripheral multiplexer selection */
		regs->PINCFG[pin].bit.PMUXEN = 1;

		/* Assign I/O pin to the EIC */
		if (pin & 1) {
			regs->PMUX[pin / 2].bit.PMUXO = PINMUX_FUNC_A;
		} else {
			regs->PMUX[pin / 2].bit.PMUXE = PINMUX_FUNC_A;
		}

		/*
		 * Determine configuration register index and offset for the
		 * external interrupt.
		 */
		if (extint_num > 7) {
			idx = 1;
			pos = (extint_num - 8) << 2;
		} else {
			idx = 0;
			pos = extint_num << 2;
		}

		/* Configure detection mode */
		if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_DOUBLE_EDGE) {
				EIC->CONFIG[idx].reg |=
					EIC_CONFIG_SENSE0_BOTH_Val << pos;
				SYS_LOG_DBG("Edge - Double");
			} else if (flags & GPIO_INT_ACTIVE_HIGH) {
				EIC->CONFIG[idx].reg |=
					EIC_CONFIG_SENSE0_RISE_Val << pos;
				SYS_LOG_DBG("Edge - High");
			} else {
				EIC->CONFIG[idx].reg |=
					EIC_CONFIG_SENSE0_FALL_Val << pos;
				SYS_LOG_DBG("Edge - Low");
			}
		} else {
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				EIC->CONFIG[idx].reg |=
					EIC_CONFIG_SENSE0_HIGH_Val << pos;
				SYS_LOG_DBG("Level - High");
			} else {
				EIC->CONFIG[idx].reg |=
					EIC_CONFIG_SENSE0_LOW_Val << pos;
				SYS_LOG_DBG("Level - Low");
			}
		}

		/* Filter external pin */
		if (flags & GPIO_INT_DEBOUNCE) {
			EIC->CONFIG[idx].reg |=
				BIT(pos + EIC_CONFIG_FILTEN0_Pos);
		}

		/* Enable wake-up from sleep modes */
		EIC->WAKEUP.reg |= BIT(extint_num);

		/* Enable the external interrupt */
		EIC->INTENSET.reg = EIC_INTENSET_EXTINT(BIT(extint_num));
#else /* CONFIG_EIC_SAM0 */
		/* TODO(mlhx): implement. */
		return -ENOTSUP;
#endif /* CONFIG_EIC_SAM0 */
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

#if CONFIG_EIC_SAM0
static int gpio_sam0_manage_callback(struct device *dev,
				     struct gpio_callback *callback,
				     bool set)
{
	struct gpio_sam0_data *data = DEV_DATA(dev);

	_gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static int gpio_sam0_enable_callback(struct device *dev,
				     int access_op,
				     u32_t pin)
{
	struct gpio_sam0_data *data = DEV_DATA(dev);

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	data->pin_callback_enables |= BIT(pin);

	return 0;
}

static int gpio_sam0_disable_callback(struct device *dev,
				      int access_op,
				      u32_t pin)
{
	struct gpio_sam0_data *data = DEV_DATA(dev);

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	data->pin_callback_enables &= ~(BIT(pin));

	return 0;
}

static u32_t gpio_sam0_get_pending_int(struct device *dev)
{
	return 0;
}

static const struct gpio_driver_api gpio_sam0_api = {
	.config = gpio_sam0_config,
	.write = gpio_sam0_write,
	.read = gpio_sam0_read,
	.manage_callback = gpio_sam0_manage_callback,
	.enable_callback = gpio_sam0_enable_callback,
	.disable_callback = gpio_sam0_disable_callback,
	.get_pending_int = gpio_sam0_get_pending_int,
};

static void eic_sam0_isr(void *arg)
{
	int i;
	u32_t pin_mask;
	u32_t clear_mask;
	struct device *dev;
	const struct gpio_sam0_config *config;
	struct gpio_sam0_data *data;
	u32_t enabled_int;

#ifdef DT_GPIO_SAM0_PORTA_BASE_ADDRESS
	pin_mask = clear_mask = 0;
	dev = DEVICE_GET(gpio_sam0_0);
	config = DEV_CFG(dev);
	data = DEV_DATA(dev);

	for (i = 0; i < config->num_extints; i++) {
		if (BIT(config->extints[i].num) & EIC->INTFLAG.reg) {
			pin_mask |= BIT(config->extints[i].pin);
			clear_mask |= BIT(config->extints[i].num);
		}
	}

	enabled_int = pin_mask & data->pin_callback_enables;

	_gpio_fire_callbacks(&data->callbacks, (struct device *)dev,
			     enabled_int);

	/* Clear PORT group A interrupts */
	EIC->INTFLAG.reg = clear_mask;
#endif

#ifdef DT_GPIO_SAM0_PORTB_BASE_ADDRESS
	pin_mask = clear_mask = 0;
	dev = DEVICE_GET(gpio_sam0_1);
	config = DEV_CFG(dev);
	data = DEV_DATA(dev);

	for (i = 0; i < config->num_extints; i++) {
		if (BIT(config->extints[i].num) & EIC->INTFLAG.reg) {
			pin_mask |= BIT(config->extints[i].pin);
			clear_mask |= BIT(config->extints[i].num);
		}
	}

	enabled_int = pin_mask & data->pin_callback_enables;

	_gpio_fire_callbacks(&data->callbacks, (struct device *)dev,
			     enabled_int);

	/* Clear PORT group B interrupts */
	EIC->INTFLAG.reg = clear_mask;
#endif
}

static int gpio_sam0_init(struct device *dev)
{
	if (!eic_peripheral_is_enabled) {
		IRQ_CONNECT(DT_EIC_SAM0_IRQ, DT_EIC_SAM0_IRQ_PRIORITY,
			    eic_sam0_isr, NULL, 0);

		irq_enable(DT_EIC_SAM0_IRQ);

		/* Enable the GCLK */
		GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_EIC |
				    GCLK_CLKCTRL_GEN_GCLK0 |
				    GCLK_CLKCTRL_CLKEN;

		/* Disable all external interrupts */
		/* EIC->INTENCLR.reg = EIC_INTENSET_MASK; */

		EIC->CTRL.bit.ENABLE = 1;

		while (EIC->STATUS.bit.SYNCBUSY) {
		}

		eic_peripheral_is_enabled = true;
	}

	return 0;
}

#ifdef DT_GPIO_SAM0_PORTA_BASE_ADDRESS

static const struct gpio_sam0_config gpio_sam0_config_0 = {
	.regs = (PortGroup *)DT_GPIO_SAM0_PORTA_BASE_ADDRESS,
	.extints = &porta_extints[0],
	.num_extints = ARRAY_SIZE(porta_extints),
};

static struct gpio_sam0_data gpio_sam0_data_0;

DEVICE_AND_API_INIT(gpio_sam0_0, DT_GPIO_SAM0_PORTA_LABEL, gpio_sam0_init,
		    &gpio_sam0_data_0, &gpio_sam0_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam0_api);
#endif /* DT_GPIO_SAM0_PORTA_BASE_ADDRESS */

#ifdef DT_GPIO_SAM0_PORTB_BASE_ADDRESS

static const struct gpio_sam0_config gpio_sam0_config_1 = {
	.regs = (PortGroup *)DT_GPIO_SAM0_PORTB_BASE_ADDRESS,
	.extints = &portb_extints[0],
	.num_extints = ARRAY_SIZE(portb_extints),
};

static struct gpio_sam0_data gpio_sam0_data_1;

DEVICE_AND_API_INIT(gpio_sam0_1, DT_GPIO_SAM0_PORTB_LABEL, gpio_sam0_init,
		    &gpio_sam0_data_1, &gpio_sam0_config_1, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam0_api);
#endif /* DT_GPIO_SAM0_PORTB_BASE_ADDRESS */

#else /* CONFIG_EIC_SAM0 */

static const struct gpio_driver_api gpio_sam0_api = {
	.config = gpio_sam0_config,
	.write = gpio_sam0_write,
	.read = gpio_sam0_read,
};

static int gpio_sam0_init(struct device *dev)
{
	return 0;
}

/* Port A */
#ifdef DT_GPIO_SAM0_PORTA_BASE_ADDRESS

static const struct gpio_sam0_config gpio_sam0_config_0 = {
	.regs = (PortGroup *)DT_GPIO_SAM0_PORTA_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(gpio_sam0_0, DT_GPIO_SAM0_PORTA_LABEL, gpio_sam0_init,
		    NULL, &gpio_sam0_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam0_api);
#endif

/* Port B */
#ifdef DT_GPIO_SAM0_PORTB_BASE_ADDRESS

static const struct gpio_sam0_config gpio_sam0_config_1 = {
	.regs = (PortGroup *)DT_GPIO_SAM0_PORTB_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(gpio_sam0_1, DT_GPIO_SAM0_PORTB_LABEL, gpio_sam0_init,
		    NULL, &gpio_sam0_config_1, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_sam0_api);
#endif

#endif /* CONFIG_EIC_SAM0 */
