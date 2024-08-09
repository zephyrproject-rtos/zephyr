/*
 * Copyright (c) 2024 Nathan Olff
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#if defined(CONFIG_DHT20_CRC)
#include <zephyr/sys/crc.h>
#endif
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define DHT20_STATUS_REGISTER 0x71

#define DHT20_STATUS_MASK (BIT(0) | BIT(1))

#define DHT20_STATUS_MASK_CHECK      0x18
#define DHT20_STATUS_MASK_POLL_STATE 0x80

#define DHT20_MASK_RESET_REGISTER 0xB0

#define DHT20_TRIGGER_MEASUREMENT_COMMAND 0xAC, 0x33, 0x00

#define DHT20_TRIGGER_MEASUREMENT_BUFFER_LENGTH 3

/** CRC polynom (1 + X^4 + X^5 + X^8) */
#define DHT20_CRC_POLYNOM (BIT(0) | BIT(4) | BIT(5))

/*
 * According to datasheet 7.4
 * Reset register 0x1B, 0x1C and 0x1E
 */
#define DHT20_RESET_REGISTER_0 0x1B
#define DHT20_RESET_REGISTER_1 0x1C
#define DHT20_RESET_REGISTER_2 0x1E

/** Length of the buffer used for data measurement */
#define DHT20_MEASUREMENT_BUFFER_LENGTH 7

/** Wait some time after reset sequence (in ms) */
#define DHT20_RESET_SEQUENCE_WAIT_MS 10

/** Wait after power on (in ms) */
#define DHT20_POWER_ON_WAIT_MS         75
/** Wait during polling after power on (in ms) */
#define DHT20_INIT_POLL_STATUS_WAIT_MS 5

LOG_MODULE_REGISTER(DHT20, CONFIG_SENSOR_LOG_LEVEL);

struct dht20_config {
	struct i2c_dt_spec bus;
};

struct dht20_data {
	uint32_t t_sample;
	uint32_t rh_sample;
};

/**
 * @brief Read status register
 *
 * @param dev Pointer to the sensor device
 * @param[out] status Pointer to which the status will be stored
 * @return 0 if successful
 */
static inline int read_status(const struct device *dev, uint8_t *status)
{
	const struct dht20_config *cfg = dev->config;
	int rc;
	uint8_t tx_buf[] = {DHT20_STATUS_REGISTER};
	uint8_t rx_buf[1];

	/* Write DHT20_STATUS_REGISTER then read to get status */
	rc = i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	/* Retrieve status from rx_buf */
	*status = rx_buf[0];
	return rc;
}

static inline int reset_register(const struct device *dev, uint8_t reg)
{
	const struct dht20_config *cfg = dev->config;
	int rc;
	uint8_t tx_buf[] = {reg, 0, 0};
	uint8_t rx_buf[3];

	/* Write and then read 3 bytes from device */
	rc = i2c_write_read_dt(&cfg->bus, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to reset register.");
		return rc;
	}

	/* Write register again, using values read earlier */
	tx_buf[0] = DHT20_MASK_RESET_REGISTER | reg;
	tx_buf[1] = rx_buf[1];
	tx_buf[2] = rx_buf[2];
	rc = i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to reset register.");
		return rc;
	}

	return rc;
}

static inline int reset_sensor(const struct device *dev)
{
	int rc;
	uint8_t status;

	rc = read_status(dev, &status);
	if (rc < 0) {
		LOG_ERR("Failed to read status");
		return rc;
	}

	if ((status & DHT20_STATUS_MASK_CHECK) != DHT20_STATUS_MASK_CHECK) {
		/*
		 * According to datasheet 7.4
		 * Reset register 0x1B, 0x1C and 0x1E if status does not match expected value
		 */
		rc = reset_register(dev, DHT20_RESET_REGISTER_0);
		if (rc < 0) {
			return rc;
		}
		rc = reset_register(dev, DHT20_RESET_REGISTER_1);
		if (rc < 0) {
			return rc;
		}
		rc = reset_register(dev, DHT20_RESET_REGISTER_2);
		if (rc < 0) {
			return rc;
		}
		/* Wait 10ms after reset sequence */
		k_msleep(DHT20_RESET_SEQUENCE_WAIT_MS);
	}

	return 0;
}

