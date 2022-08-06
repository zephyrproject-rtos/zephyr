/*
 * Copyright (c) 2022 Karol Duda <karolwojciechduda@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_ms5611

#include <zephyr/zephyr.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ms5611, CONFIG_SENSOR_LOG_LEVEL);

/* according to device datasheet - reload of registers should take 2.8ms */
#define MS5611_SLEEP_AFTER_RESET_MS	3

#define MS5611_CMD_RESET		0x1E
#define MS5611_CMD_CONVERT_D1_OSR_256	0x40
#define MS5611_CMD_CONVERT_D1_OSR_512	0x42
#define MS5611_CMD_CONVERT_D1_OSR_1024	0x44
#define MS5611_CMD_CONVERT_D1_OSR_2048	0x46
#define MS5611_CMD_CONVERT_D1_OSR_4096	0x48
#define MS5611_CMD_CONVERT_D2_OSR_256	0x50
#define MS5611_CMD_CONVERT_D2_OSR_512	0x52
#define MS5611_CMD_CONVERT_D2_OSR_1024	0x54
#define MS5611_CMD_CONVERT_D2_OSR_2048	0x56
#define MS5611_CMD_CONVERT_D2_OSR_4096	0x58
#define MS5611_CMD_ADC_READ		0x00
#define MS5611_PROM_READ_BASE		0xA0

/* 1 reserved, 6 coefficients and 4 bit CRC at the end */
#define MS5611_PROM_SIZE		8
#define MS5611_PROM_CRC_IDX		(MS5611_PROM_SIZE - 1)
#define MS5611_PROM_BYTES		(MS5611_PROM_SIZE * 2)

/* Oversampling ratios */
#define MS5611_OSR_256			256
#define MS5611_OSR_512			512
#define MS5611_OSR_1024			1024
#define MS5611_OSR_2048			2048
#define MS5611_OSR_4096			4096
#define MS5611_OSR_DEFAULT		MS5611_OSR_256

/* Max response time due to oversampling setting */
#define MS5611_RES_TIME_OSR_256_US	600
#define MS5611_RES_TIME_OSR_512_US	1170
#define MS5611_RES_TIME_OSR_1024_US	2280
#define MS5611_RES_TIME_OSR_2048_US	4540
#define MS5611_RES_TIME_OSR_4096_US	9040
#define MS5611_RES_TIME_THERSHOLD_US	100

/* MS5611 settings metadata */
#define MS5611_CHANNELS_NUMBER		2
#define MS5611_OSR_PRES_IDX		0
#define MS5611_OSR_TEMP_IDX		1

struct ms5611_osr_data {
	uint16_t ratio;
	uint16_t read_cmd;
	uint16_t resp_time;
};

struct ms5611_meas_data {
	uint32_t press;
	uint32_t temp;
};

struct ms5611_data {
	/* Calibration coefficients + CRC */
	uint16_t prom[MS5611_PROM_SIZE];

	/* Oversampling settings */
	struct ms5611_osr_data osr[MS5611_CHANNELS_NUMBER];

	/* Measurement data */
	struct ms5611_meas_data meas;
};

struct ms5611_config {
	struct i2c_dt_spec i2c_bus;
};

