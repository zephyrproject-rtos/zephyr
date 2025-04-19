/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/atmel-sam0-gpio.h>
#include <soc.h>
#include <zephyr/drivers/interrupt_controller/sam0_eic.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include "gpio_sam0.h"

#ifndef PORT_PMUX_PMUXE_A_Val
#define PORT_PMUX_PMUXE_A_Val (0)
#endif

struct gpio_sam0_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uintptr_t regs;
#ifdef CONFIG_SAM0_EIC
	uint8_t id;
#endif
};

struct gpio_sam0_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	const struct device *dev;
	gpio_port_pins_t debounce;
#ifdef CONFIG_SAM0_EIC
	sys_slist_t callbacks;
#endif
};

#ifdef CONFIG_SAM0_EIC
static void gpio_sam0_isr(uint32_t pins, void *arg)
{
	struct gpio_sam0_data *const data = (struct gpio_sam0_data *)arg;

	gpio_fire_callbacks(&data->callbacks, data->dev, pins);
}
#endif

static int gpio_sam0_config(const struct device *dev, gpio_pin_t pin,
			    gpio_flags_t flags)
{
	const struct gpio_sam0_config *config = dev->config;
	struct gpio_sam0_data *data = dev->data;
	uintptr_t regs = config->regs;
	uint8_t pincfg = 0;

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/* Supports disconnected, input, output, or bidirectional */
	if ((flags & GPIO_INPUT) != 0) {
		WRITE_BIT(pincfg, PINCFG_INEN_BIT, 1);
	}
	if ((flags & GPIO_OUTPUT) != 0) {
		/* Output is incompatible with pull */
		if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
			return -ENOTSUP;
		}
		/* Bidirectional is supported */
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			sys_write32(BIT(pin), regs + OUTCLR_OFFSET);
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			sys_write32(BIT(pin), regs + OUTSET_OFFSET);
		}
		sys_write32(BIT(pin), regs + DIRSET_OFFSET);
	} else {
		/* Not output, may be input */
		sys_write32(BIT(pin), regs + DIRCLR_OFFSET);

		/* Pull configuration is supported if not output */
		if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
			WRITE_BIT(pincfg, PINCFG_PULLEN_BIT, 1);
			if ((flags & GPIO_PULL_UP) != 0) {
				sys_write32(BIT(pin), regs + OUTSET_OFFSET);
			} else {
				sys_write32(BIT(pin), regs + OUTCLR_OFFSET);
			}
		}
	}

	/* Preserve debounce flag for interrupt configuration. */
	WRITE_BIT(data->debounce, pin,
		  ((flags & SAM0_GPIO_DEBOUNCE) != 0) && IS_BIT_SET(pincfg, PINCFG_INEN_BIT));

	/* Write the now-built pin configuration */
	sys_write8(pincfg, regs + PINCFG_OFFSET + pin);

	return 0;
}

static int gpio_sam0_port_get_raw(const struct device *dev,
				  gpio_port_value_t *value)
{
	const struct gpio_sam0_config *config = dev->config;

	*value = sys_read32(config->regs + IN_OFFSET);

	return 0;
}

static int gpio_sam0_port_set_masked_raw(const struct device *dev,
					 gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	const struct gpio_sam0_config *config = dev->config;
	uint32_t out = sys_read32(config->regs + OUT_OFFSET);

	sys_write32((out & ~mask) | (value & mask), config->regs + OUT_OFFSET);

	return 0;
}

static int gpio_sam0_port_set_bits_raw(const struct device *dev,
				       gpio_port_pins_t pins)
{
	const struct gpio_sam0_config *config = dev->config;

	sys_write32(pins, config->regs + OUTSET_OFFSET);

	return 0;
}

static int gpio_sam0_port_clear_bits_raw(const struct device *dev,
					 gpio_port_pins_t pins)
{
	const struct gpio_sam0_config *config = dev->config;

	sys_write32(pins, config->regs + OUTCLR_OFFSET);

	return 0;
}

static int gpio_sam0_port_toggle_bits(const struct device *dev,
				      gpio_port_pins_t pins)
{
	const struct gpio_sam0_config *config = dev->config;

	sys_write32(pins, config->regs + OUTTGL_OFFSET);

	return 0;
}

#ifdef CONFIG_SAM0_EIC

