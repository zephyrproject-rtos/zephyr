/* am2301b.c - Driver for Aosong AM2301B Humidity and Temperature Module */
/*
 * Copyright (c) 2022 Hisanori Yamaoku
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aosong_am2301b

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/types.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_AM2301B_CRC_CHECK
#include <zephyr/sys/crc.h>
#endif /* CONFIG_AM2301B_CRC_CHECK */

LOG_MODULE_REGISTER(AM2301B, CONFIG_SENSOR_LOG_LEVEL);

#define AM2301B_REG_SOFT_RESET                 0xBA
#define AM2301B_STATUS_INIT_COMPLETE           0x18

#define AM2301B_REG_STATUS                     0x71
#define AM2301B_MASK_MEASUREMENT_BUSY          0x80

#define AM2301B_WAIT_POWER_ON                  40U
#define AM2301B_WAIT_INIT                      10U
#define AM2301B_WAIT_MEASUREMENT               80U

#define AM2301B_LEN_MEASUREMENT_DATA           7U

#define AM2301_CRC_POLY                        0x31

struct am2301b_data {
	uint32_t temperature;
	uint32_t humidity;
};

struct am2301b_config {
	struct i2c_dt_spec i2c;
};

/* Initialization command from Chapter 5.4, Section 1 of AM2301B Product Manuals */
const static uint8_t am2301b_cmd_initialize[] = { 0xBE, 0x08, 0x00 };
/* Measurement trigger command from Chapter 5.4, Section 2 of AM2301B Product Manuals */
const static uint8_t am2301b_cmd_measurement[] = { 0xAC, 0x33, 0x00 };

static int am2301b_fetch_measument_data(const struct am2301b_config *cfg,
					struct am2301b_data *data)
{
	uint8_t buf[AM2301B_LEN_MEASUREMENT_DATA];
	int ret;

	ret = i2c_write_dt(&cfg->i2c, am2301b_cmd_measurement, sizeof(am2301b_cmd_measurement));
	if (ret < 0) {
		LOG_ERR("Failed to send the command during measurement.");
		return ret;
	}

	/* From Chapter 5.4, Section 3 of the AM2301B Product manual,
	 * wait for 80ms after sending the measurement command.
	 */
	k_sleep(K_MSEC(AM2301B_WAIT_MEASUREMENT));

	ret = i2c_read_dt(&cfg->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to receive the measurement result.");
		return ret;
	}

#ifdef CONFIG_AM2301B_CRC_CHECK
	/* Calculate CRC from Chapter 5.4, Section 4 of AM2301B Product manuals. */
	if (crc8(buf, 6, AM2301_CRC_POLY, 0xFF, false) != buf[6]) {
		LOG_ERR("CRC verification failed.");
		return -EIO;
	}
#endif /* CONFIG_AM2301B_CRC_CHECK */

	if (buf[0] & AM2301B_MASK_MEASUREMENT_BUSY) {
		LOG_ERR("A timeout was detected during measurement.");
		return -EBUSY;
	}

	/* From Chapter 5.4, Section 5 of the AM2301B Product manuals,
	 * acquire the 20 bits from the beginning of the second byte as humidity data
	 * and the following 20 bits as temperature data.
	 */
	data->humidity = sys_get_be24(&buf[1]) >> 4;
	data->temperature = sys_get_be24(&buf[3]) & 0x0fffff;

	return 0;
}

static int am2301b_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	const struct am2301b_config *cfg = dev->config;
	struct am2301b_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ALL:
		__fallthrough;
	case SENSOR_CHAN_AMBIENT_TEMP:
		__fallthrough;
	case SENSOR_CHAN_HUMIDITY:
		return am2301b_fetch_measument_data(cfg, data);
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Calculate the temperature value from the raw temperature value obtained from the sensor
 *        Calculate the temperature from the formula in Chapter 6.2 of the AM2301B Product Manual
 * @param val Temperature value after conversion
 * @param temperature_raw Raw value of temperature obtained from the sensor(Sr)
 */
static inline void am2301b_convert_temperature_value(struct sensor_value *val,
						     uint32_t temperature_raw)
{
	const static int32_t power20_of_2 = 0x100000;
	int32_t t = temperature_raw * 200 - power20_of_2 * 50;

	val->val1 = t / power20_of_2;
	/* To prevent overflow, instead of multiplying by 1000000 to convert sensor_value,
	 * we split it into multiplying by 1000 twice.
	 */
	val->val2 = ((t % power20_of_2) * 1000 /  power20_of_2) * 1000;
}

/**
 * @brief Calculate the humidity value from the raw humidity value obtained from the sensor
 *        Calculate the humidity from the formula in Chapter 6.1 of the AM2301B Product Manual
 * @param val Humidity value after conversion
 * @param humidity_raw Raw value of humidity obtained from the sensor(Srh)
 */
static inline void am2301b_convert_humidity_value(struct sensor_value *val,
						  uint32_t humidity_raw)
{
	const static int32_t power20_of_2 = 0x100000;
	uint32_t h = humidity_raw * 100;

	val->val1 = h / power20_of_2;
	/* To prevent overflow, instead of multiplying by 1000000 to convert sensor_value,
	 * we split it into multiplying by 1000 twice.
	 */
	val->val2 = ((h % power20_of_2) * 1000 /  power20_of_2) * 1000;
}

static int am2301b_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct am2301b_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		am2301b_convert_temperature_value(val, data->temperature);
		return 0;

	case SENSOR_CHAN_HUMIDITY:
		am2301b_convert_humidity_value(val, data->humidity);
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int am2301b_init(const struct device *dev)
{
	const struct am2301b_config *cfg = dev->config;
	uint8_t status;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("device is not ready.");
		return -ENODEV;
	}

	/* From Chapter 5.4, Section 1 of the AM2301B Product manual,
	 * wait for 40ms when turning on the power.
	 */
	k_sleep(K_MSEC(AM2301B_WAIT_POWER_ON));

	ret = i2c_write_dt(&cfg->i2c, am2301b_cmd_initialize, sizeof(am2301b_cmd_initialize));
	if (ret < 0) {
		LOG_ERR("Failed to send the initialization command.");
		return ret;
	}

	/* From Chapter 5.4, Section 1 of the AM2301B Product manual,
	 * wait for 10ms after sending the initialization command.
	 */
	k_sleep(K_MSEC(AM2301B_WAIT_INIT));

	ret = i2c_reg_read_byte_dt(&cfg->i2c, AM2301B_REG_STATUS, &status);
	if (ret < 0) {
		LOG_ERR("Failed to receive status.");
		return ret;
	}

	if (status != AM2301B_STATUS_INIT_COMPLETE) {
		LOG_ERR("Detected timeout on reset.");
		return -EBUSY;
	}

	return 0;
}

static const struct sensor_driver_api am2301b_driver_api = {
	.sample_fetch = am2301b_sample_fetch,
	.channel_get = am2301b_channel_get,
};

#define AM2301B_INST(inst)						      \
	static struct am2301b_data am2301b_data_##inst;			      \
	static const struct am2301b_config am2301b_config_##inst = {	      \
		.i2c = I2C_DT_SPEC_INST_GET(inst),			      \
	};								      \
	DEVICE_DT_INST_DEFINE(inst, am2301b_init, NULL, &am2301b_data_##inst, \
			      &am2301b_config_##inst, POST_KERNEL,	      \
			      CONFIG_SENSOR_INIT_PRIORITY, &am2301b_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AM2301B_INST)
