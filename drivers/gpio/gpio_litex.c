/*
 * Copyright (c) 2019-2021 Antmicro <www.antmicro.com>
 * Copyright (c) 2021 Raptor Engineering, LLC <sales@raptorengineering.com>
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include <soc.h>

BUILD_ASSERT(CONFIG_LITEX_CSR_DATA_WIDTH == 32, "CONFIG_LITEX_CSR_DATA_WIDTH must be 32 bits");

#define GPIO_LITEX_ALL_HAS_IRQ DT_ALL_INST_HAS_PROP_STATUS_OKAY(interrupts)
#define GPIO_LITEX_ANY_HAS_IRQ DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)

#define GPIO_LITEX_ALL_HAS_OE DT_ALL_INST_REG_HAS_NAME_STATUS_OKAY(oe)
#define GPIO_LITEX_ANY_HAS_OE DT_ANY_INST_REG_HAS_NAME_STATUS_OKAY(oe)

#define GPIO_LITEX_ALL_HAS_OUT DT_ALL_INST_REG_HAS_NAME_STATUS_OKAY(out)
#define GPIO_LITEX_ANY_HAS_OUT DT_ANY_INST_REG_HAS_NAME_STATUS_OKAY(out)

#define GPIO_LITEX_ALL_HAS_IN DT_ALL_INST_REG_HAS_NAME_STATUS_OKAY(in)
#define GPIO_LITEX_ANY_HAS_IN DT_ANY_INST_REG_HAS_NAME_STATUS_OKAY(in)

#define GPIO_LITEX_HAS_OE(cfg)                                                                     \
	(GPIO_LITEX_ANY_HAS_OE && (GPIO_LITEX_ALL_HAS_OE || ((cfg)->oe_addr != 0)))
#define GPIO_LITEX_HAS_OUT(cfg)                                                                    \
	(GPIO_LITEX_ANY_HAS_OUT && (GPIO_LITEX_ALL_HAS_OUT || ((cfg)->out_addr != 0)))
#define GPIO_LITEX_HAS_IN(cfg)                                                                     \
	(GPIO_LITEX_ANY_HAS_IN && (GPIO_LITEX_ALL_HAS_IN || ((cfg)->in_addr != 0)))

#include <zephyr/drivers/gpio/gpio_utils.h>

#define SUPPORTED_FLAGS (GPIO_INPUT | GPIO_OUTPUT | \
			GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH | \
			GPIO_ACTIVE_LOW | GPIO_ACTIVE_HIGH)

#define GPIO_LOW        0
#define GPIO_HIGH       1

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
LOG_MODULE_REGISTER(gpio_litex);

struct gpio_litex_cfg {
	struct gpio_driver_config common;
	mem_addr_t oe_addr;
	mem_addr_t in_addr;
	mem_addr_t out_addr;
	mem_addr_t ev_pending_addr;
	mem_addr_t ev_enable_addr;
	mem_addr_t ev_mode_addr;
	mem_addr_t ev_edge_addr;
};

struct gpio_litex_data {
	struct gpio_driver_data common;
	const struct device *dev;
	sys_slist_t cb;
};

/* Helper macros for GPIO */

#define DEV_GPIO_CFG(dev)						\
	((const struct gpio_litex_cfg *)(dev)->config)

/* Helper functions for bit / port access */

static inline void set_bit(mem_addr_t reg_addr, uint32_t bit, bool val)
{
	uint32_t reg = litex_read32(reg_addr);

	WRITE_BIT(reg, bit, val);

	litex_write32(reg, reg_addr);
}

static inline uint32_t get_bit(mem_addr_t reg_addr, int reg_size, uint32_t bit)
{
	uint32_t reg = litex_read32(reg_addr);

	return IS_BIT_SET(reg, bit);
}

static inline uint32_t get_port_out(const struct gpio_litex_cfg *gpio_config)
{
	return (litex_read32(gpio_config->out_addr) & gpio_config->common.port_pin_mask);
}

static inline void set_oe(const struct gpio_litex_cfg *gpio_config, gpio_pin_t pin, bool val)
{
	if (GPIO_LITEX_HAS_OE(gpio_config)) {
		set_bit(gpio_config->oe_addr, pin, 1);
	}
}

/* Driver functions */

