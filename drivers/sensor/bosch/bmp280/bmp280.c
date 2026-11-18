/* Bosch BMP280 pressure sensor
 *
 * Copyright (c) 2026 NBuiLe
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * BST-BMP280-DS001-10 | Revision 1.13 | November 2014
 */

#include <math.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include "bmp280.h"

#define DT_DRV_COMPAT bosch_bmp280

LOG_MODULE_REGISTER(BMP280, CONFIG_SENSOR_LOG_LEVEL);

static inline int bmp280_bus_check(const struct device *dev)
{
	const struct bmp280_config *cfg = dev->config;

	return i2c_is_ready_dt(&cfg->i2c) ? 0 : -ENODEV;
}

static inline int bmp280_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct bmp280_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->i2c, start, buf, size);
}

static inline int bmp280_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct bmp280_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, reg, val);
}

static int bmp280_wait_until_ready(const struct device *dev, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	uint8_t status;
	int ret;

	/* Wait for relevant flags to clear */
	while (1) {
		ret = bmp280_reg_read(dev, BMP280_STATUS, &status, 1);
		if (ret < 0) {
			return ret;
		}
		if (!(status & (BMP280_STATUS_MEASURING | BMP280_STATUS_IM_UPDATE))) {
			break;
		}
		/* Check if waiting has timed out */
		if (sys_timepoint_expired(end)) {
			return -EAGAIN;
		}
		k_sleep(K_MSEC(3));
	}

	return 0;
}

static int bmp280_compensate(const struct device *dev, struct bmp280_data *data)
{
	int ret;
	/*Adapt from chapter 3.11.3 Compensation formula*/
	uint8_t buf[6];
	int32_t var1, var2;
	uint16_t dig_T1 = data->calib.dig_T1;
	int16_t dig_T2 = data->calib.dig_T2;
	int16_t dig_T3 = data->calib.dig_T3;
	uint16_t dig_P1 = data->calib.dig_P1;
	int16_t dig_P2 = data->calib.dig_P2;
	int16_t dig_P3 = data->calib.dig_P3;
	int16_t dig_P4 = data->calib.dig_P4;
	int16_t dig_P5 = data->calib.dig_P5;
	int16_t dig_P6 = data->calib.dig_P6;
	int16_t dig_P7 = data->calib.dig_P7;
	int16_t dig_P8 = data->calib.dig_P8;
	int16_t dig_P9 = data->calib.dig_P9;

	/* Read raw data from sensor */
	ret = bmp280_reg_read(dev, BMP280_PRESS_MSB, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read raw data from sensor: %d", ret);
		return ret;
	}

	uint32_t adc_P = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | (buf[2] >> 4);
	uint32_t adc_T = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | (buf[5] >> 4);

	/* --- Temperature compensation --- */
	var1 = ((((int32_t)adc_T >> 3) - ((int32_t)dig_T1 << 1)) * (int32_t)dig_T2) >> 11;
	var2 = ((((((int32_t)adc_T >> 4) - (int32_t)dig_T1) *
		  (((int32_t)adc_T >> 4) - (int32_t)dig_T1)) >>
		 12) *
		(int32_t)dig_T3) >>
	       14;
	int32_t t_fine = var1 + var2;
	int32_t T_01C = (t_fine * 5 + 128) >> 8; /* 0.01°C */

	/* --- Pressure compensation --- */
	int64_t A = (int64_t)t_fine - 128000;
	int64_t B = A * A * (int64_t)dig_P6;

	B = B + ((A * (int64_t)dig_P5) << 17) + ((int64_t)dig_P4 << 35);
	A = ((A * A * (int64_t)dig_P3) >> 8) + ((A * (int64_t)dig_P2) << 12);
	A = (((((int64_t)1) << 47) + A) * (int64_t)dig_P1) >> 33;
	if (A == 0) {
		return -EAGAIN;
	}

	int64_t p64 = 1048576 - (int64_t)adc_P;

	p64 = (((p64 << 31) - B) * 3125) / A;
	A = ((int64_t)dig_P9 * (p64 >> 13) * (p64 >> 13)) >> 25;
	B = ((int64_t)dig_P8 * p64) >> 19;
	p64 = ((p64 + A + B) >> 8) + ((int64_t)dig_P7 << 4); /* Q24.8 */
	uint32_t p_Pa = (uint32_t)(p64 >> 8);                /* Pa */

	/* Output */
	data->sample.comp_press = p_Pa;
	data->sample.comp_temp = T_01C;

	return ret;
}

static int bmp280_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bmp280_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP && chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

	ret = bmp280_wait_until_ready(dev, K_MSEC(100));
	if (ret < 0) {
		return ret;
	}

	pm_device_busy_set(dev);

	ret = bmp280_compensate(dev, data);

	pm_device_busy_clear(dev);

	return ret;
}

static int bmp280_temp_channel_get(const struct device *dev, struct sensor_value *val)
{
	struct bmp280_data *data = dev->data;
	int32_t temp = data->sample.comp_temp;

	val->val1 = temp / 100;
	val->val2 = (temp % 100) * 10000;
	return 0;
}

