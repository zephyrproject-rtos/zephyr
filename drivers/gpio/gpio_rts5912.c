/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include "zephyr/drivers/gpio/gpio_utils.h"
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/gpio/realtek-gpio.h>

#include <reg/reg_gpio.h>

LOG_MODULE_REGISTER(gpio_rts5912, CONFIG_GPIO_LOG_LEVEL);

#define RTS5912_GPIOA_REG_BASE ((GPIO_Type *)(DT_REG_ADDR(DT_NODELABEL(gpioa))))

struct gpio_rts5912_config {
	struct gpio_driver_config common;
	volatile uint32_t *reg_base;
	uint8_t num_pins;
};

struct gpio_rts5912_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static int pin_is_valid(const struct gpio_rts5912_config *config, gpio_pin_t pin)
{
	if (pin >= config->num_pins) {
		return -EINVAL;
	}

	return 0;
}

static int pin_output_high(const struct device *port, gpio_pin_t pin)
{
	const struct gpio_rts5912_config *config = port->config;
	volatile uint32_t *gcr = &config->reg_base[pin];

	int err = pin_is_valid(config, pin);

	if (err) {
		return err;
	}

	if (*gcr & GPIO_GCR_OUTMD_Msk) {
		/* Switch I/O mode to input mode when configuration is open-drain with output high
		 */
		*gcr = (*gcr & ~GPIO_GCR_DIR_Msk) | GPIO_GCR_OUTCTRL_Msk;
	} else {
		*gcr |= GPIO_GCR_OUTCTRL_Msk | GPIO_GCR_DIR_Msk;
	}

	return 0;
}

static int pin_output_low(const struct device *port, gpio_pin_t pin)
{
	const struct gpio_rts5912_config *config = port->config;
	volatile uint32_t *gcr = &config->reg_base[pin];

	int err = pin_is_valid(config, pin);

	if (err) {
		return err;
	}

	*gcr = (*gcr & ~GPIO_GCR_OUTCTRL_Msk) | GPIO_GCR_DIR_Msk;

	return 0;
}

static int gpio_rts5912_configuration(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_rts5912_config *config = port->config;
	volatile uint32_t *gcr = &config->reg_base[pin];
	uint32_t cfg_val = *gcr;

	int err = pin_is_valid(config, pin);

	if (err) {
		return err;
	}

	if (flags & GPIO_INPUT) {
		cfg_val &= ~GPIO_GCR_DIR_Msk;
		cfg_val &= ~GPIO_GCR_OUTCTRL_Msk;
		cfg_val |= GPIO_GCR_INDETEN_Msk;
	}

	if (flags & GPIO_OPEN_DRAIN) {
		cfg_val |= GPIO_GCR_OUTMD_Msk;
	} else {
		cfg_val &= ~GPIO_GCR_OUTMD_Msk;
	}

	switch (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
	case GPIO_PULL_UP:
		cfg_val &= ~GPIO_GCR_PULLDWEN_Msk;
		cfg_val |= GPIO_GCR_PULLUPEN_Msk;
		break;
	case GPIO_PULL_DOWN:
		cfg_val &= ~GPIO_GCR_PULLUPEN_Msk;
		cfg_val |= GPIO_GCR_PULLDWEN_Msk;
		break;
	default:
		break;
	}

	switch (flags & RTS5912_GPIO_VOLTAGE_MASK) {
	case RTS5912_GPIO_VOLTAGE_1V8:
		cfg_val |= GPIO_GCR_INVOLMD_Msk;
		break;
	case RTS5912_GPIO_VOLTAGE_DEFAULT:
	case RTS5912_GPIO_VOLTAGE_3V3:
		cfg_val &= ~GPIO_GCR_INVOLMD_Msk;
		break;
	case RTS5912_GPIO_VOLTAGE_5V0:
		return -ENOTSUP;
	default:
		break;
	}

	*gcr = cfg_val;

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			pin_output_high(port, pin);
		} else {
			pin_output_low(port, pin);
		}
	}

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_rts5912_get_configuration(const struct device *port, gpio_pin_t pin,
					  gpio_flags_t *flags)
{
	const struct gpio_rts5912_config *config = port->config;
	volatile uint32_t *gcr = &config->reg_base[pin];
	gpio_flags_t cfg_flag = 0x0UL;

	int err = pin_is_valid(config, pin);

	if (err) {
		return err;
	}

	if (*gcr & GPIO_GCR_OUTCTRL_Msk) {
		cfg_flag |= GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH;
	} else {
		if (*gcr & GPIO_GCR_DIR_Msk) {
			cfg_flag |= GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW;
		} else {
			cfg_flag |= GPIO_INPUT;
			if (*gcr & GPIO_GCR_INVOLMD_Msk) {
				cfg_flag |= RTS5912_GPIO_VOLTAGE_1V8;
			} else {
				cfg_flag |= RTS5912_GPIO_VOLTAGE_3V3;
			}
		}
	}

	if (*gcr & GPIO_GCR_OUTMD_Msk) {
		cfg_flag |= GPIO_OPEN_DRAIN;
	}

	if (*gcr & GPIO_GCR_PULLUPEN_Msk) {
		cfg_flag |= GPIO_PULL_UP;
	} else if (*gcr & GPIO_GCR_PULLDWEN_Msk) {
		cfg_flag |= GPIO_PULL_DOWN;
	}

	*flags = cfg_flag;

	return 0;
}
#endif

