/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_siwx91x_gpio_uulp

#include "sl_si91x_driver_gpio.h"
#include "sl_status.h"

#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

/* Zephyr GPIO header must be included after driver, due to symbol conflicts
 * for GPIO_INPUT and GPIO_OUTPUT between preprocessor macros in the Zephyr
 * API and struct member register definitions for the SiWx91x device.
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#define UULP_GPIO_COUNT           5
#define UULP_REG_INTERRUPT_CONFIG 0x10

/* Types */
struct gpio_siwx91x_uulp_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
};

struct gpio_siwx91x_uulp_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

/* Functions */
static int gpio_siwx91x_uulp_pin_configure(const struct device *dev, gpio_pin_t pin,
					   gpio_flags_t flags)
{
	if (flags & (GPIO_SINGLE_ENDED | GPIO_PULL_UP | GPIO_PULL_DOWN)) {
		return -ENOTSUP;
	}

	/* Enable input */
	sl_si91x_gpio_select_uulp_npss_receiver(pin, (flags & GPIO_INPUT) ? 1 : 0);

	/* Select GPIO mode */
	sl_si91x_gpio_set_uulp_npss_pin_mux(pin, 0);

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		sl_si91x_gpio_set_uulp_npss_pin_value(pin, 1);
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		sl_si91x_gpio_set_uulp_npss_pin_value(pin, 0);
	}

	/* Enable output */
	sl_si91x_gpio_set_uulp_npss_direction(pin, (flags & GPIO_OUTPUT) ? 0 : 1);

	return 0;
}

static int gpio_siwx91x_uulp_port_get(const struct device *port, gpio_port_value_t *value)
{
	for (size_t i = 0; i < UULP_GPIO_COUNT; i++) {
		WRITE_BIT(*value, i, sl_si91x_gpio_get_uulp_npss_pin(i));
	}

	return 0;
}

static int gpio_siwx91x_uulp_port_set_masked(const struct device *port, gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	for (size_t i = 0; i < UULP_GPIO_COUNT; i++) {
		if (mask & BIT(i)) {
			sl_si91x_gpio_set_uulp_npss_pin_value(i, FIELD_GET(BIT(i), value));
		}
	}

	return 0;
}

static int gpio_siwx91x_uulp_port_set_bits(const struct device *port, gpio_port_pins_t pins)
{
	for (size_t i = 0; i < UULP_GPIO_COUNT; i++) {
		if (FIELD_GET(BIT(i), pins)) {
			sl_si91x_gpio_set_uulp_npss_pin_value(i, 1);
		}
	}

	return 0;
}

static int gpio_siwx91x_uulp_port_clear_bits(const struct device *port, gpio_port_pins_t pins)
{
	for (size_t i = 0; i < UULP_GPIO_COUNT; i++) {
		if (FIELD_GET(BIT(i), pins)) {
			sl_si91x_gpio_set_uulp_npss_pin_value(i, 0);
		}
	}

	return 0;
}

static int gpio_siwx91x_uulp_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	for (size_t i = 0; i < UULP_GPIO_COUNT; i++) {
		if (FIELD_GET(BIT(i), pins)) {
			sl_si91x_gpio_toggle_uulp_npss_pin(i);
		}
	}

	return 0;
}

static int gpio_siwx91x_uulp_manage_callback(const struct device *port,
					     struct gpio_callback *callback, bool set)
{
	struct gpio_siwx91x_uulp_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_siwx91x_uulp_interrupt_configure(const struct device *port, gpio_pin_t pin,
						 enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	uint32_t flags = 0;

	if (mode == GPIO_INT_MODE_DISABLED) {
		sl_si91x_gpio_configure_uulp_interrupt(flags, pin);
		sl_si91x_gpio_clear_uulp_interrupt(BIT(pin));
		sl_si91x_gpio_mask_uulp_npss_interrupt(BIT(pin));
	} else {
		if (trig == GPIO_INT_TRIG_LOW) {
			flags = (mode == GPIO_INT_MODE_EDGE) ? SL_GPIO_INTERRUPT_FALL_EDGE
							     : SL_GPIO_INTERRUPT_LEVEL_LOW;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			flags = (mode == GPIO_INT_MODE_EDGE) ? SL_GPIO_INTERRUPT_RISE_EDGE
							     : SL_GPIO_INTERRUPT_LEVEL_HIGH;
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			/* SL_GPIO_INTERRUPT_RISE_FALL_EDGE would make more sense, but HAL
			 * implementation is buggy.
			 */
			flags = SL_GPIO_INTERRUPT_RISE_EDGE | SL_GPIO_INTERRUPT_FALL_EDGE;
		}

		sl_si91x_gpio_configure_uulp_interrupt(flags, pin);
	}
	return 0;
}

static void gpio_siwx91x_uulp_isr(const struct device *port)
{
	struct gpio_siwx91x_uulp_data *data = port->data;
	uint8_t pins = sl_si91x_gpio_get_uulp_interrupt_status();

	sl_si91x_gpio_clear_uulp_interrupt(pins);

	gpio_fire_callbacks(&data->callbacks, port, pins);
}

static DEVICE_API(gpio, gpio_siwx91x_uulp_api) = {
	.pin_configure = gpio_siwx91x_uulp_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = NULL,
#endif
	.port_get_raw = gpio_siwx91x_uulp_port_get,
	.port_set_masked_raw = gpio_siwx91x_uulp_port_set_masked,
	.port_set_bits_raw = gpio_siwx91x_uulp_port_set_bits,
	.port_clear_bits_raw = gpio_siwx91x_uulp_port_clear_bits,
	.port_toggle_bits = gpio_siwx91x_uulp_port_toggle_bits,
	.pin_interrupt_configure = gpio_siwx91x_uulp_interrupt_configure,
	.manage_callback = gpio_siwx91x_uulp_manage_callback,
	.get_pending_int = NULL,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = NULL,
#endif
};

#define GPIO_PORT_INIT(idx)                                                                        \
	static const struct gpio_siwx91x_uulp_config gpio_siwx91x_port_config##idx = {             \
		.common.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),                      \
	};                                                                                         \
	static struct gpio_siwx91x_uulp_data gpio_siwx91x_port_data##idx;                          \
                                                                                                   \
	static int gpio_siwx91x_init_uulp_##idx(const struct device *dev)                          \
	{                                                                                          \
		sys_write32(0, DT_INST_REG_ADDR_BY_NAME(idx, int) + UULP_REG_INTERRUPT_CONFIG);    \
		IRQ_CONNECT(DT_INST_IRQ(idx, irq), DT_INST_IRQ(idx, priority),                     \
			    gpio_siwx91x_uulp_isr, DEVICE_DT_GET(DT_DRV_INST(idx)), 0);            \
		irq_enable(DT_INST_IRQ(idx, irq));                                                 \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, gpio_siwx91x_init_uulp_##idx, NULL,                             \
			      &gpio_siwx91x_port_data##idx, &gpio_siwx91x_port_config##idx,        \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_siwx91x_uulp_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_PORT_INIT)
