/*
 * Copyright (c) 2023, Gustavo Silva
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_tsl2561

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(TSL2561, CONFIG_SENSOR_LOG_LEVEL);

#define TSL2561_CHIP_ID 0x05

#define TSL2561_GAIN_1X  0x00
#define TSL2561_GAIN_16X 0x01

#define TSL2561_INTEGRATION_13MS  0x00
#define TSL2561_INTEGRATION_101MS 0x01
#define TSL2561_INTEGRATION_402MS 0x02

/* Register set */
#define TSL2561_REG_CONTROL        0x00
#define TSL2561_REG_TIMING         0x01
#define TSL2561_REG_THRESHLOWLOW   0x02
#define TSL2561_REG_THRESHLOWHIGH  0x03
#define TSL2561_REG_THRESHHIGHLOW  0x04
#define TSL2561_REG_THRESHHIGHHIGH 0x05
#define TSL2561_REG_INTERRUPT      0x06
#define TSL2561_REG_ID             0x0A
#define TSL2561_REG_DATA0LOW       0x0C
#define TSL2561_REG_DATA0HIGH      0x0D
#define TSL2561_REG_DATA1LOW       0x0E
#define TSL2561_REG_DATA1HIGH      0x0F

/* Command register fields */
#define TSL2561_COMMAND_CMD  BIT(7)
#define TSL2561_COMMAND_WORD BIT(5)

/* Control register fields */
#define TSL2561_CONTROL_POWER_UP   0x03
#define TSL2561_CONTROL_POWER_DOWN 0x00

/* Timing register fields */
#define TSL2561_TIMING_GAIN  BIT(4)
#define TSL2561_TIMING_INTEG GENMASK(1, 0)

/* ID register part number mask */
#define TSL2561_ID_PARTNO GENMASK(7, 4)

/* Lux calculation constants */
#define TSL2561_LUX_SCALE     14U
#define TSL2561_RATIO_SCALE   9U
#define TSL2561_CH_SCALE      10U
#define TSL2561_CHSCALE_TINT0 0x7517
#define TSL2561_CHSCALE_TINT1 0x0FE7

#define TSL2561_LUX_K1T 0X0040 /* 0.125   * 2^RATIO_SCALE */
#define TSL2561_LUX_B1T 0X01F2 /* 0.0304  * 2^LUX_SCALE   */
#define TSL2561_LUX_M1T 0X01BE /* 0.0272  * 2^LUX_SCALE   */
#define TSL2561_LUX_K2T 0X0080 /* 0.250   * 2^RATIO_SCALE */
#define TSL2561_LUX_B2T 0X0214 /* 0.0325  * 2^LUX_SCALE   */
#define TSL2561_LUX_M2T 0X02D1 /* 0.0440  * 2^LUX_SCALE   */
#define TSL2561_LUX_K3T 0X00C0 /* 0.375   * 2^RATIO_SCALE */
#define TSL2561_LUX_B3T 0X023F /* 0.0351  * 2^LUX_SCALE   */
#define TSL2561_LUX_M3T 0X037B /* 0.0544  * 2^LUX_SCALE   */
#define TSL2561_LUX_K4T 0X0100 /* 0.50    * 2^RATIO_SCALE */
#define TSL2561_LUX_B4T 0X0270 /* 0.0381  * 2^LUX_SCALE   */
#define TSL2561_LUX_M4T 0X03FE /* 0.0624  * 2^LUX_SCALE   */
#define TSL2561_LUX_K5T 0X0138 /* 0.61    * 2^RATIO_SCALE */
#define TSL2561_LUX_B5T 0X016F /* 0.0224  * 2^LUX_SCALE   */
#define TSL2561_LUX_M5T 0X01FC /* 0.0310  * 2^LUX_SCALE   */
#define TSL2561_LUX_K6T 0X019A /* 0.80    * 2^RATIO_SCALE */
#define TSL2561_LUX_B6T 0X00D2 /* 0.0128  * 2^LUX_SCALE   */
#define TSL2561_LUX_M6T 0X00FB /* 0.0153  * 2^LUX_SCALE   */
#define TSL2561_LUX_K7T 0X029A /* 1.3     * 2^RATIO_SCALE */
#define TSL2561_LUX_B7T 0X0018 /* 0.00146 * 2^LUX_SCALE   */
#define TSL2561_LUX_M7T 0X0012 /* 0.00112 * 2^LUX_SCALE   */
#define TSL2561_LUX_K8T 0X029A /* 1.3     * 2^RATIO_SCALE */
#define TSL2561_LUX_B8T 0X0000 /* 0.000   * 2^LUX_SCALE   */
#define TSL2561_LUX_M8T 0X0000 /* 0.000   * 2^LUX_SCALE   */