static int gpio_rts5912_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_rts5912_config *config = port->config;
	gpio_port_value_t ret_val = 0;
	uint16_t mask = 0x1U;

	for (gpio_pin_t i = 0; i < config->num_pins; i++) {
		if (config->reg_base[i] & GPIO_GCR_PINSTS_Msk) {
			ret_val |= (gpio_port_value_t)mask;
		}
		mask <<= 1;
	}

	*value = ret_val;

	return 0;
}

static int gpio_rts5912_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_rts5912_config *config = port->config;
	uint32_t pin;

	mask &= 0x0000FFFF;
	for (; mask; mask &= ~BIT(pin)) {
		pin = find_lsb_set(mask) - 1;
		if (pin >= config->num_pins) {
			break;
		}

		if (value & BIT(pin)) {
			pin_output_high(port, pin);
		} else {
			pin_output_low(port, pin);
		}
	}

	return 0;
}

static int gpio_rts5912_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_rts5912_config *config = port->config;
	volatile uint32_t *gcr = config->reg_base;
	uint32_t pin = 0;

	pins &= 0x0000FFFF;
	gpio_port_pins_t sel_pin = 1;

	for (; pins;) {
		if (pins & sel_pin) {
			pin_output_high(port, pin);
		}
		pins &= ~sel_pin;
		sel_pin <<= 1;
		gcr++;
		pin++;
	}

	return 0;
}

static int gpio_rts5912_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_rts5912_config *config = port->config;
	volatile uint32_t *gcr = config->reg_base;
	uint32_t pin = 0;

	pins &= 0x0000FFFF;
	gpio_port_pins_t sel_pin = 1;

	for (; pins;) {
		if (pins & sel_pin) {
			pin_output_low(port, pin);
		}
		pins &= ~sel_pin;
		sel_pin <<= 1;
		gcr++;
		pin++;
	}

	return 0;
}

static int gpio_rts5912_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_rts5912_config *config = port->config;
	volatile uint32_t *gcr = config->reg_base;
	uint32_t pin = 0;

	pins &= 0x0000FFFF;
	gpio_port_pins_t sel_pin = 0x1UL;

	for (; pins;) {
		if (pins & sel_pin) {
			if (*gcr & GPIO_GCR_OUTCTRL_Msk) {
				pin_output_low(port, pin);
			} else {
				pin_output_high(port, pin);
			}
		}

		pins &= ~sel_pin;
		sel_pin <<= 1;
		gcr++;
		pin++;
	}

	return 0;
}

static gpio_pin_t gpio_rts5912_get_intr_pin(volatile uint32_t *reg_base)
{
	gpio_pin_t pin = 0U;

	for (; pin < 16; pin++) {
		if (reg_base[pin] & GPIO_GCR_INTSTS_Msk) {
			break;
		}
	}

	return pin;
}

static void gpio_rts5912_isr(const void *arg)
{
	const struct device *port = arg;
	const struct gpio_rts5912_config *config = port->config;
	struct gpio_rts5912_data *data = port->data;
	volatile uint32_t *gcr = config->reg_base;
	unsigned int key = irq_lock();
	gpio_pin_t pin = gpio_rts5912_get_intr_pin(gcr);

	if (gcr[pin] & GPIO_GCR_INTSTS_Msk) {
		gcr[pin] |= GPIO_GCR_INTSTS_Msk;

		gpio_fire_callbacks(&data->callbacks, port, BIT(pin));
	}
	irq_unlock(key);
}

