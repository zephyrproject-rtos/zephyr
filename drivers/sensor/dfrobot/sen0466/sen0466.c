/*
 * Copyright (c) 2026, Iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT dfrobot_sen0466

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "sen0466.h"

LOG_MODULE_REGISTER(SEN0466, CONFIG_SENSOR_LOG_LEVEL);

/*
 * NTC curve in milli-degrees Celsius, for
 * adc = SEN0466_NTC_ADC_MIN + k * SEN0466_NTC_ADC_STEP:
 *
 *	v   = 3 * adc / 1024
 *	rth = v * 10000 / (3 - v)
 *	t   = 1 / (1 / 298.15 + ln(rth / 10000) / 3380.13) - 273.15
 *
 * Interpolation between entries adds at most 32 milli-degrees of error.
 * The table ends past SEN0466_NTC_ADC_MAX, so index + 1 is always valid.
 */
static const int32_t sen0466_ntc_table[] = {
	 49976,  47701,  45519,  43420,  41392,  39429,
	 37524,  35668,  33858,  32087,  30351,  28645,
	 26966,  25309,  23670,  22047,  20436,  18834,
	 17237,  15642,  14046,  12446,  10838,   9217,
	  7582,   5926,   4245,   2535,    789,   -999,
	 -2837,  -4735,  -6702,  -8752, -10900, -13167,
	-15579, -18169, -20985,
};

static uint8_t sen0466_checksum(const uint8_t *frame)
{
	uint8_t sum = 0U;

	/* Byte 7 is left out on purpose: the part does not include it. */
	for (uint8_t i = SEN0466_CHECKSUM_FIRST; i <= SEN0466_CHECKSUM_LAST; i++) {
		sum += frame[i];
	}

	return (uint8_t)(~sum + 1U);
}

static void sen0466_pack(uint8_t *frame, uint8_t cmd, uint8_t arg)
{
	(void)memset(frame, 0, SEN0466_FRAME_LEN);

	frame[SEN0466_TX_HEAD] = SEN0466_FRAME_HEAD;
	frame[SEN0466_TX_ADDR] = SEN0466_FRAME_ADDR;
	frame[SEN0466_TX_CMD] = cmd;
	frame[SEN0466_TX_ARG] = arg;
	frame[SEN0466_FRAME_CHECKSUM] = sen0466_checksum(frame);
}

static int sen0466_frame_validate(const uint8_t *frame, uint8_t cmd)
{
	if (frame[SEN0466_RX_HEAD] != SEN0466_FRAME_HEAD) {
		return -EIO;
	}

	if (frame[SEN0466_FRAME_CHECKSUM] != sen0466_checksum(frame)) {
		return -EIO;
	}

	if (frame[SEN0466_RX_CMD] != cmd) {
		return -EAGAIN;
	}

	return 0;
}