struct tsl2561_config {
	struct i2c_dt_spec i2c;
	uint16_t integration_time;
	uint8_t gain;
};

struct tsl2561_data {
	uint16_t ch0;
	uint16_t ch1;
	uint32_t ch_scale;
};

static int tsl2561_reg_read(const struct device *dev, uint8_t reg, uint8_t *buf, uint8_t size)
{
	int ret;
	const struct tsl2561_config *config = dev->config;
	uint8_t cmd = (TSL2561_COMMAND_CMD | TSL2561_COMMAND_WORD | reg);

	ret = i2c_write_read_dt(&config->i2c, &cmd, 1U, buf, size);
	if (ret < 0) {
		LOG_ERR("Failed reading register 0x%02x", reg);
		return ret;
	}

	return 0;
}

static int tsl2561_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	int ret;
	const struct tsl2561_config *config = dev->config;
	uint8_t buf[2];

	buf[0] = (TSL2561_COMMAND_CMD | TSL2561_COMMAND_WORD | reg);
	buf[1] = val;

	ret = i2c_write_dt(&config->i2c, buf, 2U);
	if (ret < 0) {
		LOG_ERR("Failed writing register 0x%02x", reg);
		return ret;
	}

	return 0;
}

static int tsl2561_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tsl2561_config *config = dev->config;
	struct tsl2561_data *data = dev->data;
	uint8_t bytes[2];
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_LIGHT) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	ret = tsl2561_reg_write(dev, TSL2561_REG_CONTROL, TSL2561_CONTROL_POWER_UP);
	if (ret < 0) {
		LOG_ERR("Failed to power up device");
		return ret;
	}

	/* Short sleep after power up. Not in the datasheet, but found by trial and error */
	k_msleep(5);

	k_msleep(config->integration_time);

	/* Read data register's lower and upper bytes consecutively */
	ret = tsl2561_reg_read(dev, TSL2561_REG_DATA0LOW, bytes, 2U);
	if (ret < 0) {
		LOG_ERR("Failed reading channel0 data");
		return ret;
	}
	data->ch0 = bytes[1] << 8 | bytes[0];

	ret = tsl2561_reg_read(dev, TSL2561_REG_DATA1LOW, bytes, 2U);
	if (ret < 0) {
		LOG_ERR("Failed reading channel1 data");
		return ret;
	}
	data->ch1 = bytes[1] << 8 | bytes[0];

	ret = tsl2561_reg_write(dev, TSL2561_REG_CONTROL, TSL2561_CONTROL_POWER_DOWN);
	if (ret < 0) {
		LOG_ERR("Failed to power down device");
		return ret;
	}

	LOG_DBG("channel0: 0x%x; channel1: 0x%x", data->ch0, data->ch1);

	return 0;
}

