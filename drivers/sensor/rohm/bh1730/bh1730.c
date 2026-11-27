/*
 * Copyright (c) 2025, Borislav Kereziev
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bh1730

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/sensor/bh1730.h>

LOG_MODULE_REGISTER(BH1730, CONFIG_SENSOR_LOG_LEVEL);

#define BH1730_REG_CONTROL   0x00
#define BH1730_REG_TIMING    0x01
#define BH1730_REG_GAIN      0x07
#define BH1730_REG_ID        0x12
#define BH1730_REG_DATA0LOW  0x14
#define BH1730_REG_DATA0HIGH 0x15
#define BH1730_REG_DATA1LOW  0x16
#define BH1730_REG_DATA1HIGH 0x17

#define BH1730_PART_ID                                0x71
#define BH1730_CONTROL_ADC_EN_POWER_ON_SINGLE_READING 0x0B
#define BH1730_CONTROL_ADC_EN_POWER_ON                0x03
#define BH1730_CONTROL_ADC_DATA_UPDATED               (1 << 4)

#define BH1730_CMD           0x80 /* Command register CMD bit */
#define BH1730_CMD_ADDR_MASK 0x1F

#define BH1730_ITIME_MIN_MS 3U   /* ITIME = 255 → 2.7 ms */
#define BH1730_ITIME_MAX_MS 688U /* ITIME = 1   → 688 ms (0 -> manual integration mode) */

#define BH1730_ITIME_STEP_US 2699U /* integration time per ITIME step (typ.) */

#define BH1730_GAIN_1   0x00
#define BH1730_GAIN_2   0x01
#define BH1730_GAIN_64  0x02
#define BH1730_GAIN_128 0x03

#define BH1730_GAIN_VAL_1   1
#define BH1730_GAIN_VAL_2   2
#define BH1730_GAIN_VAL_64  64
#define BH1730_GAIN_VAL_128 128

/* Data Definitions */

struct bh1730_data {
	struct k_mutex lock;
	uint16_t data0;     /* visible light */
	uint16_t data1;     /* infrared light */
	uint8_t itime;      /* current itime setting */
	uint8_t gain;       /* current gain setting */
	uint8_t meas_itime; /* itime used for data0/data1 */
	uint8_t meas_gain;  /* gain used for data0/data1 */
};

struct bh1730_config {
	struct i2c_dt_spec i2c;
	uint8_t itime; /* initial value from DT */
	uint8_t gain;  /* initial value from DT */
};

/* Prototypes */

static int bh1730_reg_read_8(const struct device *dev, uint8_t reg, uint8_t *val);
static int bh1730_reg_write_8(const struct device *dev, uint8_t reg, uint8_t val);
static int bh1730_data_read(const struct device *dev, uint16_t *data0, uint16_t *data1);

static uint32_t bh1730_get_measurement_time_ms(uint8_t itime_reg);
static uint8_t bh1730_get_gain(uint8_t gain_reg_val);
static uint32_t bh1730_get_integration_time_ms(uint8_t itime_reg);

static int bh1730_set_gain(const struct device *dev, int32_t gain);
static int bh1730_set_integration_time_ms(const struct device *dev, const struct sensor_value *val);

static int bh1730_sample_fetch(const struct device *dev, enum sensor_channel chan);
static int bh1730_init(const struct device *dev);
static int bh1730_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val);

static uint32_t bh1730_calculate_lux(uint16_t data0, uint16_t data1, uint8_t itime_reg,
				     uint8_t gain_reg);

/* Helpers */

static int bh1730_reg_read_8(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct bh1730_config *cfg = dev->config;
	uint8_t cmd = BH1730_CMD | (reg & BH1730_CMD_ADDR_MASK);

	return i2c_write_read_dt(&cfg->i2c, &cmd, sizeof(cmd), val, sizeof(*val));
}

