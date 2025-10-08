/*
 * Copyright (c) 2025 Andreas Klinger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_veml6046

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/sensor/veml6046.h>

LOG_MODULE_REGISTER(VEML6046, CONFIG_SENSOR_LOG_LEVEL);

/*
 * ID code of device
 */
#define VEML6046_DEFAULT_ID 0x01

/*
 * Bit mask to check for data ready in single measurement.
 */
#define VEML6046_AF_DATA_READY BIT(3)

/*
 * Maximum value of RGBIR data which also means that the sensor is in
 * saturation and that the measured value might be wrong.
 * In such a case the user program should reduce one or more of the following
 * attributes to get a relyable value:
 *   gain
 *   integration time
 *   effective photodiode size divider
 */
#define VEML6046_DATA_OVERFLOW 0xFFFF

/*
 * 16-bit command register addresses
 */
#define VEML6046_CMDCODE_RGB_CONF_0 0x00
#define VEML6046_CMDCODE_RGB_CONF_1 0x01
#define VEML6046_CMDCODE_G_THDH_L   0x04
#define VEML6046_CMDCODE_G_THDH_H   0x05
#define VEML6046_CMDCODE_G_THDL_L   0x06
#define VEML6046_CMDCODE_G_THDL_H   0x07
#define VEML6046_CMDCODE_R_DATA_L   0x10
#define VEML6046_CMDCODE_R_DATA_H   0x11
#define VEML6046_CMDCODE_G_DATA_L   0x12
#define VEML6046_CMDCODE_G_DATA_H   0x13
#define VEML6046_CMDCODE_B_DATA_L   0x14
#define VEML6046_CMDCODE_B_DATA_H   0x15
#define VEML6046_CMDCODE_IR_DATA_L  0x16
#define VEML6046_CMDCODE_IR_DATA_H  0x17
#define VEML6046_CMDCODE_ID_L       0x18
#define VEML6046_CMDCODE_ID_H       0x19
#define VEML6046_CMDCODE_INT_L      0x1A
#define VEML6046_CMDCODE_INT_H      0x1B

/*
 * Resolution matrix for values to convert between data provided
 * by the sensor ("counts") and lux.
 *
 * These values depend on the current size, gain and integration time settings.
 * The enumerators of enum veml6046_pdd, enum veml60xx_gain and enum
 * veml6046_als_it are used for indices into this matrix.
 */
static const float
	veml6046_resolution[VEML6046_PDD_COUNT][VEML60XX_GAIN_COUNT][VEML60XX_IT_COUNT] = {
		/*3.125ms   6.25ms   12.5ms     25ms     50ms    100ms    200ms     400ms IT */
		/* size 2/2 */
		{
			{1.3440f, 0.6720f, 0.3360f, 0.1680f, 0.0840f, 0.0420f, 0.0210f,
			 0.0105f}, /* Gain 1    */
			{0.6720f, 0.3360f, 0.1680f, 0.0840f, 0.0420f, 0.0210f, 0.0105f,
			 0.0053f}, /* Gain 2    */
			{2.0364f, 1.0182f, 0.5091f, 0.2545f, 0.1273f, 0.0636f, 0.0318f,
			 0.0159f}, /* Gain 0.66 */
			{2.6880f, 1.3440f, 0.6720f, 0.3360f, 0.1680f, 0.0840f, 0.0420f,
			 0.0210f}, /* Gain 0.5  */
		},
		{
			/* size 1/2 */
			{2.6880f, 1.3440f, 0.6720f, 0.3360f, 0.1680f, 0.0840f, 0.0420f,
			 0.0210f}, /* Gain 1    */
			{1.3440f, 0.6720f, 0.3360f, 0.1680f, 0.0840f, 0.0420f, 0.0210f,
			 0.0105f}, /* Gain 2    */
			{4.0727f, 2.0364f, 1.0182f, 0.5091f, 0.2545f, 0.1273f, 0.0636f,
			 0.0318f}, /* Gain 0.66 */
			{5.3760f, 2.6880f, 1.3440f, 0.6720f, 0.3360f, 0.1680f, 0.0840f,
			 0.0420f}, /* Gain 0.5  */
		},
};

struct veml6046_config {
	struct i2c_dt_spec bus;
};

