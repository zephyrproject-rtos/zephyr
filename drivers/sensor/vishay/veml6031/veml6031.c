/*
 * Copyright (c) 2025 Andreas Klinger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_veml6031

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/sensor/veml6031.h>

LOG_MODULE_REGISTER(VEML6031, CONFIG_SENSOR_LOG_LEVEL);

/*
 * ID code of device
 */
#define VEML6031_DEFAULT_ID 0x01

/*
 * Bit mask to check for data ready in single measurement.
 */
#define VEML6031_ALS_AF_DATA_READY BIT(3)

/*
 * Maximum value of ALS data which also means that the sensor is in saturation
 * and that the measured value might be wrong.
 * In such a case the user program should reduce one or more of the following
 * attributes to get a relyable value:
 *   gain
 *   integration time
 *   effective photodiode size
 *
 */
#define VEML6031_ALS_DATA_OVERFLOW 0xFFFF

/*
 * 16-bit command register addresses
 */
#define VEML6031_CMDCODE_ALS_CONF_0 0x00
#define VEML6031_CMDCODE_ALS_CONF_1 0x01
#define VEML6031_CMDCODE_ALS_WH_L   0x04
#define VEML6031_CMDCODE_ALS_WH_H   0x05
#define VEML6031_CMDCODE_ALS_WL_L   0x06
#define VEML6031_CMDCODE_ALS_WL_H   0x07
#define VEML6031_CMDCODE_ALS_DATA_L 0x10
#define VEML6031_CMDCODE_ALS_DATA_H 0x11
#define VEML6031_CMDCODE_IR_DATA_L  0x12
#define VEML6031_CMDCODE_IR_DATA_H  0x13
#define VEML6031_CMDCODE_ID_L       0x14
#define VEML6031_CMDCODE_ID_H       0x15
#define VEML6031_CMDCODE_ALS_INT    0x17

/*
 * ALS integration time struct.
 */
struct veml6031_it_data {
	enum veml6031_it num;
	uint8_t val;
	int us;
};

/*
 * ALS integration time setting values.
 *
 * The enumerators of <tt>enum veml6031_it</tt> provide
 * indices into this array to get the related value for the
 * ALS_IT configuration bits.
 */
static const struct veml6031_it_data veml6031_it_values[] = {
	{VEML6031_IT_3_125, 0x00, 3125}, /*   3.125 - 0b0000 */
	{VEML6031_IT_6_25, 0x01, 6250},  /*   6.25  - 0b0001 */
	{VEML6031_IT_12_5, 0x02, 12500}, /*  12.5   - 0b0010 */
	{VEML6031_IT_25, 0x03, 25000},   /*  25     - 0b0011 */
	{VEML6031_IT_50, 0x04, 50000},   /*  50     - 0b0100 */
	{VEML6031_IT_100, 0x05, 100000}, /* 100     - 0b0101 */
	{VEML6031_IT_200, 0x06, 200000}, /* 200     - 0b0110 */
	{VEML6031_IT_400, 0x07, 400000}, /* 400     - 0b0111 */
};

/*
 * Resolution matrix for values to convert between data provided
 * by the sensor ("counts") and lux.
 *
 * These values depend on the current size, gain and integration time settings.
 * The enumerators of <tt>enum veml6031_div4</tt>, <tt>enum veml6031_gain</tt>
 * and <tt>enum veml6031_als_it</tt> are used for indices into this matrix.
 */
static const float
	veml6031_resolution[VEML6031_DIV4_COUNT][VEML6031_GAIN_COUNT][VEML6031_IT_COUNT] = {
		/*3.125ms   6.25ms   12.5ms     25ms     50ms    100ms    200ms     400ms IT */
		/* size 4/4 */
		{
			{0.8704f, 0.4352f, 0.2176f, 0.1088f, 0.0544f, 0.0272f, 0.0136f,
			 0.0068f}, /* Gain 1    */
			{0.4352f, 0.2176f, 0.1088f, 0.0544f, 0.0272f, 0.0136f, 0.0068f,
			 0.0034f}, /* Gain 2    */
			{1.3188f, 0.6504f, 0.3297f, 0.1648f, 0.0824f, 0.0412f, 0.0206f,
			 0.0103f}, /* Gain 0.66 */
			{1.7408f, 0.8704f, 0.4352f, 0.2176f, 0.1088f, 0.0544f, 0.0272f,
			 0.0136f}, /* Gain 0.5  */
		},
		{
			/* size 1/4 */
			{3.4816f, 1.7408f, 0.8704f, 0.4352f, 0.2176f, 0.1088f, 0.0544f,
			 0.0272f}, /* Gain 1    */
			{1.7408f, 0.8704f, 0.4352f, 0.2176f, 0.1088f, 0.0544f, 0.0272f,
			 0.0136f}, /* Gain 2    */
			{5.2752f, 2.6376f, 1.3188f, 0.6594f, 0.3297f, 0.1648f, 0.0824f,
			 0.0412f}, /* Gain 0.66 */
			{6.9632f, 3.4816f, 1.7408f, 0.8704f, 0.4352f, 0.2176f, 0.1088f,
			 0.0544f}, /* Gain 0.5  */
		},
};

