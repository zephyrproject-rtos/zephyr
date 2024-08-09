/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "adxl34x_attr.h"
#include "adxl34x_configure.h"
#include "adxl34x_decoder.h"
#include "adxl34x_private.h"
#include "adxl34x_reg.h"
#include "adxl34x_rtio.h"
#include "adxl34x_trigger.h"
#include "adxl34x_convert.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include "adxl34x_i2c.h"
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include "adxl34x_spi.h"
#endif

LOG_MODULE_REGISTER(adxl34x, CONFIG_SENSOR_LOG_LEVEL);

/**
 * Convert a raw sensor value from the sensor registery to a value in g
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] value Pointer to store the result
 * @param[in] raw_value The value of the raw sensor data
 * @param[in] range_scale The scale of range
 */
static void adxl34x_convert_sample(struct sensor_value *value, int16_t raw_value,
				   uint16_t range_scale)
{
	int32_t val_ug = raw_value * range_scale * 100;

	sensor_ug_to_ms2(val_ug, value);
}

/**
 * Callback API for fetching data of the sensor
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] chan The channel that needs updated
 * @return 0 if successful, negative errno code if failure.
 * @see sensor_sample_fetch() for more information.
 */
static int adxl34x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	int rc;
	uint8_t rx_buf[6];
	enum pm_device_state pm_state;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ACCEL_X && chan != SENSOR_CHAN_ACCEL_Y &&
	    chan != SENSOR_CHAN_ACCEL_Z && chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	rc = pm_device_state_get(dev, &pm_state);
	if (rc == 0 && pm_state != PM_DEVICE_STATE_ACTIVE) {
		LOG_DBG("Device is suspended, fetch is unavailable");
		return -EIO;
	}

	data->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());

#if CONFIG_ADXL34X_ADXL345_COMPATIBLE
	struct adxl34x_fifo_status fifo_status;

	rc = adxl34x_get_fifo_status(dev, &fifo_status);
	if (rc != 0) {
		LOG_ERR("Failed to read from device");
		return -EIO;
	}

	__ASSERT_NO_MSG(ARRAY_SIZE(data->accel_x) >= fifo_status.entries);
	for (uint8_t i = 0; i < fifo_status.entries; i++) {
		/* Read accel x, y and z values */
		rc = config->bus_read_buf(dev, ADXL34X_REG_DATA, rx_buf, sizeof(rx_buf));
		if (rc != 0) {
			LOG_ERR("Failed to read from device");
			return -EIO;
		}

		if (chan == SENSOR_CHAN_ACCEL_X || chan == SENSOR_CHAN_ACCEL_XYZ ||
		    chan == SENSOR_CHAN_ALL) {
			data->accel_x[i] = sys_get_le16(rx_buf);
		}
		if (chan == SENSOR_CHAN_ACCEL_Y || chan == SENSOR_CHAN_ACCEL_XYZ ||
		    chan == SENSOR_CHAN_ALL) {
			data->accel_y[i] = sys_get_le16(rx_buf + 2);
		}
		if (chan == SENSOR_CHAN_ACCEL_Z || chan == SENSOR_CHAN_ACCEL_XYZ ||
		    chan == SENSOR_CHAN_ALL) {
			data->accel_z[i] = sys_get_le16(rx_buf + 4);
		}
	}

	data->sample_number = 0;
	return fifo_status.entries; /* Return the number of samples fetched */

#else
	/* Read accel x, y and z values */
	rc = config->bus_read_buf(dev, ADXL34X_REG_DATA, rx_buf, sizeof(rx_buf));
	if (rc != 0) {
		LOG_ERR("Failed to read from device");
		return -EIO;
	}

	if (chan == SENSOR_CHAN_ACCEL_X || chan == SENSOR_CHAN_ACCEL_XYZ ||
	    chan == SENSOR_CHAN_ALL) {
		data->accel_x = sys_get_le16(rx_buf);
	}
	if (chan == SENSOR_CHAN_ACCEL_Y || chan == SENSOR_CHAN_ACCEL_XYZ ||
	    chan == SENSOR_CHAN_ALL) {
		data->accel_y = sys_get_le16(rx_buf + 2);
	}
	if (chan == SENSOR_CHAN_ACCEL_Z || chan == SENSOR_CHAN_ACCEL_XYZ ||
	    chan == SENSOR_CHAN_ALL) {
		data->accel_z = sys_get_le16(rx_buf + 4);
	}
	return 0;
