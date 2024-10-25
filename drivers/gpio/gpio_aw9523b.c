/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT awinic_aw9523b_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/aw9523b.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_aw9523b, CONFIG_GPIO_LOG_LEVEL);

#define AW9523B_GPOMD BIT(4)

enum read_write_toggle_t {
	READ,
	WRITE,
	TOGGLE,
};

struct gpio_aw9523b_config {
	struct gpio_driver_config common;
	const struct device *mfd_dev;
	struct i2c_dt_spec i2c;
	bool port0_push_pull;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(rstn_gpios)
	struct gpio_dt_spec rstn_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(intn_gpios)
	struct gpio_dt_spec intn_gpio;
	gpio_callback_handler_t intn_cb;
#endif
};

struct gpio_aw9523b_data {
	struct gpio_driver_data common;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(intn_gpios)
	const struct device *dev;
	sys_slist_t callbacks;
	struct gpio_callback gpio_callback;
	struct k_work intr_worker;
	gpio_port_value_t prev_value;
	gpio_port_pins_t rising_event_pins;
	gpio_port_pins_t falling_event_pins;
#endif
};

static int gpio_aw9523b_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_aw9523b_config *const config = dev->config;
	const uint8_t port = (pin < 8) ? 0 : 1;
	const uint8_t mask = BIT(port ? pin - 8 : pin);
	const uint8_t input_en = (flags & GPIO_INPUT) ? 0xFF : 0x00;
	int err;

	/* Can't do I2C operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Does not support disconnected pin */
	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	/* Open-drain support is per port, not per pin.
	 * So can't really support the API as-is.
	 */
	if (flags & GPIO_SINGLE_ENDED) {
		return -ENOTSUP;
	}

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);

	err = i2c_reg_update_byte_dt(&config->i2c, AW9523B_REG_ADDR_CONFIG0 + port, mask, input_en);
	if (err) {
		LOG_ERR("%s: Faield to set pin%d direction (%d)", dev->name, pin, err);
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(intn_gpios)
	if (input_en) {
		struct gpio_aw9523b_data *const data = dev->data;
		uint8_t buf[2];

		/* Read initial pin state */
		err = i2c_burst_read_dt(&config->i2c, AW9523B_REG_ADDR_INPUT0, buf, 2);
		if (err) {
			LOG_ERR("%s: Failed to read initial pin state (%d)", dev->name, err);
			return err;
		}

		WRITE_BIT(data->prev_value, pin, !!((buf[1] << 8 | buf[0]) & BIT(pin)));
	} else {
		struct gpio_aw9523b_data *const data = dev->data;

		WRITE_BIT(data->falling_event_pins, pin, 0);
		WRITE_BIT(data->rising_event_pins, pin, 0);
	}
#endif

	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	return err;
}

static int gpio_aw9523b_port_read_write_toggle(const struct device *dev, gpio_port_pins_t mask,
					       gpio_port_value_t *value,
					       enum read_write_toggle_t mode)
{
	const struct gpio_aw9523b_config *const config = dev->config;
	uint8_t buf[2];
	int err;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);

	err = i2c_burst_read_dt(&config->i2c, AW9523B_REG_ADDR_INPUT0, buf, 2);

	if (err) {
		LOG_ERR("%s: Faield to read port status (%d)", dev->name, err);
	} else if (mode != READ) {
		gpio_port_value_t old_value = (buf[1] << 8 | buf[0]);
		gpio_port_value_t new_value;

		if (mode == WRITE) {
			new_value = (old_value & ~mask) | (*value & mask);
		} else {
			new_value = (old_value & ~mask) | (~old_value & mask);
		}

		if (new_value != old_value) {
			buf[0] = (new_value >> 0) & 0xFF;
			buf[1] = (new_value >> 8) & 0xFF;
			err = i2c_burst_write_dt(&config->i2c, AW9523B_REG_ADDR_OUTPUT0, buf, 2);
			if (err) {
				LOG_ERR("%s: Faield to set port (%d)", dev->name, err);
			}
		}
	}

	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	if (err == 0 && mode == READ) {
		*value = (buf[1] << 8 | buf[0]);
	}

	return err;
}

static int gpio_aw9523b_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	return gpio_aw9523b_port_read_write_toggle(dev, UINT16_MAX, value, READ);
}

static int gpio_aw9523b_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	return gpio_aw9523b_port_read_write_toggle(dev, mask, &value, WRITE);
}

static int gpio_aw9523b_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_aw9523b_port_read_write_toggle(dev, pins, &pins, WRITE);
}

