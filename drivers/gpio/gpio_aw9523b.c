/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT awinic_aw9523b_gpio

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/aw9523b.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_aw9523b, CONFIG_GPIO_LOG_LEVEL);

#define AW9523B_GPOMD             BIT(4)
#define AW9523B_RESET_PULSE_WIDTH 20
#define AW9523B_REG_CONFIG(n)     (AW9523B_REG_CONFIG0 + n)
#define AW9523B_REG_INT(n)        (AW9523B_REG_INT0 + n)
#define AW9523B_REG_OUTPUT(n)     (AW9523B_REG_OUTPUT0 + n)

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
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct gpio_dt_spec int_gpio;
	gpio_callback_handler_t int_cb;
#endif
};

struct gpio_aw9523b_data {
	struct gpio_driver_data common;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct device *dev;
	sys_slist_t callbacks;
	struct gpio_callback gpio_callback;
	struct k_work intr_worker;
	gpio_port_value_t prev_value;
	gpio_port_pins_t rising_event_pins;
	gpio_port_pins_t falling_event_pins;
#endif
};

static int gpio_aw9523b_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_aw9523b_config *const config = dev->config;
	const uint8_t port = (pin < 8) ? 0 : 1;
	const uint8_t mask = BIT(pin % 8);
	const uint8_t input_en = (flags & GPIO_INPUT) ? mask : 0x00;
	const uint8_t out_high = (flags & GPIO_OUTPUT_INIT_HIGH) ? mask : 0x00;
	int err;

	/* Can't do I2C operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Either INPUT or OUTPUT must be set */
	if ((!(flags & GPIO_INPUT) && !(flags & GPIO_OUTPUT)) ||
	    ((flags & GPIO_INPUT) && (flags & GPIO_OUTPUT))) {
		return -ENOTSUP;
	}

	/* Open-drain support is per port, not per pin.
	 * So can't really support the API as-is.
	 */
	if (port == 0 && !config->port0_push_pull) {
		if (!((flags & GPIO_SINGLE_ENDED) && (flags & GPIO_LINE_OPEN_DRAIN))) {
			return -ENOTSUP;
		}
	} else {
		if (flags & GPIO_SINGLE_ENDED) {
			return -ENOTSUP;
		}
	}

	if ((flags & GPIO_INPUT) && ((flags & GPIO_PULL_UP) || (flags & GPIO_PULL_DOWN))) {
		return -ENOTSUP;
	}

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);

	err = i2c_reg_update_byte_dt(&config->i2c, AW9523B_REG_CONFIG(port), mask, input_en);
	if (err) {
		LOG_ERR("%s: Failed to set pin%d direction (%d)", dev->name, pin, err);
		goto on_error;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (config->int_gpio.port) {
		if (input_en) {
			struct gpio_aw9523b_data *const data = dev->data;
			uint8_t buf[2];

			/* Read initial pin state */
			err = i2c_burst_read_dt(&config->i2c, AW9523B_REG_INPUT0, buf, sizeof(buf));
			if (err) {
				LOG_ERR("%s: Read initial pin state failed (%d)", dev->name, err);
				goto on_error;
			}

			WRITE_BIT(data->prev_value, pin, sys_get_le16(buf) & BIT(pin));
		} else {
			struct gpio_aw9523b_data *const data = dev->data;

			WRITE_BIT(data->falling_event_pins, pin, 0);
			WRITE_BIT(data->rising_event_pins, pin, 0);
		}
	}
#endif

	err = i2c_reg_update_byte_dt(&config->i2c, AW9523B_REG_OUTPUT(port), mask, out_high);
	if (err) {
		LOG_ERR("%s: Failed to set initial pin state (%d)", dev->name, err);
		return err;
	}

on_error:
	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	return err;
}

/**
 * Common implementation of Read, Write, and Toggle
 *
 * @param[in]       dev Specify device instance.
 * @param[in]      mask Register mask to select pins to operate.
 * @param[in,out] value When mode is READ, this param is pointer to result value storing region.
 *                      When mode is WRITE, this param is used as input value.
 *                      When mode is TOGGLE, this param will ignored.
 * @param[in]      mode Choose mode from READ, WRITE or TOGGLE.
 */
