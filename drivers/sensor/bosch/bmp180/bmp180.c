/* Bosch BMP180 pressure sensor
 *
 * Copyright (c) 2024 , Chris Ruehl
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.mouser.hk/datasheet/2/783/BST-BMP180-DS000-1509579.pdf
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pm/device.h>
#include <math.h>

#include "bmp180.h"

LOG_MODULE_REGISTER(BMP180, CONFIG_SENSOR_LOG_LEVEL);

static inline int bmp180_bus_check(const struct device *dev)
{
	const struct bmp180_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static inline int bmp180_reg_read(const struct device *dev,
				  uint8_t start, uint8_t *buf, int size)
{
	const struct bmp180_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, start, buf, size);
}

static inline int bmp180_reg_write(const struct device *dev, uint8_t reg,
				   uint8_t val)
{
	const struct bmp180_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, val);
}

int bmp180_reg_field_update(const struct device *dev,
			    uint8_t reg,
			    uint8_t mask,
			    uint8_t val)
{
	int rc = 0;
	uint8_t old_value, new_value;
	const struct bmp180_config *cfg = dev->config;

	rc = cfg->bus_io->read(&cfg->bus, reg, &old_value, 1);
	if (rc != 0) {
		return rc;
	}

	new_value = (old_value & ~mask) | (val & mask);
	if (new_value == old_value) {
		return 0;
	}

	return cfg->bus_io->write(&cfg->bus, reg, new_value);
}

#ifdef CONFIG_BMP180_OSR_RUNTIME
static int bmp180_attr_set_oversampling(const struct device *dev,
					enum sensor_channel chan,
					uint16_t val)
{
	struct bmp180_data *data = dev->data;

	/* Value must be a positive value 0-3 */
	if(val > 3) {
		return -EINVAL;
	}

	if (chan == SENSOR_CHAN_PRESS) {
	     data->osr_pressure = val;
	}
	else {
		return -EINVAL;
	}

	return 0;
}
#endif

static int bmp180_attr_set(const struct device *dev,
			   enum sensor_channel chan,
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
#endif

	switch (attr) {
#ifdef CONFIG_BMP180_OSR_RUNTIME
	case SENSOR_ATTR_OVERSAMPLING:
		ret = bmp180_attr_set_oversampling(dev, chan, val->val1);
		break;
#endif

	default:
		ret = -EINVAL;
	}

	return ret;
}


static int readRawTemperature(const struct device *dev)
{
	int ret;
	bool wait = true;
	uint8_t regControl, msb, lsb;
	struct bmp180_data *data = dev->data;
	/* send temperature measurement command */
	ret = bmp180_reg_write(dev, BMP180_REG_MEAS_CTRL, BMP180_CMD_GET_TEMPERATURE);
	if (ret)
	  return ret;

	/* for the first while read delay is 3000+1000 */
	k_busy_wait(3000);
	
	while(wait) {
		k_busy_wait(1000);
		ret = bmp180_reg_read(dev, BMP180_REG_MEAS_CTRL, &regControl, 1);
		if (ret)
			return ret;

		/* bit 5 is 1 until data registers are ready */ 
		if ((regControl & BMP180_STATUS_CMD_RDY)==0) {
			wait = false;
		}
	} 

	ret = bmp180_reg_read(dev, BMP180_REG_MSB, &msb, 1);
		if (ret)
			return ret;
	ret = bmp180_reg_read(dev, BMP180_REG_LSB, &lsb, 1);
		if (ret)
			return ret;
	
	data->sample.raw_temp = ((int32_t)msb<<8) | (int32_t)lsb;
	data->sample.comp_temp = 0;

	return 0;
}

static int readRawPressure(const struct device *dev)
{
	int ret;
	bool wait = true;
	uint8_t regControl, msb, lsb, xlsb;
	uint32_t delay;
	int32_t pressure;
	struct bmp180_data *data = dev->data;

	switch (data->osr_pressure) {
		case BMP180_ULTRALOWPOWER:
			regControl = BMP180_CMD_GET_OSS0_PRESS;
			delay = BMP180_CMD_GET_OSS0_DELAY;
			break;

		case BMP180_STANDARD:
			regControl = BMP180_CMD_GET_OSS1_PRESS;
			delay = BMP180_CMD_GET_OSS1_DELAY;
			break;

		case BMP180_HIGHRES:
			regControl = BMP180_CMD_GET_OSS2_PRESS;
			delay = BMP180_CMD_GET_OSS2_DELAY;
			break;

		case BMP180_ULTRAHIGH:
			regControl = BMP180_CMD_GET_OSS3_PRESS;
			delay = BMP180_CMD_GET_OSS3_DELAY;
			break;
		default:
			return -EINVAL;
	}

	ret = bmp180_reg_write(dev, BMP180_REG_MEAS_CTRL, regControl);
	if (ret)
		return ret;

	k_busy_wait(delay);

	while(wait) {
		k_busy_wait(1000);
		ret = bmp180_reg_read(dev, BMP180_REG_MEAS_CTRL, &regControl, 1);
		if (ret)
			return ret;

		if ((regControl & BMP180_STATUS_CMD_RDY)==0) {
			wait = false;
		}
	} 

	ret = bmp180_reg_read(dev, BMP180_REG_MSB, &msb, 1);
	if (ret)
		return ret;
	
	ret = bmp180_reg_read(dev, BMP180_REG_LSB, &lsb, 1);
	if (ret)
		return ret;
	
	ret = bmp180_reg_read(dev, BMP180_REG_XLSB, &xlsb, 1);
	if (ret)
		return ret;

	pressure = ((int32_t)msb<<16) | ((int32_t)lsb<<8) | (int32_t)xlsb;
	pressure >>= (8 - data->osr_pressure);
	data->sample.raw_press = pressure;

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
#endif

	pm_device_busy_set(dev);

	ret = readRawTemperature(dev);
	if (ret < 0) {
		goto error;
	}

	ret = readRawPressure(dev);
	if (ret < 0) {
		goto error;
	}

error:
	pm_device_busy_clear(dev);
	return ret;
}

