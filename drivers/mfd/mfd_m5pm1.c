/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT m5stack_m5pm1

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/m5pm1.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mfd_m5pm1, CONFIG_MFD_LOG_LEVEL);

#define M5PM1_REG_DEVICE_ID    0x00
#define M5PM1_REG_DEVICE_MODEL 0x01
#define M5PM1_REG_I2C_CFG      0x09

#define M5PM1_DEVICE_ID    0x50
#define M5PM1_DEVICE_MODEL 0x20

/* I2C_CFG[3:0] = SLP_TO, the idle-sleep timeout in seconds (0 = idle-sleep disabled). */
#define M5PM1_I2C_CFG_SLEEP_MASK GENMASK(3, 0)

#define M5PM1_WAKE_DELAY_MS 10
#define M5PM1_SLEEP_SAFETY_MARGIN_MS 100

#define M5PM1_GPIO_COUNT 5U

/*
 * GPIO function-select registers. Each pin selects its function through a 2-bit field: pins 0..3
 * are packed into GPIO_FUNC0, pin 4 lives at offset 0 of GPIO_FUNC1. These registers are shared by
 * every multiplexed peripheral (GPIO, ADC, PWM, NeoPixel), so the MFD owns them and arbitrates
 * access through mfd_m5pm1_pin_request().
 */
#define M5PM1_REG_GPIO_FUNC0 0x16
#define M5PM1_REG_GPIO_FUNC1 0x17

#define M5PM1_GPIO_FUNC_REG(pin)   ((pin) == 4U ? M5PM1_REG_GPIO_FUNC1 : M5PM1_REG_GPIO_FUNC0)
#define M5PM1_GPIO_FUNC_SHIFT(pin) (((pin) == 4U ? 0U : (pin)) * 2U)
#define M5PM1_GPIO_FUNC_MASK(pin)  (BIT_MASK(2) << M5PM1_GPIO_FUNC_SHIFT(pin))

/* 2-bit FUNC field values. */
#define M5PM1_GPIO_FUNC_VAL_GPIO    0x0U
#define M5PM1_GPIO_FUNC_VAL_IRQ     0x1U
#define M5PM1_GPIO_FUNC_VAL_SPECIAL 0x3U /* per-pin: NeoPixel (IO0), ADC (IO1/2), PWM (IO3/4) */

/*
 * Per-pin set of allowed functions. IO1 omits IRQ on purpose: per the datasheet its interrupt line
 * collides with the I2C SDA line.
 */
static const uint8_t m5pm1_pin_caps[M5PM1_GPIO_COUNT] = {
	[0] = BIT(M5PM1_PIN_FUNC_GPIO) | BIT(M5PM1_PIN_FUNC_IRQ) | BIT(M5PM1_PIN_FUNC_NEOPIXEL),
	[1] = BIT(M5PM1_PIN_FUNC_GPIO) | BIT(M5PM1_PIN_FUNC_ADC),
	[2] = BIT(M5PM1_PIN_FUNC_GPIO) | BIT(M5PM1_PIN_FUNC_IRQ) | BIT(M5PM1_PIN_FUNC_ADC),
	[3] = BIT(M5PM1_PIN_FUNC_GPIO) | BIT(M5PM1_PIN_FUNC_IRQ) | BIT(M5PM1_PIN_FUNC_PWM),
	[4] = BIT(M5PM1_PIN_FUNC_GPIO) | BIT(M5PM1_PIN_FUNC_IRQ) | BIT(M5PM1_PIN_FUNC_PWM),
};

struct m5pm1_config {
	struct i2c_dt_spec i2c;
	uint8_t sleep_timeout;
};

struct m5pm1_data {
	struct k_mutex lock;
	const struct device *pin_owner[M5PM1_GPIO_COUNT];
	bool sleep_enabled;
	int64_t last_comm_time;
};

