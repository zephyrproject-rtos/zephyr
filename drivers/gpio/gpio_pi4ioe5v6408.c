/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT diodes_pi4ioe5v6408

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pi4ioe5v6408, CONFIG_GPIO_LOG_LEVEL);

#include <zephyr/drivers/gpio/gpio_utils.h>

#define REG_DEVICE_ID        0x01
#define REG_IO_DIR           0x03
#define REG_OUT_SET          0x05
#define REG_OUT_HIZ          0x07
#define REG_IN_DEFAULT_STATE 0x09
#define REG_PULL_ENABLE      0x0B
#define REG_PULL_SELECT      0x0D
#define REG_IN_STATE         0x0F
#define REG_INT_MASK         0x11
#define REG_INT_STATUS       0x13

#define DEVICE_ID_SW_RESET BIT(0)

#define NUM_PINS 8U
#define ALL_PINS ((uint8_t)BIT_MASK(NUM_PINS))

struct pi4ioe5v6408_pin_state {
	uint8_t dir;
	uint8_t hiz;
	uint8_t output;
	uint8_t pull_enable;
	uint8_t pull_select;
};

struct pi4ioe5v6408_irq_state {
	uint8_t rising;
	uint8_t falling;
	uint8_t last_input;
};

struct pi4ioe5v6408_data {
	struct gpio_driver_data common;
	struct pi4ioe5v6408_pin_state pin_state;
	struct pi4ioe5v6408_irq_state irq_state;
	struct k_sem lock;
	struct gpio_callback int_cb;
	struct k_work work;
	const struct device *dev;
	sys_slist_t callbacks;
};

struct pi4ioe5v6408_config {
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
	bool interrupt_enabled;
};

static void pi4ioe5v6408_handle_interrupt(const struct device *dev)
{
	const struct pi4ioe5v6408_config *cfg = dev->config;
	struct pi4ioe5v6408_data *data = dev->data;
	struct pi4ioe5v6408_irq_state *irq = &data->irq_state;
	uint8_t prev, curr, transitioned;
	uint8_t fired = 0;
	uint8_t status;
	int rc;

	k_sem_take(&data->lock, K_FOREVER);

	if (!irq->rising && !irq->falling) {
		k_sem_give(&data->lock);
		return;
	}

	prev = irq->last_input;
	rc = i2c_reg_read_byte_dt(&cfg->i2c, REG_IN_STATE, &curr);
	if (rc) {
		k_sem_give(&data->lock);
		return;
	}
	irq->last_input = curr;
	transitioned = prev ^ curr;

	fired = irq->rising & transitioned & curr;
	fired |= irq->falling & transitioned & prev;

	(void)i2c_reg_read_byte_dt(&cfg->i2c, REG_INT_STATUS, &status);

	k_sem_give(&data->lock);

	if (fired) {
		gpio_fire_callbacks(&data->callbacks, dev, fired);
	}
}

static void pi4ioe5v6408_work_handler(struct k_work *work)
{
	struct pi4ioe5v6408_data *data = CONTAINER_OF(work, struct pi4ioe5v6408_data, work);

	pi4ioe5v6408_handle_interrupt(data->dev);
}

static void pi4ioe5v6408_int_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct pi4ioe5v6408_data *data = CONTAINER_OF(cb, struct pi4ioe5v6408_data, int_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}

static int pi4ioe5v6408_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct pi4ioe5v6408_config *cfg = dev->config;
	struct pi4ioe5v6408_data *data = dev->data;
	struct pi4ioe5v6408_pin_state *st = &data->pin_state;
	int rc = 0;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}
	if ((flags & GPIO_INPUT) != 0 && (flags & GPIO_OUTPUT) != 0) {
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if ((flags & GPIO_OUTPUT) != 0) {
		st->dir |= BIT(pin);
		st->hiz &= ~BIT(pin);
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			st->output |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			st->output &= ~BIT(pin);
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		st->dir &= ~BIT(pin);
	} else {
		rc = -ENOTSUP;
		goto out;
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		st->pull_enable |= BIT(pin);
		st->pull_select |= BIT(pin);
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		st->pull_enable |= BIT(pin);
		st->pull_select &= ~BIT(pin);
	} else {
		st->pull_enable &= ~BIT(pin);
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, REG_PULL_SELECT, st->pull_select);
	if (rc != 0) {
		goto out;
	}
	rc = i2c_reg_write_byte_dt(&cfg->i2c, REG_PULL_ENABLE, st->pull_enable);
	if (rc != 0) {
		goto out;
	}
	rc = i2c_reg_write_byte_dt(&cfg->i2c, REG_OUT_SET, st->output);
	if (rc != 0) {
		goto out;
	}
	rc = i2c_reg_write_byte_dt(&cfg->i2c, REG_OUT_HIZ, st->hiz);
	if (rc != 0) {
		goto out;
	}
	rc = i2c_reg_write_byte_dt(&cfg->i2c, REG_IO_DIR, st->dir);

out:
	k_sem_give(&data->lock);
	return rc;
}

static int pi4ioe5v6408_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct pi4ioe5v6408_config *cfg = dev->config;
	struct pi4ioe5v6408_data *data = dev->data;
	uint8_t input;
	int rc;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&data->lock, K_FOREVER);
	rc = i2c_reg_read_byte_dt(&cfg->i2c, REG_IN_STATE, &input);
	k_sem_give(&data->lock);

	if (rc != 0) {
		return rc;
	}

	*value = (gpio_port_value_t)input;
	return 0;
}

