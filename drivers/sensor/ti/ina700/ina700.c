/*
 * Copyright (c) 2024 Esco Medical ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina700

#include "ina700.h"
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(INA700, CONFIG_SENSOR_LOG_LEVEL);

static void ina700_lock_data(const struct device *dev);
static void ina700_unlock_data(const struct device *dev);

static int reg_read_24(const struct device *dev, uint8_t addr, uint32_t *data)
{
	const struct ina700_config *cfg = dev->config;
	uint8_t rx_buf[3];
	int rc = i2c_burst_read_dt(&cfg->bus, addr, rx_buf, sizeof(rx_buf));

	if (rc == 0) {
		*data = sys_get_be24(rx_buf);
	}

	return rc;
}

static int reg_read_16(const struct device *dev, uint8_t addr, uint16_t *data)
{
	const struct ina700_config *cfg = dev->config;
	uint8_t rx_buf[2];
	int rc = i2c_burst_read_dt(&cfg->bus, addr, rx_buf, sizeof(rx_buf));

	if (rc == 0) {
		*data = sys_get_be16(rx_buf);
	}

	return rc;
}

static int reg_write_16(const struct device *dev, uint8_t addr, uint16_t data)
{
	const struct ina700_config *cfg = dev->config;
	uint8_t tx_buf[3];

	tx_buf[0] = addr;
	sys_put_be16(data, &tx_buf[1]);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int ina700_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int rc;
	struct ina700_data *data = dev->data;

	ina700_lock_data(dev);

	if ((chan == SENSOR_CHAN_VOLTAGE) || (chan == SENSOR_CHAN_ALL)) {
		rc = reg_read_16(dev, INA700_REG_VBUS, &data->voltage);

		if (rc < 0) {
			goto exit;
		}
	}

	if ((chan == SENSOR_CHAN_CURRENT) || (chan == SENSOR_CHAN_ALL)) {
		rc = reg_read_16(dev, INA700_REG_CURRENT, &data->current);

		if (rc < 0) {
			goto exit;
		}
	}

	if ((chan == SENSOR_CHAN_POWER) || (chan == SENSOR_CHAN_ALL)) {
		rc = reg_read_24(dev, INA700_REG_POWER, &data->power);

		if (rc < 0) {
			goto exit;
		}
	}

	if ((chan == SENSOR_CHAN_DIE_TEMP) || (chan == SENSOR_CHAN_ALL)) {
		rc = reg_read_16(dev, INA700_REG_DIE_TEMPERATURE, &data->temperature);

		if (rc < 0) {
			goto exit;
		}
	}

exit:
	ina700_unlock_data(dev);
	return 0;
}

static int ina700_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	int rc = 0;
	int32_t temp = 0;
	struct ina700_data *data = dev->data;

	ina700_lock_data(dev);

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		temp = data->voltage * INA700_VOLTAGE_LSB;
		val->val1 = temp / 1000000U;
		val->val2 = temp % 1000000U;
		break;

	case SENSOR_CHAN_CURRENT:
		temp = data->current * INA700_CURRENT_LSB;
		val->val1 = temp / 1000000L;
		val->val2 = temp % 1000000L;
		break;

	case SENSOR_CHAN_POWER:
		temp = data->power * INA700_POWER_LSB;
		val->val1 = temp / 1000000U;
		val->val2 = temp % 1000000U;
		break;

	case SENSOR_CHAN_DIE_TEMP:
		temp = FIELD_GET(IAN700_DIE_TEMPERATURE, data->temperature);
		temp *= INA700_TEMPERATURE_LSB; /* m°C */
		val->val1 = temp / 1000L;
		val->val2 = temp % 1000L;
		break;

	default:
		rc = -ENOTSUP;
		break;
	}

	ina700_unlock_data(dev);

	return 0;
}

#ifdef CONFIG_INA700_TRIGGER

static void ina700_lock_data(const struct device *dev)
{
	struct ina700_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);
}

static void ina700_unlock_data(const struct device *dev)
{
	struct ina700_data *data = dev->data;

	k_mutex_unlock(&data->mutex);
}

static void ina700_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct ina700_data *data = CONTAINER_OF(cb, struct ina700_data, gpio_cb);

	k_work_submit(&data->work);
}

static void ina700_work_cb(struct k_work *work)
{
	uint16_t status = 0;
	struct ina700_data *data = CONTAINER_OF(work, struct ina700_data, work);

	ina700_lock_data(data->this);

	if (reg_read_16(data->this, INA700_REG_ALERT_DIAGNOSTICS, &status) < 0) {
		goto exit;
	}

	if (!(status & INA700_ALERT_DIAG_CONVERSION_READY)) {
		goto exit;
	}

	if (ina700_sample_fetch(data->this, SENSOR_CHAN_ALL) < 0) {
		goto exit;
	}

	if (data->handler_alert) {
		data->handler_alert(data->this, data->trigger_alert);
	}

exit:
	ina700_unlock_data(data->this);
}