static void m5pm1_force_wake(const struct device *dev)
{
	const struct m5pm1_config *config = dev->config;
	uint8_t dummy;

	if (i2c_reg_read_byte_dt(&config->i2c, M5PM1_REG_DEVICE_ID, &dummy) != 0) {
		k_msleep(M5PM1_WAKE_DELAY_MS);
	}
}

static void m5pm1_wake_if_needed(const struct device *dev)
{
	const struct m5pm1_config *config = dev->config;
	struct m5pm1_data *data = dev->data;
	int64_t wake_after_ms;

	if (!data->sleep_enabled) {
		return;
	}

	wake_after_ms =
		(int64_t)config->sleep_timeout * MSEC_PER_SEC - M5PM1_SLEEP_SAFETY_MARGIN_MS;
	if ((k_uptime_get() - data->last_comm_time) < wake_after_ms) {
		return;
	}

	m5pm1_force_wake(dev);
}

int mfd_m5pm1_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct m5pm1_config *config = dev->config;
	struct m5pm1_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	m5pm1_wake_if_needed(dev);
	ret = i2c_reg_read_byte_dt(&config->i2c, reg, val);
	data->last_comm_time = k_uptime_get();
	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_m5pm1_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct m5pm1_config *config = dev->config;
	struct m5pm1_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	m5pm1_wake_if_needed(dev);
	ret = i2c_reg_write_byte_dt(&config->i2c, reg, val);
	data->last_comm_time = k_uptime_get();
	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_m5pm1_update_reg(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	const struct m5pm1_config *config = dev->config;
	struct m5pm1_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	m5pm1_wake_if_needed(dev);
	ret = i2c_reg_update_byte_dt(&config->i2c, reg, mask, val);
	data->last_comm_time = k_uptime_get();
	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_m5pm1_toggle_reg(const struct device *dev, uint8_t reg, uint8_t mask)
{
	const struct m5pm1_config *config = dev->config;
	struct m5pm1_data *data = dev->data;
	uint8_t val;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	m5pm1_wake_if_needed(dev);
	ret = i2c_reg_read_byte_dt(&config->i2c, reg, &val);
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&config->i2c, reg, val ^ mask);
	}
	data->last_comm_time = k_uptime_get();
	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_m5pm1_burst_read(const struct device *dev, uint8_t reg, uint8_t *buf, size_t len)
{
	const struct m5pm1_config *config = dev->config;
	struct m5pm1_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	m5pm1_wake_if_needed(dev);
	ret = i2c_burst_read_dt(&config->i2c, reg, buf, len);
	data->last_comm_time = k_uptime_get();
	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_m5pm1_burst_write(const struct device *dev, uint8_t reg, const uint8_t *buf, size_t len)
{
	const struct m5pm1_config *config = dev->config;
	struct m5pm1_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	m5pm1_wake_if_needed(dev);
	ret = i2c_burst_write_dt(&config->i2c, reg, buf, len);
	data->last_comm_time = k_uptime_get();
	k_mutex_unlock(&data->lock);

	return ret;
}

static int m5pm1_pin_func_to_reg(enum m5pm1_pin_func func)
{
	switch (func) {
	case M5PM1_PIN_FUNC_GPIO:
		return M5PM1_GPIO_FUNC_VAL_GPIO;
	case M5PM1_PIN_FUNC_IRQ:
		return M5PM1_GPIO_FUNC_VAL_IRQ;
	case M5PM1_PIN_FUNC_ADC:
	case M5PM1_PIN_FUNC_PWM:
	case M5PM1_PIN_FUNC_NEOPIXEL:
		return M5PM1_GPIO_FUNC_VAL_SPECIAL;
	default:
		return -EINVAL;
	}
}

/* Program a pin's 2-bit FUNC field. Caller must hold data->lock. */
static int m5pm1_write_pin_func(const struct device *dev, uint8_t pin, uint8_t func_val)
{
	const struct m5pm1_config *config = dev->config;
	struct m5pm1_data *data = dev->data;
	int ret;

	m5pm1_wake_if_needed(dev);
	ret = i2c_reg_update_byte_dt(&config->i2c, M5PM1_GPIO_FUNC_REG(pin),
				     M5PM1_GPIO_FUNC_MASK(pin),
				     (uint8_t)(func_val << M5PM1_GPIO_FUNC_SHIFT(pin)));
	data->last_comm_time = k_uptime_get();

	return ret;
}

int mfd_m5pm1_pin_request(const struct device *dev, const struct device *client, uint8_t pin,
			  enum m5pm1_pin_func func)
{
	struct m5pm1_data *data = dev->data;
	int func_val;
	int ret;

	if (pin >= M5PM1_GPIO_COUNT) {
		return -EINVAL;
	}

	if ((m5pm1_pin_caps[pin] & BIT(func)) == 0U) {
		LOG_ERR("Pin %u does not support function %d", pin, func);
		return -ENOTSUP;
	}

	func_val = m5pm1_pin_func_to_reg(func);
	if (func_val < 0) {
		return func_val;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->pin_owner[pin] != NULL && data->pin_owner[pin] != client) {
		LOG_ERR("Pin %u already owned by %s; refused request from %s", pin,
			data->pin_owner[pin]->name, client->name);
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	ret = m5pm1_write_pin_func(dev, pin, (uint8_t)func_val);
	if (ret == 0) {
		data->pin_owner[pin] = client;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_m5pm1_pin_release(const struct device *dev, const struct device *client, uint8_t pin)
{
	struct m5pm1_data *data = dev->data;
	int ret = 0;

	if (pin >= M5PM1_GPIO_COUNT) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->pin_owner[pin] == client) {
		ret = m5pm1_write_pin_func(dev, pin, M5PM1_GPIO_FUNC_VAL_GPIO);
		if (ret == 0) {
			data->pin_owner[pin] = NULL;
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int m5pm1_init(const struct device *dev)
{
	const struct m5pm1_config *config = dev->config;
	struct m5pm1_data *data = dev->data;
	uint8_t id;
	uint8_t model;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(config->i2c.bus);
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	/*
	 * The PMIC may boot with idle-sleep enabled (left set by prior firmware) regardless of the
	 * configured timeout, so wake unconditionally before the first probe access.
	 */
	data->sleep_enabled = true;
	m5pm1_force_wake(dev);
	data->last_comm_time = k_uptime_get();

	ret = mfd_m5pm1_read_reg(dev, M5PM1_REG_DEVICE_ID, &id);
	if (ret < 0) {
		LOG_ERR("Failed to read M5PM1 device ID: %d", ret);
		return ret;
	}

	ret = mfd_m5pm1_read_reg(dev, M5PM1_REG_DEVICE_MODEL, &model);
	if (ret < 0) {
		LOG_ERR("Failed to read M5PM1 device model: %d", ret);
		return ret;
	}

	if (id != M5PM1_DEVICE_ID || model != M5PM1_DEVICE_MODEL) {
		LOG_ERR("Unexpected M5PM1 ID/model 0x%02x/0x%02x", id, model);
		return -ENODEV;
	}

	ret = mfd_m5pm1_update_reg(dev, M5PM1_REG_I2C_CFG, M5PM1_I2C_CFG_SLEEP_MASK,
				   config->sleep_timeout);
	if (ret < 0) {
		LOG_ERR("Failed to set M5PM1 I2C idle-sleep timeout: %d", ret);
		return ret;
	}

	data->sleep_enabled = (config->sleep_timeout != 0U);

	return 0;
}

#define M5PM1_DEFINE(inst)                                                                         \
	static const struct m5pm1_config m5pm1_config_##inst = {                                   \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.sleep_timeout = DT_INST_PROP(inst, idle_sleep_timeout_seconds),                   \
	};                                                                                         \
	static struct m5pm1_data m5pm1_data_##inst;                                                \
	DEVICE_DT_INST_DEFINE(inst, m5pm1_init, NULL, &m5pm1_data_##inst, &m5pm1_config_##inst,    \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(M5PM1_DEFINE)