#endif /* CONFIG_ADXL34X_ADXL345_COMPATIBLE */
}

/**
 * Callback API for getting a reading from a sensor
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] chan The channel to read
 * @param[out] val Where to store the value
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl34x_dev_data *data = dev->data;
	uint16_t range_scale = adxl34x_range_conv[data->cfg.data_format.range];

#if CONFIG_ADXL34X_ADXL345_COMPATIBLE
	if (data->sample_number >= ADXL34X_FIFO_SIZE) {
		return -ENODATA;
	}
	switch (chan) { /* NOLINT(clang-diagnostic-switch-enum) */
	case SENSOR_CHAN_ACCEL_X:
		adxl34x_convert_sample(val, data->accel_x[data->sample_number], range_scale);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl34x_convert_sample(val, data->accel_y[data->sample_number], range_scale);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl34x_convert_sample(val, data->accel_z[data->sample_number], range_scale);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ALL:
		adxl34x_convert_sample(&val[0], data->accel_x[data->sample_number], range_scale);
		adxl34x_convert_sample(&val[1], data->accel_y[data->sample_number], range_scale);
		adxl34x_convert_sample(&val[2], data->accel_z[data->sample_number], range_scale);
		break;
	default:
		return -ENOTSUP;
	}
	data->sample_number++;

#else
	switch (chan) { /* NOLINT(clang-diagnostic-switch-enum) */
	case SENSOR_CHAN_ACCEL_X:
		adxl34x_convert_sample(val, data->accel_x, range_scale);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl34x_convert_sample(val, data->accel_y, range_scale);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl34x_convert_sample(val, data->accel_z, range_scale);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ALL:
		adxl34x_convert_sample(&val[0], data->accel_x, range_scale);
		adxl34x_convert_sample(&val[1], data->accel_y, range_scale);
		adxl34x_convert_sample(&val[2], data->accel_z, range_scale);
		break;
	default:
		return -ENOTSUP;
	}
#endif /* CONFIG_ADXL34X_ADXL345_COMPATIBLE */

	return 0;
}

/**
 * @brief The sensor driver API callbacks
 * @var adxl34x_api
 */
static const struct sensor_driver_api adxl34x_api = {
	.sample_fetch = &adxl34x_sample_fetch,
	.channel_get = &adxl34x_channel_get,
	.attr_set = adxl34x_attr_set,
	.attr_get = adxl34x_attr_get,
#ifdef CONFIG_ADXL34X_TRIGGER
	.trigger_set = adxl34x_trigger_set,
#endif
#ifdef CONFIG_ADXL34X_DECODER
	.get_decoder = adxl34x_get_decoder,
#endif
#ifdef CONFIG_ADXL34X_ASYNC_API
	.submit = adxl34x_submit,
#endif
};

/**
 * The init function of the driver
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_init(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	int rc;
	uint8_t devid;

	if (config->bus_init(dev)) {
		LOG_ERR("Device not ready");
		return -ENODEV;
	}

	rc = adxl34x_get_devid(dev, &devid);
	const bool known_device = devid == ADXL343_DEVID || devid == ADXL344_DEVID ||
				  devid == ADXL345_DEVID || /* NOLINT(misc-redundant-expression) */
				  devid == ADXL346_DEVID;   /* NOLINT(misc-redundant-expression) */
	if (rc != 0 || !known_device) {
		LOG_ERR("Failed to read id from device (%s)", dev->name);
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_ADXL34X_TRIGGER)) {
		rc = adxl34x_trigger_init(dev);
		if (rc != 0) {
			LOG_ERR("Failed to initialize device (%s) triggers", dev->name);
			return -EIO;
		}
	}

	/* Check if the configuration in the device tree provided is valid. */
	if (config->dt_int_pin != 1 && config->dt_int_pin != 2) {
		LOG_ERR("Failed to configure device (%s), invalid int-pin provided (%d)", dev->name,
			config->dt_int_pin);
		return -ENOTSUP;
	}
	if (config->dt_packet_size < 1 || config->dt_packet_size > 31) {
		LOG_ERR("Failed to configure device (%s), invalid packet-size provided (%d)",
			dev->name, config->dt_packet_size);
		return -ENOTSUP;
	}

	/* The adxl34x doesn't have a reset option, so defaults are set explicitly. */
	rc = adxl34x_get_configuration(dev);
	if (rc != 0) {
		LOG_ERR("Failed to read configuration from device (%s)", dev->name);
		return -EIO;
	}
	struct adxl34x_cfg cfg = {
		/* Initialize the sensor in the suspended state when power management is active. */
		.power_ctl.measure = COND_CODE_1(CONFIG_PM_DEVICE_RUNTIME, (0), (1)),
		/* Directly enable stream mode when in adxl345 compatibility mode. */
		.fifo_ctl.fifo_mode =
			COND_CODE_1(CONFIG_ADXL34X_ADXL345_COMPATIBLE, (ADXL34X_FIFO_MODE_STREAM),
				    (ADXL34X_FIFO_MODE_BYPASS)),
		.bw_rate.rate = config->dt_rate,
		.data_format.range = config->dt_range,
	};

