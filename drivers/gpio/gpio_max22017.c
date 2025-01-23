/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT adi_max22017_gpio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_max22017, CONFIG_GPIO_LOG_LEVEL);

#include <zephyr/drivers/mfd/max22017.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

struct gpio_adi_max22017_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *parent;
};

struct gpio_adi_max22017_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
#ifdef CONFIG_GPIO_MAX22017_INT_QUIRK
	struct k_timer int_quirk_timer;
#endif
};

#ifdef CONFIG_GPIO_MAX22017_INT_QUIRK
void isr_quirk_handler(struct k_timer *int_quirk_timer)
{
	int ret;
	struct max22017_data *data = k_timer_user_data_get(int_quirk_timer);

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = k_work_submit(&data->int_work);
	if (ret < 0) {
		LOG_WRN("Could not submit int work: %d", ret);
	}

	k_mutex_unlock(&data->lock);
}
#endif

static int adi_max22017_gpio_set_output(const struct device *dev, uint8_t pin, bool initial_value)
{
	int ret;
	uint16_t gpio_data, gpio_ctrl;
	struct max22017_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(dev, MAX22017_GEN_GPIO_DATA_OFF, &gpio_data);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_read(dev, MAX22017_GEN_GPIO_CTRL_OFF, &gpio_ctrl);
	if (ret) {
		goto fail;
	}

	if (initial_value) {
		gpio_data |= FIELD_PREP(MAX22017_GEN_GPIO_DATA_GPO_DATA, BIT(pin));
	} else {
		gpio_data &= ~FIELD_PREP(MAX22017_GEN_GPIO_DATA_GPO_DATA, BIT(pin));
	}

	gpio_ctrl |= FIELD_PREP(MAX22017_GEN_GPIO_CTRL_GPIO_EN, BIT(pin)) |
		     FIELD_PREP(MAX22017_GEN_GPIO_CTRL_GPIO_DIR, BIT(pin));

	ret = max22017_reg_write(dev, MAX22017_GEN_GPIO_DATA_OFF, gpio_data);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_write(dev, MAX22017_GEN_GPIO_CTRL_OFF, gpio_ctrl);

fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int adi_max22017_gpio_set_input(const struct device *dev, uint8_t pin)
{
	int ret;
	uint16_t gpio_ctrl;
	struct max22017_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(dev, MAX22017_GEN_GPIO_CTRL_OFF, &gpio_ctrl);
	if (ret) {
		goto fail;
	}

	gpio_ctrl |= FIELD_PREP(MAX22017_GEN_GPIO_CTRL_GPIO_EN, BIT(pin));
	gpio_ctrl &= ~FIELD_PREP(MAX22017_GEN_GPIO_CTRL_GPIO_DIR, BIT(pin));

	ret = max22017_reg_write(dev, MAX22017_GEN_GPIO_CTRL_OFF, gpio_ctrl);
fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

int adi_max22017_gpio_deconfigure(const struct device *dev, uint8_t pin)
{
	int ret;
	uint16_t gpio_ctrl;
	struct max22017_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(dev, MAX22017_GEN_GPIO_CTRL_OFF, &gpio_ctrl);
	if (ret) {
		goto fail;
	}

	gpio_ctrl &= ~FIELD_PREP(MAX22017_GEN_GPIO_CTRL_GPIO_EN, BIT(pin));

	ret = max22017_reg_write(dev, MAX22017_GEN_GPIO_CTRL_OFF, gpio_ctrl);
fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

int adi_max22017_gpio_set_pin_value(const struct device *dev, uint8_t pin, bool value)
{
	int ret;
	uint16_t gpio_data;
	struct max22017_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(dev, MAX22017_GEN_GPIO_DATA_OFF, &gpio_data);
	if (ret) {
		goto fail;
	}

	if (value) {
		gpio_data |= FIELD_PREP(MAX22017_GEN_GPIO_DATA_GPO_DATA, BIT(pin));
	} else {
		gpio_data &= ~FIELD_PREP(MAX22017_GEN_GPIO_DATA_GPO_DATA, BIT(pin));
	}

	ret = max22017_reg_write(dev, MAX22017_GEN_GPIO_DATA_OFF, gpio_data);
fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

int adi_max22017_gpio_get_pin_value(const struct device *dev, uint8_t pin, bool *value)
{
	int ret;
	uint16_t gpio_data;
	struct max22017_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(dev, MAX22017_GEN_GPIO_DATA_OFF, &gpio_data);
	if (ret) {
		goto fail;
	}

	*value = FIELD_GET(MAX22017_GEN_GPIO_DATA_GPI_DATA, gpio_data) & BIT(pin);

fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int adi_max22017_gpio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
						 gpio_port_value_t value)
{
	int ret;
	uint16_t gpio_data, tmp_val;
	struct max22017_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(dev, MAX22017_GEN_GPIO_DATA_OFF, &gpio_data);
	if (ret) {
		goto fail;
	}

	tmp_val = FIELD_GET(MAX22017_GEN_GPIO_DATA_GPO_DATA, gpio_data);
	tmp_val = (tmp_val & ~mask) | (value & mask);
	gpio_data = FIELD_PREP(MAX22017_GEN_GPIO_DATA_GPO_DATA, tmp_val) |
		    FIELD_PREP(MAX22017_GEN_GPIO_DATA_GPI_DATA,
			       FIELD_GET(MAX22017_GEN_GPIO_DATA_GPI_DATA, gpio_data));

	ret = max22017_reg_write(dev, MAX22017_GEN_GPIO_DATA_OFF, gpio_data);
fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int gpio_adi_max22017_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_adi_max22017_config *config = dev->config;
	int err = -EINVAL;

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		return adi_max22017_gpio_deconfigure(config->parent, pin);
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
		err = adi_max22017_gpio_set_input(config->parent, pin);
		break;
	case GPIO_OUTPUT:
		err = adi_max22017_gpio_set_output(config->parent, pin,
						   (flags & GPIO_OUTPUT_INIT_HIGH) != 0);
		break;
	default:
		return -ENOTSUP;
	}
	return err;
}

static int gpio_adi_max22017_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						     enum gpio_int_mode mode,
						     enum gpio_int_trig trig)
{
	int ret;
	uint16_t gpio_int, gen_int_en;
	const struct gpio_adi_max22017_config *config = dev->config;
	const struct device *parent = config->parent;
	struct max22017_data *data = parent->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (mode == GPIO_INT_MODE_DISABLED) {
		ret = -ENOTSUP;
		goto fail;
	}

	ret = max22017_reg_read(parent, MAX22017_GEN_GPI_INT_OFF, &gpio_int);
	if (ret) {
		goto fail;
	}

	if (mode & GPIO_INT_EDGE_RISING) {
		gpio_int |= FIELD_PREP(MAX22017_GEN_GPI_INT_GPI_POS_EDGE_INT, BIT(pin));
	}
	if (mode & GPIO_INT_EDGE_FALLING) {
		gpio_int |= FIELD_PREP(MAX22017_GEN_GPI_INT_GPI_NEG_EDGE_INT, BIT(pin));
	}

	ret = max22017_reg_write(parent, MAX22017_GEN_GPI_INT_OFF, gpio_int);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_read(parent, MAX22017_GEN_INTEN_OFF, &gen_int_en);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_write(parent, MAX22017_GEN_INTEN_OFF,
				 gen_int_en | FIELD_PREP(MAX22017_GEN_INTEN_GPI_INTEN, 1));
fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int gpio_adi_max22017_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	int ret;
	const struct gpio_adi_max22017_config *config = dev->config;
	const struct device *parent = config->parent;
	struct max22017_data *data = parent->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(parent, MAX22017_GEN_GPIO_DATA_OFF, (uint16_t *)value);
	if (ret) {
		goto fail;
	}

	*value = FIELD_GET(MAX22017_GEN_GPIO_DATA_GPI_DATA, *value);

fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int gpio_adi_max22017_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
						 gpio_port_value_t value)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_gpio_port_set_masked_raw(config->parent, mask, value);
}

static int gpio_adi_max22017_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_gpio_port_set_masked_raw(config->parent, pins, pins);
}

