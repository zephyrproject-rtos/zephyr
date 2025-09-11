/*
 * Copyright (c) 2025 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tms570_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

/* port registers */
#define REG_DIR		0x0000	/* Data Direction Register */
#define REG_DIN		0x0004	/* Data Input Register */
#define REG_DOUT	0x0008	/* Data Output Register */
#define REG_DSET	0x000C	/* Data Output Set Register */
#define REG_DCLR	0x0010	/* Data Output Clear Register */
#define REG_PDR		0x0014	/* Open Drain Register */
#define REG_PULDIS	0x0018	/* Pullup Disable Register */
#define REG_PSL		0x001C	/* Pull Up/Down Selection Register */

/* GIO base registers */
#define REG_GCR0	0x0000	/* Global Control Register */
#define REG_INTDET	0x0008	/* Interrupt Detect Register*/
#define REG_POL		0x000C	/* Interrupt Polarity Register */
#define REG_ENASET	0x0010	/* Interrupt Enable Set Register */
#define REG_ENACLR	0x0014	/* Interrupt Enable Clear Register */
#define REG_LVLSET	0x0018	/* Interrupt Priority Set Register */
#define REG_LVLCLR	0x001C	/* Interrupt Priority Clear Register */
#define REG_FLG		0x0020	/* Interrupt Flag Register */
#define REG_OFF1	0x0024	/* Interrupt Offset A Register */
#define REG_OFF2	0x0028	/* Interrupt Offset B Register */
#define REG_EMU1	0x002C	/* Emulation 1 Register */
#define REG_EMU2	0x0030	/* Emulation 2 Register */

struct gpio_tms570_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint32_t reg_gio;
	uint32_t reg_port;
};

struct gpio_tms570_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

static int gpio_tms570_set_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tms570_config *config = dev->config;

	sys_write32(pins, config->reg_port + REG_DSET);

	return 0;
}

static int gpio_tms570_clear_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tms570_config *config = dev->config;
	uint32_t val;

	val = sys_read32(config->reg_port + REG_DIN);
	sys_write32(val & pins, config->reg_port + REG_DCLR);

	return 0;
}

static int gpio_tms570_port_set_masked_raw(const struct device *dev,
	gpio_port_pins_t mask,
	gpio_port_value_t value)
{
	const struct gpio_tms570_config *config = dev->config;
	uint32_t cur_out;
	uint32_t cur_dir;
	uint32_t val_set;
	uint32_t val_clr;

	cur_out = sys_read32(config->reg_port + REG_DIN);
	cur_dir = sys_read32(config->reg_port + REG_DIR);
	val_clr = cur_dir & cur_out & ~value & mask;
	val_set = cur_dir & ~cur_out & value & mask;

	sys_write32(val_clr, config->reg_port + REG_DCLR);
	sys_write32(val_set, config->reg_port + REG_DSET);

	return 0;
}

static int gpio_tms570_port_toggle_bits(const struct device *dev,
	gpio_port_pins_t pins)
{
	const struct gpio_tms570_config *config = dev->config;
	uint32_t cur_out;
	uint32_t cur_dir;
	uint32_t val_set;
	uint32_t val_clr;

	cur_out = sys_read32(config->reg_port + REG_DIN);
	cur_dir = sys_read32(config->reg_port + REG_DIR);
	val_clr = cur_dir & cur_out & pins;
	val_set = cur_dir & ~cur_out & pins;
	sys_write32(val_clr, config->reg_port + REG_DCLR);
	sys_write32(val_set, config->reg_port + REG_DSET);

	return 0;
}


static int gpio_tms570_get(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_tms570_config *config = dev->config;

	*value = sys_read32(config->reg_port + REG_DIN);

	return 0;
}

static int gpio_tms570_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_tms570_config *config = dev->config;
	uint32_t current_config;
	int ret;

	/* Read the current configuration of the pins */
	current_config = sys_read32(config->reg_port + REG_DIR);

	/* We only support changes in the direction of the pins */
	if ((flags & GPIO_INPUT) != 0U) {
		/* Pins specified as input will have their DIR register's bit set to 0 */
		sys_write32(current_config & ~BIT(pin), config->reg_port + REG_DIR);
	} else if ((flags & GPIO_OUTPUT) != 0U) {
		/* Pins specified as output will have their DIR register's bit set to 1 */
		sys_write32(current_config | BIT(pin), config->reg_port + REG_DIR);

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			ret = gpio_tms570_set_bits(dev, (gpio_port_pins_t)BIT(pin));
			if (ret < 0) {
				return ret;
			}
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			ret = gpio_tms570_clear_bits(dev, (gpio_port_pins_t)BIT(pin));
			if (ret < 0) {
				return ret;
			}
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int gpio_tms570_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

static int gpio_tms570_manage_callback(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	return -ENOTSUP;
}

static const struct gpio_driver_api gpio_tms570_api = {
	.port_get_raw = gpio_tms570_get,
	.port_set_masked_raw = gpio_tms570_port_set_masked_raw,
	.port_set_bits_raw = gpio_tms570_set_bits,
	.port_clear_bits_raw = gpio_tms570_clear_bits,
	.pin_configure = gpio_tms570_configure,
	.port_toggle_bits = gpio_tms570_port_toggle_bits,
	.pin_interrupt_configure = gpio_tms570_pin_interrupt_configure,
	.manage_callback = gpio_tms570_manage_callback,
};

static int gpio_tms570_init(const struct device *dev)
{
	const struct gpio_tms570_config *config = dev->config;
	static int gpio_tms570_init_done;

	if (gpio_tms570_init_done == 0) {
		gpio_tms570_init_done = 1;
		sys_write32(1, config->reg_gio + REG_GCR0);
		sys_write32(0xFFU, config->reg_gio + REG_ENACLR);
		sys_write32(0xFFU, config->reg_gio + REG_LVLCLR);
	}

	return 0;
}

#define TMS570_GPIO_INIT(n)							\
	static struct gpio_tms570_data gpio_tms570_data_##n = {			\
	};									\
	static struct gpio_tms570_config gpio_tms570_config_##n = {		\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},								\
		.reg_port = DT_INST_REG_ADDR_BY_IDX(n, 0),			\
		.reg_gio = DT_INST_REG_ADDR_BY_IDX(n, 1),			\
	};									\
	DEVICE_DT_INST_DEFINE(n, gpio_tms570_init, NULL,			\
				&gpio_tms570_data_##n, &gpio_tms570_config_##n,	\
				POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,		\
				&gpio_tms570_api);

DT_INST_FOREACH_STATUS_OKAY(TMS570_GPIO_INIT)