int ina700_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct ina700_data *data = dev->data;

	data->handler_alert = handler;
	data->trigger_alert = trig;

	return 0;
}

int ina700_trigger_init(const struct device *dev)
{
	int rc = 0;
	struct ina700_data *data = dev->data;
	const struct ina700_config *cfg = dev->config;

	k_mutex_init(&data->mutex);
	data->this = dev; /* for the trigger callback */
	data->work.handler = ina700_work_cb;

	if (!gpio_is_ready_dt(&cfg->alert_gpio)) {
		LOG_ERR("Alert GPIO device not ready");
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&cfg->alert_gpio, GPIO_INPUT);

	if (rc < 0) {
		LOG_ERR("Failed to configure alert GPIO pin");
		return rc;
	}

	gpio_init_callback(&data->gpio_cb, ina700_gpio_callback, BIT(cfg->alert_gpio.pin));
	rc = gpio_add_callback(cfg->alert_gpio.port, &data->gpio_cb);

	if (rc < 0) {
		LOG_ERR("Failed to add alert GPIO callback");
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	if (rc < 0) {
		LOG_ERR("Failed to configure alert GPIO interrupt");
		return rc;
	}

	/* only trigger on conversion ready, after averaging is done */
	uint16_t alert_config =
		INA700_ALERT_DIAG_CONVERSION_ALERT_ENABLE | INA700_ALERT_DIAG_SLOW_ALERT_MODE;

	return reg_write_16(dev, INA700_REG_ALERT_DIAGNOSTICS, alert_config);
}

#else

static void ina700_lock_data(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void ina700_unlock_data(const struct device *dev)
{
	ARG_UNUSED(dev);
}

#endif

static int ina700_init(const struct device *dev)
{
	uint16_t id = 0;
	const struct ina700_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C bus %s is not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	if (reg_read_16(dev, INA700_REG_MANUFACTURER_ID, &id) < 0) {
		LOG_ERR("Failed to read manufacturer ID");
		return -EIO;
	}

	if (id != INA700_MANUFACTURER_ID) {
		LOG_ERR("Invalid manufacturer ID");
		return -ENODEV;
	}

	uint16_t config =
		FIELD_PREP(INA700_ADC_CONFIG_MODE, cfg->mode) |
		FIELD_PREP(INA700_ADC_CONFIG_VBUS_CONVERSION_TIME, cfg->vbus_conv_time) |
		FIELD_PREP(INA700_ADC_CONFIG_SHUNT_VOLTAGE_CONVERSION_TIME,
			   cfg->shunt_voltage_conv_time) |
		FIELD_PREP(INA700_ADC_CONFIG_TEMPERATURE_CONVERSION_TIME,
			   cfg->temperature_conv_time) |
		FIELD_PREP(INA700_ADC_CONFIG_SAMPLE_AVERAGING_COUNT, cfg->sample_averaging_count);

	if (reg_write_16(dev, INA700_REG_ADC_CONFIG, config) < 0) {
		LOG_ERR("Failed to write ADC config register");
		return -EIO;
	}

#ifdef CONFIG_INA700_TRIGGER
	if (ina700_trigger_init(dev) < 0) {
		LOG_ERR("Failed to initialize trigger");
		return -EIO;
	}
#endif

	return 0;
}

static const struct sensor_driver_api ina700_api = {
	.sample_fetch = ina700_sample_fetch,
	.channel_get = ina700_channel_get,
#ifdef CONFIG_INA700_TRIGGER
	.trigger_set = ina700_trigger_set,
#endif
};

#ifdef CONFIG_INA700_TRIGGER
#define INA700_TRIGGER_INIT(inst) .alert_gpio = GPIO_DT_SPEC_INST_GET(inst, alert_gpios),
#else
#define INA700_TRIGGER_INIT(inst)
#endif

#define INA700_DRIVER_INIT(inst)                                                                   \
	static const struct ina700_config ina700_config_##inst = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.mode = DT_INST_ENUM_IDX(inst, adc_mode),                                          \
		.vbus_conv_time = DT_INST_ENUM_IDX(inst, vbus_conversion_time_us),                 \
		.shunt_voltage_conv_time = DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us),      \
		.temperature_conv_time = DT_INST_ENUM_IDX(inst, temperature_conversion_time_us),   \
		.sample_averaging_count = DT_INST_ENUM_IDX(inst, sample_averaging_count),          \
		INA700_TRIGGER_INIT(inst)};                                                        \
                                                                                                   \
	static struct ina700_data ina700_data_##inst;                                              \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina700_init, NULL, &ina700_data_##inst,                \
				     &ina700_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &ina700_api);

DT_INST_FOREACH_STATUS_OKAY(INA700_DRIVER_INIT);
