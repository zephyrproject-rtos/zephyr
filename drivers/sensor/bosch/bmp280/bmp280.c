/* Bosch BMP280 pressure sensor
 *
 * Copyright (c) 2023 Russ Webber
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf
 */

#define DT_DRV_COMPAT bosch_bmp280

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

#include "bmp280.h"

LOG_MODULE_REGISTER(BMP280, CONFIG_SENSOR_LOG_LEVEL);

static int bmp280_read_i2c(const struct device *dev,
			   uint8_t reg,
			   void *data,
			   size_t length)
{
	const struct bmp280_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->i2c, reg, data, length);
}

static int bmp280_byte_read_i2c(const struct device *dev,
				uint8_t reg,
				uint8_t *byte)
{
	const struct bmp280_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->i2c, reg, byte);
}

static int bmp280_byte_write_i2c(const struct device *dev,
				 uint8_t reg,
				 uint8_t byte)
{
	const struct bmp280_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, reg, byte);
}

#ifdef CONFIG_BMP280_ODR_RUNTIME
static int bmp280_freq_to_odr_val(uint16_t freq_int, uint16_t freq_milli)
{
	// size_t i;

	// /* An ODR of 0 Hz is not allowed */
	// if (freq_int == 0U && freq_milli == 0U) {
	// 	return -EINVAL;
	// }

	// for (i = 0; i < ARRAY_SIZE(bmp280_odr_map); i++) {
	// 	if (freq_int < bmp280_odr_map[i].freq_int ||
	// 	    (freq_int == bmp280_odr_map[i].freq_int &&
	// 	     freq_milli <= bmp280_odr_map[i].freq_milli)) {
	// 		return (ARRAY_SIZE(bmp280_odr_map) - 1) - i;
	// 	}
	// }

	return -EINVAL;
}

static int bmp280_attr_set_odr(const struct device *dev,
			       uint16_t freq_int,
			       uint16_t freq_milli)
{
	int err;
	// struct bmp280_data *data = dev->data;
	// int odr = bmp280_freq_to_odr_val(freq_int, freq_milli);

	// if (odr < 0) {
	// 	return odr;
	// }

	// // err = bmp280_reg_field_update_i2c(dev,
	// // 			      BMP280_REG_ODR,
	// // 			      BMP280_ODR_MASK,
	// // 			      (uint8_t)odr);
	// if (err == 0) {
	// 	data->odr = odr;
	// }

	return err;
}
#endif

#ifdef CONFIG_BMP280_OSR_RUNTIME
static int bmp280_attr_set_oversampling(const struct device *dev,
					enum sensor_channel chan,
					uint16_t val)
{
	// uint8_t reg_val = 0;
	// uint32_t pos, mask;
	int err;

	// struct bmp280_data *data = dev->data;

	// /* Value must be a positive power of 2 <= 32. */
	// if ((val <= 0) || (val > 32) || ((val & (val - 1)) != 0)) {
	// 	return -EINVAL;
	// }

	// if (chan == SENSOR_CHAN_PRESS) {
	// 	pos = BMP280_OSR_PRESSURE_POS;
	// 	mask = BMP280_OSR_PRESSURE_MASK;
	// } else if ((chan == SENSOR_CHAN_AMBIENT_TEMP) ||
	// 	   (chan == SENSOR_CHAN_DIE_TEMP)) {
	// 	pos = BMP280_OSR_TEMP_POS;
	// 	mask = BMP280_OSR_TEMP_MASK;
	// } else {
	// 	return -EINVAL;
	// }

	// /* Determine exponent: this corresponds to register setting. */
	// while ((val % 2) == 0) {
	// 	val >>= 1;
	// 	++reg_val;
	// }

	// // err = bmp280_reg_field_update_i2c(dev,
	// // 			      BMP280_REG_OSR,
	// // 			      mask,
	// // 			      reg_val << pos);
	// if (err < 0) {
	// 	return err;
	// }

	// /* Store for future use in converting RAW values. */
	// // if (chan == SENSOR_CHAN_PRESS) {
	// // 	data->osr_pressure = reg_val;
	// // } else {
	// // 	data->osr_temp = reg_val;
	// // }

	return err;
}
#endif