static int ms5611_pressure_osr_set(const struct device *dev,
				   const struct sensor_value *val)
{
	struct ms5611_data *data = dev->data;

	switch (val->val1) {
	case MS5611_OSR_256:
		data->osr[MS5611_OSR_PRES_IDX].ratio = MS5611_OSR_256;
		data->osr[MS5611_OSR_PRES_IDX].read_cmd =
				MS5611_CMD_CONVERT_D1_OSR_256;
		data->osr[MS5611_OSR_PRES_IDX].resp_time =
				MS5611_RES_TIME_OSR_256_US;
		break;
	case MS5611_OSR_512:
		data->osr[MS5611_OSR_PRES_IDX].ratio = MS5611_OSR_512;
		data->osr[MS5611_OSR_PRES_IDX].read_cmd =
				MS5611_CMD_CONVERT_D1_OSR_512;
		data->osr[MS5611_OSR_PRES_IDX].resp_time =
				MS5611_RES_TIME_OSR_512_US;
		break;
	case MS5611_OSR_1024:
		data->osr[MS5611_OSR_PRES_IDX].ratio = MS5611_OSR_1024;
		data->osr[MS5611_OSR_PRES_IDX].read_cmd =
				MS5611_CMD_CONVERT_D1_OSR_1024;
		data->osr[MS5611_OSR_PRES_IDX].resp_time =
				MS5611_RES_TIME_OSR_1024_US;
		break;
	case MS5611_OSR_2048:
		data->osr[MS5611_OSR_PRES_IDX].ratio = MS5611_OSR_2048;
		data->osr[MS5611_OSR_PRES_IDX].read_cmd =
				MS5611_CMD_CONVERT_D1_OSR_2048;
		data->osr[MS5611_OSR_PRES_IDX].resp_time =
				MS5611_RES_TIME_OSR_2048_US;
		break;
	case MS5611_OSR_4096:
		data->osr[MS5611_OSR_PRES_IDX].ratio = MS5611_OSR_4096;
		data->osr[MS5611_OSR_PRES_IDX].read_cmd =
				MS5611_CMD_CONVERT_D1_OSR_4096;
		data->osr[MS5611_OSR_PRES_IDX].resp_time =
				MS5611_RES_TIME_OSR_4096_US;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ms5611_temperature_osr_set(const struct device *dev,
				      const struct sensor_value *val)
{
	struct ms5611_data *data = dev->data;

	switch (val->val1) {
	case MS5611_OSR_256:
		data->osr[MS5611_OSR_TEMP_IDX].ratio = MS5611_OSR_256;
		data->osr[MS5611_OSR_TEMP_IDX].read_cmd =
				MS5611_CMD_CONVERT_D2_OSR_256;
		data->osr[MS5611_OSR_TEMP_IDX].resp_time =
				MS5611_RES_TIME_OSR_256_US;
		break;
	case MS5611_OSR_512:
		data->osr[MS5611_OSR_TEMP_IDX].ratio = MS5611_OSR_512;
		data->osr[MS5611_OSR_TEMP_IDX].read_cmd =
				MS5611_CMD_CONVERT_D2_OSR_512;
		data->osr[MS5611_OSR_TEMP_IDX].resp_time =
				MS5611_RES_TIME_OSR_512_US;
		break;
	case MS5611_OSR_1024:
		data->osr[MS5611_OSR_TEMP_IDX].ratio = MS5611_OSR_1024;
		data->osr[MS5611_OSR_TEMP_IDX].read_cmd =
				MS5611_CMD_CONVERT_D2_OSR_1024;
		data->osr[MS5611_OSR_TEMP_IDX].resp_time =
				MS5611_RES_TIME_OSR_1024_US;
		break;
	case MS5611_OSR_2048:
		data->osr[MS5611_OSR_TEMP_IDX].ratio = MS5611_OSR_2048;
		data->osr[MS5611_OSR_TEMP_IDX].read_cmd =
				MS5611_CMD_CONVERT_D2_OSR_2048;
		data->osr[MS5611_OSR_TEMP_IDX].resp_time =
				MS5611_RES_TIME_OSR_2048_US;
		break;
	case MS5611_OSR_4096:
		data->osr[MS5611_OSR_TEMP_IDX].ratio = MS5611_OSR_4096;
		data->osr[MS5611_OSR_TEMP_IDX].read_cmd =
				MS5611_CMD_CONVERT_D2_OSR_4096;
		data->osr[MS5611_OSR_TEMP_IDX].resp_time =
				MS5611_RES_TIME_OSR_4096_US;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ms5611_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	int ret;

	if (attr != SENSOR_ATTR_OVERSAMPLING) {
		return -ENOTSUP;
	}

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		ret = ms5611_temperature_osr_set(dev, val);
		break;
	case SENSOR_CHAN_PRESS:
		ret = ms5611_pressure_osr_set(dev, val);
		break;
	case SENSOR_CHAN_ALL:
		ret = ms5611_pressure_osr_set(dev, val);
		if (ret != 0)
			break;

		ret = ms5611_temperature_osr_set(dev, val);
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}

static int ms5611_fetch_temp_and_press(const struct device *dev)
{
	const struct ms5611_config *cfg = dev->config;
	struct ms5611_data *data = dev->data;
	uint8_t temp_data[3], press_data[3];
	uint32_t raw_temp, raw_press;
	int ret;
	int32_t dT, temp, t2, p;
	uint8_t i2c_cmd;
	int64_t off, off2, sens, sens2;

	/* Request temperature conversion */
	i2c_cmd = data->osr[MS5611_OSR_TEMP_IDX].read_cmd;
	ret = i2c_write_dt(&cfg->i2c_bus, &i2c_cmd, sizeof(i2c_cmd));
	if (ret < 0) {
		LOG_ERR("Failed to request temperature conversion");
		return ret;
	}

	/* Wait for conversion to be finished */
	k_sleep(K_USEC(data->osr[MS5611_OSR_TEMP_IDX].resp_time));

	i2c_cmd = MS5611_CMD_ADC_READ;
	ret = i2c_write_dt(&cfg->i2c_bus, &i2c_cmd, sizeof(i2c_cmd));
	if (ret < 0) {
		LOG_ERR("Failed to request temperature read");
		return ret;
	}

	ret = i2c_read_dt(&cfg->i2c_bus, temp_data, sizeof(temp_data));
	if (ret < 0) {
		LOG_ERR("Failed to read temperature measurement");
		return ret;
	}

	raw_temp = sys_get_be24(temp_data);

	if (raw_temp == 0) {
		LOG_DBG("Invalid temperature data obtained");
		return -EIO;
	}

	/* Request pressure conversion */
	i2c_cmd = data->osr[MS5611_OSR_PRES_IDX].read_cmd;
	ret = i2c_write_dt(&cfg->i2c_bus, &i2c_cmd, sizeof(i2c_cmd));
	if (ret < 0) {
		LOG_ERR("Failed to request pressure conversion");
		return ret;
	}

	/* Wait for conversion to be finished */
	k_sleep(K_USEC(data->osr[MS5611_OSR_PRES_IDX].resp_time));

	i2c_cmd = MS5611_CMD_ADC_READ;
	ret = i2c_write_dt(&cfg->i2c_bus, &i2c_cmd, sizeof(i2c_cmd));
	if (ret < 0) {
		LOG_ERR("Failed to request pressure read");
		return ret;
	}

	ret = i2c_read_dt(&cfg->i2c_bus, press_data, sizeof(press_data));
	if (ret < 0) {
		LOG_ERR("Failed to read pressure measurement");
		return ret;
	}

	raw_press = sys_get_be24(press_data);

	if (raw_press == 0) {
		LOG_DBG("Invalid pressure data obtained");
		return -EIO;
	}


	/* Algorithms used below can be found in the device datasheet under the
	 * link:
	 * https://www.te.com/commerce/DocumentDelivery/DDEController?Action=
	 * showdoc&DocId=Data+Sheet%7FMS5611-01BA03%7FB3%7Fpdf%7FEnglish%7FENG
	 * _DS_MS5611-01BA03_B3.pdf%7FCAT-BLPS0036
	 *
	 * In sections: "Pressure and temperature calculation" and
	 * "Second order temperature compensation".
	 */

	/* Calculate compensated temperature value */
	dT = raw_temp - (data->prom[5] << 8);
	temp = 2000 + (((int64_t)dT * data->prom[6]) >> 23);

	/* Second order temperature compensation */
	if (temp < 2000) {
		t2 = ((dT ^ 2) >> 31);
		off2 = 5 * ((temp - 2000) ^ 2) / 2;
		sens2 = 5 * (temp - 2000) ^ 2 / 4;

		if (temp < -1500) {
			off2 = off2 + (7 * (temp + 1500) ^ 2);
			sens2 = sens2 + (11 * ((temp + 1500) ^ 2)) / 2;
		}
	} else {
		t2 = 0;
		off2 = 0;
		sens2 = 0;
	}

	/* Calculate values with respect to offsets */
	temp = temp - t2;

	off = (uint32_t)(data->prom[2] << 16) +
	      (((int64_t)data->prom[4] * dT) >> 7);
	off = off - off2;

	sens = (data->prom[1] << 15) + (((int64_t)data->prom[3] * dT) >> 8);
	sens = sens - sens2;

	p = (((uint64_t)(raw_press * sens) >> 21) - off) >> 15;

	data->meas.temp = temp;
	/* 10mbar = 1kPa */
	data->meas.press = p / 10;

	return 0;
}

static int ms5611_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret = ms5611_fetch_temp_and_press(dev);

	if (ret < 0) {
		LOG_ERR("Failed to fetch temperature and pressure");
		return ret;
	}

	return 0;
}

static int ms5611_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ms5611_data *data = dev->data;
	struct ms5611_meas_data *meas = &data->meas;

	switch (chan) {
	case SENSOR_CHAN_PRESS:
		val->val1 = meas->press / 100;
		val->val2 = meas->press % 100;
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = meas->temp / 100;
		val->val2 = meas->temp % 100;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ms5611_fetch_prom(const struct device *dev)
{
	const struct ms5611_config *cfg = dev->config;
	struct ms5611_data *data = dev->data;
	int ret;
	uint8_t i;
	uint8_t i2c_cmd;
	uint8_t data_bytes[2];

	for (i = 0; i < MS5611_PROM_SIZE; i++) {

		i2c_cmd = MS5611_PROM_READ_BASE;

		/* last bit is not significant */
		i2c_cmd |= i << 1;

		ret = i2c_write_dt(&cfg->i2c_bus, &i2c_cmd, sizeof(i2c_cmd));
		if (ret < 0) {
			LOG_ERR("Failed to request PROM address %d of ms5611",
				i);
			return ret;
		}

		ret = i2c_read_dt(&cfg->i2c_bus, data_bytes,
				  sizeof(data_bytes));

		if (ret < 0) {
			LOG_ERR("Failed to get PROM address %d of ms5611", i);
			return ret;
		}

		/* convert data to little-endian */
		data->prom[i] = sys_get_be16(data_bytes);
	}

	return 0;
}

static uint8_t ms5611_calculate_crc4(const struct device *dev)
{
	struct ms5611_data *data = dev->data;
	int cnt;
	unsigned int n_rem = 0;
	unsigned int crc_read;
	unsigned char n_bit;

	/* store CRC value read from device */
	crc_read = data->prom[MS5611_PROM_CRC_IDX];

	/* CRC value is being replaced by 0 for the time of computations */
	data->prom[MS5611_PROM_CRC_IDX] = (0xFF00 &
					  (data->prom[MS5611_PROM_CRC_IDX]));

	for (cnt = 0; cnt < MS5611_PROM_BYTES ; cnt++) {
		if (cnt % 2 == 1) {
			n_rem ^= (uint8_t)((data->prom[cnt >> 1]) & 0x00FF);
		} else {
			n_rem ^= (uint8_t)(data->prom[cnt >> 1] >> 8);
		}

		for (n_bit = 8; n_bit > 0; n_bit--) {
			if (n_rem & (0x8000)) {
				n_rem = (n_rem << 1) ^ 0x3000;
			} else {
				n_rem = (n_rem << 1);
			}
		}
	}
	/* final 4-bit reminder is CRC code */
	n_rem =  (0x000F & (n_rem >> 12));
	/* restore the crc_read to its original place */
	data->prom[MS5611_PROM_CRC_IDX] = crc_read;
	return (n_rem ^ 0x00);
}

static int ms5611_check_coef_crc(const struct device *dev)
{
	const struct ms5611_config *cfg = dev->config;
	struct ms5611_data *data = dev->data;
	int ret;
	uint8_t crc_bytes[2];
	uint8_t crc_value;
	uint8_t i2c_cmd;

	i2c_cmd = MS5611_PROM_READ_BASE;
	i2c_cmd |= (MS5611_PROM_CRC_IDX) << 1;

	ret = i2c_write_dt(&cfg->i2c_bus, &i2c_cmd, sizeof(i2c_cmd));
	if (ret < 0) {
		LOG_ERR("Failed to request CRC of ms5611 PROM");
		return ret;
	}

	ret = i2c_read_dt(&cfg->i2c_bus, crc_bytes, sizeof(crc_bytes));
	if (ret < 0) {
		LOG_ERR("Failed to get crc of ms5611 PROM");
		return ret;
	}

	/* convert data to little-endian */
	data->prom[MS5611_PROM_CRC_IDX] = sys_get_be16(crc_bytes);

	crc_value = ms5611_calculate_crc4(dev);

	/* check if calculated CRC matches the one in PROM */
	if (crc_value != data->prom[MS5611_PROM_CRC_IDX]) {
		return -ENOTSUP;
	}

	return 0;
}

static int ms5611_init(const struct device *dev)
{
	int ret;
	uint8_t i2c_cmd;
	const struct ms5611_config *cfg = dev->config;
	struct sensor_value osr_config;

	/* reset to make sure that calibration PROM gets loaded into internal
	 * register
	 */
	i2c_cmd = MS5611_CMD_RESET;
	ret = i2c_write_dt(&cfg->i2c_bus, &i2c_cmd, sizeof(i2c_cmd));
	if (ret < 0) {
		LOG_ERR("Failed to reset ms5611");
		return ret;
	}

	/* Wait for reset to take place */
	k_msleep(MS5611_SLEEP_AFTER_RESET_MS);

	/* Read the PROM memory with calibration coefficients and CRC */
	ret = ms5611_fetch_prom(dev);
	if (ret < 0) {
		LOG_ERR("Failed to fetch coefficients of ms5611");
		return ret;
	}

	/* Check calibration coefficients CRC */
	ret = ms5611_check_coef_crc(dev);
	if (ret < 0) {
		LOG_ERR("Check of ms5611 coefficients failed");
		return ret;
	}

	/* Set default oversampling levels */
	osr_config.val1 = MS5611_OSR_DEFAULT;
	ret = ms5611_pressure_osr_set(dev, &osr_config);
	if (ret < 0) {
		LOG_ERR("Failed to set pressure oversampling level");
		return ret;
	}

	ret = ms5611_temperature_osr_set(dev, &osr_config);
	if (ret < 0) {
		LOG_ERR("Failed to set temperature oversampling level");
		return ret;
	}

	return 0;
}

static const struct sensor_driver_api ms5611_api = {
	.attr_set = ms5611_attr_set,
	.sample_fetch = ms5611_sample_fetch,
	.channel_get = ms5611_channel_get,
};

#define MS5611_DEVICE(inst)						\
	static struct ms5611_data ms5611_data_##inst;			\
	static const struct ms5611_config ms5611_cfg_##inst = {		\
		.i2c_bus = I2C_DT_SPEC_INST_GET(inst),			\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
			      ms5611_init,				\
			      NULL,					\
			      &ms5611_data_##inst,			\
			      &ms5611_cfg_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &ms5611_api)

DT_INST_FOREACH_STATUS_OKAY(MS5611_DEVICE);
