/*
 * Copyright (c) 2025, Borislav Kereziev
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bh1730

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

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
#define BH1730_GAIN_DEFAULT                           0x0
#define BH1730_ITIME_DEFAULT                          0xDA
#define BH1730_CONTROL_ADC_EN_POWER_ON_SINGLE_READING 0x0B
#define BH1730_CONTROL_ADC_EN_POWER_ON                0x03
#define BH1730_CONTROL_ADC_DATA_UPDATED               (1 << 4)
#define BH1730_TINT                                   2.8E-9 /* Internal clock period Tint */

#define BH1730_CMD           0x80 /* Command register CMD bit */
#define BH1730_CMD_ADDR_MASK 0x1F

/* Data definitions */

struct bh1730_data {
	uint16_t data0; /* visible light */
	uint16_t data1; /* infrared light */
};

struct bh1730_config {
	struct i2c_dt_spec i2c;
	uint8_t gain;
	uint8_t itime;
};

/* Prototypes */

static int bh1730_reg_read_8(const struct device *dev, uint8_t reg, uint8_t *val);
static int bh1730_reg_write_8(const struct device *dev, uint8_t reg, uint8_t val);
static int bh1730_data_read(const struct device *dev, uint16_t *data0, uint16_t *data1);
static uint32_t bh1730_get_integration_time_ms(uint8_t itime_reg);
static uint8_t bh1730_get_gain(uint8_t gain_reg_val);
static uint32_t bh1730_calculate_lux(uint16_t data0, uint16_t data1, uint8_t itime_reg,
				     uint8_t gain_reg);
static int bh1730_sample_fetch(const struct device *dev, enum sensor_channel chan);
static int bh1730_sample_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val);
static int bh1730_init(const struct device *dev);

/* Functions */

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

static inline uint32_t bh1730_get_integration_time_ms(uint8_t itime_reg)
{
	/* Convert register to integration time in ms */
	const uint32_t K_scaled = 2699200; /* 2.8e-6 * 964 * 1000 * 1e6 */
	uint32_t integration_time_ms;

	integration_time_ms = (K_scaled * (256 - itime_reg)) / 1000000;

	return integration_time_ms;
}

static inline uint8_t bh1730_get_gain(uint8_t gain_reg_val)
{
	/* Convert register value to gain */
	if (gain_reg_val == 0x0) {
		return 1;
	}
	if (gain_reg_val == 0x01) {
		return 2;
	}
	if (gain_reg_val == 0x02) {
		return 64;
	}
	if (gain_reg_val == 0x03) {
		return 128;
	}

	return 1;
}

static uint32_t bh1730_calculate_lux(uint16_t data0, uint16_t data1, uint8_t itime_reg,
				     uint8_t gain_reg)
{
	/* Check page 13 of datasheet for lux calculation formula */
	const uint32_t SCALE = 1000;
	const uint32_t scale1026 = 102600; /* 102.6 * 1000 */

	/* prevent division by zero */
	if (data0 == 0) {
		return 0;
	}

	uint32_t itime_ms = bh1730_get_integration_time_ms(itime_reg);
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
	uint64_t denominator = (uint64_t)gain * scale1026;

	if (numerator <= 0 || denominator == 0) {
		return 0;
	}

	uint32_t lux = (uint32_t)((numerator * itime_ms) / denominator);

	return lux;
}

static int bh1730_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bh1730_data *data = dev->data;
	const struct bh1730_config *cfg = dev->config;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_LIGHT)) {
		LOG_ERR("Unsupported channel %d", chan);
		return -ENOTSUP;
	}

	/* Trigger measurement */
	int ret = bh1730_reg_write_8(dev, BH1730_REG_CONTROL,
				     BH1730_CONTROL_ADC_EN_POWER_ON_SINGLE_READING);
	if (ret) {
		LOG_ERR("Failed writing to CONTROL register");
		return -EIO;
	}

	/* Wait for conversion */
	int32_t sleep_period_ms = bh1730_get_integration_time_ms(cfg->itime);

	k_msleep(sleep_period_ms);

	/* Read conversion result from device */
	ret = bh1730_data_read(dev, &data->data0, &data->data1);
	if (ret) {
		LOG_ERR("Failed reading data from sensor");
		return -EIO;
	}

	return 0;
}

static int bh1730_sample_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct bh1730_data *data = dev->data;
	const struct bh1730_config *cfg = dev->config;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	uint32_t lux = bh1730_calculate_lux(data->data0, data->data1, cfg->itime, cfg->gain);

	val->val1 = lux;
	val->val2 = 0;

	return 0;
}

static int bh1730_init(const struct device *dev)
{
	const struct bh1730_config *cfg = dev->config;
	int ret = 0;

	LOG_DBG("Initializing");

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C device not ready");
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

	/* Configure part with gain */
	if (cfg->gain != BH1730_GAIN_DEFAULT) {
		ret = bh1730_reg_write_8(dev, BH1730_REG_GAIN, cfg->gain);
		if (ret) {
			LOG_ERR("Failed writing to gain register");
			return -EIO;
		}
	}

	/* Configure part with itime */
	if (cfg->itime != BH1730_ITIME_DEFAULT) {
		ret = bh1730_reg_write_8(dev, BH1730_REG_TIMING, cfg->itime);
		if (ret) {
			LOG_ERR("Failed writing to ITIME register");
			return -EIO;
		}
	}

	return ret;
}

static DEVICE_API(sensor, bh1730_api) = {.sample_fetch = bh1730_sample_fetch,
					 .channel_get = bh1730_sample_get};

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