struct veml6046_data {
	uint8_t sd;              /* Band gap and LDO shutdown */
	uint8_t int_en;          /* ALS interrupt enable */
	uint8_t trig;            /* ALS active force trigger */
	enum veml6046_pdd pdd;   /* effective photodiode size divider */
	enum veml60xx_gain gain; /* gain selection */
	enum veml60xx_it itim;   /* ALS integration time */
	enum veml60xx_pers pers; /* ALS persistens protect */
	uint16_t thresh_high;
	uint16_t thresh_low;
	uint16_t red_data;
	uint16_t green_data;
	uint16_t blue_data;
	uint16_t ir_data;
	uint32_t red_lux;
	uint32_t green_lux;
	uint32_t blue_lux;
	uint32_t ir_lux;
};

static bool veml6046_pdd_in_range(int32_t pdd)
{
	return pdd >= VEML6046_SIZE_2_2 && pdd <= VEML6046_SIZE_1_2;
}

static int veml6046_read16(const struct device *dev, uint8_t cmd, uint16_t *data)
{
	const struct veml6046_config *conf = dev->config;
	int ret;

	ret = i2c_burst_read_dt(&conf->bus, cmd, (uint8_t *)data, 2);
	if (ret < 0) {
		return ret;
	}

	*data = sys_le16_to_cpu(*data);

	return 0;
}

/*
 * This function excepts an array of uint8_t data[2] with the two corresponding
 * values set accordingly to the register map of the sensor.
 */
static int veml6046_write16(const struct device *dev, uint8_t cmd, uint8_t *data)
{
	const struct veml6046_config *conf = dev->config;

	return i2c_burst_write_dt(&conf->bus, cmd, data, 2);
}

static int veml6046_write_conf(const struct device *dev)
{
	int ret;
	struct veml6046_data *data = dev->data;
	uint8_t conf[2] = {0, 0};

	/* Bits 7 -> RGB_ON_1 */
	if (data->sd) {
		conf[1] |= BIT(7);
	}
	/* Bits 6 -> Effective photodiode size */
	conf[1] |= data->pdd << 6;
	/* Bit 5 -> reserved */
	/* Bits 4:3 -> Gain selection */
	conf[1] |= data->gain << 3;
	/* Bits 2:1 -> ALS persistence protect number */
	conf[1] |= data->pers << 1;
	/* Bit 0 -> Calibration should always be 1 when using the sensor */
	conf[1] |= BIT(0);
	/* Bit 7 -> reserved, have to be 0 */
	/* Bits 6:4 -> integration time (ALS_IT) */
	conf[0] |= data->itim << 4;
	/* Bit 3 -> Active force mode is always enabled
	 * Auto mode would continuously deliver data which is not what we want
	 * in this driver
	 */
	conf[0] |= BIT(3);
	/* Bit 2 -> ALS active force trigger */
	if (data->trig) {
		conf[0] |= BIT(2);
	}
	/* Bit 1 -> ALS interrupt enable */
	if (data->int_en) {
		conf[0] |= BIT(1);
	}
	/* Bit 0 -> shut down setting (SD) */
	if (data->sd) {
		conf[0] |= BIT(0);
	}

	ret = veml6046_write16(dev, VEML6046_CMDCODE_RGB_CONF_0, conf);
	if (ret) {
		LOG_ERR("Error while writing conf[0] ret: %d", ret);
		return ret;
	}

	return 0;
}

static int veml6046_write_thresh_high(const struct device *dev)
{
	int ret;
	const struct veml6046_data *data = dev->data;
	uint8_t val[2];

	val[0] = data->thresh_high & 0xFF;
	val[1] = data->thresh_high >> 8;

	LOG_DBG("Writing high threshold counts: %d", data->thresh_high);
	ret = veml6046_write16(dev, VEML6046_CMDCODE_G_THDH_L, val);
	if (ret) {
		return ret;
	}

	return 0;
}

static int veml6046_write_thresh_low(const struct device *dev)
{
	int ret;
	const struct veml6046_data *data = dev->data;
	uint8_t val[2];

	val[0] = data->thresh_low & 0xFF;
	val[1] = data->thresh_low >> 8;

	LOG_DBG("Writing low threshold counts: %d", data->thresh_low);
	ret = veml6046_write16(dev, VEML6046_CMDCODE_G_THDL_L, val);
	if (ret) {
		return ret;
	}

	return 0;
}

