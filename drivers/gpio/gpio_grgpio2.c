/*
 * Copyright (c) 2023 Frontgrade Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for GRLIB GRGPIO revision 2.
 * - iflag determine pending interrupt.
 * - interrupt map decides interrupt number if implemented.
 * - logic or/and/xor registers used when possible
 */

#define DT_DRV_COMPAT gaisler_grgpio

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include "gpio_grgpio.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_grgpio2);

struct cfg {
	struct gpio_driver_config common;
	volatile struct grgpio_regs *regs;
	int interrupt;
};

struct data {
	struct gpio_driver_data common;
	struct k_spinlock lock;
	sys_slist_t cb;
	uint32_t imask;
	uint32_t connected;
	int irqgen;
};

static void grgpio_isr(const struct device *dev);

static int pin_configure(const struct device *dev,
				gpio_pin_t pin, gpio_flags_t flags)
{
	const struct cfg *cfg = dev->config;
	struct data *data = dev->data;
	volatile struct grgpio_regs *regs = cfg->regs;
	uint32_t mask = 1 << pin;

	if (flags & GPIO_SINGLE_ENDED) {
		return -ENOTSUP;
	}

	if (flags == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_DIR_MASK) == (GPIO_INPUT | GPIO_OUTPUT)) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT) {
		k_spinlock_key_t key;

		/*
		 * Register operations are atomic, but do the sequence under
		 * lock so it serializes.
		 */
		key = k_spin_lock(&data->lock);
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			regs->output_or = mask;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			regs->output_and = ~mask;
		}
		regs->dir_or = mask;
		k_spin_unlock(&data->lock, key);
	} else {
		regs->dir_and = ~mask;
	}

	return 0;
}

static int port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct cfg *cfg = dev->config;

	*value = cfg->regs->data;
	return 0;
}

static int port_set_masked_raw(const struct device *dev,
					  gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct cfg *cfg = dev->config;
	struct data *data = dev->data;
	volatile struct grgpio_regs *regs = cfg->regs;
	uint32_t port_val;
	k_spinlock_key_t key;

	value &= mask;
	key = k_spin_lock(&data->lock);
	port_val = (regs->output & ~mask) | value;
	regs->output = port_val;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct cfg *cfg = dev->config;
	volatile struct grgpio_regs *regs = cfg->regs;

	regs->output_or = pins;
	return 0;
}

static int port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct cfg *cfg = dev->config;
	volatile struct grgpio_regs *regs = cfg->regs;

	regs->output_and = ~pins;
	return 0;
}

static int port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct cfg *cfg = dev->config;
	volatile struct grgpio_regs *regs = cfg->regs;

	regs->output_xor = pins;
	return 0;
}

static uint32_t get_pending_int(const struct device *dev)
{
	const struct cfg *cfg = dev->config;
	volatile struct grgpio_regs *regs = cfg->regs;

	return regs->iflag;
}

static int pin_interrupt_configure(const struct device *dev,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	const struct cfg *cfg = dev->config;
	struct data *data = dev->data;
	volatile struct grgpio_regs *regs = cfg->regs;
	int ret = 0;
	const uint32_t mask = 1 << pin;
	uint32_t polmask;
	k_spinlock_key_t key;

	if ((mask & data->imask) == 0) {
		/* This pin can not generate interrupt */
		return -ENOTSUP;
	}
	if (mode != GPIO_INT_MODE_DISABLED) {
		if (trig == GPIO_INT_TRIG_LOW) {
			polmask = 0;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			polmask = mask;
		} else {
			return -ENOTSUP;
		}
	}
	key = k_spin_lock(&data->lock);
	if (mode == GPIO_INT_MODE_DISABLED) {
		regs->imask_and = ~mask;
	} else if (mode == GPIO_INT_MODE_LEVEL) {
		regs->imask_and = ~mask;
		regs->iedge &= ~mask;
		regs->ipol = (regs->ipol & ~mask) | polmask;
		regs->imask_or = mask;
	} else if (mode == GPIO_INT_MODE_EDGE) {
		regs->imask_and = ~mask;
		regs->iedge |= mask;
		regs->ipol = (regs->ipol & ~mask) | polmask;
		regs->imask_or = mask;
	} else {
		ret = -ENOTSUP;
	}
	k_spin_unlock(&data->lock, key);

	/* Remove old interrupt history for this pin. */
	regs->iflag = mask;

	int interrupt = cfg->interrupt;
	const int irqgen = data->irqgen;

	if (irqgen == 0) {
		interrupt += pin;
	} else if (irqgen == 1) {
		;
	} else if (irqgen < 32) {
		/* look up interrupt number in GRGPIO interrupt map */
		uint32_t val = regs->irqmap[pin/4];

		val >>= (3 - pin % 4) * 8;
		interrupt += (val & 0x1f);
	}

	if (interrupt && ((1 << interrupt) & data->connected) == 0) {
		irq_connect_dynamic(
			interrupt,
			0,
			(void (*)(const void *)) grgpio_isr,
			dev,
			0
		);
		irq_enable(interrupt);
		data->connected |= 1 << interrupt;
	}

	return ret;
}

static int manage_callback(const struct device *dev,
			   struct gpio_callback *callback,
			   bool set)
{
	struct data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static void grgpio_isr(const struct device *dev)
{
	const struct cfg *cfg = dev->config;
	struct data *data = dev->data;
	volatile struct grgpio_regs *regs = cfg->regs;
	uint32_t pins;

	/* no locking needed when iflag is implemented */
	pins = regs->iflag;
	if (pins == 0) {
		return;
	}
	regs->iflag = pins;
	gpio_fire_callbacks(&data->cb, dev, pins);
}

static int grgpio_init(const struct device *dev)
{
	const struct cfg *cfg = dev->config;
	struct data *data = dev->data;
	volatile struct grgpio_regs *regs = cfg->regs;

	data->irqgen = (regs->cap & GRGPIO_CAP_IRQGEN) >> GRGPIO_CAP_IRQGEN_BIT;
	regs->dir = 0;
	/* Mask all Interrupts */
	regs->imask = 0;
	/* Make IRQ Rising edge triggered default */
	regs->ipol = 0xffffffff;
	regs->iedge = 0xffffffff;
	regs->iflag = 0xffffffff;
	/* Read what I/O lines have IRQ support */
	data->imask = regs->ipol;

	return 0;
}

static const struct gpio_driver_api driver_api = {
	.pin_configure                  = pin_configure,
	.port_get_raw                   = port_get_raw,
	.port_set_masked_raw            = port_set_masked_raw,
	.port_set_bits_raw              = port_set_bits_raw,
	.port_clear_bits_raw            = port_clear_bits_raw,
	.port_toggle_bits               = port_toggle_bits,
	.pin_interrupt_configure        = pin_interrupt_configure,
	.manage_callback                = manage_callback,
	.get_pending_int                = get_pending_int,
};

#define GRGPIO_INIT(n)							\
	static const struct cfg cfg_##n = {				\
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),\
		},							\
		.regs = (void *) DT_INST_REG_ADDR(n),			\
		.interrupt = DT_INST_IRQN(n),				\
	};								\
	static struct data data_##n;					\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    grgpio_init,				\
			    NULL,					\
			    &data_##n,					\
			    &cfg_##n,					\
			    POST_KERNEL,				\
			    CONFIG_GPIO_INIT_PRIORITY,			\
			    &driver_api					\
			   );

DT_INST_FOREACH_STATUS_OKAY(GRGPIO_INIT)