static int dht20_read_sample(const struct device *dev, uint32_t *t_sample, uint32_t *rh_sample)
{
	const struct dht20_config *cfg = dev->config;
	/*
	 * Datasheet shows content of the measurement data as follow
	 *
	 * +------+----------------------------------------+
	 * | Byte | Content                                |
	 * +------+----------------------------------------+
	 * | 0    | State                                  |
	 * | 1    | Humidity                               |
	 * | 2    | Humidity                               |
	 * | 3    | Humidity (4 MSb) | Temperature (4 LSb) |
	 * | 4    | Temperature                            |
	 * | 5    | Temperature                            |
	 * | 6    | CRC                                    |
	 * +------+----------------------------------------+
	 */
	uint8_t rx_buf[DHT20_MEASUREMENT_BUFFER_LENGTH];
	int rc;
	uint8_t status;

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	status = rx_buf[0];
	/* Extract 20 bits for humidity data */
	*rh_sample = sys_get_be24(&rx_buf[1]) >> 4;
	/* Extract 20 bits for temperature data */
	*t_sample = sys_get_be24(&rx_buf[3]) & 0x0FFFFF;

#if defined(CONFIG_DHT20_CRC)
	/* Compute and check CRC with last byte of measurement data */
	uint8_t crc = crc8(rx_buf, 6, DHT20_CRC_POLYNOM, 0xFF, false);

	if (crc != rx_buf[6]) {
		rc = -EIO;
	}
#endif

	return rc;
}

static int dht20_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct dht20_data *data = dev->data;
	const struct dht20_config *cfg = dev->config;
	int rc;
	uint8_t tx_buf[DHT20_TRIGGER_MEASUREMENT_BUFFER_LENGTH] = {
		DHT20_TRIGGER_MEASUREMENT_COMMAND};
	uint8_t status;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	/* Reset sensor if needed */
	reset_sensor(dev);

	/* Send trigger measurement command */
	rc = i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}

	/*
	 * According to datasheet maximum time to make temperature and humidity
	 * measurements is 80ms
	 */
	k_msleep(DHT20_POWER_ON_WAIT_MS);

	do {
		k_msleep(DHT20_INIT_POLL_STATUS_WAIT_MS);

		rc = read_status(dev, &status);
		if (rc < 0) {
			LOG_ERR("Failed to read status.");
			return rc;
		}
	} while ((status & DHT20_STATUS_MASK_POLL_STATE) != 0);

	rc = dht20_read_sample(dev, &data->t_sample, &data->rh_sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch data.");
		return rc;
	}

	return 0;
}

static void dht20_temp_convert(struct sensor_value *val, uint32_t raw)
{
	int32_t micro_c;

	/*
	 * Convert to micro Celsius
	 * DegCT = (S / 2^20) * 200 - 50
	 * uDegCT = (S * 1e6 * 200 - 50 * 1e6) / (1 << 20)
	 */
	micro_c = ((int64_t)raw * 1000000 * 200) / BIT(20) - 50 * 1000000;

	val->val1 = micro_c / 1000000;
	val->val2 = micro_c % 1000000;
}

static void dht20_rh_convert(struct sensor_value *val, uint32_t raw)
{
	int32_t micro_rh;

	/*
	 * Convert to micro %RH
	 * %RH = (S / 2^20) * 100%
	 * u%RH = (S * 1e6 * 100) / (1 << 20)
	 */
	micro_rh = ((uint64_t)raw * 1000000 * 100) / BIT(20);

	val->val1 = micro_rh / 1000000;
	val->val2 = micro_rh % 1000000;
}

static int dht20_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct dht20_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		dht20_temp_convert(val, data->t_sample);
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		dht20_rh_convert(val, data->rh_sample);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int dht20_init(const struct device *dev)
{
	const struct dht20_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	return 0;
}

static const struct sensor_driver_api dht20_driver_api = {.sample_fetch = dht20_sample_fetch,
							  .channel_get = dht20_channel_get};

#define DT_DRV_COMPAT aosong_dht20
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define DEFINE_DHT20(n)                                                                            \
	static struct dht20_data dht20_data_##n;                                                   \
                                                                                                   \
	static const struct dht20_config dht20_config_##n = {.bus = I2C_DT_SPEC_INST_GET(n)};      \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, dht20_init, NULL, &dht20_data_##n, &dht20_config_##n,      \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &dht20_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_DHT20)

#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT aosong_aht20
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define DEFINE_AHT20(n)                                                                            \
	static struct dht20_data aht20_data_##n;                                                   \
                                                                                                   \
	static const struct dht20_config aht20_config_##n = {.bus = I2C_DT_SPEC_INST_GET(n)};      \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, dht20_init, NULL, &aht20_data_##n, &aht20_config_##n,      \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &dht20_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_AHT20)

#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT aosong_am2301b
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define DEFINE_AM2301B(n)                                                                          \
	static struct dht20_data am2301b_data_##n;                                                 \
                                                                                                   \
	static const struct dht20_config am2301b_config_##n = {.bus = I2C_DT_SPEC_INST_GET(n)};    \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, dht20_init, NULL, &am2301b_data_##n, &am2301b_config_##n,  \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &dht20_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_AM2301B)

#endif
#undef DT_DRV_COMPAT
