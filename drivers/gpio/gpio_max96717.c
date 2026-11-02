/*
 * Copyright (c) 2026 Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * GPIO controller of the MAX96717 serializer: its MFP pins, reached through the GMSL2 link.
 *
 * The node is a child of the serializer, which brings the link up before this controller
 * initializes. Every pin operation is one or two register accesses over the link. GPIO forwarding
 * over the link and interrupts are not supported.
 */

#define DT_DRV_COMPAT adi_max96717_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(gpio_max96717, CONFIG_GPIO_LOG_LEVEL);

#define MAX96717_GPIO_COUNT 11

/* GPIO_A: direction, level and link forwarding of one MFP. */
#define MAX96717_REG_GPIO_A(n)  (0x02BE + (n) * 3)
#define MAX96717_GPIO_A_OUT_DIS BIT(0) /* 1: output driver off */
#define MAX96717_GPIO_A_TX_EN   BIT(1) /* forward the pin level over the link */
#define MAX96717_GPIO_A_RX_EN   BIT(2) /* drive the pin from the link */
#define MAX96717_GPIO_A_IN      BIT(3) /* pin level, read-only */
#define MAX96717_GPIO_A_OUT     BIT(4) /* output level while RX_EN is 0 */

/* GPIO_B: output type and pull resistor of one MFP. */
#define MAX96717_REG_GPIO_B(n)    (0x02BF + (n) * 3)
#define MAX96717_GPIO_B_OUT_TYPE  BIT(5) /* 1: push-pull, 0: open-drain */
#define MAX96717_GPIO_B_PULL_MASK GENMASK(7, 6)
#define MAX96717_GPIO_B_PULL_NONE 0
#define MAX96717_GPIO_B_PULL_UP   1
#define MAX96717_GPIO_B_PULL_DOWN 2

struct gpio_max96717_config {
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
	/* The serializer whose pins these are. */
	const struct device *owner;
};

struct gpio_max96717_data {
	struct gpio_driver_data common;
};

/* The registers are 8-bit, at 16-bit addresses. Transfer failures are logged here. */
static int gpio_max96717_read(const struct i2c_dt_spec *i2c, uint16_t reg, uint8_t *val)
{
	const uint8_t addr[] = {reg >> 8, reg & 0xFF};
	int ret;

	ret = i2c_write_read_dt(i2c, addr, sizeof(addr), val, sizeof(*val));
	if (ret < 0) {
		LOG_ERR("Failed to read register 0x%04x from %s@0x%02x (err %d)", reg,
			i2c->bus->name, i2c->addr, ret);
	}

	return ret;
}

static int gpio_max96717_write(const struct i2c_dt_spec *i2c, uint16_t reg, uint8_t val)
{
	const uint8_t buf[] = {reg >> 8, reg & 0xFF, val};
	int ret;

	ret = i2c_write_dt(i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to write register 0x%04x to %s@0x%02x (err %d)", reg,
			i2c->bus->name, i2c->addr, ret);
	}

	return ret;
}

static int gpio_max96717_update(const struct i2c_dt_spec *i2c, uint16_t reg, uint8_t mask,
				uint8_t val)
{
	uint8_t reg_val;
	int ret;

	ret = gpio_max96717_read(i2c, reg, &reg_val);
	if (ret < 0) {
		return ret;
	}

	return gpio_max96717_write(i2c, reg, (reg_val & ~mask) | (val & mask));
}

static int gpio_max96717_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_max96717_config *cfg = dev->config;
	uint8_t a_mask = MAX96717_GPIO_A_OUT_DIS | MAX96717_GPIO_A_TX_EN | MAX96717_GPIO_A_RX_EN;
	uint8_t a_val = 0;
	uint8_t b_val = 0;
	int ret;

	if (!gpio_port_pin_is_supported(cfg->common.port_pin_mask, pin)) {
		return -EINVAL;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0 && (flags & GPIO_LINE_OPEN_DRAIN) == 0) {
		/* No open-source output. */
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			a_mask |= MAX96717_GPIO_A_OUT;
			a_val |= MAX96717_GPIO_A_OUT;
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			a_mask |= MAX96717_GPIO_A_OUT;
		}
	} else {
		/* Input or disconnected: output driver off. */
		a_val |= MAX96717_GPIO_A_OUT_DIS;
	}

	if ((flags & GPIO_SINGLE_ENDED) == 0) {
		b_val |= MAX96717_GPIO_B_OUT_TYPE;
	}
	if ((flags & GPIO_PULL_UP) != 0) {
		b_val |= FIELD_PREP(MAX96717_GPIO_B_PULL_MASK, MAX96717_GPIO_B_PULL_UP);
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		b_val |= FIELD_PREP(MAX96717_GPIO_B_PULL_MASK, MAX96717_GPIO_B_PULL_DOWN);
	}

	ret = gpio_max96717_update(&cfg->i2c, MAX96717_REG_GPIO_B(pin),
				   MAX96717_GPIO_B_OUT_TYPE | MAX96717_GPIO_B_PULL_MASK, b_val);
	if (ret < 0) {
		return ret;
	}

	return gpio_max96717_update(&cfg->i2c, MAX96717_REG_GPIO_A(pin), a_mask, a_val);
}