static int gpio_aw9523b_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	gpio_port_value_t zero = 0;

	return gpio_aw9523b_port_read_write_toggle(dev, pins, &zero, WRITE);
}

static int gpio_aw9523b_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	gpio_port_value_t zero = 0;

	return gpio_aw9523b_port_read_write_toggle(dev, pins, &zero, TOGGLE);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(intn_gpios)
static void gpio_aw9523b_interrupt_worker(struct k_work *work)
{
	struct gpio_aw9523b_data *const data =
		CONTAINER_OF(work, struct gpio_aw9523b_data, intr_worker);
	gpio_port_value_t value, rising, falling;

	gpio_aw9523b_port_get_raw(data->dev, &value);

	rising = (value ^ data->prev_value) & (value & data->rising_event_pins);
	falling = (value ^ data->prev_value) & (~value & data->falling_event_pins);

	data->prev_value = value;

	gpio_fire_callbacks(&data->callbacks, data->dev, rising | falling);
}

static int gpio_aw9523b_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_aw9523b_config *const config = dev->config;
	struct gpio_aw9523b_data *const data = dev->data;
	const uint8_t port = (pin < 8) ? 0 : 1;
	const uint8_t mask = BIT(port ? pin - 8 : pin);
	const uint8_t n_int_en = (mode & GPIO_INT_MODE_EDGE) ? 0x00 : 0xFF;
	uint8_t buf[2];
	int err;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);

	err = i2c_reg_update_byte_dt(&config->i2c, AW9523B_REG_ADDR_INT0 + port, mask, n_int_en);
	if (err) {
		LOG_ERR("%s: Failed to configure pin interruption (%d)", dev->name, err);
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(intn_gpios)
	if (data->common.invert & BIT(pin)) {
		WRITE_BIT(data->falling_event_pins, pin, !!(trig & GPIO_INT_LOW_0));
		WRITE_BIT(data->rising_event_pins, pin, !!(trig & GPIO_INT_HIGH_1));
	} else {
		WRITE_BIT(data->falling_event_pins, pin, !!(trig & GPIO_INT_HIGH_1));
		WRITE_BIT(data->rising_event_pins, pin, !!(trig & GPIO_INT_LOW_0));
	}

	if (!n_int_en) {
		/* Read initial pin state */
		err = i2c_burst_read_dt(&config->i2c, AW9523B_REG_ADDR_INPUT0, buf, 2);
		if (err) {
			LOG_ERR("%s: Failed to read initial pin state (%d)", dev->name, err);
			return err;
		}

		WRITE_BIT(data->prev_value, pin, !!((buf[1] << 8 | buf[0]) & BIT(pin)));
	} else {
		WRITE_BIT(data->falling_event_pins, pin, 0);
		WRITE_BIT(data->rising_event_pins, pin, 0);
	}
#endif
	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	return err;
}

static int gpio_aw9523b_manage_callback(const struct device *dev, struct gpio_callback *callback,
					bool set)
{
	const struct gpio_aw9523b_config *const config = dev->config;
	struct gpio_aw9523b_data *const data = dev->data;
	int err;

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);

	err = gpio_manage_callback(&data->callbacks, callback, set);
	if (err) {
		LOG_ERR("%s: gpio_manage_callback faield (%d)", dev->name, err);
	}

	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	return err;
}
#endif /* CONFIG_GPIO_AW9523B_INTERRUPT */

static const struct gpio_driver_api gpio_aw9523b_api = {
	.pin_configure = gpio_aw9523b_configure,
	.port_get_raw = gpio_aw9523b_port_get_raw,
	.port_set_masked_raw = gpio_aw9523b_port_set_masked_raw,
	.port_set_bits_raw = gpio_aw9523b_port_set_bits_raw,
	.port_clear_bits_raw = gpio_aw9523b_port_clear_bits_raw,
	.port_toggle_bits = gpio_aw9523b_port_toggle_bits,
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(intn_gpios)
	.pin_interrupt_configure = gpio_aw9523b_pin_interrupt_configure,
	.manage_callback = gpio_aw9523b_manage_callback,
#endif
};

