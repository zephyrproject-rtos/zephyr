/*
 * Copyright (c) 2021, Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>

/* pico-sdk includes */
#include <hardware/gpio.h>
#include <hardware/regs/intctrl.h>
#include <hardware/structs/iobank0.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#define DT_DRV_COMPAT raspberrypi_pico_gpio

#define ALL_EVENTS (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE \
		| GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH)

struct gpio_rpi_config {
	struct gpio_driver_config common;
	void (*bank_config_func)(void);
};

struct gpio_rpi_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	uint32_t int_enabled_mask;
	uint32_t single_ended_mask;
	uint32_t open_drain_mask;
};

static int gpio_rpi_configure(const struct device *dev,
				gpio_pin_t pin,
				gpio_flags_t flags)
{
	struct gpio_rpi_data *data = dev->data;

	gpio_set_pulls(pin,
		(flags & GPIO_PULL_UP) != 0U,
		(flags & GPIO_PULL_DOWN) != 0U);

	/* Avoid gpio_init, since that also clears previously set direction/high/low */
	gpio_set_function(pin, GPIO_FUNC_SIO);

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
			data->single_ended_mask |= BIT(pin);

			/* Setting the initial state of output data, and output enable.
			 * The output data will not change from here on, only output
			 * enable will. If none of the GPIO_OUTPUT_INIT_* flags have
			 * been set then fall back to the non-agressive input mode.
			 */
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				data->open_drain_mask |= BIT(pin);
				gpio_put(pin, 0);
				gpio_set_dir(pin, flags & GPIO_OUTPUT_INIT_LOW);
			} else {
				data->open_drain_mask &= ~(BIT(pin));
				gpio_put(pin, 1);
				gpio_set_dir(pin, flags & GPIO_OUTPUT_INIT_HIGH);
			}
		} else {
			data->single_ended_mask &= ~(BIT(pin));
			if (flags & GPIO_OUTPUT_INIT_HIGH) {
				gpio_put(pin, 1);
			} else if (flags & GPIO_OUTPUT_INIT_LOW) {
				gpio_put(pin, 0);
			}
			gpio_set_dir(pin, GPIO_OUT);
		}
	} else if (flags & GPIO_INPUT) {
		gpio_set_dir(pin, GPIO_IN);
	}

	return 0;
}

static int gpio_rpi_port_get_raw(const struct device *dev, uint32_t *value)
{
	*value = gpio_get_all();
	return 0;
}

static int gpio_rpi_port_set_masked_raw(const struct device *port,
					uint32_t mask, uint32_t value)
{
	struct gpio_rpi_data *data = port->data;
	/* First handle push-pull pins: */
	gpio_put_masked(mask & ~data->single_ended_mask, value);
	/* Then handle open-drain pins: */
	gpio_set_dir_masked(mask & data->single_ended_mask & data->open_drain_mask, ~value);
	/* Then handle open-source pins: */
	gpio_set_dir_masked(mask & data->single_ended_mask & ~data->open_drain_mask, value);
	return 0;
}

static int gpio_rpi_port_set_bits_raw(const struct device *port,
					uint32_t pins)
{
	struct gpio_rpi_data *data = port->data;
	/* First handle push-pull pins: */
	gpio_set_mask(pins & ~data->single_ended_mask);
	/* Then handle open-drain pins: */
	gpio_set_dir_in_masked(pins & data->single_ended_mask & data->open_drain_mask);
	/* Then handle open-source pins: */
	gpio_set_dir_out_masked(pins & data->single_ended_mask & ~data->open_drain_mask);
	return 0;
}

static int gpio_rpi_port_clear_bits_raw(const struct device *port,
					uint32_t pins)
{
	struct gpio_rpi_data *data = port->data;
	/* First handle push-pull pins: */
	gpio_clr_mask(pins & ~data->single_ended_mask);
	/* Then handle open-drain pins: */
	gpio_set_dir_out_masked(pins & data->single_ended_mask & data->open_drain_mask);
	/* Then handle open-source pins: */
	gpio_set_dir_in_masked(pins & data->single_ended_mask & ~data->open_drain_mask);
	return 0;
}

static int gpio_rpi_port_toggle_bits(const struct device *port,
					uint32_t pins)
{
	struct gpio_rpi_data *data = port->data;
	/* First handle push-pull pins: */
	gpio_xor_mask(pins & ~data->single_ended_mask);
	/* Then handle single-ended pins: */
	/* (unfortunately there's no pico-sdk api call that can be used for this,
	 * but it's possible by accessing the registers directly)
	 */
	sio_hw->gpio_oe_togl = (pins & data->single_ended_mask);
	return 0;
}

static int gpio_rpi_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	struct gpio_rpi_data *data = dev->data;
	uint32_t events = 0;

	gpio_set_irq_enabled(pin, ALL_EVENTS, false);
	if (mode != GPIO_INT_DISABLE) {
		if (mode & GPIO_INT_EDGE) {
			if (trig & GPIO_INT_LOW_0) {
				events |= GPIO_IRQ_EDGE_FALL;
			}
			if (trig & GPIO_INT_HIGH_1) {
				events |= GPIO_IRQ_EDGE_RISE;
			}
		} else {
			if (trig & GPIO_INT_LOW_0) {
				events |= GPIO_IRQ_LEVEL_LOW;
			}
			if (trig & GPIO_INT_HIGH_1) {
				events |= GPIO_IRQ_LEVEL_HIGH;
			}
		}
		gpio_set_irq_enabled(pin, events, true);
	}
	WRITE_BIT(data->int_enabled_mask, pin, mode != GPIO_INT_DISABLE);
	return 0;
}

static int gpio_rpi_manage_callback(const struct device *dev,
				struct gpio_callback *callback, bool set)
{
	struct gpio_rpi_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(gpio, gpio_rpi_driver_api) = {
	.pin_configure = gpio_rpi_configure,
	.port_get_raw = gpio_rpi_port_get_raw,
	.port_set_masked_raw = gpio_rpi_port_set_masked_raw,
	.port_set_bits_raw = gpio_rpi_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rpi_port_clear_bits_raw,
	.port_toggle_bits = gpio_rpi_port_toggle_bits,
	.pin_interrupt_configure = gpio_rpi_pin_interrupt_configure,
	.manage_callback = gpio_rpi_manage_callback,
};

static void gpio_rpi_isr(const struct device *dev)
{
	struct gpio_rpi_data *data = dev->data;
	io_bank0_irq_ctrl_hw_t *irq_ctrl_base;
	const io_rw_32 *status_reg;
	uint32_t events;
	uint32_t pin;

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

static int gpio_rpi_bank_init(const struct device *dev)
{
	const struct gpio_rpi_config *config = dev->config;

	config->bank_config_func();
	return 0;
}

#define GPIO_RPI_INIT(idx)							\
	static void bank_##idx##_config_func(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority),	\
			    gpio_rpi_isr, DEVICE_DT_INST_GET(idx), 0);		\
		irq_enable(DT_INST_IRQN(idx));					\
	}									\
	static const struct gpio_rpi_config gpio_rpi_##idx##_config = {		\
		.bank_config_func = bank_##idx##_config_func,			\
		.common =							\
		{								\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),	\
		}								\
	};									\
										\
	static struct gpio_rpi_data gpio_rpi_##idx##_data;			\
										\
	DEVICE_DT_INST_DEFINE(idx, gpio_rpi_bank_init, NULL,			\
				&gpio_rpi_##idx##_data,				\
				&gpio_rpi_##idx##_config,			\
				POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,		\
				&gpio_rpi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RPI_INIT)