static int bmp280_press_channel_get(const struct device *dev, struct sensor_value *val)
{
	struct bmp280_data *data = dev->data;
	uint32_t press = data->sample.comp_press;

	val->val1 = press / 1000;
	val->val2 = (press % 1000) * 1000;
	return 0;
}

static int bmp280_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_PRESS:
		return bmp280_press_channel_get(dev, val);
	case SENSOR_CHAN_DIE_TEMP:
		return bmp280_temp_channel_get(dev, val);

	default:
		LOG_DBG("Channel not supported.");
		return -ENOTSUP;
	}
}

static int bmp280_get_calibration_data(const struct device *dev)
{
	uint8_t b[24];
	int ret;
	struct bmp280_data *data = dev->data;
	struct bmp280_calib_data *calib = &data->calib;

	ret = bmp280_reg_read(dev, BMP280_REG_CALIB0, b, sizeof(b));
	if (ret < 0) {
		return ret;
	}

	calib->dig_T1 = (uint16_t)(b[1] << 8U | b[0]);
	calib->dig_T2 = (int16_t)(b[3] << 8U | b[2]);
	calib->dig_T3 = (int16_t)(b[5] << 8U | b[4]);
	calib->dig_P1 = (uint16_t)(b[7] << 8U | b[6]);
	calib->dig_P2 = (int16_t)(b[9] << 8U | b[8]);
	calib->dig_P3 = (int16_t)(b[11] << 8U | b[10]);
	calib->dig_P4 = (int16_t)(b[13] << 8U | b[12]);
	calib->dig_P5 = (int16_t)(b[15] << 8U | b[14]);
	calib->dig_P6 = (int16_t)(b[17] << 8U | b[16]);
	calib->dig_P7 = (int16_t)(b[19] << 8U | b[18]);
	calib->dig_P8 = (int16_t)(b[21] << 8U | b[20]);
	calib->dig_P9 = (int16_t)(b[23] << 8U | b[22]);

	if (calib->dig_P1 == 0U) {
		LOG_ERR("Invalid calibration data: dig_P1 is zero");
		return -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int bmp280_pm_action(const struct device *dev, enum pm_device_action action)
{
	/* TBI */
	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, bmp280_api) = {
	.sample_fetch = bmp280_sample_fetch,
	.channel_get = bmp280_channel_get,
};

static int bmp280_init(const struct device *dev)
{
	uint8_t val = 0U;
	int ret;
	struct bmp280_data *bmp280 = dev->data;

	if (bmp280_bus_check(dev) < 0) {
		LOG_DBG("bus check failed");
		return -ENODEV;
	}

	/* Reset sensor */
	ret = bmp280_reg_write(dev, BMP280_RESET, BMP280_SOFTRESET_VALUE);
	if (ret < 0) {
		LOG_ERR("Cannot reboot chip: %d", ret);
		return ret;
	}

	k_sleep(K_MSEC(2));

	ret = bmp280_reg_read(dev, BMP280_ID, &val, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read chip id.");
		return ret;
	}

	if (val != BMP280_CHIPID) {
		LOG_ERR("Unsupported chip detected (0x%x)!", val);
		return -ENODEV;
	}

	/* Read calibration data */
	ret = bmp280_get_calibration_data(dev);
	if (ret < 0) {
		LOG_ERR("Failed to read calibration data.");
		return ret;
	}

	/* Sets BMP280 configuration register */
	uint8_t config = (bmp280->t_sb << 5U) | (bmp280->filter << 2U) | (bmp280->spi3w_en);

	ret = bmp280_reg_write(dev, BMP280_CONFIG, config);
	if (ret < 0) {
		LOG_ERR("Failed to set configuration register.");
		return ret;
	}
	/* Sets BMP280 control measurement register */
	uint8_t control = (bmp280->osrs_t << 5U) | (bmp280->osrs_p << 2U) | (bmp280->mode);

	ret = bmp280_reg_write(dev, BMP280_CTRL_MEAS, control);
	if (ret < 0) {
		LOG_ERR("Failed to set control measurement register.");
		return ret;
	}
	return ret;
}

#define BMP280_INST(inst)                                                                          \
	static struct bmp280_data bmp280_data_##inst = {                                           \
		.osrs_p = DT_INST_PROP(inst, osrs_press),                                          \
		.osrs_t = DT_INST_PROP(inst, osrs_temp),                                           \
		.mode = DT_INST_PROP(inst, mode),                                                  \
		.t_sb = DT_INST_PROP(inst, t_sb),                                                  \
		.filter = DT_INST_PROP(inst, filter),                                              \
	};                                                                                         \
	static const struct bmp280_config bmp280_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, bmp280_pm_action);                                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmp280_init, PM_DEVICE_DT_INST_GET(inst),               \
				     &bmp280_data_##inst, &bmp280_config_##inst, POST_KERNEL,      \
				     CONFIG_SENSOR_INIT_PRIORITY, &bmp280_api);

DT_INST_FOREACH_STATUS_OKAY(BMP280_INST)