static int tsl2561_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct tsl2561_data *data = dev->data;
	uint32_t channel0;
	uint32_t channel1;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	channel0 = (data->ch0 * data->ch_scale) >> TSL2561_CH_SCALE;
	channel1 = (data->ch1 * data->ch_scale) >> TSL2561_CH_SCALE;

	uint32_t ratio1 = 0;

	if (channel0 != 0) {
		ratio1 = (channel1 << (TSL2561_RATIO_SCALE + 1)) / channel0;
	}

	/* Round the ratio value */
	uint32_t ratio = (ratio1 + 1) >> 1;

	uint32_t b = 0;
	uint32_t m = 0;

	if (ratio <= TSL2561_LUX_K1T) {
		b = TSL2561_LUX_B1T;
		m = TSL2561_LUX_M1T;
	} else if (ratio <= TSL2561_LUX_K2T) {
		b = TSL2561_LUX_B2T;
		m = TSL2561_LUX_M2T;
	} else if (ratio <= TSL2561_LUX_K3T) {
		b = TSL2561_LUX_B3T;
		m = TSL2561_LUX_M3T;
	} else if (ratio <= TSL2561_LUX_K4T) {
		b = TSL2561_LUX_B4T;
		m = TSL2561_LUX_M4T;
	} else if (ratio <= TSL2561_LUX_K5T) {
		b = TSL2561_LUX_B5T;
		m = TSL2561_LUX_M5T;
	} else if (ratio <= TSL2561_LUX_K6T) {
		b = TSL2561_LUX_B6T;
		m = TSL2561_LUX_M6T;
	} else if (ratio <= TSL2561_LUX_K7T) {
		b = TSL2561_LUX_B7T;
		m = TSL2561_LUX_M7T;
	} else if (ratio > TSL2561_LUX_K8T) {
		b = TSL2561_LUX_B8T;
		m = TSL2561_LUX_M8T;
	}

	int32_t tmp = ((channel0 * b) - (channel1 * m));

	/* Round LSB (2^(LUX_SCALE−1)) */
	tmp += (1 << (TSL2561_LUX_SCALE - 1));

	/* Strip off fractional portion */
	val->val1 = tmp >> TSL2561_LUX_SCALE;

	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, tsl2561_driver_api) = {
	.sample_fetch = tsl2561_sample_fetch,
	.channel_get = tsl2561_channel_get
};

static int tsl2561_sensor_setup(const struct device *dev)
{
	const struct tsl2561_config *config = dev->config;
	struct tsl2561_data *data = dev->data;
	uint8_t timing_reg;
	uint8_t chip_id;
	uint8_t tmp;
	int ret;

	ret = tsl2561_reg_read(dev, TSL2561_REG_ID, &chip_id, 1U);
	if (ret < 0) {
		LOG_ERR("Failed reading chip ID");
		return ret;
	}

	if (FIELD_GET(TSL2561_ID_PARTNO, chip_id) != TSL2561_CHIP_ID) {
		LOG_ERR("Chip ID is invalid! Device @%02x is not TSL2561!", config->i2c.addr);
		return -EIO;
	}

	switch (config->integration_time) {
	case 13:
		tmp = TSL2561_INTEGRATION_13MS;
		data->ch_scale = TSL2561_CHSCALE_TINT0;
		break;
	case 101:
		tmp = TSL2561_INTEGRATION_101MS;
		data->ch_scale = TSL2561_CHSCALE_TINT1;
		break;
	case 402:
		tmp = TSL2561_INTEGRATION_402MS;
		data->ch_scale = (1 << TSL2561_CH_SCALE);
		break;
	default:
		LOG_ERR("Invalid integration time");
		return -EINVAL;
	}

	timing_reg = TSL2561_TIMING_INTEG & tmp;

	switch (config->gain) {
	case 1:
		tmp = TSL2561_GAIN_1X;
		data->ch_scale = data->ch_scale << 4;
		break;
	case 16:
		tmp = TSL2561_GAIN_16X;
		break;
	default:
		LOG_ERR("Invalid ADC gain");
		return -EINVAL;
	}

	timing_reg |= FIELD_PREP(TSL2561_TIMING_GAIN, tmp);

	ret = tsl2561_reg_write(dev, TSL2561_REG_TIMING, timing_reg);
	if (ret < 0) {
		LOG_ERR("Failed setting timing register");
		return ret;
	}

	return 0;
}

static int tsl2561_init(const struct device *dev)
{
	const struct tsl2561_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C dev %s not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	ret = tsl2561_sensor_setup(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure device");
		return ret;
	}

	return 0;
}

#define TSL2561_INIT_INST(n)                                                                       \
	static struct tsl2561_data tsl2561_data_##n;                                               \
	static const struct tsl2561_config tsl2561_config_##n = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.integration_time = DT_INST_PROP(n, integration_time),                             \
		.gain = DT_INST_PROP(n, gain)};                                                    \
	SENSOR_DEVICE_DT_INST_DEFINE(n, tsl2561_init, NULL, &tsl2561_data_##n,                     \
				     &tsl2561_config_##n, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &tsl2561_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TSL2561_INIT_INST)