static void bmp180_compensate_temp(struct bmp180_data *data)
{
	int32_t partial_data1;
	int32_t partial_data2;
	struct bmp180_cal_data *cal = &data->cal;

	partial_data1 = (data->sample.raw_temp - cal->ac6) * cal->ac5 / 0x8000;
	partial_data2 = cal->mc * 0x800 / (partial_data1 + cal->md);

	/* Store for pressure calculation */
	data->sample.comp_temp = (partial_data1 + partial_data2);
}

static int bmp180_temp_channel_get(const struct device *dev,
				   struct sensor_value *val)
{
	struct bmp180_data *data = dev->data;

	if (data->sample.comp_temp == 0) {
		bmp180_compensate_temp(data);
	}

	float ftmp = ((data->sample.comp_temp+8)>>4) / 10.0;
	int64_t tmp = floor(ftmp * 1000000);

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
	int32_t raw_pressure = data->sample.raw_press;
	struct bmp180_cal_data *cal = &data->cal;

	partial_B6 = data->sample.comp_temp - 4000;
	partial_X1 = (cal->b2 * partial_B6 * partial_B6) / 0x800000;
	partial_X2 = (cal->ac2 * partial_B6) / 0x800;
	partial_X3 = partial_X1 + partial_X2;
	partial_B3 = (((cal->ac1 * 4 + partial_X3)<<data->osr_pressure)+2) / 4;

	partial_X1 = (cal->ac3 * partial_B6) / 0x2000;
	partial_X2 = (cal->b1 * partial_B6 * partial_B6) / 0x10000000;
	partial_X3 = (partial_X1 + partial_X2 + 2) / 4;
	partial_B4 = (cal->ac4 * (partial_X3 + 32768)) / 0x8000;
	partial_B7 = (raw_pressure - partial_B3) * (50000>>data->osr_pressure);

	comp_press = (uint32_t)(partial_B7 / partial_B4 * 2);

	partial_X1 = (comp_press>>8)*(comp_press>>8);
	partial_X1 = (partial_X1 * 3038) / 0x10000;
	partial_X2 = ((int32_t)(-7357 * comp_press))/ 0x10000;
	comp_press += (partial_X1 + partial_X2 + 3791) / 16;

	/* returned value is Pa */
	return comp_press;
}

static int bmp180_press_channel_get(const struct device *dev,
				    struct sensor_value *val)
{
	struct bmp180_data *data = dev->data;

	if (data->sample.comp_temp == 0) {
		bmp180_compensate_temp(data);
	}

	uint32_t tmp = bmp180_compensate_press(data);

	/* tmp is in hundredths of Pa. Convert to kPa as specified in sensor
	 * interface.
	 */
	val->val1 = tmp / 100000;
	val->val2 = (tmp % 100000) * 10;

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
	case SENSOR_CHAN_AMBIENT_TEMP:
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

	if (bmp180_reg_read(dev, BMP180_REG_CALIB0, (uint8_t *)cal, sizeof(struct bmp180_cal_data)) < 0) {
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

	LOG_DBG("ac1 = %d", cal->ac1);
	LOG_DBG("ac2 = %d", cal->ac2);
	LOG_DBG("ac3 = %d", cal->ac3);
	LOG_DBG("ac4 = %d", cal->ac4);
	LOG_DBG("ac5 = %d", cal->ac5);
	LOG_DBG("ac6 = %d", cal->ac6);
	LOG_DBG("b1 = %d", cal->b1);
	LOG_DBG("b2 = %d", cal->b2);
	LOG_DBG("mb = %d", cal->mb);
	LOG_DBG("mc = %d", cal->mc);
	LOG_DBG("md = %d", cal->md);

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

static const struct sensor_driver_api bmp180_api = {
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

	k_busy_wait(2000);

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

/* Initializes a struct bmp180_config for an instance on an I2C bus. */
#define BMP180_CONFIG_I2C(inst)			       \
	.bus.i2c = I2C_DT_SPEC_INST_GET(inst),	       \
	.bus_io = &bmp180_bus_io_i2c,

#define BMP180_INST(inst)						   \
	static struct bmp180_data bmp180_data_##inst = {		   \
		.osr_pressure = DT_INST_ENUM_IDX(inst, osr_press),	   \
	};								   \
	static const struct bmp180_config bmp180_config_##inst = {	   \
		BMP180_CONFIG_I2C(inst)					   \
	};								   \
	PM_DEVICE_DT_INST_DEFINE(inst, bmp180_pm_action);		   \
	SENSOR_DEVICE_DT_INST_DEFINE(					   \
		inst,							   \
		bmp180_init,						   \
		PM_DEVICE_DT_INST_GET(inst),				   \
		&bmp180_data_##inst,					   \
		&bmp180_config_##inst,					   \
		POST_KERNEL,						   \
		CONFIG_SENSOR_INIT_PRIORITY,				   \
		&bmp180_api);

DT_INST_FOREACH_STATUS_OKAY(BMP180_INST)