static int gpio_aw9523b_port_read_write_toggle(const struct device *dev, gpio_port_pins_t mask,
					       gpio_port_value_t *value,
					       enum read_write_toggle_t mode)
{
	const struct gpio_aw9523b_config *const config = dev->config;
	uint8_t buf[2];
	gpio_port_value_t old_value;
	gpio_port_value_t new_value;
	int err;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);

	/*
	 * As with interrupts, the INPUT values are read for each address
	 * to keep the internal state correct.
	 */
	err = i2c_burst_read_dt(&config->i2c, AW9523B_REG_INPUT0, &buf[0], 1);
	if (err) {
		LOG_ERR("%s: Failed to read port0 status (%d)", dev->name, err);
		goto end;
	}
	err = i2c_burst_read_dt(&config->i2c, AW9523B_REG_INPUT1, &buf[1], 1);
	if (err) {
		LOG_ERR("%s: Failed to read port1 status (%d)", dev->name, err);
		goto end;
	}

	if (mode == READ) {
		goto end;
	}

	old_value = sys_get_le16(buf);

	if (mode == WRITE) {
		new_value = (old_value & ~mask) | (*value & mask);
	} else {
		new_value = (old_value & ~mask) | (~old_value & mask);
	}

	if (new_value == old_value) {
		goto end;
	}

	*(uint16_t *)buf = sys_get_le16((uint8_t *)&new_value);
	err = i2c_burst_write_dt(&config->i2c, AW9523B_REG_OUTPUT0, buf, sizeof(buf));
	if (err) {
		LOG_ERR("%s: Failed to set port (%d)", dev->name, err);
	}

end:
	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	if (err == 0 && mode == READ) {
		*value = sys_get_le16(buf);
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
	const gpio_port_value_t zero = 0;

	/* If WRITE is specified, this pointer is used for reading only. It can cast. */
	return gpio_aw9523b_port_read_write_toggle(dev, pins, (gpio_port_value_t *)&zero, WRITE);
}

static int gpio_aw9523b_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_aw9523b_port_read_write_toggle(dev, pins, NULL, TOGGLE);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static __maybe_unused void gpio_aw9523b_interrupt_worker(struct k_work *work)
{
	struct gpio_aw9523b_data *const data =
		CONTAINER_OF(work, struct gpio_aw9523b_data, intr_worker);
	const struct gpio_aw9523b_config *const config = data->dev->config;
	gpio_port_value_t value, rising, falling;
	uint8_t buf[2];
	int err;

	/*
	 * We need to read INPUT0 to deassert INTN when that is asserted by
	 * pin0-7 interruption, and same also INPUT1 for pin8-15.
	 * It cannot deassert by burst-read.
	 */
	err = i2c_burst_read_dt(&config->i2c, AW9523B_REG_INPUT0, &buf[0], 1);
	if (err) {
		LOG_ERR("%s: Failed to read INPUT0 %d", data->dev->name, err);
	}
	err = i2c_burst_read_dt(&config->i2c, AW9523B_REG_INPUT1, &buf[1], 1);
	if (err) {
		LOG_ERR("%s: Failed to read INPUT1 %d", data->dev->name, err);
	}

	value = sys_get_le16(buf);

	rising = (value ^ data->prev_value) & (value & data->rising_event_pins);
	falling = (value ^ data->prev_value) & (~value & data->falling_event_pins);

	data->prev_value = value;

	gpio_fire_callbacks(&data->callbacks, data->dev, rising | falling);
}

static __maybe_unused int gpio_aw9523b_pin_interrupt_configure(const struct device *dev,
							       gpio_pin_t pin,
							       enum gpio_int_mode mode,
							       enum gpio_int_trig trig)
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
	if (data->common.invert & BIT(pin)) {
		WRITE_BIT(data->falling_event_pins, pin, trig & GPIO_INT_HIGH_1);
		WRITE_BIT(data->rising_event_pins, pin, trig & GPIO_INT_LOW_0);
	} else {
		WRITE_BIT(data->falling_event_pins, pin, trig & GPIO_INT_LOW_0);
		WRITE_BIT(data->rising_event_pins, pin, trig & GPIO_INT_HIGH_1);
	}

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);

	err = i2c_reg_update_byte_dt(&config->i2c, AW9523B_REG_INT(port), mask, n_int_en);
	if (err) {
		LOG_ERR("%s: Failed to configure pin interruption (%d)", dev->name, err);
		goto end;
	}

	if (!n_int_en) {
		/* Read initial pin state */
		err = i2c_burst_read_dt(&config->i2c, AW9523B_REG_INPUT0, buf, sizeof(buf));
		if (err) {
			LOG_ERR("%s: Failed to read initial pin state (%d)", dev->name, err);
			goto end;
		}

		WRITE_BIT(data->prev_value, pin, sys_get_le16(buf) & BIT(pin));
	} else {
		WRITE_BIT(data->falling_event_pins, pin, 0);
		WRITE_BIT(data->rising_event_pins, pin, 0);
	}

end:
	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	return err;
}

static __maybe_unused int gpio_aw9523b_manage_callback(const struct device *dev,
						       struct gpio_callback *callback, bool set)
{
	const struct gpio_aw9523b_config *const config = dev->config;
	struct gpio_aw9523b_data *const data = dev->data;
	int err;

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);

	err = gpio_manage_callback(&data->callbacks, callback, set);
	if (err) {
		LOG_ERR("%s: gpio_manage_callback failed (%d)", dev->name, err);
	}

	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	return err;
}

