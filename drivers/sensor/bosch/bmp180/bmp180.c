/* Bosch BMP180 pressure sensor
 *
 * Copyright (c) 2024 , Chris Ruehl
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.mouser.hk/datasheet/2/783/BST-BMP180-DS000-1509579.pdf
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

#include "bmp180.h"

#define DT_DRV_COMPAT bosch_bmp180

LOG_MODULE_REGISTER(BMP180, CONFIG_SENSOR_LOG_LEVEL);

/*
 * byte swap added for signed (int16_t)
 * and named _u16 and _s16 to make clear what its used for
 * macro from include/zephyr/sys/byteorder.h
 */
#define BSWAP_u16(x) sys_cpu_to_be16(x)
#define BSWAP_s16(x) ((int16_t) sys_cpu_to_be16(x))

/* Calibration Registers structure */
struct bmp180_cal_data {
	int16_t ac1;
	int16_t ac2;
	int16_t ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t b1;
	int16_t b2;
	int16_t mb;
	int16_t mc;
	int16_t md;
} __packed;

struct bmp180_config {
	const struct i2c_dt_spec i2c;
};

struct bmp180_data {
	uint8_t osr_pressure;
	int32_t raw_press;
	int32_t raw_temp;
	int32_t comp_temp;
	struct bmp180_cal_data cal;
};

static inline int bmp180_bus_check(const struct device *dev)
{
	const struct bmp180_config *cfg = dev->config;

	return i2c_is_ready_dt(&cfg->i2c) ? 0 : -ENODEV;
}

static inline int bmp180_reg_read(const struct device *dev, uint8_t start,
				 uint8_t *buf, int size)
{
	const struct bmp180_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->i2c, start, buf, size);
}

static inline int bmp180_reg_write(const struct device *dev, uint8_t reg,
				   uint8_t val)
{
	const struct bmp180_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, reg, val);
}

#ifdef CONFIG_BMP180_OSR_RUNTIME
static int bmp180_attr_set_oversampling(const struct device *dev,
					enum sensor_channel chan, uint16_t val)
{
	struct bmp180_data *data = dev->data;

	/* Value must be a positive value 0-3 */
	if ((chan != SENSOR_CHAN_PRESS) || (val > 3)) {
		return -EINVAL;
	}

	data->osr_pressure = val;

	return 0;
}
#endif /* CONFIG_BMP180_OSR_RUNTIME */

static int bmp180_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	int ret;

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif /* CONFIG_PM_DEVICE */

	switch (attr) {
#ifdef CONFIG_BMP180_OSR_RUNTIME
	case SENSOR_ATTR_OVERSAMPLING:
		ret = bmp180_attr_set_oversampling(dev, chan, val->val1);
		break;
#endif /* CONFIG_BMP180_OSR_RUNTIME */

	default:
		ret = -EINVAL;
	}

	return ret;
}