static int gpio_aw9523b_init(const struct device *dev)
{
	const struct gpio_aw9523b_config *const config = dev->config;
	const uint8_t int_init_data[] = {0xFF, 0xFF};
	uint8_t buf[2];
	int err;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(intn_gpios)
	struct gpio_aw9523b_data *const data = dev->data;
#endif

	if (!device_is_ready(config->i2c.bus)) {
		return -ENODEV;
	}

	k_sem_init(aw9523b_get_lock(config->mfd_dev), 1, 1);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(intn_gpios)
	/* Store self-reference for interrupt handling */
	data->dev = dev;

	/* Prepare interrupt worker */
	k_work_init(&data->intr_worker, gpio_aw9523b_interrupt_worker);

	if (!config->intn_gpio.port) {
		goto end_init_intn_gpio;
	}

	if (!gpio_is_ready_dt(&config->intn_gpio)) {
		LOG_ERR("%s: Interrupt GPIO not ready", dev->name);
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->intn_gpio, GPIO_INPUT);
	if (err) {
		LOG_ERR("%s: Failed to configure interrupt pin %d (%d)", dev->name,
			config->intn_gpio.pin, err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->intn_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("%s: Failed to configure interrupt %d (%d)", dev->name,
			config->intn_gpio.pin, err);
		return err;
	}

	gpio_init_callback(&data->gpio_callback, config->intn_cb, BIT(config->intn_gpio.pin));
	err = gpio_add_callback(config->intn_gpio.port, &data->gpio_callback);
	if (err) {
		LOG_ERR("%s: Failed to add interrupt callback for pin %d (%d)", dev->name,
			config->intn_gpio.pin, err);
		return err;
	}

end_init_intn_gpio:
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(intn_gpios)
	if (!config->rstn_gpio.port) {
		goto end_hw_reset;
	}

	if (!gpio_is_ready_dt(&config->rstn_gpio)) {
		LOG_ERR("%s: Reset GPIO not ready", dev->name);
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->rstn_gpio, 0);
	if (err) {
		LOG_ERR("%s: Failed to configure reset pin %d (%d)", dev->name,
			config->rstn_gpio.pin, err);
		return err;
	}

	err = gpio_pin_set_dt(&config->rstn_gpio, 0);
	if (err) {
		LOG_ERR("%s: Failed to set 0 reset pin %d (%d)", dev->name, config->rstn_gpio.pin,
			err);
		return err;
	}

	err = gpio_pin_set_dt(&config->rstn_gpio, 1);
	if (err) {
		LOG_ERR("%s: Failed to set 1 reset pin %d (%d)", dev->name, config->rstn_gpio.pin,
			err);
		return err;
	}

end_hw_reset:
#endif

	/* Software reset */
	err = i2c_reg_read_byte_dt(&config->i2c, AW9523B_REG_ADDR_SW_RSTN, buf);
	if (err) {
		LOG_ERR("%s: Failed to software reset (%d)", dev->name, err);
		return err;
	}

	/* Disabling all interrupts */
	err = i2c_burst_write_dt(&config->i2c, AW9523B_REG_ADDR_INT0, int_init_data, 2);
	if (err) {
		LOG_ERR("%s: Failed to disable all interrupts (%d)", dev->name, err);
		return err;
	}

	if (!config->port0_push_pull) {
		return 0;
	}

	/* Configure port0 to push-pull mode */
	err = i2c_reg_update_byte_dt(&config->i2c, AW9523B_REG_ADDR_CTL, AW9523B_GPOMD, 0xFF);
	if (err) {
		LOG_ERR("%s: Failed to configure port0 to push-pull (%d)", dev->name, err);
		return err;
	}

	return 0;
}

#define GPIO_AW9523B_DEFINE(inst)                                                                  \
	static struct gpio_aw9523b_data gpio_aw9523b_data##inst;                                   \
                                                                                                   \
	IF_ENABLED(DT_INST_PROP_HAS_IDX(inst, intn_gpios, 0), (                                    \
		static void gpio_aw9523b_intn_cb##inst(const struct device *dev,                   \
							     struct gpio_callback *cb,             \
							     gpio_port_pins_t pins)                \
		{                                                                                  \
			k_work_submit(&gpio_aw9523b_data##inst.intr_worker);                       \
		}                                                                                  \
	))                                                                                         \
												   \
	static const struct gpio_aw9523b_config gpio_aw9523b_config##inst = {                      \
		.common = {                                                                        \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),                    \
		},                                                                                 \
		.mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.port0_push_pull = DT_INST_PROP_OR(inst, port0_push_pull, false),                  \
		IF_ENABLED(DT_INST_PROP_HAS_IDX(inst, intn_gpios, 0), (                            \
			   .intn_gpio = GPIO_DT_SPEC_INST_GET(inst, intn_gpios),                   \
			   .intn_cb = gpio_aw9523b_intn_cb##inst,                                  \
		))                                                                                 \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(inst, gpio_aw9523b_init, NULL, &gpio_aw9523b_data##inst,             \
			      &gpio_aw9523b_config##inst, POST_KERNEL,                             \
			      CONFIG_GPIO_AW9523B_INIT_PRIORITY, &gpio_aw9523b_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_AW9523B_DEFINE)