static int veml6046_fetch(const struct device *dev)
{
	struct veml6046_data *data = dev->data;
	int ret;

	ret = veml6046_read16(dev, VEML6046_CMDCODE_R_DATA_L, &data->red_data);
	if (ret) {
		return ret;
	}

	ret = veml6046_read16(dev, VEML6046_CMDCODE_G_DATA_L, &data->green_data);
	if (ret) {
		return ret;
	}

	ret = veml6046_read16(dev, VEML6046_CMDCODE_B_DATA_L, &data->blue_data);
	if (ret) {
		return ret;
	}

	ret = veml6046_read16(dev, VEML6046_CMDCODE_IR_DATA_L, &data->ir_data);
	if (ret) {
		return ret;
	}

	data->red_lux = data->red_data * veml6046_resolution[data->pdd][data->gain][data->itim];
	data->green_lux = data->green_data * veml6046_resolution[data->pdd][data->gain][data->itim];
	data->blue_lux = data->blue_data * veml6046_resolution[data->pdd][data->gain][data->itim];
	data->ir_lux = data->ir_data * veml6046_resolution[data->pdd][data->gain][data->itim];

	LOG_DBG("Read (R/G/B/IR): counts=%d/%d/%d/%d, lux=%d/%d/%d/%d",
		data->red_data, data->green_data, data->blue_data, data->ir_data,
		data->red_lux, data->green_lux, data->blue_lux, data->ir_lux);

	if ((data->red_data == VEML6046_DATA_OVERFLOW) ||
	    (data->green_data == VEML6046_DATA_OVERFLOW) ||
	    (data->blue_data == VEML6046_DATA_OVERFLOW) ||
	    (data->ir_data == VEML6046_DATA_OVERFLOW)) {
		return -E2BIG;
	}

	return 0;
}

