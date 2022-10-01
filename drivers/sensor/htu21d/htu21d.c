/* htu21d.c - Driver for HTU21D humidity and temperature sensor */

/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_htu21d

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(HTU21D, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "HTU21D driver enabled without any devices"
#endif

#define HTU21D_TEMPERATURE_MEASUREMENT_HOLD_MASTER    0xE3
#define HTU21D_HUMIDITY_MEASUREMENT_HOST_MASTER       0xE5
#define HTU21D_TEMPERATURE_MEASUREMENT_NO_HOLD_MASTER 0xF3
#define HTU21D_HUMIDITY_MEASUREMENT_NO_HOLD_MASTER    0xF5
#define HTU21D_WRITE_USER_REGISTER                    0xE6
#define HTU21D_READ_USER_REGISTER                     0xE7
#define HTU21D_SOFT_RESET                             0xFE

struct htu21d_data {
	uint16_t humidity_raw_val;
	uint16_t temperature_raw_val;
};

struct htu21d_config {
	struct i2c_dt_spec i2c;
};

static inline int htu21d_is_ready(const struct device *dev)
{
	const struct htu21d_config *cfg = dev->config;

	return device_is_ready(cfg->i2c.bus) ? 0 : -ENODEV;
}

static inline int htu21d_read(const struct device *dev, uint8_t *buf, int size)
{
	const struct htu21d_config *cfg = dev->config;

	return i2c_read_dt(&cfg->i2c, buf, size);
}

static inline int htu21d_write(const struct device *dev, uint8_t val)
{
	const struct htu21d_config *cfg = dev->config;
	uint8_t buf = val;

	return i2c_write_dt(&cfg->i2c, &buf, sizeof(buf));
}

static uint8_t htu21d_compute_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);
	return crc8(buf, 2, 0x31, 0x00, false);
}

static int htu21d_humidity_fetch(const struct device *dev)
{
	uint8_t buf[3];
	uint16_t tmp;
	uint8_t crc;
	int ret;

	ret = htu21d_write(dev, HTU21D_HUMIDITY_MEASUREMENT_NO_HOLD_MASTER);
	if (ret < 0) {
		LOG_DBG("Humidity measurement failed: %d", ret);
		return ret;
	}

	/* Wait for the measure to be ready */
	k_sleep(K_MSEC(16)); /* Max at 12 bit resolution */

	ret = htu21d_read(dev, buf, sizeof(buf));
	if (ret < 0) {
		LOG_DBG("Read failed: %d", ret);
		return ret;
	}

	/* Check for the status */
	ret = buf[1] & 0x03;
	if (ret == 0x11) {
		LOG_DBG("Status failed: %d", ret);
		return -1;
	}

	tmp = sys_get_be16(&buf[0]);
	crc = htu21d_compute_crc(tmp);
	if (crc != buf[2]) {
		LOG_DBG("CRC invalid: %02x, expected: %02x", crc, buf[2]);
		return -1;
	}

	return tmp;
}

static int htu21d_temperature_fetch(const struct device *dev)
{
	uint8_t buf[3];
	uint16_t tmp;
	uint8_t crc;
	int ret;

	ret = htu21d_write(dev, HTU21D_TEMPERATURE_MEASUREMENT_NO_HOLD_MASTER);
	if (ret < 0) {
		LOG_DBG("Temperature measurement failed: %d", ret);
		return ret;
	}

	/* Wait for the measure to be ready */
	k_sleep(K_MSEC(50)); /* Max at 14 bit resolution */

	ret = htu21d_read(dev, buf, sizeof(buf));
	if (ret < 0) {
		LOG_DBG("Read failed: %d", ret);
		return ret;
	}

	/* Check for the status */
	ret = buf[1] & 0x03;
	if (ret == 0x11) {
		LOG_DBG("Status failed: %d", ret);
		return -1;
	}

	tmp = sys_get_be16(&buf[0]);
	crc = htu21d_compute_crc(tmp);
	if (crc != buf[2]) {
		LOG_DBG("CRC invalid: %02x, expected: %02x", crc, buf[2]);
		return -1;
	}

	return tmp;
}

static int htu21d_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct htu21d_data *data = dev->data;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret = htu21d_humidity_fetch(dev);
	if (ret < 0) {
		return ret;
	}
	data->humidity_raw_val = ret;

	ret = htu21d_temperature_fetch(dev);
	if (ret < 0) {
		return ret;
	}
	data->temperature_raw_val = ret;

	return 0;
}

static int htu21d_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct htu21d_data *data = dev->data;
	long long tmp;

	switch (chan) {
	case SENSOR_CHAN_HUMIDITY:
		/*
		 * The documentation says the following about the conversion at
		 * section Conversion of signal output p.15:
		 *
		 * Relative Humidity conversion
		 *
		 * With the relative humidity signal output SRH, the relative
		 * humidity is obtained by the following formula (result in
		 * %RH), no matter which resolution is chosen:
		 *
		 * RH = -6 + 125 * SRH / 2^16
		 */
		tmp = data->humidity_raw_val;
		tmp = ((125000000LL * tmp) / 65536LL) - 6000000LL;
		val->val1 = tmp / 1000000;
		val->val2 = tmp % 1000000;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		/*
		 * The documentation says the following about the conversion at
		 * section Conversion of signal output p.15:
		 *
		 * Temperature conversion
		 *
		 * The temperature T is calculated by inserting temperature
		 * signal output STemp into the following formula (result in
		 * °C), no matter which resolution is chosen:
		 *
		 * temp = -46.85 + 175.72 * Stemp / 2^16
		 */
		tmp = data->temperature_raw_val;
		tmp = ((175720000LL * tmp) / 65636LL) - 46850000LL;
		val->val1 = tmp / 1000000;
		val->val2 = tmp % 1000000;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api htu21d_api_funcs = {
	.sample_fetch = htu21d_sample_fetch,
	.channel_get = htu21d_channel_get,
};

static int htu21d_chip_init(const struct device *dev)
{
	int ret;

	ret = htu21d_is_ready(dev);
	if (ret < 0) {
		LOG_DBG("I2C bus check failed: %d", ret);
		return ret;
	}

	ret = htu21d_write(dev, HTU21D_SOFT_RESET);
	if (ret < 0) {
		LOG_DBG("Soft reset failed: %d", ret);
		return ret;
	}

	/* Wait for the sensor to be ready */
	k_sleep(K_MSEC(15)); /* The soft reset takes less than 15ms */

	LOG_DBG("\"%s\" OK", dev->name);
	return 0;
}

#define HTU21D_DEFINE(inst)						\
	static struct htu21d_data htu21d_data_##inst;			\
	static const struct htu21d_config htu21d_config_##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			 htu21d_chip_init,				\
			 NULL,						\
			 &htu21d_data_##inst,				\
			 &htu21d_config_##inst,				\
			 POST_KERNEL,					\
			 CONFIG_SENSOR_INIT_PRIORITY,			\
			 &htu21d_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(HTU21D_DEFINE)