static inline int bmp180_conv_ready(const struct device *dev, uint32_t time_wait_ms)
{
	int ret;
	uint8_t ctrlreg;
	uint8_t retry = 2;

	k_sleep(K_MSEC(time_wait_ms));

	/*
	 * for the first while read 'delay+1' ms which is the convension time
	 * descripted in the data-sheet in case the register not yet ready wait again
	 * and return error if retry exhaused
	 */
	while (true) {
		k_sleep(K_MSEC(1));
		ret = bmp180_reg_read(dev, BMP180_REG_MEAS_CTRL, &ctrlreg, 1);
		if (ret) {
			return ret;
		}

		/* bit 5 is 1 until data registers are ready */
		if ((ctrlreg & BMP180_STATUS_CMD_RDY) == 0) {
			break;
		}

		--retry;
		if (retry == 0) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int read_raw_temperature(const struct device *dev)
{
	int ret;
	uint8_t reg16[2];
	struct bmp180_data *data = dev->data;

	/* send temperature measurement command */
	ret = bmp180_reg_write(dev, BMP180_REG_MEAS_CTRL, BMP180_CMD_GET_TEMPERATURE);
	if (ret) {
		return ret;
	}

	ret = bmp180_conv_ready(dev, BMP180_CMD_GET_TEMP_DELAY);
	if (ret) {
		return ret;
	}

	/* read msb and lsb */
	ret = bmp180_reg_read(dev, BMP180_REG_MSB, (uint8_t *)&reg16, 2);
	if (ret) {
		return ret;
	}

	data->raw_temp = (int32_t)(sys_get_be16(reg16));
	data->comp_temp = 0;

	return 0;
}

static int read_raw_pressure(const struct device *dev)
{
	int ret;
	uint8_t ctrlreg;
	uint8_t reg24[3];
	uint32_t delay;
	int32_t pressure;
	struct bmp180_data *data = dev->data;

	switch (data->osr_pressure) {
	case BMP180_ULTRALOWPOWER:
		ctrlreg = BMP180_CMD_GET_OSS0_PRESS;
		delay = BMP180_CMD_GET_OSS0_DELAY;
		break;
	case BMP180_STANDARD:
		ctrlreg = BMP180_CMD_GET_OSS1_PRESS;
		delay = BMP180_CMD_GET_OSS1_DELAY;
		break;
	case BMP180_HIGHRES:
		ctrlreg = BMP180_CMD_GET_OSS2_PRESS;
		delay = BMP180_CMD_GET_OSS2_DELAY;
		break;
	case BMP180_ULTRAHIGH:
		ctrlreg = BMP180_CMD_GET_OSS3_PRESS;
		delay = BMP180_CMD_GET_OSS3_DELAY;
		break;
	default:
		return -EINVAL;
	}

	ret = bmp180_reg_write(dev, BMP180_REG_MEAS_CTRL, ctrlreg);
	if (ret) {
		return ret;
	}

	ret = bmp180_conv_ready(dev, delay);
	if (ret) {
		return ret;
	}

	/* read msb,lsb and xlsb */
	ret = bmp180_reg_read(dev, BMP180_REG_MSB, (uint8_t *)&reg24, 3);
	if (ret) {
		return ret;
	}

	pressure = (int32_t)sys_get_be24(reg24);
	pressure >>= (8 - data->osr_pressure);
	data->raw_press = pressure;

	return 0;
}

static int bmp180_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	int ret = 0;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif /* CONFIG_PM_DEVICE */

	pm_device_busy_set(dev);

	ret = read_raw_temperature(dev);
	if (ret < 0) {
		goto error;
	}

	ret = read_raw_pressure(dev);

error:
	pm_device_busy_clear(dev);
	return ret;
}

static void bmp180_compensate_temp(struct bmp180_data *data)
{
	int32_t partial_data1;
	int32_t partial_data2;
	int32_t divisor;
	struct bmp180_cal_data *cal = &data->cal;

	partial_data1 = (data->raw_temp - cal->ac6) * cal->ac5 / 0x8000;

	/* Check divisor before division */
	divisor = partial_data1 + cal->md;
	__ASSERT(divisor != 0, "divisor is zero: partial_data1=%d, md=%d", partial_data1, cal->md);

	partial_data2 = cal->mc * 0x800 / divisor;

	/* Store for pressure calculation */
	data->comp_temp = (partial_data1 + partial_data2);
}

static int bmp180_temp_channel_get(const struct device *dev,
				   struct sensor_value *val)
{
	struct bmp180_data *data = dev->data;

	if (data->comp_temp == 0) {
		bmp180_compensate_temp(data);
	}

	float ftmp = ((data->comp_temp + 8) >> 4) / 10.0;
	int64_t tmp = (int64_t)floorf(ftmp * 1000000);

	val->val1 = tmp / 1000000;
	val->val2 = tmp % 1000000;

	return 0;
}

static uint32_t bmp180_compensate_press(struct bmp180_data *data)
{
	uint64_t partial_B7;
	uint64_t partial_B4;
	int32_t partial_X1;
	int32_t partial_X2;
	int32_t partial_X3;
	int32_t partial_B3;
	int32_t partial_B6;
	uint32_t comp_press;
	int32_t raw_pressure = data->raw_press;
	struct bmp180_cal_data *cal = &data->cal;

	partial_B6 = data->comp_temp - 4000;
	partial_X1 = cal->b2 * partial_B6 * (float)(1.0f * partial_B6 / 0x800000);
	partial_X2 = (cal->ac2 * partial_B6) / 0x800;
	partial_X3 = partial_X1 + partial_X2;
	partial_B3 = (((cal->ac1 * 4 + partial_X3) << data->osr_pressure) + 2) / 4;

	partial_X1 = (cal->ac3 * partial_B6) / 0x2000;
	partial_X2 = cal->b1 * partial_B6 * (float)(1.0f * partial_B6 / 0x8000000);
	partial_X3 = (partial_X1 + partial_X2 + 2) / 4;
	partial_B4 = (uint64_t)(cal->ac4 * ((int64_t)(partial_X3) + 32768)) >> 15;
	partial_B7 = (uint64_t)(raw_pressure - partial_B3) * (50000 >> data->osr_pressure);

	comp_press = (uint32_t)(partial_B7 / partial_B4 * 2);

	partial_X1 = comp_press * (float)(1.0f * comp_press / 0x10000);
	partial_X1 = (partial_X1 * 3038) / 0x10000;
	partial_X2 = ((int32_t)(-7357 * comp_press)) / 0x10000;
	comp_press += (partial_X1 + partial_X2 + 3791) / 16;

	/* returned value is Pa */
	return comp_press;
}

static int bmp180_press_channel_get(const struct device *dev,
				    struct sensor_value *val)
{
	struct bmp180_data *data = dev->data;

	if (data->comp_temp == 0) {
		bmp180_compensate_temp(data);
	}

	uint32_t tmp = bmp180_compensate_press(data);

	/* tmp is Pa. Convert to kPa as specified in sensor
	 * interface.
	 */
	val->val1 = tmp / 1000;
	val->val2 = (tmp % 1000) * 1000;

	return 0;
}

static int bmp180_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_PRESS:
		bmp180_press_channel_get(dev, val);
		break;

	case SENSOR_CHAN_DIE_TEMP:
		bmp180_temp_channel_get(dev, val);
		break;

	default:
		LOG_DBG("Channel not supported.");
		return -ENOTSUP;
	}

	return 0;
}