static int gpio_litex_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);

	if (flags & ~SUPPORTED_FLAGS) {
		return -ENOTSUP;
	}

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_OUTPUT:
		if (!GPIO_LITEX_HAS_OUT(gpio_config)) {
			return -EINVAL;
		}

		set_oe(gpio_config, pin, true);

		if (flags & (GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOW)) {
			set_bit(gpio_config->out_addr, pin, (flags & GPIO_OUTPUT_INIT_HIGH) != 0);
		}

		break;
	case GPIO_INPUT:
		if (!GPIO_LITEX_HAS_IN(gpio_config)) {
			return -EINVAL;
		}

		set_oe(gpio_config, pin, false);

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_litex_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);

	*value = GPIO_LITEX_HAS_IN(gpio_config)
			 ? (litex_read32(gpio_config->in_addr) & gpio_config->common.port_pin_mask)
			 : 0;

	return 0;
}

static int gpio_litex_port_set_masked_raw(const struct device *dev,
					  gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	if (GPIO_LITEX_HAS_OUT(gpio_config)) {
		port_val = get_port_out(gpio_config);
		port_val = (port_val & ~mask) | (value & mask);
		litex_write32(port_val, gpio_config->out_addr);
	}

	return 0;
}

static int gpio_litex_port_set_bits_raw(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	if (GPIO_LITEX_HAS_OUT(gpio_config)) {
		port_val = get_port_out(gpio_config);
		port_val |= pins;
		litex_write32(port_val, gpio_config->out_addr);
	}

	return 0;
}

static int gpio_litex_port_clear_bits_raw(const struct device *dev,
					  gpio_port_pins_t pins)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	if (GPIO_LITEX_HAS_OUT(gpio_config)) {
		port_val = get_port_out(gpio_config);
		port_val &= ~pins;
		litex_write32(port_val, gpio_config->out_addr);
	}
	return 0;
}

static int gpio_litex_port_toggle_bits(const struct device *dev,
				       gpio_port_pins_t pins)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	if (GPIO_LITEX_HAS_OUT(gpio_config)) {
		port_val = get_port_out(gpio_config);
		port_val ^= pins;
		litex_write32(port_val, gpio_config->out_addr);
	}

	return 0;
}

#if GPIO_LITEX_ANY_HAS_IRQ
static void gpio_litex_irq_handler(const struct device *dev)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	struct gpio_litex_data *data = dev->data;

	uint32_t int_status = litex_read32(gpio_config->ev_pending_addr);
	uint32_t ev_enabled = litex_read32(gpio_config->ev_enable_addr);

	/* clear events */
	litex_write32(int_status, gpio_config->ev_pending_addr);

	gpio_fire_callbacks(&data->cb, dev, int_status & ev_enabled);
}

static int gpio_litex_manage_callback(const struct device *dev,
				      struct gpio_callback *callback, bool set)
{
	struct gpio_litex_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static int gpio_litex_pin_interrupt_configure(const struct device *dev,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t ev_enabled, ev_mode, ev_edge;

	if (!GPIO_LITEX_HAS_IN(gpio_config) ||
	    !(GPIO_LITEX_ALL_HAS_IRQ || gpio_config->ev_enable_addr != 0)) {
		return -ENOTSUP;
	}

	if (GPIO_LITEX_HAS_OE(gpio_config) &&
	    IS_BIT_SET(litex_read32(gpio_config->oe_addr), pin)) {
		return -EINVAL;
	}

	ev_enabled = litex_read32(gpio_config->ev_enable_addr);

	if (mode == GPIO_INT_MODE_EDGE) {
		ev_mode = litex_read32(gpio_config->ev_mode_addr);
		ev_edge = litex_read32(gpio_config->ev_edge_addr);

		litex_write32(ev_enabled | BIT(pin), gpio_config->ev_enable_addr);

		WRITE_BIT(ev_mode, pin, (trig == GPIO_INT_TRIG_BOTH));

		litex_write32(ev_mode, gpio_config->ev_mode_addr);

		switch (trig & GPIO_INT_TRIG_BOTH) {
		case GPIO_INT_TRIG_HIGH:
			/* Rising edge */
			litex_write32(ev_edge & ~BIT(pin), gpio_config->ev_edge_addr);
			break;
		case GPIO_INT_TRIG_LOW:
			/* Falling edge */
			litex_write32(ev_edge | BIT(pin), gpio_config->ev_edge_addr);
			break;
		default:
			break;
		}
		return 0;
	}

	if (mode == GPIO_INT_DISABLE) {
		litex_write32(ev_enabled & ~BIT(pin), gpio_config->ev_enable_addr);
		return 0;
	}

	return -ENOTSUP;
}
#endif /* GPIO_LITEX_ANY_HAS_IRQ */

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_litex_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					 gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_litex_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t oe_reg;