static int gpio_sam0_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_sam0_config *config = dev->config;
	struct gpio_sam0_data *const data = dev->data;
	uintptr_t regs = config->regs;
	uint8_t pincfg = sys_read8(regs + PINCFG_OFFSET + pin);
	enum sam0_eic_trigger trigger;
	int rc = 0;
	uint8_t tmp;

	data->dev = dev;

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		WRITE_BIT(pincfg, PINCFG_PMUXEN_BIT, 0);
		rc = sam0_eic_disable_interrupt(config->id, pin);
		if (rc == -EBUSY) {
			/* Ignore diagnostic disabling disabled */
			rc = 0;
		}
		if (rc == 0) {
			rc = sam0_eic_release(config->id, pin);
		}
		break;
	case GPIO_INT_MODE_LEVEL:
	case GPIO_INT_MODE_EDGE:
		/* Enabling interrupts on a pin requires disconnecting
		 * the pin from the I/O pin controller (PORT) module
		 * and connecting it to the External Interrupt
		 * Controller (EIC).  This would prevent using the pin
		 * as an output, so interrupts are only supported if
		 * the pin is configured as input-only.
		 */
		if (!IS_BIT_SET(pincfg, PINCFG_INEN_BIT) || sys_test_bit(regs + DIR_OFFSET, pin)) {
			rc = -ENOTSUP;
			break;
		}

		/* Transfer control to EIC */
		WRITE_BIT(pincfg, PINCFG_PMUXEN_BIT, 1);
		tmp = sys_read8(regs + PMUX_OFFSET + (pin / 2U));
		if ((pin & 1U) != 0) {
			tmp &= ~PMUX_PMUXO_MASK;
		} else {
			tmp &= ~PMUX_PMUXE_MASK;
		}
		sys_write8(tmp, regs + PMUX_OFFSET + (pin / 2U));

		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			trigger = (mode == GPIO_INT_MODE_LEVEL)
				? SAM0_EIC_LOW
				: SAM0_EIC_FALLING;
			break;
		case GPIO_INT_TRIG_HIGH:
			trigger = (mode == GPIO_INT_MODE_LEVEL)
				? SAM0_EIC_HIGH
				: SAM0_EIC_RISING;
			break;
		case GPIO_INT_TRIG_BOTH:
			trigger = SAM0_EIC_BOTH;
			break;
		default:
			rc = -EINVAL;
			break;
		}

		if (rc == 0) {
			rc = sam0_eic_acquire(config->id, pin, trigger,
					      (data->debounce & BIT(pin)) != 0,
					      gpio_sam0_isr, data);
		}
		if (rc == 0) {
			rc = sam0_eic_enable_interrupt(config->id, pin);
		}

		break;
	default:
		rc = -EINVAL;
		break;
	}

	if (rc == 0) {
		/* Update the pin configuration */
		sys_write8(pincfg, regs + PINCFG_OFFSET + pin);
	}

	return rc;
}


static int gpio_sam0_manage_callback(const struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct gpio_sam0_data *const data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static uint32_t gpio_sam0_get_pending_int(const struct device *dev)
{
	const struct gpio_sam0_config *config = dev->config;

	return sam0_eic_interrupt_pending(config->id);
}

#endif

static DEVICE_API(gpio, gpio_sam0_api) = {
	.pin_configure = gpio_sam0_config,
	.port_get_raw = gpio_sam0_port_get_raw,
	.port_set_masked_raw = gpio_sam0_port_set_masked_raw,
	.port_set_bits_raw = gpio_sam0_port_set_bits_raw,
	.port_clear_bits_raw = gpio_sam0_port_clear_bits_raw,
	.port_toggle_bits = gpio_sam0_port_toggle_bits,
#ifdef CONFIG_SAM0_EIC
	.pin_interrupt_configure = gpio_sam0_pin_interrupt_configure,
	.manage_callback = gpio_sam0_manage_callback,
	.get_pending_int = gpio_sam0_get_pending_int,
#endif
};

static int gpio_sam0_init(const struct device *dev) { return 0; }

/* Port A */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(porta))

static const struct gpio_sam0_config gpio_sam0_config_0 = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
	},
	.regs = DT_REG_ADDR(DT_NODELABEL(porta)),
#ifdef CONFIG_SAM0_EIC
	.id = 0,
#endif
};

static struct gpio_sam0_data gpio_sam0_data_0;

DEVICE_DT_DEFINE(DT_NODELABEL(porta),
		    gpio_sam0_init, NULL,
		    &gpio_sam0_data_0, &gpio_sam0_config_0,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_sam0_api);
#endif

/* Port B */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portb))

static const struct gpio_sam0_config gpio_sam0_config_1 = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(1),
	},
	.regs = DT_REG_ADDR(DT_NODELABEL(portb)),
#ifdef CONFIG_SAM0_EIC
	.id = 1,
#endif
};

static struct gpio_sam0_data gpio_sam0_data_1;

DEVICE_DT_DEFINE(DT_NODELABEL(portb),
		    gpio_sam0_init, NULL,
		    &gpio_sam0_data_1, &gpio_sam0_config_1,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_sam0_api);
#endif

/* Port C */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portc))

static const struct gpio_sam0_config gpio_sam0_config_2 = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(2),
	},
	.regs = DT_REG_ADDR(DT_NODELABEL(portc)),
#ifdef CONFIG_SAM0_EIC
	.id = 2,
#endif
};

static struct gpio_sam0_data gpio_sam0_data_2;

DEVICE_DT_DEFINE(DT_NODELABEL(portc),
		    gpio_sam0_init, NULL,
		    &gpio_sam0_data_2, &gpio_sam0_config_2,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_sam0_api);
#endif

/* Port D */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(portd))

static const struct gpio_sam0_config gpio_sam0_config_3 = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(3),
	},
	.regs = DT_REG_ADDR(DT_NODELABEL(portd)),
#ifdef CONFIG_SAM0_EIC
	.id = 3,
#endif
};

static struct gpio_sam0_data gpio_sam0_data_3;

DEVICE_DT_DEFINE(DT_NODELABEL(portd),
		    gpio_sam0_init, NULL,
		    &gpio_sam0_data_3, &gpio_sam0_config_3,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_sam0_api);
#endif