static int bmp280_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	int ret;

	switch (attr) {
#ifdef CONFIG_BMP280_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = bmp280_attr_set_odr(dev, val->val1, val->val2 / 1000);
		break;
#endif

#ifdef CONFIG_BMP280_OSR_RUNTIME
	case SENSOR_ATTR_OVERSAMPLING:
		ret = bmp280_attr_set_oversampling(dev, chan, val->val1);
		break;
#endif

	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static uint32_t bmp280_get_be20(const uint8_t src[3])
{
	/* sensor stores 4 bits of XLSB in src[2] bits 7:4 */
	// return ((uint32_t)src[2] >> 4) | (sys_get_be16(&src[0]) << 4);
	return sys_get_be24(&src[0]) >> 4;
}

static int bmp280_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct bmp280_data *bmp280 = dev->data;
	struct bmp280_ctrl_meas *ctrl_meas = &bmp280->ctrl_meas;
	uint8_t status;
	int ret = 0;
	uint8_t raw[BMP280_SAMPLE_BUFFER_SIZE];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (bmp280_byte_read_i2c(dev, BMP280_REG_STATUS, &status) < 0) {
		LOG_ERR("Failed to read STATUS byte");
		return -EIO;
	}

	ctrl_meas->power_mode = BMP280_PWR_CTRL_MODE_FORCED;

	if (bmp280_byte_write_i2c(dev, BMP280_REG_CTRL_MEAS, *(uint8_t*)ctrl_meas) < 0) {
		LOG_ERR("Cannot write CTRL_MEAS.");
		return -EIO;
	}

	k_sleep(K_MSEC(20));

	if (bmp280_byte_read_i2c(dev, BMP280_REG_STATUS, &status) < 0) {
		LOG_ERR("Failed to read STATUS byte");
		return -EIO;
	}

	LOG_DBG("STATUS: 0x%x", status);

	if (status & BMP280_STATUS_MEASURING) {
		LOG_ERR("Sensor busy");
		return -EBUSY;
	}

	ret = bmp280_read_i2c(dev,
			  BMP280_REG_RAW_PRESSURE,
			  raw,
			  BMP280_SAMPLE_BUFFER_SIZE);

	if (ret < 0) {
		LOG_ERR("Could not read sample register");
		return ret;
	}

	bmp280->sample.raw_pressure = bmp280_get_be20(&raw[BMP280_SAMPLE_PRESSURE_POS]);
	bmp280->sample.raw_temp = bmp280_get_be20(&raw[BMP280_SAMPLE_TEMPERATURE_POS]);
	bmp280->sample.temp_fine = 0;
	return ret;
}

static int32_t bmp280_compensate_temp(struct bmp280_data *data)
{
	/* Adapted from:
	 * https://github.com/boschsensortec/BMP280_driver/blob/master/bmp280.c
	 */

	struct bmp280_cal_data *cal = &data->cal;
	int32_t tmp1, tmp2, raw_temp;

	raw_temp = data->sample.raw_temp;

	tmp1 = ((((raw_temp / 8) - ((int32_t) cal->t1 << 1))) * ((int32_t) cal->t2)) /
		2048;
	tmp2 = (((((raw_temp / 16) - ((int32_t) cal->t1)) *
			((raw_temp / 16) - ((int32_t) cal->t1))) / 4096) *
			((int32_t) cal->t3)) / 16384;

	/* Store for pressure compensation calculation */
	data->sample.temp_fine = tmp1 + tmp2;
	return (data->sample.temp_fine * 5 + 128) / 256;
}

static int bmp280_temp_channel_get(const struct device *dev,
				   struct sensor_value *val)
{
	struct bmp280_data *data = dev->data;

	/* tmp is in 1/100 deg C */
	int64_t tmp = bmp280_compensate_temp(data);
	val->val1 = tmp / 100;
	val->val2 = (tmp % 100) * 10000;

