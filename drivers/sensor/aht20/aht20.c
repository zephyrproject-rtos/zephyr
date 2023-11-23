/*
 * Copyright (c) 2023 Konrad Sikora
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aosong_aht20

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>

#if defined(CONFIG_AHT20_CRC)
#include <zephyr/sys/crc.h>
#endif

#define AHT20_INITIALIZATION_CMD     0xBE
#define AHT20_INITIALIZATION_PARAM0  0x08
#define AHT20_INITIALIZATION_PARAM1  0x00
#define AHT20_TRIGGER_MEASURE_CMD    0xAC
#define AHT20_TRIGGER_MEASURE_PARAM0 0x33
#define AHT20_TRIGGER_MEASURE_PARAM1 0x00
#define AHT20_SOFT_RESET_CMD         0xBA
#define AHT20_GET_STATUS_CMD         0x71

#define AHT20_STATUS_CALIBRATED_BIT     BIT(3)
#define AHT20_STATUS_BUSY_BIT           BIT(7)
#define AHT20_STATUS_CALIBRATED(status) ((status) & AHT20_STATUS_CALIBRATED_BIT)
#define AHT20_STATUS_BUSY(status)       ((status) & AHT20_STATUS_BUSY_BIT)

#define AHT20_MEAS_STATUS_IDX      0
#define AHT20_MEAS_HUMIDITY_IDX    1
#define AHT20_MEAS_HUM_TEMP_IDX    3
#define AHT20_MEAS_TEMPERATURE_IDX 4
#define AHT20_MEAS_CRC_IDX         6
#define AHT20_MEAS_FRAME_SIZE      7

#define AHT20_CRC8_POLYNOMIAL 0x31
#define AHT20_CRC8_INIT       0xFF

#define AHT20_POWER_ON_INIT_TIME 40
#define AHT20_CALIBRATION_TIME   10
#define AHT20_MEASUREMENT_TIME   80

struct aht20_config {
	const struct i2c_dt_spec bus;
};

struct aht20_data {
	int32_t temperature;
	int32_t humidity;
};

LOG_MODULE_REGISTER(AHT20, CONFIG_SENSOR_LOG_LEVEL);

static int aht20_init(const struct device *dev)
{
	const struct aht20_config *cfg = dev->config;
	const uint8_t status_cmd[] = {AHT20_GET_STATUS_CMD};
	uint8_t frame;
	int rc;

	k_sleep(K_MSEC(AHT20_POWER_ON_INIT_TIME));

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	rc = i2c_write_read_dt(&cfg->bus, status_cmd, sizeof(status_cmd), &frame, sizeof(frame));
	if (rc < 0) {
		LOG_ERR("Failed to get AHT20 status");
		return rc;
	}

	if (!AHT20_STATUS_CALIBRATED(frame)) {
		const uint8_t init_cmd[] = {AHT20_INITIALIZATION_CMD, AHT20_INITIALIZATION_PARAM0,
					    AHT20_INITIALIZATION_PARAM1};

		rc = i2c_write_dt(&cfg->bus, init_cmd, sizeof(init_cmd));
		if (rc < 0) {
			LOG_ERR("Failed to initialize AHT20");
			return rc;
		}
		k_sleep(K_MSEC(AHT20_CALIBRATION_TIME));
	}

	return 0;
}

static void aht20_convert_temp_to_celsius(int32_t raw_val, struct sensor_value *const val)
{
	/* Fixed-point arithmetic using scaling factor of 1000000 */
	const int64_t scaling_factor = 1000000;

	/* First multiply raw value by 200 */
	int64_t temperature = (int64_t)raw_val * 200 * scaling_factor;

	/* Divide by 2^20 and subtract 50 */
	temperature = (temperature >> 20);
	temperature -= 50 * scaling_factor;

	/* Get the integer part and fractional part */
	val->val1 = temperature / scaling_factor;
	val->val2 = temperature % scaling_factor;
}

static void aht20_convert_humidity_to_pct(int32_t raw_val, struct sensor_value *const val)
{
	/* Fixed-point arithmetic using scaling factor of 1000000 */
	const int64_t scaling_factor = 1000000;

	/* First multiply by 100 */
	int64_t humidity = (int64_t)raw_val * 100 * scaling_factor;

	/* Divide by 2^20 */
	humidity = (humidity >> 20);

	/* Get the integer part and fractional part */
	val->val1 = humidity / scaling_factor;
	val->val2 = humidity % scaling_factor;
}

static int aht20_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct aht20_data *const data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		aht20_convert_temp_to_celsius(data->temperature, val);
		break;
	case SENSOR_CHAN_HUMIDITY:
		aht20_convert_humidity_to_pct(data->humidity, val);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int aht20_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct aht20_config *cfg = dev->config;
	struct aht20_data *data = dev->data;
	uint8_t frame[AHT20_MEAS_FRAME_SIZE];
	int rc;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP) &&
	    (chan != SENSOR_CHAN_HUMIDITY)) {
		return -ENOTSUP;
	}

	const uint8_t measure_cmd[] = {AHT20_TRIGGER_MEASURE_CMD, AHT20_TRIGGER_MEASURE_PARAM0,
				       AHT20_TRIGGER_MEASURE_PARAM1};
	rc = i2c_write_dt(&cfg->bus, measure_cmd, sizeof(measure_cmd));
	if (rc < 0) {
		LOG_ERR("Failed to trigger AHT20 measurement");
		return rc;
	}

	/* Wait for the measurement to be completed. */
	k_sleep(K_MSEC(AHT20_MEASUREMENT_TIME));
	do {
		rc = i2c_read_dt(&cfg->bus, frame, sizeof(frame));
		if (rc < 0) {
			return rc;
		}
		k_sleep(K_MSEC(3));
	} while (AHT20_STATUS_BUSY(frame[AHT20_MEAS_STATUS_IDX]));

#if defined(CONFIG_AHT20_CRC)
	uint8_t crc = crc8(frame, AHT20_MEAS_FRAME_SIZE - 1, AHT20_CRC8_POLYNOMIAL, AHT20_CRC8_INIT,
			   false);
	if (crc != frame[AHT20_MEAS_CRC_IDX]) {
		LOG_WRN("CRC mismatch");
		return -EIO;
	}
#endif

	/* Get the first 20 bits for the humidity */
	data->humidity = sys_get_be24(&frame[AHT20_MEAS_HUMIDITY_IDX]) >> 4;
	/* And the next 20 bits for the temperature */
	data->temperature = sys_get_be24(&frame[AHT20_MEAS_HUM_TEMP_IDX]) & 0x000FFFFF;

	return 0;
}

static const struct sensor_driver_api aht20_driver_api = {
	.sample_fetch = aht20_sample_fetch,
	.channel_get = aht20_channel_get,
};

#define DEFINE_AHT20(_num)                                                                         \
	static struct aht20_data aht20_data_##_num;                                                \
	static const struct aht20_config aht20_config_##_num = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(_num),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, aht20_init, NULL, &aht20_data_##_num,                   \
				     &aht20_config_##_num, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &aht20_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_AHT20)
