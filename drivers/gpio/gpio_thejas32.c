/*
 * Copyright (c) 2026 Anuj Deshpande
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdac_thejas32_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/sys_io.h>

/*
 * Each controller handles 16 pins as 16-bit registers.
 *
 * Pin data is accessed through address masking: bits [17:2] of the address
 * form a pin mask, and a data access only operates on the pins whose mask
 * bit is set. Reads return the masked pin states; writes update only the
 * masked pins. This makes per-pin set/clear/write atomic without
 * read-modify-write.
 *
 * The direction register lives above the data window at offset 0x40000;
 * setting a bit makes the pin an output. All pins reset to input.
 *
 * Register accesses need an explicit fence to take effect, as in C-DAC's
 * own drivers.
 */
#define THEJAS32_GPIO_DIR_OFFSET 0x40000UL

#define DATA_ADDR(base, mask) ((base) + (((mem_addr_t)(mask)) << 2))

struct gpio_thejas32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	mem_addr_t base;
};

struct gpio_thejas32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* protects the direction register read-modify-write */
	struct k_spinlock lock;
};

static int gpio_thejas32_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_thejas32_config *config = dev->config;
	struct gpio_thejas32_data *data = dev->data;
	const mem_addr_t dir = config->base + THEJAS32_GPIO_DIR_OFFSET;
	const uint16_t bit = BIT(pin);

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN | GPIO_SINGLE_ENDED)) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0U) {
		/* Set the initial level before switching to output. */
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			sys_write16(bit, DATA_ADDR(config->base, bit));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			sys_write16(0U, DATA_ADDR(config->base, bit));
		}
		barrier_dmem_fence_full();

		K_SPINLOCK(&data->lock) {
			sys_write16(sys_read16(dir) | bit, dir);
		}
	} else if ((flags & GPIO_INPUT) != 0U) {
		K_SPINLOCK(&data->lock) {
			sys_write16(sys_read16(dir) & (uint16_t)~bit, dir);
		}
	} else {
		return -ENOTSUP;
	}
	barrier_dmem_fence_full();

	return 0;
}

static int gpio_thejas32_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_thejas32_config *config = dev->config;

	*value = sys_read16(DATA_ADDR(config->base, 0xffffU));

	return 0;
}

static int gpio_thejas32_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	const struct gpio_thejas32_config *config = dev->config;

	sys_write16((uint16_t)value, DATA_ADDR(config->base, mask & 0xffffU));
	barrier_dmem_fence_full();

	return 0;
}

static int gpio_thejas32_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_thejas32_port_set_masked_raw(dev, pins, pins);
}

static int gpio_thejas32_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_thejas32_port_set_masked_raw(dev, pins, 0U);
}

static int gpio_thejas32_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_thejas32_config *config = dev->config;
	const mem_addr_t addr = DATA_ADDR(config->base, pins & 0xffffU);

	sys_write16(~sys_read16(addr), addr);
	barrier_dmem_fence_full();

	return 0;
}

static int gpio_thejas32_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						 enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);

	if (mode == GPIO_INT_MODE_DISABLED) {
		return 0;
	}

	return -ENOTSUP;
}

static DEVICE_API(gpio, gpio_thejas32_api) = {
	.pin_configure = gpio_thejas32_configure,
	.port_get_raw = gpio_thejas32_port_get_raw,
	.port_set_masked_raw = gpio_thejas32_port_set_masked_raw,
	.port_set_bits_raw = gpio_thejas32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_thejas32_port_clear_bits_raw,
	.port_toggle_bits = gpio_thejas32_port_toggle_bits,
	.pin_interrupt_configure = gpio_thejas32_pin_interrupt_configure,
};

#define GPIO_THEJAS32_INIT(n)                                                                      \
	static const struct gpio_thejas32_config gpio_thejas32_config_##n = {                      \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	static struct gpio_thejas32_data gpio_thejas32_data_##n;                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &gpio_thejas32_data_##n, &gpio_thejas32_config_##n,   \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_thejas32_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_THEJAS32_INIT)