static int gpio_rts5912_intr_config(const struct device *port, gpio_pin_t pin,
				    enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_rts5912_config *config = port->config;
	volatile uint32_t *gcr = &config->reg_base[pin];
	uint32_t cfg_val = *gcr;
	uint32_t pin_index =
		DT_IRQ_BY_IDX(DT_NODELABEL(gpioa), 0, irq) +
		((uint32_t)(&config->reg_base[pin]) - (uint32_t)(RTS5912_GPIOA_REG_BASE)) / 4;

	int err = pin_is_valid(config, pin);

	if (err) {
		return err;
	}

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		cfg_val &= ~GPIO_GCR_INTEN_Msk;
		irq_disable(pin_index);
		*gcr = cfg_val;
		return 0;
	case GPIO_INT_MODE_LEVEL:
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			cfg_val &= ~GPIO_GCR_INTCTRL_Msk;
			cfg_val |= 0x03UL << GPIO_GCR_INTCTRL_Pos;
			break;
		case GPIO_INT_TRIG_HIGH:
			cfg_val &= ~GPIO_GCR_INTCTRL_Msk;
			cfg_val |= 0x04UL << GPIO_GCR_INTCTRL_Pos;
			break;
		default:
			return -EINVAL;
		}
		break;
	case GPIO_INT_MODE_EDGE:
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			cfg_val &= ~GPIO_GCR_INTCTRL_Msk;
			cfg_val |= 0x01UL << GPIO_GCR_INTCTRL_Pos;
			break;
		case GPIO_INT_TRIG_HIGH:
			cfg_val &= ~GPIO_GCR_INTCTRL_Msk;
			cfg_val |= 0x00UL << GPIO_GCR_INTCTRL_Pos;
			break;
		case GPIO_INT_TRIG_BOTH:
			cfg_val &= ~GPIO_GCR_INTCTRL_Msk;
			cfg_val |= 0x2UL << GPIO_GCR_INTCTRL_Pos;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	cfg_val |= GPIO_GCR_INTEN_Msk;
	*gcr = cfg_val;

	irq_enable(pin_index);

	return 0;
}

static int gpio_rts5912_manage_cb(const struct device *port, struct gpio_callback *cb, bool set)
{
	struct gpio_rts5912_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}

static DEVICE_API(gpio, gpio_rts5912_driver_api) = {
	.pin_configure = gpio_rts5912_configuration,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_rts5912_get_configuration,
#endif
	.port_get_raw = gpio_rts5912_port_get_raw,
	.port_set_masked_raw = gpio_rts5912_port_set_masked_raw,
	.port_set_bits_raw = gpio_rts5912_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rts5912_port_clear_bits_raw,
	.port_toggle_bits = gpio_rts5912_port_toggle_bits,
	.pin_interrupt_configure = gpio_rts5912_intr_config,
	.manage_callback = gpio_rts5912_manage_cb,
};

#ifdef CONFIG_GEN_ISR_TABLES
#define RTS5912_GPIO_DTNAMIC_IRQ(id)                                                               \
	for (int i = 0; i < 16 && (DT_INST_IRQ_BY_IDX(id, 0, irq) + i) < 132; i++) {               \
		irq_connect_dynamic((DT_INST_IRQ_BY_IDX(id, 0, irq) + i),                          \
				    DT_INST_IRQ(id, priority), gpio_rts5912_isr,                   \
				    DEVICE_DT_INST_GET(id), 0U);                                   \
	}
#else
#define RTS5912_GPIO_DTNAMIC_IRQ(id)                                                               \
	IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), gpio_rts5912_isr,                 \
		    DEVICE_DT_INST_GET(id), 0U);
#endif

#define GPIO_RTS5912_INIT(id)                                                                      \
	static int gpio_rts5912_init_##id(const struct device *dev)                                \
	{                                                                                          \
		if (!(DT_INST_IRQ_HAS_CELL(id, irq))) {                                            \
			return 0;                                                                  \
		}                                                                                  \
                                                                                                   \
		RTS5912_GPIO_DTNAMIC_IRQ(id)                                                       \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static struct gpio_rts5912_data gpio_rts5912_data_##id;                                    \
                                                                                                   \
	static const struct gpio_rts5912_config gpio_rts5912_config_##id = {                       \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(id)},                  \
		.reg_base = (volatile uint32_t *)DT_INST_REG_ADDR(id),                             \
		.num_pins = DT_INST_PROP(id, ngpios),                                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, gpio_rts5912_init_##id, NULL, &gpio_rts5912_data_##id,           \
			      &gpio_rts5912_config_##id, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,   \
			      &gpio_rts5912_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RTS5912_INIT)