	return 0;
}

static uint64_t bmp280_compensate_press(struct bmp280_data *data)
{
	/* Adapted from:
	 * https://github.com/boschsensortec/BMP280_driver/blob/master/bmp280.c
	 */

    int64_t tmp1, tmp2, comp_press;

	struct bmp280_cal_data *cal = &data->cal;

	tmp1 = ((int64_t) (data->sample.temp_fine)) - 128000;
	tmp2 = tmp1 * tmp1 * (int64_t) cal->p6;
	tmp2 = tmp2 + ((tmp1 * (int64_t) cal->p5) * 131072);
	tmp2 = tmp2 + (((int64_t) cal->p4) * 34359738368);
	tmp1 = ((tmp1 * tmp1 * (int64_t) cal->p3) / 256) +
			((tmp1 * (int64_t) cal->p2) * 4096);
	tmp1 = ((0x800000000000 + tmp1) * ((int64_t)cal->p1)) / 8589934592;

	if (tmp1 == 0)
	{
		return 0;
	}

	comp_press = 1048576 - data->sample.raw_pressure;
	comp_press = (((((comp_press * 2147483648U)) - tmp2) * 3125) / tmp1);
	tmp1 = (((int64_t) cal->p9) * (comp_press / 8192) * (comp_press / 8192)) / 33554432;
	tmp2 = (((int64_t) cal->p8) * comp_press) / 524288;
	comp_press = ((comp_press + tmp1 + tmp2) / 256) + (((int64_t)cal->p7) * 16);

	/* returned value is in 1/256 Pa. */
	return comp_press;
}

static int bmp280_press_channel_get(const struct device *dev,
				    struct sensor_value *val)
{
	struct bmp280_data *data = dev->data;

	if (data->sample.temp_fine == 0) {
		bmp280_compensate_temp(data);
	}

	uint64_t tmp = bmp280_compensate_press(data);

	/* tmp is in 1/256 Pa. Convert to kPa as specified in sensor interface. */
	val->val1 = tmp / (256 * 1000);
	val->val2 = (tmp % (256 * 1000)) * 10;

	return 0;
}

static int bmp280_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	switch (chan) {
		case SENSOR_CHAN_PRESS:
			bmp280_press_channel_get(dev, val);
			break;

		case SENSOR_CHAN_DIE_TEMP:
		case SENSOR_CHAN_AMBIENT_TEMP:
			bmp280_temp_channel_get(dev, val);
			break;

		default:
			LOG_ERR("Channel not supported.");
			return -ENOTSUP;
	}

	return 0;
}

static int bmp280_get_compensation_params(const struct device *dev)
{
	struct bmp280_data *data = dev->data;
	struct bmp280_cal_data *cal = &data->cal;

	if (bmp280_read_i2c(dev, BMP280_REG_COMPENSATION_PARAMS, cal, sizeof(*cal)) < 0) {
		return -EIO;
	}

	cal->t1 = sys_le16_to_cpu(cal->t1);
	cal->t2 = sys_le16_to_cpu(cal->t2);
	cal->t3 = sys_le16_to_cpu(cal->t3);
	cal->p1 = sys_le16_to_cpu(cal->p1);
	cal->p2 = sys_le16_to_cpu(cal->p2);
	cal->p3 = sys_le16_to_cpu(cal->p3);
	cal->p4 = sys_le16_to_cpu(cal->p4);
	cal->p5 = sys_le16_to_cpu(cal->p5);
	cal->p6 = sys_le16_to_cpu(cal->p6);
	cal->p7 = sys_le16_to_cpu(cal->p7);
	cal->p8 = sys_le16_to_cpu(cal->p8);
	cal->p9 = sys_le16_to_cpu(cal->p9);

	LOG_DBG("t1: %d", cal->t1);
	LOG_DBG("t2: %d", cal->t2);
	LOG_DBG("t3: %d", cal->t3);
	LOG_DBG("p1: %d", cal->p1);
	LOG_DBG("p2: %d", cal->p2);
	LOG_DBG("p3: %d", cal->p3);
	LOG_DBG("p4: %d", cal->p4);
	LOG_DBG("p5: %d", cal->p5);
	LOG_DBG("p6: %d", cal->p6);
	LOG_DBG("p7: %d", cal->p7);
	LOG_DBG("p8: %d", cal->p8);
	LOG_DBG("p9: %d", cal->p9);

	return 0;
}

