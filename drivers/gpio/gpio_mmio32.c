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

#include <gpio/gpio_mmio32.h>
#include <errno.h>

static int gpio_mmio32_config(struct device *dev, int access_op,
					u32_t pin, int flags)
{
	struct gpio_mmio32_context *context = dev->driver_data;
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
		volatile u32_t *reg = config->reg;

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

static int gpio_mmio32_write(struct device *dev, int access_op,
					u32_t pin, u32_t value)
{
	struct gpio_mmio32_context *context = dev->driver_data;
	const struct gpio_mmio32_config *config = context->config;
	volatile u32_t *reg = config->reg;
	u32_t mask = config->mask;
	u32_t invert = context->common.invert;
	unsigned int key;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		mask &= 1 << pin;
		if (!mask) {
			return -EINVAL; /* Pin not in our validity mask */
		}
		value = value ? mask : 0;
	}

	value = (value ^ invert) & mask;

	/* Update pin state atomically */
	key = irq_lock();
	*reg = (*reg & ~mask) | value;
	irq_unlock(key);

	return 0;
}

static int gpio_mmio32_read(struct device *dev, int access_op,
					u32_t pin, u32_t *value)
{
	struct gpio_mmio32_context *context = dev->driver_data;
	const struct gpio_mmio32_config *config = context->config;
	u32_t bits;

	bits = (*config->reg ^ context->common.invert) & config->mask;
	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (bits >> pin) & 1;
		if ((config->mask & (1 << pin)) == 0) {
			return -EINVAL; /* Pin not in our validity mask */
		}
	} else {
		*value = bits;
	}

	return 0;
}

static int gpio_mmio32_port_get_raw(struct device *dev, u32_t *value)
{
	struct gpio_mmio32_context *context = dev->driver_data;
	const struct gpio_mmio32_config *config = context->config;

	*value = *config->reg & config->mask;

	return 0;
}

static int gpio_mmio32_port_set_masked_raw(struct device *dev, u32_t mask,
					  u32_t value)
{
	struct gpio_mmio32_context *context = dev->driver_data;
	const struct gpio_mmio32_config *config = context->config;
	volatile u32_t *reg = config->reg;
	unsigned int key;

	mask &= config->mask;
	value &= mask;

	/* Update pin state atomically */
	key = irq_lock();
	*reg = (*reg & ~mask) | value;
	irq_unlock(key);

	return 0;
}

static int gpio_mmio32_port_set_bits_raw(struct device *dev, u32_t mask)
{
	struct gpio_mmio32_context *context = dev->driver_data;
	const struct gpio_mmio32_config *config = context->config;
	volatile u32_t *reg = config->reg;
	unsigned int key;

	mask &= config->mask;

	/* Update pin state atomically */
	key = irq_lock();
	*reg = (*reg | mask);
	irq_unlock(key);

	return 0;
}

static int gpio_mmio32_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	struct gpio_mmio32_context *context = dev->driver_data;
	const struct gpio_mmio32_config *config = context->config;
	volatile u32_t *reg = config->reg;
	unsigned int key;

	mask &= config->mask;

	/* Update pin state atomically */
	key = irq_lock();
	*reg = (*reg & ~mask);
	irq_unlock(key);

	return 0;
}

static int gpio_mmio32_port_toggle_bits(struct device *dev, u32_t mask)
{
	struct gpio_mmio32_context *context = dev->driver_data;
	const struct gpio_mmio32_config *config = context->config;
	volatile u32_t *reg = config->reg;
	unsigned int key;

	mask &= config->mask;

	/* Update pin state atomically */
	key = irq_lock();
	*reg = (*reg ^ mask);
	irq_unlock(key);

	return 0;
}

static int gpio_mmio32_pin_interrupt_configure(struct device *dev,
		unsigned int pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig)
{
	if (mode != GPIO_INT_MODE_DISABLED) {
		return -ENOTSUP;
	}

	return 0;
}

static const struct gpio_driver_api gpio_mmio32_api = {
	.config = gpio_mmio32_config,
	.write = gpio_mmio32_write,
	.read = gpio_mmio32_read,
	.port_get_raw = gpio_mmio32_port_get_raw,
	.port_set_masked_raw = gpio_mmio32_port_set_masked_raw,
	.port_set_bits_raw = gpio_mmio32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mmio32_port_clear_bits_raw,
	.port_toggle_bits = gpio_mmio32_port_toggle_bits,
	.pin_interrupt_configure = gpio_mmio32_pin_interrupt_configure,
};

int gpio_mmio32_init(struct device *dev)
{
	struct gpio_mmio32_context *context = dev->driver_data;
	const struct gpio_mmio32_config *config = dev->config->config_info;

	context->config = config;
	dev->driver_api = &gpio_mmio32_api;

	return 0;
}