struct veml6031_config {
	struct i2c_dt_spec bus;
};

struct veml6031_data {
	uint8_t sd;              /* Band gap and LDO shutdown */
	uint8_t int_en;          /* ALS interrupt enable */
	uint8_t trig;            /* ALS active force trigger */
	uint8_t af;              /* Active force mode */
	uint8_t ir_sd;           /* ALS and IR channel shutdown */
	uint8_t cal;             /* Power on ready */
	enum veml6031_div4 div4; /* effective photodiode size */
	enum veml6031_gain gain; /* gain selection */
	enum veml6031_it itim;   /* ALS integration time */
	enum veml6031_pers pers; /* ALS persistens protect */
	uint16_t thresh_high;
	uint16_t thresh_low;
	uint16_t als_data;
	uint32_t als_lux;
	uint16_t ir_data;
	uint32_t int_flags;
};

static void veml6031_sleep_by_integration_time(const struct veml6031_data *data)
{
	k_sleep(K_USEC(veml6031_it_values[data->itim].us));
}

static int veml6031_check_gain(const struct sensor_value *val)
{
	return val->val1 >= VEML6031_GAIN_1 && val->val1 <= VEML6031_GAIN_0_5;
}

static int veml6031_check_it(const struct sensor_value *val)
{
	return val->val1 >= VEML6031_IT_3_125 && val->val1 <= VEML6031_IT_400;
}

static int veml6031_check_div4(const struct sensor_value *val)
{
	return val->val1 >= VEML6031_SIZE_4_4 && val->val1 <= VEML6031_SIZE_1_4;
}

static int veml6031_check_pers(const struct sensor_value *val)
{
	return val->val1 >= VEML6031_PERS_1 && val->val1 <= VEML6031_PERS_8;
}

static int veml6031_read(const struct device *dev, uint8_t cmd, uint8_t *data)
{
	const struct veml6031_config *conf = dev->config;

	uint8_t recv_buf;
	int ret;

	ret = i2c_reg_read_byte_dt(&conf->bus, cmd, &recv_buf);
	if (ret < 0) {
		return ret;
	}

	*data = recv_buf;

	return 0;
}