static int bh1730_reg_write_8(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct bh1730_config *cfg = dev->config;
	uint8_t cmd = BH1730_CMD | (reg & BH1730_CMD_ADDR_MASK);
	uint8_t buf[2] = {cmd, val};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int bh1730_data_read(const struct device *dev, uint16_t *data0, uint16_t *data1)
{
	const struct bh1730_config *cfg = dev->config;

	/* Ensure data has been updated */
	uint8_t control_reg = 0x0;
	int err = bh1730_reg_read_8(dev, BH1730_REG_CONTROL, &control_reg);

	if (err) {
		LOG_ERR("Failed reading CONTROL register");
		return -EIO;
	}
	if ((control_reg & BH1730_CONTROL_ADC_DATA_UPDATED) == 0) {
		LOG_ERR("Data not updated");
		return -ENODATA;
	}

	/* Read data0 and data1 bytes */
	uint8_t buffer[4] = {0};
	uint8_t cmd = BH1730_CMD | BH1730_REG_DATA0LOW;

	err = i2c_write_read_dt(&cfg->i2c, &cmd, sizeof(cmd), &buffer, sizeof(buffer));
	if (err) {
		return -EIO;
	}

	/* Data is shifted LSB first from sensor - swap */
	*data0 = (buffer[1] << 8 | buffer[0]);
	*data1 = (buffer[3] << 8 | buffer[2]);

	return 0;
}

static inline uint32_t bh1730_get_measurement_time_ms(uint8_t itime_reg)
{
	/*
	 * Datasheet: Tmt = Tint * (964 * (256 - ITIME) + 714),
	 * Use max Tint (4 us) from the datasheet so the conversion is complete before reading.
	 */

	const uint32_t tint_max_us = 4U;
	uint32_t cycles = 964U * (256U - itime_reg) + 714U;

	return DIV_ROUND_UP(cycles * tint_max_us, 1000U);
}

static inline uint32_t bh1730_get_integration_time_ms(uint8_t itime_reg)
{
	/*
	 * Typical integration time: Tint * 964 * (256 - ITIME), Tint = 2.8 us,
	 * rounded to the nearest millisecond.
	 */

	return DIV_ROUND_CLOSEST(BH1730_ITIME_STEP_US * (256U - itime_reg), 1000U);
}

static int bh1730_set_integration_time_ms(const struct device *dev, const struct sensor_value *val)
{
	uint32_t integration_time_ms = val->val1;
	struct bh1730_data *data = dev->data;
	uint8_t itime_reg;
	int ret;

	if ((integration_time_ms < BH1730_ITIME_MIN_MS) ||
	    (integration_time_ms > BH1730_ITIME_MAX_MS)) {
		LOG_ERR("Integration time %u ms out of range (%u..%u)", integration_time_ms,
			BH1730_ITIME_MIN_MS, BH1730_ITIME_MAX_MS);
		return -EINVAL;
	}

	/* Convert integration time in ms to register */
	uint32_t steps = DIV_ROUND_CLOSEST(integration_time_ms * 1000U, BH1730_ITIME_STEP_US);

	itime_reg = (uint8_t)(256U - steps);

	ret = bh1730_reg_write_8(dev, BH1730_REG_TIMING, itime_reg);
	if (ret < 0) {
		LOG_ERR("Failed writing to ITIME register");
		return ret;
	}

	data->itime = itime_reg;

	return 0;
}

static inline uint8_t bh1730_get_gain(uint8_t gain_reg_val)
{
	switch (gain_reg_val) {
	case BH1730_GAIN_1:
		return BH1730_GAIN_VAL_1;
	case BH1730_GAIN_2:
		return BH1730_GAIN_VAL_2;
	case BH1730_GAIN_64:
		return BH1730_GAIN_VAL_64;
	case BH1730_GAIN_128:
		return BH1730_GAIN_VAL_128;
	default:
		return BH1730_GAIN_VAL_1;
	}
}

static int bh1730_set_gain(const struct device *dev, int32_t gain)
{
	struct bh1730_data *data = dev->data;
	uint8_t gain_reg_val;
	int ret;

	switch (gain) {
	case BH1730_GAIN_VAL_1:
		gain_reg_val = BH1730_GAIN_1;
		break;
	case BH1730_GAIN_VAL_2:
		gain_reg_val = BH1730_GAIN_2;
		break;
	case BH1730_GAIN_VAL_64:
		gain_reg_val = BH1730_GAIN_64;
		break;
	case BH1730_GAIN_VAL_128:
		gain_reg_val = BH1730_GAIN_128;
		break;
	default:
		LOG_ERR("Invalid gain %d (allowed: 1, 2, 64, 128)", gain);
		return -EINVAL;
	}

	ret = bh1730_reg_write_8(dev, BH1730_REG_GAIN, gain_reg_val);
	if (ret < 0) {
		LOG_ERR("Failed writing to gain register");
		return ret;
	}

	data->gain = gain_reg_val;

	return 0;
}

static uint32_t bh1730_calculate_lux(uint16_t data0, uint16_t data1, uint8_t itime_reg,
				     uint8_t gain_reg)
{
	/*
	 * Check page 13 of datasheet for lux calculation formula:
	 * Lx = (k0 * DATA0 - k1 * DATA1) / Gain * 102.6 / ITIME_ms
	 * with ITIME_ms = Tint * 964 * (256 - ITIME) ~= 2.7 ms * (256 - ITIME).
	 * Since 102.6 / 2.7 = 38, this is exact in integer math as
	 * Lx = (k0 * DATA0 - k1 * DATA1) * 38 / (Gain * (256 - ITIME))
	 */
	const uint32_t SCALE = 1000;
	const uint32_t LUX_FACTOR = 38;

	/* prevent division by zero */
	if (data0 == 0) {
		return 0;
	}

	uint8_t gain = bh1730_get_gain(gain_reg);

	/* Compute ratio = (DATA1 / DATA0) scaled by 1000 */
	uint32_t ratio = ((uint32_t)data1 * SCALE) / data0;
	uint32_t k0_scaled, k1_scaled;

	if (ratio < 260) {
		k0_scaled = 1290;
		k1_scaled = 2733;
	} else if (ratio < 550) {
		k0_scaled = 795;
		k1_scaled = 859;
	} else if (ratio < 1090) {
		k0_scaled = 510;
		k1_scaled = 345;
	} else if (ratio < 2130) {
		k0_scaled = 276;
		k1_scaled = 130;
	} else {
		return 0;
	}

	/* Compute numerator (scaled values, may be negative → use int64_t) */
	int64_t numerator = (int64_t)data0 * k0_scaled - (int64_t)data1 * k1_scaled;
	uint64_t denominator = (uint64_t)SCALE * gain * (256U - itime_reg);

	if (numerator <= 0) {
		return 0;
	}

	uint32_t lux = (uint32_t)(((uint64_t)numerator * LUX_FACTOR) / denominator);

	return lux;
}

/* API Functions */

static int bh1730_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bh1730_data *data = dev->data;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_LIGHT) &&
	    (chan != SENSOR_CHAN_AMBIENT_LIGHT) && (chan != SENSOR_CHAN_IR)) {
		LOG_ERR("Unsupported channel %d", chan);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Trigger measurement */
	ret = bh1730_reg_write_8(dev, BH1730_REG_CONTROL,
				 BH1730_CONTROL_ADC_EN_POWER_ON_SINGLE_READING);
	if (ret < 0) {
		LOG_ERR("Failed writing to CONTROL register");
		goto unlock;
	}

	/* Wait for conversion */
	k_msleep(bh1730_get_measurement_time_ms(data->itime));

	/* Read conversion result from device */
	ret = bh1730_data_read(dev, &data->data0, &data->data1);
	if (ret == 0) {
		/* Remember the settings this sample was taken with */
		data->meas_itime = data->itime;
		data->meas_gain = data->gain;
	}

unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int bh1730_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bh1730_data *data = dev->data;
	uint32_t lux = 0;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_LIGHT:
		lux = bh1730_calculate_lux(data->data0, data->data1, data->meas_itime,
					   data->meas_gain);
		val->val1 = lux;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_AMBIENT_LIGHT:
		val->val1 = data->data0;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_IR:
		val->val1 = data->data1;
		val->val2 = 0;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int bh1730_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	const struct bh1730_data *data = dev->data;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_LIGHT) &&
	    (chan != SENSOR_CHAN_AMBIENT_LIGHT) && (chan != SENSOR_CHAN_IR)) {
		LOG_ERR("Unsupported channel %d", chan);
		return -ENOTSUP;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_BH1730_SENSOR_INTEGRATION_TIME:
		val->val1 = bh1730_get_integration_time_ms(data->itime);
		break;
	case SENSOR_ATTR_GAIN:
		val->val1 = bh1730_get_gain(data->gain);
		break;
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

static int bh1730_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	struct bh1730_data *data = dev->data;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_LIGHT) &&
	    (chan != SENSOR_CHAN_AMBIENT_LIGHT) && (chan != SENSOR_CHAN_IR)) {
		LOG_ERR("Unsupported channel %d", chan);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (attr == SENSOR_ATTR_GAIN) {
		ret = bh1730_set_gain(dev, val->val1);
	} else if ((int)attr == SENSOR_ATTR_BH1730_SENSOR_INTEGRATION_TIME) {
		ret = bh1730_set_integration_time_ms(dev, val);
	} else {
		ret = -ENOTSUP;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/* Driver Initialization */

static int bh1730_init(const struct device *dev)
{
	const struct bh1730_config *cfg = dev->config;
	struct bh1730_data *data = dev->data;
	int ret = 0;

	k_mutex_init(&data->lock);

	LOG_DBG("Initializing");

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	/* Ensure BH1730 has valid ID */
	uint8_t val = 0;

	ret = bh1730_reg_read_8(dev, BH1730_REG_ID, &val);
	if (ret != 0) {
		LOG_ERR("Failed reading ID reg");
		return -ENODEV;
	}

	/* Part number in bits 7:4 */
	if ((val >> 4) != (BH1730_PART_ID >> 4)) {
		LOG_ERR("Part number does not match, received 0x%1X", val >> 4);
		return -ENODEV;
	}

	/* Start values from devicetree; runtime changes go through the setters */
	data->gain = cfg->gain;
	data->itime = cfg->itime;

	data->meas_gain = cfg->gain;
	data->meas_itime = cfg->itime;

	/* Always write, chip may hold old runtime values after an MCU reset */
	ret = bh1730_reg_write_8(dev, BH1730_REG_GAIN, data->gain);
	if (ret != 0) {
		LOG_ERR("Failed writing to gain register");
		return -EIO;
	}

	ret = bh1730_reg_write_8(dev, BH1730_REG_TIMING, data->itime);
	if (ret != 0) {
		LOG_ERR("Failed writing to ITIME register");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(sensor, bh1730_api) = {
	.sample_fetch = bh1730_sample_fetch,
	.channel_get = bh1730_channel_get,
	.attr_get = bh1730_attr_get,
	.attr_set = bh1730_attr_set,
};

#define BH1730_DEFINE(inst)                                                                        \
	static struct bh1730_data bh1730_data_##inst;                                              \
	static const struct bh1730_config bh1730_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.gain = DT_INST_PROP(inst, gain),                                                  \
		.itime = DT_INST_PROP(inst, itime),                                                \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bh1730_init, NULL, &bh1730_data_##inst,                 \
				     &bh1730_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &bh1730_api);

DT_INST_FOREACH_STATUS_OKAY(BH1730_DEFINE)