static int pi4ioe5v6408_port_write(const struct device *dev, gpio_port_pins_t mask,
				   gpio_port_value_t value, gpio_port_value_t toggle)
{
	const struct pi4ioe5v6408_config *cfg = dev->config;
	struct pi4ioe5v6408_data *data = dev->data;
	uint8_t out;
	int rc;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&data->lock, K_FOREVER);
	out = ((data->pin_state.output & ~mask) | (value & mask)) ^ toggle;
	rc = i2c_reg_write_byte_dt(&cfg->i2c, REG_OUT_SET, out);
	if (rc != 0) {
		k_sem_give(&data->lock);
		return rc;
	}

	data->pin_state.output = out;
	k_sem_give(&data->lock);

	return 0;
}

static int pi4ioe5v6408_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	return pi4ioe5v6408_port_write(dev, mask, value, 0);
}

static int pi4ioe5v6408_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return pi4ioe5v6408_port_write(dev, pins, pins, 0);
}

static int pi4ioe5v6408_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return pi4ioe5v6408_port_write(dev, pins, 0, 0);
}

static int pi4ioe5v6408_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return pi4ioe5v6408_port_write(dev, 0, 0, pins);
}

static int pi4ioe5v6408_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct pi4ioe5v6408_config *cfg = dev->config;
	struct pi4ioe5v6408_data *data = dev->data;
	struct pi4ioe5v6408_irq_state *irq = &data->irq_state;
	uint8_t mask;
	int rc;

	if (!cfg->interrupt_enabled) {
		return -ENOTSUP;
	}
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (mode == GPIO_INT_MODE_DISABLED) {
		irq->rising &= ~BIT(pin);
		irq->falling &= ~BIT(pin);
	} else {
		switch (trig) {
		case GPIO_INT_TRIG_BOTH:
			irq->rising |= BIT(pin);
			irq->falling |= BIT(pin);
			break;
		case GPIO_INT_TRIG_HIGH:
			irq->rising |= BIT(pin);
			irq->falling &= ~BIT(pin);
			break;
		case GPIO_INT_TRIG_LOW:
			irq->rising &= ~BIT(pin);
			irq->falling |= BIT(pin);
			break;
		default:
			rc = -ENOTSUP;
			goto out;
		}
	}

	mask = irq->rising | irq->falling;
	rc = i2c_reg_write_byte_dt(&cfg->i2c, REG_INT_MASK, mask);

out:
	k_sem_give(&data->lock);
	return rc;
}

static int pi4ioe5v6408_manage_callback(const struct device *dev, struct gpio_callback *cb,
					bool set)
{
	struct pi4ioe5v6408_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}

static int pi4ioe5v6408_init(const struct device *dev)
{
	const struct pi4ioe5v6408_config *cfg = dev->config;
	struct pi4ioe5v6408_data *data = dev->data;
	uint8_t scratch;
	int rc;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, REG_DEVICE_ID, DEVICE_ID_SW_RESET);
	if (rc) {
		LOG_ERR("%s: software reset failed: %d", dev->name, rc);
		return rc;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, REG_OUT_HIZ, ALL_PINS);
	if (rc) {
		return rc;
	}

	rc = i2c_reg_read_byte_dt(&cfg->i2c, REG_IN_STATE, &data->irq_state.last_input);
	if (rc) {
		return rc;
	}
	(void)i2c_reg_read_byte_dt(&cfg->i2c, REG_INT_STATUS, &scratch);

	if (cfg->interrupt_enabled) {
		if (!gpio_is_ready_dt(&cfg->int_gpio)) {
			LOG_ERR("%s: INT gpio not ready", dev->name);
			return -ENODEV;
		}

		data->dev = dev;
		k_work_init(&data->work, pi4ioe5v6408_work_handler);

		rc = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
		if (rc) {
			return rc;
		}
		rc = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (rc) {
			return rc;
		}
		gpio_init_callback(&data->int_cb, pi4ioe5v6408_int_cb, BIT(cfg->int_gpio.pin));
		rc = gpio_add_callback(cfg->int_gpio.port, &data->int_cb);
		if (rc) {
			return rc;
		}
	}

	return 0;
}

static DEVICE_API(gpio, pi4ioe5v6408_api) = {
	.pin_configure = pi4ioe5v6408_pin_configure,
	.port_get_raw = pi4ioe5v6408_port_get_raw,
	.port_set_masked_raw = pi4ioe5v6408_port_set_masked_raw,
	.port_set_bits_raw = pi4ioe5v6408_port_set_bits_raw,
	.port_clear_bits_raw = pi4ioe5v6408_port_clear_bits_raw,
	.port_toggle_bits = pi4ioe5v6408_port_toggle_bits,
	.pin_interrupt_configure = pi4ioe5v6408_pin_interrupt_configure,
	.manage_callback = pi4ioe5v6408_manage_callback,
};

#define GPIO_PI4IOE5V6408_INIT(n)                                                                  \
	static const struct pi4ioe5v6408_config pi4ioe5v6408_cfg_##n = {                           \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),                           \
		.interrupt_enabled = DT_INST_NODE_HAS_PROP(n, int_gpios),                          \
	};                                                                                         \
                                                                                                   \
	static struct pi4ioe5v6408_data pi4ioe5v6408_data_##n = {                                  \
		.lock = Z_SEM_INITIALIZER(pi4ioe5v6408_data_##n.lock, 1, 1),                       \
		.pin_state =                                                                       \
			{                                                                          \
				.hiz = ALL_PINS,                                                   \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, pi4ioe5v6408_init, NULL, &pi4ioe5v6408_data_##n,                  \
			      &pi4ioe5v6408_cfg_##n, POST_KERNEL,                                  \
			      CONFIG_GPIO_PI4IOE5V6408_INIT_PRIORITY, &pi4ioe5v6408_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_PI4IOE5V6408_INIT)