static int bmp180_get_calibration_data(const struct device *dev)
{
	struct bmp180_data *data = dev->data;
	struct bmp180_cal_data *cal = &data->cal;

	if (bmp180_reg_read(dev, BMP180_REG_CALIB0, (uint8_t *)cal,
			    sizeof(struct bmp180_cal_data)) < 0) {
		return -EIO;
	}

	cal->ac1 = BSWAP_s16(cal->ac1);
	cal->ac2 = BSWAP_s16(cal->ac2);
	cal->ac3 = BSWAP_s16(cal->ac3);
	cal->ac4 = BSWAP_u16(cal->ac4);
	cal->ac5 = BSWAP_u16(cal->ac5);
	cal->ac6 = BSWAP_u16(cal->ac6);
	cal->b1 = BSWAP_s16(cal->b1);
	cal->b2 = BSWAP_s16(cal->b2);
	cal->mb = BSWAP_s16(cal->mb);
	cal->mc = BSWAP_s16(cal->mc);
	cal->md = BSWAP_s16(cal->md);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int bmp180_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	/* no power saving feature in bmp180 */
	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, bmp180_api) = {
	.attr_set = bmp180_attr_set,
	.sample_fetch = bmp180_sample_fetch,
	.channel_get = bmp180_channel_get,
};

static int bmp180_init(const struct device *dev)
{
	uint8_t val = 0U;

	if (bmp180_bus_check(dev) < 0) {
		LOG_DBG("bus check failed");
		return -ENODEV;
	}

	/* reboot the chip */
	if (bmp180_reg_write(dev, BMP180_REG_CMD, BMP180_CMD_SOFT_RESET) < 0) {
		LOG_ERR("Cannot reboot chip.");
		return -EIO;
	}

	k_sleep(K_MSEC(2));

	if (bmp180_reg_read(dev, BMP180_REG_CHIPID, &val, 1) < 0) {
		LOG_ERR("Failed to read chip id.");
		return -EIO;
	}

	if (val != BMP180_CHIP_ID) {
		LOG_ERR("Unsupported chip detected (0x%x)!", val);
		return -ENODEV;
	}

	/* Read calibration data */
	if (bmp180_get_calibration_data(dev) < 0) {
		LOG_ERR("Failed to read calibration data.");
		return -EIO;
	}

	return 0;
}

#define BMP180_INST(inst) \
	static struct bmp180_data bmp180_data_##inst = {\
		.osr_pressure = DT_INST_ENUM_IDX(inst, osr_press),\
	};\
	static const struct bmp180_config bmp180_config_##inst = {\
		.i2c = I2C_DT_SPEC_INST_GET(inst),\
	};\
	PM_DEVICE_DT_INST_DEFINE(inst, bmp180_pm_action);\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmp180_init, PM_DEVICE_DT_INST_GET(inst),\
				    &bmp180_data_##inst, &bmp180_config_##inst,\
				    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,\
				    &bmp180_api);

DT_INST_FOREACH_STATUS_OKAY(BMP180_INST)