static int veml6046_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct veml6046_data *data = dev->data;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/* SENSOR_ATTR_.*_THRESH are not in enum sensor_attribute_veml6046 */
	switch ((int)attr) {
	case SENSOR_ATTR_VEML6046_IT:
		if (veml60xx_it_in_range(val->val1)) {
			data->itim = (enum veml60xx_it)val->val1;
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_VEML6046_PDD:
		if (veml6046_pdd_in_range(val->val1)) {
			data->pdd = (enum veml6046_pdd)val->val1;
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_VEML6046_GAIN:
		if (veml60xx_gain_in_range(val->val1)) {
			data->gain = (enum veml60xx_gain)val->val1;
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_VEML6046_PERS:
		if (veml60xx_pers_in_range(val->val1)) {
			data->pers = (enum veml60xx_pers)val->val1;
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		data->thresh_low =
			val->val1 / veml6046_resolution[data->pdd][data->gain][data->itim];
		return veml6046_write_thresh_low(dev);
	case SENSOR_ATTR_UPPER_THRESH:
		data->thresh_high =
			val->val1 / veml6046_resolution[data->pdd][data->gain][data->itim];
		return veml6046_write_thresh_high(dev);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int veml6046_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct veml6046_data *data = dev->data;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/* SENSOR_ATTR_.*_THRESH are not in enum sensor_attribute_veml6046 */
	switch ((int)attr) {
	case SENSOR_ATTR_VEML6046_IT:
		val->val1 = data->itim;
		break;
	case SENSOR_ATTR_VEML6046_PDD:
		val->val1 = data->pdd;
		break;
	case SENSOR_ATTR_VEML6046_GAIN:
		val->val1 = data->gain;
		break;
	case SENSOR_ATTR_VEML6046_PERS:
		val->val1 = data->pers;
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		val->val1 = data->thresh_low
			    * veml6046_resolution[data->pdd][data->gain][data->itim];
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		val->val1 = data->thresh_high
			    * veml6046_resolution[data->pdd][data->gain][data->itim];
		break;
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

static int veml6046_perform_single_measurement(const struct device *dev)
{
	struct veml6046_data *data = dev->data;
	int ret;
	uint16_t val;
	int cnt = 0;

	data->trig = 1;
	data->int_en = 0;
	data->sd = 0;

	ret = veml6046_write_conf(dev);
	if (ret) {
		return ret;
	}

	ret = veml6046_read16(dev, VEML6046_CMDCODE_INT_L, &val);
	if (ret) {
		return ret;
	}

	k_sleep(K_USEC(veml60xx_it_values[data->itim].us));

	while (1) {
		ret = veml6046_read16(dev, VEML6046_CMDCODE_INT_L, &val);
		if (ret) {
			return ret;
		}

		if ((val >> 8) & VEML6046_AF_DATA_READY) {
			break;
		}

		if (cnt > 10) {
			return -EAGAIN;
		}

		k_sleep(K_USEC(veml60xx_it_values[data->itim].us / 10));

		cnt++;
	}

	LOG_DBG("read VEML6046_CMDCODE_INT_H: %02X (%d)", val >> 8, cnt);

	return 0;
}

static int veml6046_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret;

	/* Start sensor for new measurement */
	if (chan == SENSOR_CHAN_RED || chan == SENSOR_CHAN_GREEN ||
	    chan == SENSOR_CHAN_BLUE || chan == SENSOR_CHAN_IR ||
	    chan == SENSOR_CHAN_ALL) {
		ret = veml6046_perform_single_measurement(dev);
		if (ret < 0) {
			return ret;
		}

		return veml6046_fetch(dev);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int veml6046_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct veml6046_data *data = dev->data;

	switch ((int)chan) {
	case SENSOR_CHAN_RED:
		val->val1 = data->red_lux;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = data->green_lux;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = data->blue_lux;
		break;
	case SENSOR_CHAN_IR:
		val->val1 = data->ir_lux;
		break;
	case SENSOR_CHAN_VEML6046_RED_RAW_COUNTS:
		val->val1 = data->red_data;
		break;
	case SENSOR_CHAN_VEML6046_GREEN_RAW_COUNTS:
		val->val1 = data->green_data;
		break;
	case SENSOR_CHAN_VEML6046_BLUE_RAW_COUNTS:
		val->val1 = data->blue_data;
		break;
	case SENSOR_CHAN_VEML6046_IR_RAW_COUNTS:
		val->val1 = data->ir_data;
		break;
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int veml6046_set_shutdown_flag(const struct device *dev, uint8_t new_val)
{
	struct veml6046_data *data = dev->data;
	uint8_t prev_sd;
	int ret;

	prev_sd = data->sd;
	data->sd = new_val;

	ret = veml6046_write_conf(dev);
	if (ret < 0) {
		data->sd = prev_sd;
	}
	return ret;
}

static int veml6046_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return veml6046_set_shutdown_flag(dev, 1);

	case PM_DEVICE_ACTION_RESUME:
		return veml6046_set_shutdown_flag(dev, 0);

	default:
		return -ENOTSUP;
	}
	return 0;
}

#endif /* CONFIG_PM_DEVICE */

static int veml6046_init(const struct device *dev)
{
	const struct veml6046_config *conf = dev->config;
	int ret;
	uint16_t val;

	if (!i2c_is_ready_dt(&conf->bus)) {
		LOG_ERR("VEML device not ready");
		return -ENODEV;
	}

	ret = veml6046_read16(dev, VEML6046_CMDCODE_ID_L, &val);
	if (ret) {
		LOG_ERR("Error while reading ID. ret: %d", ret);
		return ret;
	}
	if ((val & 0x00FF) != VEML6046_DEFAULT_ID) {
		LOG_ERR("Device ID wrong: %d", val & 0x00FF);
		return -EIO;
	}

	LOG_DBG("veml6046 found package: %02d address: %02X version: %3s",
		val >> 14, val >> 12 & 0x03 ? 0x10 : 0x29,
		val >> 8 & 0x0F ? "XXX" : "A01");

	/* Initialize sensor configuration */
	ret = veml6046_write_thresh_low(dev);
	if (ret < 0) {
		LOG_ERR("Error while writing thresh low. ret: %d", ret);
		return ret;
	}

	ret = veml6046_write_thresh_high(dev);
	if (ret < 0) {
		LOG_ERR("Error while writing thresh high. ret: %d", ret);
		return ret;
	}

	ret = veml6046_write_conf(dev);
	if (ret < 0) {
		LOG_ERR("Error while writing config. ret: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, veml6046_api) = {
	.sample_fetch = veml6046_sample_fetch,
	.channel_get = veml6046_channel_get,
	.attr_set = veml6046_attr_set,
	.attr_get = veml6046_attr_get,
};

#define VEML6046_INIT(n)                                                                           \
	static struct veml6046_data veml6046_data_##n = {.trig = 0,                                \
							 .pdd = VEML6046_SIZE_2_2,                 \
							 .gain = VEML60XX_GAIN_1,                  \
							 .itim = VEML60XX_IT_100,                  \
							 .pers = VEML60XX_PERS_1,                  \
							 .thresh_high = 0xFFFF};                   \
                                                                                                   \
	static const struct veml6046_config veml6046_config_##n = {                                \
		.bus = I2C_DT_SPEC_INST_GET(n)};                                                   \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, veml6046_pm_action);                                           \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, veml6046_init, PM_DEVICE_DT_INST_GET(n),                   \
				     &veml6046_data_##n, &veml6046_config_##n, POST_KERNEL,        \
				     CONFIG_SENSOR_INIT_PRIORITY, &veml6046_api);

DT_INST_FOREACH_STATUS_OKAY(VEML6046_INIT)