	map &= gpio_config->common.port_pin_mask;

	if (!GPIO_LITEX_HAS_OE(gpio_config)) {
		oe_reg = GPIO_LITEX_HAS_OUT(gpio_config) ? UINT32_MAX : 0;
	} else {
		oe_reg = litex_read32(gpio_config->oe_addr);
	}

	if (inputs != NULL) {
		*inputs = map & ~oe_reg;
	}

	if (outputs != NULL) {
		*outputs = map & oe_reg;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static DEVICE_API(gpio, gpio_litex_driver_api) = {
	.pin_configure = gpio_litex_configure,
	.port_get_raw = gpio_litex_port_get_raw,
	.port_set_masked_raw = gpio_litex_port_set_masked_raw,
	.port_set_bits_raw = gpio_litex_port_set_bits_raw,
	.port_clear_bits_raw = gpio_litex_port_clear_bits_raw,
	.port_toggle_bits = gpio_litex_port_toggle_bits,
#if GPIO_LITEX_ANY_HAS_IRQ
	.pin_interrupt_configure = gpio_litex_pin_interrupt_configure,
	.manage_callback = gpio_litex_manage_callback,
#endif /* GPIO_LITEX_ANY_HAS_IRQ */
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_litex_port_get_direction,
#endif /* CONFIG_GPIO_GET_DIRECTION */
};

/* Device Instantiation */
#define GPIO_LITEX_IRQ(n)                                                                          \
	BUILD_ASSERT(DT_INST_REG_HAS_NAME(n, mode) &&						   \
		     DT_INST_REG_HAS_NAME(n, edge) &&						   \
		     DT_INST_REG_HAS_NAME(n, ev_pending) &&					   \
		     DT_INST_REG_HAS_NAME(n, ev_enable),					   \
		     "registers for interrupts missing");                                          \
                                                                                                   \
	static int gpio_litex_port_init_##n(const struct device *dev)                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), gpio_litex_irq_handler,     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
												   \
		return 0;                                                                          \
	}

#define GPIO_LITEX_INIT(n)                                                                         \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0), (GPIO_LITEX_IRQ(n)))				   \
                                                                                                   \
	BUILD_ASSERT(DT_INST_REG_HAS_NAME(n, in) || DT_INST_REG_HAS_NAME(n, out),                  \
		     "Either 'in' or 'out' register must be defined");                             \
	BUILD_ASSERT((DT_INST_REG_HAS_NAME(n, in) && DT_INST_REG_HAS_NAME(n, out)) ==              \
			     DT_INST_REG_HAS_NAME(n, oe),                                          \
		     "oe' register is needed, if both 'in' and 'out' registers are defined");      \
	BUILD_ASSERT(DT_INST_PROP(n, ngpios) <= 32,						   \
		     "Number of gpios exceeds what can be handled");				   \
                                                                                                   \
	static const struct gpio_litex_cfg gpio_litex_cfg_##n = {                                  \
		.common = { .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n) },                 \
		.oe_addr = DT_INST_REG_ADDR_BY_NAME_OR(n, oe, 0),                                  \
		.in_addr = DT_INST_REG_ADDR_BY_NAME_OR(n, in, 0),                                  \
		.out_addr = DT_INST_REG_ADDR_BY_NAME_OR(n, out, 0),                                \
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0), (						   \
			.ev_mode_addr    = DT_INST_REG_ADDR_BY_NAME(n, mode),			   \
			.ev_edge_addr    = DT_INST_REG_ADDR_BY_NAME(n, edge),			   \
			.ev_pending_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_pending),		   \
			.ev_enable_addr  = DT_INST_REG_ADDR_BY_NAME(n, ev_enable),		   \
		)) };										   \
                                                                                                   \
	static struct gpio_litex_data gpio_litex_data_##n;					   \
												   \
	DEVICE_DT_INST_DEFINE(n, COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0),				   \
			      (gpio_litex_port_init_##n), (NULL)), NULL, &gpio_litex_data_##n,     \
			      &gpio_litex_cfg_##n, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,         \
			      &gpio_litex_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_LITEX_INIT)