#ifdef CONFIG_ADXL34X_EXTENDED_API
	if (devid == ADXL344_DEVID || devid == ADXL346_DEVID) {
		cfg.orient_conf = (struct adxl34x_orient_conf){
			.dead_zone = ADXL34X_DEAD_ZONE_ANGLE_15_2,
			.divisor = ADXL34X_DIVISOR_ODR_400,
		};
	}
#endif /* CONFIG_ADXL34X_EXTENDED_API */

	rc = adxl34x_configure(dev, &cfg);
	if (rc != 0) {
		LOG_ERR("Failed to write configuration to device (%s)", dev->name);
		return -EIO;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	/* Enable device runtime power management. */
	pm_device_init_suspended(dev);

	rc = pm_device_runtime_enable(dev);
	if (rc < 0 && rc != -ENOSYS) {
		LOG_ERR("Failed to enabled runtime power management");
		return -EIO;
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	return 0;
}

#ifdef CONFIG_PM_DEVICE_RUNTIME

/**
 * Set the power state to active or inactive
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] active The new state of the sensor
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_set_active_state(const struct device *dev, bool active)
{
	int rc;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_power_ctl power_ctl = data->cfg.power_ctl;

	power_ctl.measure = active;
	rc = adxl34x_set_power_ctl(dev, &power_ctl);
	if (rc != 0) {
		LOG_WRN("Failed to set device into %s mode", active ? "active" : "suspended");
	}
	return rc;
}

/**
 * Callback API for power management when PM state has changed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] action Requested action
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_pm(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		adxl34x_set_active_state(dev, true);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		adxl34x_set_active_state(dev, false);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
	default:
		return -ENOTSUP;
	}

	return 0;
}

#endif /* CONFIG_PM_DEVICE_RUNTIME */

#define ADXL34X_INIT(i)                                                                            \
	static struct adxl34x_dev_data adxl34x_dev_data_##i;                                       \
                                                                                                   \
	static const struct adxl34x_dev_config adxl34x_dev_config_##i = {                          \
		COND_CODE_1(DT_INST_ON_BUS(i, spi), (ADXL34X_CONFIG_SPI(i)),                       \
			    (ADXL34X_CONFIG_I2C(i))),                                              \
		.gpio_int1 = GPIO_DT_SPEC_INST_GET_OR(i, int_gpios, {0}),                          \
		.dt_int_pin = DT_INST_PROP(i, int_pin),                                            \
		.dt_packet_size = DT_INST_PROP(i, packet_size),                                    \
		.dt_rate = DT_INST_ENUM_IDX(i, accel_frequency),                                   \
		.dt_range = DT_INST_ENUM_IDX(i, accel_range),                                      \
	};                                                                                         \
                                                                                                   \
	IF_ENABLED(CONFIG_PM_DEVICE_RUNTIME, (PM_DEVICE_DT_INST_DEFINE(i, adxl34x_pm)));           \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(                                                              \
		i, adxl34x_init,                                                                   \
		COND_CODE_1(CONFIG_PM_DEVICE_RUNTIME, (PM_DEVICE_DT_INST_GET(i)), (NULL)),         \
		&adxl34x_dev_data_##i, &adxl34x_dev_config_##i, POST_KERNEL,                       \
		CONFIG_SENSOR_INIT_PRIORITY, &adxl34x_api);

DT_INST_FOREACH_STATUS_OKAY(ADXL34X_INIT)
