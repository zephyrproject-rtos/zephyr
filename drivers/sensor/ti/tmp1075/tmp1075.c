/*
 * Copyright (c) 2024 Arrow Electronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp1075

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "tmp1075.h"

LOG_MODULE_REGISTER(TMP1075, CONFIG_SENSOR_LOG_LEVEL);

#define I2C_REG_ADDR_SIZE   1
#define I2C_REG_SENSOR_SIZE sizeof(uint16_t)
#define I2C_BUFFER_SIZE     I2C_REG_ADDR_SIZE + I2C_REG_SENSOR_SIZE

#define I2C_REG_ADDR_OFFSET   0
#define I2C_WRITE_DATA_OFFSET 1

static int tmp1075_reg_read(const struct tmp1075_config *cfg, uint8_t reg, uint16_t *val)
{
	if (i2c_burst_read_dt(&cfg->bus, reg, (uint8_t *)val, sizeof(*val)) < 0) {
		return -EIO;
	}
	*val = sys_be16_to_cpu(*val);
	return 0;
}

static int tmp1075_reg_write(const struct tmp1075_config *cfg, uint8_t reg, uint16_t val)
{
	uint8_t buf[I2C_REG_ADDR_SIZE + I2C_REG_SENSOR_SIZE];

	buf[I2C_REG_ADDR_OFFSET] = reg;
	sys_put_be16(val, &buf[I2C_WRITE_DATA_OFFSET]);

	return i2c_write_dt(&cfg->bus, buf, sizeof(buf));
}

#if CONFIG_TMP1075_ALERT_INTERRUPTS
static int set_threshold_attribute(const struct device *dev, uint8_t reg, int16_t value,
				   const char *error_msg)
{
	if (tmp1075_reg_write(dev->config, reg, value) < 0) {
		LOG_ERR("Failed to set %s attribute!", error_msg);
		return -EIO;
	}
	return 0;
}
#endif

static int tmp1075_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
#if CONFIG_TMP1075_ALERT_INTERRUPTS
	case SENSOR_ATTR_LOWER_THRESH:
		return set_threshold_attribute(dev, TMP1075_REG_TLOW, val->val1 << 8,
					       "SENSOR_ATTR_LOWER_THRESH");

	case SENSOR_ATTR_UPPER_THRESH:
		return set_threshold_attribute(dev, TMP1075_REG_THIGH, val->val1 << 8,
					       "SENSOR_ATTR_UPPER_THRESH");
#endif

	default:
		return -ENOTSUP;
	}
}

#if CONFIG_TMP1075_ALERT_INTERRUPTS
static int get_threshold_attribute(const struct device *dev, uint8_t reg, struct sensor_value *val,
				   const char *error_msg)
{
	uint16_t value;

	if (tmp1075_reg_read(dev->config, reg, &value) < 0) {
		LOG_ERR("Failed to get %s attribute!", error_msg);
		return -EIO;
	}
	val->val1 = value >> 8;
	return 0;
}
#endif

static int tmp1075_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
#if CONFIG_TMP1075_ALERT_INTERRUPTS
	case SENSOR_ATTR_LOWER_THRESH:
		return get_threshold_attribute(dev, TMP1075_REG_TLOW, val,
					       "SENSOR_ATTR_LOWER_THRESH");

	case SENSOR_ATTR_UPPER_THRESH:
		return get_threshold_attribute(dev, TMP1075_REG_THIGH, val,
					       "SENSOR_ATTR_UPPER_THRESH");
#endif

	default:
		return -ENOTSUP;
	}
}

static int tmp1075_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct tmp1075_data *drv_data = dev->data;
	const struct tmp1075_config *cfg = dev->config;
	uint16_t val;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (tmp1075_reg_read(cfg, TMP1075_REG_TEMPERATURE, &val) < 0) {
		return -EIO;
	}
	drv_data->sample = arithmetic_shift_right((int16_t)val, TMP1075_DATA_NORMAL_SHIFT);
	return 0;
}

static int tmp1075_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct tmp1075_data *drv_data = dev->data;
	int32_t uval;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	uval = (int32_t)drv_data->sample * TMP1075_TEMP_SCALE;
	val->val1 = uval / uCELSIUS_IN_CELSIUS;
	val->val2 = uval % uCELSIUS_IN_CELSIUS;

	return 0;
}

static const struct sensor_driver_api tmp1075_driver_api = {
	.attr_set = tmp1075_attr_set,
	.attr_get = tmp1075_attr_get,
	.sample_fetch = tmp1075_sample_fetch,
	.channel_get = tmp1075_channel_get,
#ifdef CONFIG_TMP1075_ALERT_INTERRUPTS
	.trigger_set = tmp1075_trigger_set,
#endif
};

#ifdef CONFIG_TMP1075_ALERT_INTERRUPTS
static int setup_interrupts(const struct device *dev)
{
	struct tmp1075_data *drv_data = dev->data;
	const struct tmp1075_config *config = dev->config;
	const struct gpio_dt_spec *alert_gpio = &config->alert_gpio;
	int result;

	if (!gpio_is_ready_dt(alert_gpio)) {
		LOG_ERR("tmp1075: gpio controller %s not ready", alert_gpio->port->name);
		return -ENODEV;
	}

	result = gpio_pin_configure_dt(alert_gpio, GPIO_INPUT);

	if (result < 0) {
		return result;
	}

	gpio_init_callback(&drv_data->temp_alert_gpio_cb, tmp1075_trigger_handle_alert,
			   BIT(alert_gpio->pin));

	result = gpio_add_callback(alert_gpio->port, &drv_data->temp_alert_gpio_cb);

	if (result < 0) {
		return result;
	}

	result = gpio_pin_interrupt_configure_dt(alert_gpio, GPIO_INT_EDGE_BOTH);

	if (result < 0) {
		return result;
	}

	return 0;
}
#endif

static int tmp1075_init(const struct device *dev)
{
	const struct tmp1075_config *cfg = dev->config;
	struct tmp1075_data *data = dev->data;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -EINVAL;
	}
#ifdef CONFIG_TMP1075_ALERT_INTERRUPTS
	int result = setup_interrupts(dev);

	if (result < 0) {
		LOG_ERR("Couldn't setup interrupts");
		return -EIO;
	}
#endif
	data->tmp1075_dev = dev;

	uint16_t config_reg = 0;

	TMP1075_SET_ONE_SHOT_CONVERSION(config_reg, cfg->one_shot);
	TMP1075_SET_CONVERSION_RATE(config_reg, cfg->cr);
	TMP1075_SET_CONSECUTIVE_FAULT_MEASUREMENTS(config_reg, cfg->cf);
	TMP1075_SET_ALERT_PIN_POLARITY(config_reg, cfg->alert_pol);
	TMP1075_SET_ALERT_PIN_FUNCTION(config_reg, cfg->interrupt_mode);
	TMP1075_SET_SHUTDOWN_MODE(config_reg, cfg->shutdown_mode);

	int rc = tmp1075_reg_write(dev->config, TMP1075_REG_CONFIG, config_reg);

	if (rc == 0) {
		data->config_reg = config_reg;
	}
	return rc;
}

#define TMP1075_INST(inst)                                                                         \
	static struct tmp1075_data tmp1075_data_##inst;                                            \
	static const struct tmp1075_config tmp1075_config_##inst = {                               \
		.cr = DT_INST_ENUM_IDX(inst, conversion_rate),                                     \
		.cf = DT_INST_ENUM_IDX(inst, consecutive_fault_measurements),                      \
		.alert_pol = DT_INST_PROP(inst, alert_pin_active_high),                            \
		.interrupt_mode = DT_INST_PROP(inst, interrupt_mode),                              \
		.shutdown_mode = DT_INST_PROP(inst, shutdown_mode),                                \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),                    \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tmp1075_init, NULL, &tmp1075_data_##inst,               \
                                                                                                   \
				     &tmp1075_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmp1075_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMP1075_INST)