static int veml6031_read16(const struct device *dev, uint8_t cmd, uint8_t *data)
{
	const struct veml6031_config *conf = dev->config;
	int ret;

	ret = i2c_burst_read_dt(&conf->bus, cmd, data, 2);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int veml6031_write16(const struct device *dev, uint8_t cmd, uint8_t *data)
{
	const struct veml6031_config *conf = dev->config;

	return i2c_burst_write_dt(&conf->bus, cmd, data, 2);
}

static int veml6031_write_conf(const struct device *dev)
{
	int ret;
	struct veml6031_data *data = dev->data;
	uint8_t conf[2] = {0, 0};

	/* Bits 7 -> ALS and IR channel shutdown */
	conf[1] |= data->ir_sd << 7;
	/* Bits 6 -> Effective photodiode size */
	conf[1] |= data->div4 << 6;
	/* Bit 5 -> reserved */
	/* Bits 4:3 -> Gain selection */
	conf[1] |= data->gain << 3;
	/* Bits 2:1 -> ALS persistence protect number */
	conf[1] |= data->pers << 1;
	/* Bit 0 -> Power on ready */
	if (data->cal) {
		conf[1] |= BIT(0);
	}
	/* Bit 7 -> reserved, have to be 0 */
	/* Bits 6:4 -> integration time (ALS_IT) */
	conf[0] |= data->itim << 4;
	/* Bit 3 -> Active force mode enable */
	if (data->af) {
		conf[0] |= BIT(3);
	}
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

	ret = veml6031_write16(dev, VEML6031_CMDCODE_ALS_CONF_0, conf);
	if (ret) {
		LOG_ERR("Error while writing conf[0] ret: %d", ret);
		return ret;
	}

	return 0;
}

static int veml6031_write_thresh_high(const struct device *dev)
{
	int ret;
	const struct veml6031_data *data = dev->data;
	uint8_t val[2];

	val[0] = data->thresh_high & 0xFF;
	val[1] = data->thresh_high >> 8;

	LOG_DBG("Writing high threshold counts: %d", data->thresh_high);
	ret = veml6031_write16(dev, VEML6031_CMDCODE_ALS_WH_L, val);
	if (ret) {
		return ret;
	}

	return 0;
}

static int veml6031_write_thresh_low(const struct device *dev)
{
	int ret;
	const struct veml6031_data *data = dev->data;
	uint8_t val[2];

	val[0] = data->thresh_low & 0xFF;
	val[1] = data->thresh_low >> 8;

	LOG_DBG("Writing low threshold counts: %d", data->thresh_low);
	ret = veml6031_write16(dev, VEML6031_CMDCODE_ALS_WL_L, val);
	if (ret) {
		return ret;
	}

	return 0;
}

static int veml6031_fetch(const struct device *dev)
{
	struct veml6031_data *data = dev->data;
	int ret;

	ret = veml6031_read16(dev, VEML6031_CMDCODE_ALS_DATA_L, (uint8_t *)&data->als_data);
	if (ret < 0) {
		return ret;
	}
	data->als_data = sys_le16_to_cpu(data->als_data);

	ret = veml6031_read16(dev, VEML6031_CMDCODE_IR_DATA_L, (uint8_t *)&data->ir_data);
	if (ret < 0) {
		return ret;
	}
	data->ir_data = sys_le16_to_cpu(data->ir_data);

	data->als_lux = data->als_data * veml6031_resolution[data->div4][data->gain][data->itim];

	LOG_DBG("Read ALS measurement: counts=%d, lux=%d ir=%d", data->als_data, data->als_lux,
		data->ir_data);

	if (data->als_data == VEML6031_ALS_DATA_OVERFLOW) {
		return -E2BIG;
	}

	return 0;
}

static int veml6031_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct veml6031_data *data = dev->data;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/* SENSOR_ATTR_.*_THRESH are not in enum sensor_attribute_veml6031 */
	switch ((int)attr) {
	case SENSOR_ATTR_VEML6031_IT:
		if (veml6031_check_it(val)) {
			data->itim = (enum veml6031_it)val->val1;
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_VEML6031_DIV4:
		if (veml6031_check_div4(val)) {
			data->div4 = (enum veml6031_div4)val->val1;
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_VEML6031_GAIN:
		if (veml6031_check_gain(val)) {
			data->gain = (enum veml6031_gain)val->val1;
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_VEML6031_PERS:
		if (veml6031_check_pers(val)) {
			data->pers = (enum veml6031_pers)val->val1;
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		data->thresh_low =
			val->val1 / veml6031_resolution[data->div4][data->gain][data->itim];
		return veml6031_write_thresh_low(dev);
	case SENSOR_ATTR_UPPER_THRESH:
		data->thresh_high =
			val->val1 / veml6031_resolution[data->div4][data->gain][data->itim];
		return veml6031_write_thresh_high(dev);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int veml6031_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct veml6031_data *data = dev->data;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/* SENSOR_ATTR_.*_THRESH are not in enum sensor_attribute_veml6031 */
	switch ((int)attr) {
	case SENSOR_ATTR_VEML6031_IT:
		val->val1 = data->itim;
		break;
	case SENSOR_ATTR_VEML6031_DIV4:
		val->val1 = data->div4;
		break;
	case SENSOR_ATTR_VEML6031_GAIN:
		val->val1 = data->gain;
		break;
	case SENSOR_ATTR_VEML6031_PERS:
		val->val1 = data->pers;
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		val->val1 = data->thresh_low;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		val->val1 = data->thresh_high;
		break;
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

static int veml6031_perform_single_measurement(const struct device *dev)
{
	struct veml6031_data *data = dev->data;
	int ret;
	uint8_t val;
	int cnt = 0;

	data->ir_sd = 0;
	data->cal = 1;
	data->af = 1;
	data->trig = 1;
	data->int_en = 0;
	data->sd = 0;

	ret = veml6031_write_conf(dev);
	if (ret) {
		return ret;
	}

	ret = veml6031_read(dev, VEML6031_CMDCODE_ALS_INT, &val);

	veml6031_sleep_by_integration_time(data);

	while (1) {
		ret = veml6031_read(dev, VEML6031_CMDCODE_ALS_INT, &val);
		if (ret) {
			return ret;
		}

		if (val & VEML6031_ALS_AF_DATA_READY) {
			break;
		}

		k_sleep(K_MSEC(1));

		cnt++;
	}

	LOG_DBG("read VEML6031_CMDCODE_ALS_INT: %02X (%d)", val, cnt);

	return 0;
}

static int veml6031_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret;

	/* Start sensor for new measurement */
	if (chan == SENSOR_CHAN_LIGHT || chan == SENSOR_CHAN_ALL) {
		ret = veml6031_perform_single_measurement(dev);
		if (ret < 0) {
			return ret;
		}

		return veml6031_fetch(dev);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int veml6031_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct veml6031_data *data = dev->data;

	switch ((int)chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = data->als_lux;
		break;
	case SENSOR_CHAN_VEML6031_ALS_RAW_COUNTS:
		val->val1 = data->als_data;
		break;
	case SENSOR_CHAN_VEML6031_IR_RAW_COUNTS:
		val->val1 = data->ir_data;
		break;
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int veml6031_set_shutdown_flag(const struct device *dev, uint8_t new_val)
{
	struct veml6031_data *data = dev->data;
	uint8_t prev_sd;
	uint8_t prev_ir_sd;
	int ret;

	prev_sd = data->sd;
	prev_ir_sd = data->ir_sd;
	data->sd = new_val;
	data->ir_sd = new_val;

	ret = veml6031_write_conf(dev);
	if (ret < 0) {
		data->ir_sd = prev_ir_sd;
		data->sd = prev_sd;
	}
	return ret;
}

static int veml6031_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct veml6031_data *data = dev->data;

	if (!data->sd) {
		switch (action) {
		case PM_DEVICE_ACTION_SUSPEND:
			return veml6031_set_shutdown_flag(dev, 1);

		case PM_DEVICE_ACTION_RESUME:
			return veml6031_set_shutdown_flag(dev, 0);

		default:
			return -ENOTSUP;
		}
	}

	return 0;
}

#endif /* CONFIG_PM_DEVICE */

static int veml6031_init(const struct device *dev)
{
	const struct veml6031_config *conf = dev->config;
	int ret;
	uint8_t val8;

	if (!i2c_is_ready_dt(&conf->bus)) {
		LOG_ERR("VEML device not ready");
		return -ENODEV;
	}

	ret = veml6031_read(dev, VEML6031_CMDCODE_ID_L, &val8);
	if (ret < 0) {
		LOG_ERR("Error while reading ID low. ret: %d", ret);
		return ret;
	}
	if (val8 != VEML6031_DEFAULT_ID) {
		LOG_ERR("Device ID wrong: %d", val8);
		return -EIO;
	}
	ret = veml6031_read(dev, VEML6031_CMDCODE_ID_H, &val8);
	if (ret < 0) {
		LOG_ERR("Error while reading ID high. ret: %d", ret);
		return ret;
	}
	LOG_DBG("veml6031 found package: %02d address: %02X version: %3s", val8 >> 6,
		val8 >> 4 & 0x03 ? 0x10 : 0x29, val8 & 0x0F ? "XXX" : "A01");

	/* Initialize sensor configuration */
	ret = veml6031_write_thresh_low(dev);
	if (ret < 0) {
		LOG_ERR("Error while writing thresh low. ret: %d", ret);
		return ret;
	}

	ret = veml6031_write_thresh_high(dev);
	if (ret < 0) {
		LOG_ERR("Error while writing thresh high. ret: %d", ret);
		return ret;
	}

	ret = veml6031_write_conf(dev);
	if (ret < 0) {
		LOG_ERR("Error while writing conf. ret: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, veml6031_api) = {
	.sample_fetch = veml6031_sample_fetch,
	.channel_get = veml6031_channel_get,
	.attr_set = veml6031_attr_set,
	.attr_get = veml6031_attr_get,
};

#define VEML6031_INIT(n)                                                                           \
	static struct veml6031_data veml6031_data_##n = {.trig = 1,                                \
							 .af = 1,                                  \
							 .div4 = VEML6031_SIZE_4_4,                \
							 .gain = VEML6031_GAIN_1,                  \
							 .itim = VEML6031_IT_100,                  \
							 .pers = VEML6031_PERS_1,                  \
							 .thresh_high = 0xFFFF};                   \
                                                                                                   \
	static const struct veml6031_config veml6031_config_##n = {                                \
		.bus = I2C_DT_SPEC_INST_GET(n)};                                                   \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, veml6031_pm_action);                                           \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, veml6031_init, PM_DEVICE_DT_INST_GET(n),                   \
				     &veml6031_data_##n, &veml6031_config_##n, POST_KERNEL,        \
				     CONFIG_SENSOR_INIT_PRIORITY, &veml6031_api);

DT_INST_FOREACH_STATUS_OKAY(VEML6031_INIT)