static int gpio_adi_max22017_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_gpio_port_set_masked_raw(config->parent, pins, 0);
}

static int gpio_adi_max22017_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	uint16_t gpio_data, tmp_val;
	const struct gpio_adi_max22017_config *config = dev->config;
	const struct device *parent = config->parent;
	struct max22017_data *data = parent->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(parent, MAX22017_GEN_GPIO_DATA_OFF, &gpio_data);
	if (ret) {
		goto fail;
	}

	tmp_val = FIELD_GET(MAX22017_GEN_GPIO_DATA_GPO_DATA, gpio_data);
	tmp_val = (tmp_val ^ pins);
	gpio_data = FIELD_PREP(MAX22017_GEN_GPIO_DATA_GPO_DATA, tmp_val) |
		    FIELD_PREP(MAX22017_GEN_GPIO_DATA_GPI_DATA,
			       FIELD_GET(MAX22017_GEN_GPIO_DATA_GPI_DATA, gpio_data));

	ret = max22017_reg_write(parent, MAX22017_GEN_GPIO_DATA_OFF, gpio_data);
fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int gpio_adi_max22017_manage_cb(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	int ret;
	const struct gpio_adi_max22017_config *config = dev->config;
	struct max22017_data *data = config->parent->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = gpio_manage_callback(&data->callbacks_gpi, callback, set);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int gpio_adi_max22017_init(const struct device *dev)
{
	const struct gpio_adi_max22017_config *config = dev->config;
	const struct device *parent = config->parent;

	if (!device_is_ready(parent)) {
		LOG_ERR("parent adi_max22017 MFD device '%s' not ready", config->parent->name);
		return -EINVAL;
	}

#ifdef CONFIG_GPIO_MAX22017_INT_QUIRK
	struct gpio_adi_max22017_data *data = dev->data;
	struct k_timer *t = &data->int_quirk_timer;

	k_timer_init(t, isr_quirk_handler, NULL);
	k_timer_user_data_set(t, parent->data);
	k_timer_start(t, K_MSEC(25), K_MSEC(25));
#endif
	return 0;
}

static DEVICE_API(gpio, gpio_adi_max22017_api) = {
	.pin_configure = gpio_adi_max22017_configure,
	.port_set_masked_raw = gpio_adi_max22017_port_set_masked_raw,
	.port_set_bits_raw = gpio_adi_max22017_port_set_bits_raw,
	.port_clear_bits_raw = gpio_adi_max22017_port_clear_bits_raw,
	.port_toggle_bits = gpio_adi_max22017_port_toggle_bits,
	.port_get_raw = gpio_adi_max22017_port_get_raw,
	.pin_interrupt_configure = gpio_adi_max22017_pin_interrupt_configure,
	.manage_callback = gpio_adi_max22017_manage_cb,
};

#define GPIO_MAX22017_DEVICE(id)                                                                   \
	static const struct gpio_adi_max22017_config gpio_adi_max22017_##id##_cfg = {              \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(id),              \
			},                                                                         \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(id)),                                       \
	};                                                                                         \
	static struct gpio_adi_max22017_data gpio_adi_max22017_##id##_data;                        \
	DEVICE_DT_INST_DEFINE(id, gpio_adi_max22017_init, NULL, &gpio_adi_max22017_##id##_data,    \
			      &gpio_adi_max22017_##id##_cfg, POST_KERNEL,                          \
			      CONFIG_GPIO_MAX22017_INIT_PRIORITY, &gpio_adi_max22017_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MAX22017_DEVICE)
