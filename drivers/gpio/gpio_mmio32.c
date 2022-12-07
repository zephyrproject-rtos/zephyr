/*
 * Copyright (c) 2016 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver to provide the GPIO API for a simple 32-bit i/o register
 *
 * This is a driver for accessing a simple, fixed purpose, 32-bit
 * memory-mapped i/o register using the same APIs as GPIO drivers. This is
 * useful when an SoC or board has registers that aren't part of a GPIO IP
 * block and these registers are used to control things that Zephyr normally
 * expects to be specified using a GPIO pin, e.g. for driving an LED, or
 * chip-select line for an SPI device.
 *
 * The implementation expects that all bits of the hardware register are both
 * readable and writable, and that for any bits that act as outputs, the value
 * read will have the value that was last written to it. This requirement
 * stems from the use of a read-modify-write method for all changes.
 *
 * It is possible to specify a restricted mask of bits that are valid for
 * access, and whenever the register is written, the value of bits outside this
 * mask will be preserved, even when the whole port is written to using
 * gpio_port_write.
 */

#include <zephyr/drivers/gpio/gpio_mmio32.h>
#include <zephyr/irq.h>
#include <errno.h>

static int gpio_mmio32_config(const struct device *dev,
			      gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_mmio32_context *context = dev->data;
	const struct gpio_mmio32_config *config = context->config;

	if ((config->mask & (1 << pin)) == 0) {
		return -EINVAL; /* Pin not in our validity mask */
	}

	if (flags & ~(GPIO_INPUT | GPIO_OUTPUT |
		      GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH |
		      GPIO_ACTIVE_LOW)) {
		/* We ignore direction and fake polarity, rest is unsupported */
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		unsigned int key;
		volatile uint32_t *reg = config->reg;

		key = irq_lock();
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			*reg = (*reg | (1 << pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			*reg = (*reg & (config->mask & ~(1 << pin)));
		}
		irq_unlock(key);
	}

	return 0;
}

static int gpio_mmio32_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct gpio_mmio32_context *context = dev->data;
	const struct gpio_mmio32_config *config = context->config;

	*value = *config->reg & config->mask;

	return 0;
}

static int gpio_mmio32_port_set_masked_raw(const struct device *dev,
					   uint32_t mask,
					   uint32_t value)
{
	struct gpio_mmio32_context *context = dev->data;
	const struct gpio_mmio32_config *config = context->config;
	volatile uint32_t *reg = config->reg;
	unsigned int key;

	mask &= config->mask;
	value &= mask;

	/* Update pin state atomically */
	key = irq_lock();
	*reg = (*reg & ~mask) | value;
	irq_unlock(key);

	return 0;
}

static int gpio_mmio32_port_set_bits_raw(const struct device *dev,
					 uint32_t mask)
{
	struct gpio_mmio32_context *context = dev->data;
	const struct gpio_mmio32_config *config = context->config;
	volatile uint32_t *reg = config->reg;
	unsigned int key;

	mask &= config->mask;

	/* Update pin state atomically */
	key = irq_lock();
	*reg = (*reg | mask);
	irq_unlock(key);

	return 0;
}

static int gpio_mmio32_port_clear_bits_raw(const struct device *dev,
					   uint32_t mask)
{
	struct gpio_mmio32_context *context = dev->data;
	const struct gpio_mmio32_config *config = context->config;
	volatile uint32_t *reg = config->reg;
	unsigned int key;

	mask &= config->mask;

	/* Update pin state atomically */
	key = irq_lock();
	*reg = (*reg & ~mask);
	irq_unlock(key);

	return 0;
}

static int gpio_mmio32_port_toggle_bits(const struct device *dev,
					uint32_t mask)
{
	struct gpio_mmio32_context *context = dev->data;
	const struct gpio_mmio32_config *config = context->config;
	volatile uint32_t *reg = config->reg;
	unsigned int key;

	mask &= config->mask;

	/* Update pin state atomically */
	key = irq_lock();
	*reg = (*reg ^ mask);
	irq_unlock(key);

	return 0;
}

static int gpio_mmio32_pin_interrupt_configure(const struct device *dev,
					       gpio_pin_t pin,
					       enum gpio_int_mode mode,
					       enum gpio_int_trig trig)
{
	if (mode != GPIO_INT_MODE_DISABLED) {
		return -ENOTSUP;
	}

	return 0;
}

const struct gpio_driver_api gpio_mmio32_api = {
	.pin_configure = gpio_mmio32_config,
	.port_get_raw = gpio_mmio32_port_get_raw,
	.port_set_masked_raw = gpio_mmio32_port_set_masked_raw,
	.port_set_bits_raw = gpio_mmio32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mmio32_port_clear_bits_raw,
	.port_toggle_bits = gpio_mmio32_port_toggle_bits,
	.pin_interrupt_configure = gpio_mmio32_pin_interrupt_configure,
};

int gpio_mmio32_init(const struct device *dev)
{
	struct gpio_mmio32_context *context = dev->data;
	const struct gpio_mmio32_config *config = dev->config;

	context->config = config;

	return 0;
}