static int bmp280_init(const struct device *dev)
{
	struct bmp280_data *bmp280 = dev->data;
	const struct bmp280_config *cfg = dev->config;
	struct bmp280_config_byte *config_byte = &bmp280->config_byte;
	struct bmp280_ctrl_meas *ctrl_meas = &bmp280->ctrl_meas;
	uint8_t val;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus device is not ready");
		return -EINVAL;
	}

	if (bmp280_byte_read_i2c(dev, BMP280_REG_CHIP_ID, &val) < 0) {
		LOG_ERR("Failed to read chip id.");
		return -EIO;
	}

	if (!(val == BMP280_CHIP_ID1 || val == BMP280_CHIP_ID2 || val == BMP280_CHIP_ID3)) {
		LOG_ERR("Unsupported chip detected (0x%x)!", val);
		return -ENODEV;
	}

	LOG_DBG("Chip id: 0x%x", val);

	/* reboot the chip */
	if (bmp280_byte_write_i2c(dev, BMP280_REG_RESET, BMP280_CMD_SOFT_RESET) < 0) {
		LOG_ERR("Cannot reboot chip.");
		return -EIO;
	}

	/* wait 2ms for chip to reboot */
	k_busy_wait(2000);

	/* Read calibration data */
	if (bmp280_get_compensation_params(dev) < 0) {
		LOG_ERR("Failed to read compensation parameters.");
		return -EIO;
	}

	/* Set config - IIR filter coefficient & ODR */
	if (bmp280_byte_write_i2c(dev, BMP280_REG_CONFIG, *(uint8_t*)config_byte) < 0) {
		LOG_ERR("Cannot write CONFIG.");
		return -EIO;
	}

	/* Enable normal mode */
	// ctrl_meas->power_mode = BMP280_PWR_CTRL_MODE_NORMAL;

	if (bmp280_byte_write_i2c(dev, BMP280_REG_CTRL_MEAS, *(uint8_t*)ctrl_meas) < 0) {
		LOG_ERR("Cannot write CTRL_MEAS.");
		return -EIO;
	}

	return 0;
}

static const struct sensor_driver_api bmp280_api = {
	.attr_set = bmp280_attr_set,
	.sample_fetch = bmp280_sample_fetch,
	.channel_get = bmp280_channel_get,
};

// DT_INST_ENUM(inst, osr_press)

#define BMP280_INST(inst)						   \
	static struct bmp280_data bmp280_data_##inst = {		   \
		.odr = DT_INST_ENUM_IDX(inst, odr),			\
		.ctrl_meas = { \
			.os_res_pressure = BMP280_OSRS[DT_INST_ENUM_IDX(inst, osr_pressure)],			\
			.os_res_temp = BMP280_OSRS[DT_INST_ENUM_IDX(inst, osr_temperature)] },			\
		.config_byte = { \
			.iir_filter = 0,			\
			.t_standby = DT_INST_ENUM_IDX(inst, odr) }			\
	};								   \
	static const struct bmp280_config bmp280_config_##inst = {	   \
		.i2c = I2C_DT_SPEC_INST_GET(inst),					   \
	};								   \
	DEVICE_DT_INST_DEFINE(						   \
		inst,							   \
		bmp280_init,						   \
		NULL,						   \
		&bmp280_data_##inst,					   \
		&bmp280_config_##inst,					   \
		POST_KERNEL,						   \
		CONFIG_SENSOR_INIT_PRIORITY,				   \
		&bmp280_api);

DT_INST_FOREACH_STATUS_OKAY(BMP280_INST)