static __maybe_unused void gpio_aw9523b_int_handler(const struct device *gpio_dev,
						    struct gpio_callback *cb, uint32_t pins)
{
	struct gpio_aw9523b_data *data = CONTAINER_OF(cb, struct gpio_aw9523b_data, gpio_callback);

	k_work_submit(&data->intr_worker);
}
#endif

static DEVICE_API(gpio, gpio_aw9523b_api) = {
	.pin_configure = gpio_aw9523b_pin_configure,
	.port_get_raw = gpio_aw9523b_port_get_raw,
	.port_set_masked_raw = gpio_aw9523b_port_set_masked_raw,
	.port_set_bits_raw = gpio_aw9523b_port_set_bits_raw,
	.port_clear_bits_raw = gpio_aw9523b_port_clear_bits_raw,
	.port_toggle_bits = gpio_aw9523b_port_toggle_bits,
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
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
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct gpio_aw9523b_data *const data = dev->data;

	if (!config->int_gpio.port) {
		goto end_init_int_gpio;
	}

	/* Store self-reference for interrupt handling */
	data->dev = dev;

	/* Prepare interrupt worker */
	k_work_init(&data->intr_worker, gpio_aw9523b_interrupt_worker);

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("%s: Interrupt GPIO not ready", dev->name);
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (err) {
		LOG_ERR("%s: Failed to configure interrupt pin %d (%d)", dev->name,
			config->int_gpio.pin, err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("%s: Failed to configure interrupt %d (%d)", dev->name,
			config->int_gpio.pin, err);
		return err;
	}

	gpio_init_callback(&data->gpio_callback, config->int_cb, BIT(config->int_gpio.pin));
	err = gpio_add_callback(config->int_gpio.port, &data->gpio_callback);
	if (err) {
		LOG_ERR("%s: Failed to add interrupt callback for pin %d (%d)", dev->name,
			config->int_gpio.pin, err);
		return err;
	}

end_init_int_gpio:
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (!config->reset_gpio.port) {
		goto end_hw_reset;
	}

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		LOG_ERR("%s: Reset GPIO not ready", dev->name);
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (err) {
		LOG_ERR("%s: Failed to configure reset pin %d (%d)", dev->name,
			config->reset_gpio.pin, err);
		return err;
	}

	k_busy_wait(AW9523B_RESET_PULSE_WIDTH);

	err = gpio_pin_set_dt(&config->reset_gpio, 0);
	if (err) {
		LOG_ERR("%s: Failed to set 0 reset pin %d (%d)", dev->name, config->reset_gpio.pin,
			err);
		return err;
	}

end_hw_reset:
#endif

	if (!device_is_ready(config->i2c.bus)) {
		return -ENODEV;
	}

	k_sem_init(aw9523b_get_lock(config->mfd_dev), 1, 1);

	/* Software reset */
	err = i2c_reg_read_byte_dt(&config->i2c, AW9523B_REG_SW_RSTN, buf);
	if (err) {
		LOG_ERR("%s: Failed to software reset (%d)", dev->name, err);
		return err;
	}

	/* Disabling all interrupts */
	err = i2c_burst_write_dt(&config->i2c, AW9523B_REG_INT0, int_init_data, sizeof(buf));
	if (err) {
		LOG_ERR("%s: Failed to disable all interrupts (%d)", dev->name, err);
		return err;
	}

	if (!config->port0_push_pull) {
		/* Configure port0 to push-pull mode */
		err = i2c_reg_update_byte_dt(&config->i2c, AW9523B_REG_CTL, AW9523B_GPOMD, 0xFF);
		if (err) {
			LOG_ERR("%s: Failed to configure port0 to push-pull (%d)", dev->name, err);
			return err;
		}
	}

	return 0;
}

#define GPIO_AW9523B_DEFINE(inst)                                                                  \
	static struct gpio_aw9523b_data gpio_aw9523b_data##inst;                                   \
                                                                                                   \
	static const struct gpio_aw9523b_config gpio_aw9523b_config##inst = {                      \
		.common = {                                                                        \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),                    \
		},                                                                                 \
		.mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.port0_push_pull = DT_INST_PROP_OR(inst, port0_push_pull, false),                  \
		IF_ENABLED(DT_INST_PROP_HAS_IDX(inst, int_gpios, 0), (                             \
			   .int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                     \
			   .int_cb = gpio_aw9523b_int_handler,                                     \
		))                                                                                 \
		IF_ENABLED(DT_INST_PROP_HAS_IDX(inst, reset_gpios, 0), (                           \
			   .reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                 \
		))                                                                                 \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(inst, gpio_aw9523b_init, NULL, &gpio_aw9523b_data##inst,             \
			      &gpio_aw9523b_config##inst, POST_KERNEL,                             \
			      CONFIG_MFD_INIT_PRIORITY, &gpio_aw9523b_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_AW9523B_DEFINE)