static int gpio_max96717_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_max96717_config *cfg = dev->config;
	uint8_t reg;
	int ret;

	*value = 0;

	for (gpio_pin_t pin = 0; pin < MAX96717_GPIO_COUNT; pin++) {
		if ((cfg->common.port_pin_mask & BIT(pin)) == 0) {
			continue;
		}

		ret = gpio_max96717_read(&cfg->i2c, MAX96717_REG_GPIO_A(pin), &reg);
		if (ret < 0) {
			return ret;
		}

		if ((reg & MAX96717_GPIO_A_IN) != 0) {
			*value |= BIT(pin);
		}
	}

	return 0;
}

static int gpio_max96717_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	const struct gpio_max96717_config *cfg = dev->config;
	int ret;

	mask &= cfg->common.port_pin_mask;

	for (gpio_pin_t pin = 0; pin < MAX96717_GPIO_COUNT; pin++) {
		if ((mask & BIT(pin)) == 0) {
			continue;
		}

		ret = gpio_max96717_update(&cfg->i2c, MAX96717_REG_GPIO_A(pin), MAX96717_GPIO_A_OUT,
					   (value & BIT(pin)) != 0 ? MAX96717_GPIO_A_OUT : 0);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int gpio_max96717_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_max96717_port_set_masked_raw(dev, pins, pins);
}

static int gpio_max96717_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_max96717_port_set_masked_raw(dev, pins, 0);
}

static int gpio_max96717_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_max96717_config *cfg = dev->config;
	uint8_t reg;
	int ret;

	pins &= cfg->common.port_pin_mask;

	for (gpio_pin_t pin = 0; pin < MAX96717_GPIO_COUNT; pin++) {
		if ((pins & BIT(pin)) == 0) {
			continue;
		}

		ret = gpio_max96717_read(&cfg->i2c, MAX96717_REG_GPIO_A(pin), &reg);
		if (ret < 0) {
			return ret;
		}

		ret = gpio_max96717_write(&cfg->i2c, MAX96717_REG_GPIO_A(pin),
					  reg ^ MAX96717_GPIO_A_OUT);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int gpio_max96717_init(const struct device *dev)
{
	const struct gpio_max96717_config *cfg = dev->config;

	if (!device_is_ready(cfg->owner)) {
		LOG_ERR("%s: %s not ready", dev->name, cfg->owner->name);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(gpio, gpio_max96717_api) = {
	.pin_configure = gpio_max96717_pin_configure,
	.port_get_raw = gpio_max96717_port_get_raw,
	.port_set_masked_raw = gpio_max96717_port_set_masked_raw,
	.port_set_bits_raw = gpio_max96717_port_set_bits_raw,
	.port_clear_bits_raw = gpio_max96717_port_clear_bits_raw,
	.port_toggle_bits = gpio_max96717_port_toggle_bits,
};

BUILD_ASSERT(CONFIG_GPIO_MAX96717_INIT_PRIORITY > CONFIG_VIDEO_MAX96717_INIT_PRIORITY,
	     "the MAX96717 GPIO controller must initialize after the serializer");
BUILD_ASSERT(CONFIG_GPIO_MAX96717_INIT_PRIORITY < CONFIG_VIDEO_INIT_PRIORITY,
	     "the MAX96717 GPIO controller must initialize before the camera sensors");

#define GPIO_MAX96717_DEFINE(inst)                                                                 \
	static const struct gpio_max96717_config gpio_max96717_cfg_##inst = {                      \
		.common = {.port_pin_mask = GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(                 \
				   inst, DT_INST_PROP(inst, ngpios))},                             \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.owner = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                      \
	};                                                                                         \
	static struct gpio_max96717_data gpio_max96717_data_##inst;                                \
	DEVICE_DT_INST_DEFINE(inst, gpio_max96717_init, NULL, &gpio_max96717_data_##inst,          \
			      &gpio_max96717_cfg_##inst, POST_KERNEL,                              \
			      CONFIG_GPIO_MAX96717_INIT_PRIORITY, &gpio_max96717_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MAX96717_DEFINE)