static int sen0466_transceive(const struct device *dev, uint8_t cmd, uint8_t arg, uint8_t *rx)
{
	const struct sen0466_config *cfg = dev->config;
	struct sen0466_data *data = dev->data;
	const uint8_t reg = SEN0466_REG;
	uint8_t tx[SEN0466_FRAME_LEN];
	int ret = -EIO;

	sen0466_pack(tx, cmd, arg);

	k_mutex_lock(&data->lock, K_FOREVER);

	for (uint8_t attempt = 0U; attempt < SEN0466_MAX_ATTEMPTS; attempt++) {
		ret = i2c_burst_write_dt(&cfg->i2c, reg, tx, sizeof(tx));
		if (ret != 0) {
			break;
		}

		k_sleep(K_MSEC(SEN0466_RESPONSE_DELAY_MS));

		ret = i2c_write_dt(&cfg->i2c, &reg, sizeof(reg));
		if (ret != 0) {
			break;
		}

		ret = i2c_read_dt(&cfg->i2c, rx, SEN0466_FRAME_LEN);
		if (ret != 0) {
			break;
		}

		/* The part may still be uploading a frame every second,
		 * so read past anything that is not this answer.
		 */
		ret = sen0466_frame_validate(rx, cmd);
		if (ret == 0) {
			break;
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int sen0466_ntc_to_milli_c(uint16_t adc, int32_t *milli_c)
{
	uint16_t offset;
	uint16_t index;
	uint16_t frac;
	int32_t low;
	int32_t high;

	if ((adc < SEN0466_NTC_ADC_MIN) || (adc > SEN0466_NTC_ADC_MAX)) {
		return -ERANGE;
	}

	offset = adc - SEN0466_NTC_ADC_MIN;
	index = offset / SEN0466_NTC_ADC_STEP;
	frac = offset % SEN0466_NTC_ADC_STEP;

	low = sen0466_ntc_table[index];
	high = sen0466_ntc_table[index + 1U];

	*milli_c = low + (((high - low) * (int32_t)frac) / (int32_t)SEN0466_NTC_ADC_STEP);

	return 0;
}

static int sen0466_compensate(int32_t raw_milli_ppm, int32_t temp_milli_c, int32_t *milli_ppm)
{
	int64_t value = raw_milli_ppm;
	int32_t divisor;

	/* The compensation is only defined inside this range. */
	if ((temp_milli_c <= SEN0466_COMP_MIN_MILLI_C) ||
	    (temp_milli_c > SEN0466_COMP_MAX_MILLI_C)) {
		return -ERANGE;
	}

	/* 0.005 * t + 0.9, in milli-units. */
	divisor = ((5 * temp_milli_c) / 1000) + 900;
	value = (value * 1000) / divisor;

	if (temp_milli_c > SEN0466_COMP_MID_MILLI_C) {
		/* Above 20 degrees, also subtract 0.3 * t - 6. */
		value -= (((int64_t)300 * temp_milli_c) / 1000) - 6000;
	}

	*milli_ppm = (int32_t)CLAMP(value, SEN0466_CO_MIN_MILLI_PPM, SEN0466_CO_MAX_MILLI_PPM);

	return 0;
}

static int sen0466_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct sen0466_data *data = dev->data;
	uint8_t rx[SEN0466_FRAME_LEN];
	int32_t raw_milli_ppm;
	uint16_t concentration;
	uint8_t decimals;
	uint16_t adc;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_CO) &&
	    (chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		return -ENOTSUP;
	}

	ret = sen0466_transceive(dev, SEN0466_CMD_GET_ALL_DATA, 0U, rx);
	if (ret != 0) {
		return ret;
	}

	if (rx[SEN0466_RX_GAS_TYPE] != SEN0466_GAS_TYPE_CO) {
		LOG_ERR("Unexpected gas type 0x%02x", rx[SEN0466_RX_GAS_TYPE]);
		return -EIO;
	}

	decimals = rx[SEN0466_RX_DECIMALS];
	if (decimals > SEN0466_DECIMALS_MAX) {
		LOG_ERR("Unexpected decimal places %u", decimals);
		return -EIO;
	}

	concentration = sys_get_be16(&rx[SEN0466_RX_CONC_HI]);
	adc = sys_get_be16(&rx[SEN0466_RX_TEMP_HI]);

	/* One decimal place scales the reading by 0.1, two by 0.01. */
	raw_milli_ppm = (int32_t)concentration * 1000;
	for (uint8_t i = 0U; i < decimals; i++) {
		raw_milli_ppm /= 10;
	}

	ret = sen0466_ntc_to_milli_c(adc, &data->temp_milli_c);
	data->temp_valid = (ret == 0);

	if (data->temp_valid) {
		ret = sen0466_compensate(raw_milli_ppm, data->temp_milli_c, &data->co_milli_ppm);
		data->co_valid = (ret == 0);
	} else {
		data->co_valid = false;
	}

	return 0;
}

static int sen0466_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct sen0466_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_CO:
		if (!data->co_valid) {
			return -ERANGE;
		}
		return sensor_value_from_milli(val, data->co_milli_ppm);
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (!data->temp_valid) {
			return -ERANGE;
		}
		return sensor_value_from_milli(val, data->temp_milli_c);
	default:
		return -ENOTSUP;
	}
}

static int sen0466_init(const struct device *dev)
{
	const struct sen0466_config *cfg = dev->config;
	struct sen0466_data *data = dev->data;
	uint8_t rx[SEN0466_FRAME_LEN];
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	ret = sen0466_transceive(dev, SEN0466_CMD_SET_MODE, SEN0466_MODE_PASSIVE, rx);
	if (ret != 0) {
		LOG_ERR("No answer from the module (%d)", ret);
		return -ENODEV;
	}

	if (rx[SEN0466_RX_ACK] != SEN0466_ACK_OK) {
		/* Reads cope with either mode, so this is not fatal. */
		LOG_DBG("Passive mode takes effect after the next power cycle");
	}

	return 0;
}

static DEVICE_API(sensor, sen0466_api) = {
	.sample_fetch = sen0466_sample_fetch,
	.channel_get = sen0466_channel_get,
};

#define SEN0466_INIT(inst)                                                                         \
	static struct sen0466_data sen0466_data_##inst;                                            \
	static const struct sen0466_config sen0466_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sen0466_init, NULL, &sen0466_data_##inst,               \
				     &sen0466_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &sen0466_api);

DT_INST_FOREACH_STATUS_OKAY(SEN0466_INIT)
