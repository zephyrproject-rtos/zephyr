/*
 * Copyright (c) 2021, Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <drivers/gpio.h>
#include <hardware/gpio.h>
#include <hardware/regs/intctrl.h>
#include <hardware/structs/iobank0.h>

#include "gpio_utils.h"

#define DT_DRV_COMPAT raspberrypi_rp2_gpio

#define IRQ_PRIORITY 0
#define ALL_EVENTS (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE \
		| GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH)

struct gpio_raspberrypi_config {
	struct gpio_driver_config common;
};

struct gpio_raspberrypi_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	uint32_t int_enabled_mask;
};

static int gpio_raspberrypi_configure(const struct device *dev,
				gpio_pin_t pin,
				gpio_flags_t flags)
{
	if (flags & GPIO_SINGLE_ENDED) {
		return -ENOTSUP;
	}

	gpio_init(pin);

	if (flags & GPIO_OUTPUT) {
		gpio_set_dir(pin, GPIO_OUT);

		if (flags & GPIO_OUTPUT_INIT_HIGH)
			gpio_put(pin, 1);
		else if (flags & GPIO_OUTPUT_INIT_LOW)
			gpio_put(pin, 0);

	} else if (flags & GPIO_INPUT) {
		gpio_set_dir(pin, GPIO_IN);
		if (flags & GPIO_PULL_UP)
			gpio_pull_up(pin);
		else if (flags & GPIO_PULL_DOWN)
			gpio_pull_down(pin);
	}

	return 0;
}

static int gpio_raspberrypi_port_get_raw(const struct device *dev, uint32_t *value)
{
	*value = gpio_get_all();
	return 0;
}

static int gpio_raspberrypi_port_set_masked_raw(const struct device *port,
					uint32_t mask, uint32_t value)
{
	gpio_put_masked(mask, value);
	return 0;
}

static int gpio_raspberrypi_port_set_bits_raw(const struct device *port,
					uint32_t pins)
{
	gpio_set_mask(pins);
	return 0;
}

static int gpio_raspberrypi_port_clear_bits_raw(const struct device *port,
					uint32_t pins)
{
	gpio_clr_mask(pins);
	return 0;
}

static int gpio_raspberrypi_port_toggle_bits(const struct device *port,
					uint32_t pins)
{
	gpio_xor_mask(pins);
	return 0;
}

static int gpio_raspberrypi_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	uint32_t events = 0;
	struct gpio_raspberrypi_data *data = dev->data;

	if (mode != GPIO_INT_DISABLE) {
		if (mode & GPIO_INT_EDGE) {
			if (trig & GPIO_INT_LOW_0)
				events |= GPIO_IRQ_EDGE_FALL;
			if (trig & GPIO_INT_HIGH_1)
				events |= GPIO_IRQ_EDGE_RISE;
		} else {
			if (trig & GPIO_INT_LOW_0)
				events |= GPIO_IRQ_LEVEL_LOW;
			if (trig & GPIO_INT_HIGH_1)
				events |= GPIO_IRQ_LEVEL_HIGH;
		}
		gpio_set_irq_enabled(pin, events, true);
	}
	WRITE_BIT(data->int_enabled_mask, pin, mode != GPIO_INT_DISABLE);
	return 0;
}

static int gpio_raspberrypi_manage_callback(const struct device *dev,
				struct gpio_callback *callback, bool set)
{
	struct gpio_raspberrypi_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static const struct gpio_driver_api gpio_raspberrypi_driver_api = {
	.pin_configure = gpio_raspberrypi_configure,
	.port_get_raw = gpio_raspberrypi_port_get_raw,
	.port_set_masked_raw = gpio_raspberrypi_port_set_masked_raw,
	.port_set_bits_raw = gpio_raspberrypi_port_set_bits_raw,
	.port_clear_bits_raw = gpio_raspberrypi_port_clear_bits_raw,
	.port_toggle_bits = gpio_raspberrypi_port_toggle_bits,
	.pin_interrupt_configure = gpio_raspberrypi_pin_interrupt_configure,
	.manage_callback = gpio_raspberrypi_manage_callback,
};

static void gpio_raspberrypi_isr(const struct device *dev)
{
	struct gpio_raspberrypi_data *data = dev->data;
	uint32_t pin;
	uint32_t events;
	io_irq_ctrl_hw_t *irq_ctrl_base;
	io_rw_32 *status_reg;

	irq_ctrl_base = &iobank0_hw->proc0_irq_ctrl;
	for (pin = 0; pin < NUM_BANK0_GPIOS; pin++) {
		status_reg = &irq_ctrl_base->ints[pin / 8];
		events = (*status_reg >> 4 * (pin % 8)) & ALL_EVENTS;
		if (events) {
			gpio_acknowledge_irq(pin, ALL_EVENTS);
			gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));
		}
	}
}

static int gpio_raspberrypi_init(const struct device *dev)
{
	IRQ_CONNECT(IO_IRQ_BANK0, IRQ_PRIORITY, gpio_raspberrypi_isr,
			DEVICE_DT_GET(DT_INST(0, raspberrypi_rp2_gpio)), 0);
	irq_enable(IO_IRQ_BANK0);
	return 0;
}

#define GPIO_RASPBERRYPI_INIT(idx) \
static const struct gpio_raspberrypi_config gpio_raspberrypi_##idx##_config = {	\
};										\
										\
static struct gpio_raspberrypi_data gpio_raspberrypi_##idx##_data;		\
										\
DEVICE_DT_INST_DEFINE(idx, gpio_raspberrypi_init, NULL,				\
			&gpio_raspberrypi_##idx##_data,				\
			&gpio_raspberrypi_##idx##_config,			\
			POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			&gpio_raspberrypi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RASPBERRYPI_INIT)
