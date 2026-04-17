/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_ms5637

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "ms5637.h"

LOG_MODULE_REGISTER(MS5637, CONFIG_SENSOR_LOG_LEVEL);

/**
 * Validate PROM contents using CRC-4 (MS56xx application note AN520).
 *
 * The CRC nibble is stored in bits [15:12] of PROM word 0. The algorithm
 * processes all 8 PROM words (16 bytes) with the CRC nibble zeroed out.
 */
static bool ms5637_prom_crc_valid(uint16_t *prom)
{
	uint16_t n_rem = 0U;
	uint16_t crc_read;
	uint8_t expected_crc;

	expected_crc = (uint8_t)(prom[MS5637_PROM_CRC_IDX] >> MS5637_CRC_NIBBLE_SHIFT);
	crc_read = prom[MS5637_PROM_WORD_COUNT - 1U];
	prom[MS5637_PROM_WORD_COUNT - 1U] = 0xFF00U & prom[MS5637_PROM_WORD_COUNT - 1U];

	for (uint8_t cnt = 0U; cnt < (MS5637_PROM_WORD_COUNT * 2U); cnt++) {
		if ((cnt % 2U) == 1U) {
			n_rem ^= (uint16_t)(prom[cnt >> 1] & 0x00FFU);
		} else {
			n_rem ^= (uint16_t)(prom[cnt >> 1] >> 8);
		}

		for (uint8_t n_bit = 8U; n_bit > 0U; n_bit--) {
			if ((n_rem & 0x8000U) != 0U) {
				n_rem = (n_rem << 1) ^ MS5637_CRC4_POLY;
			} else {
				n_rem = n_rem << 1;
			}
		}
	}

	n_rem = MS5637_CRC_NIBBLE_MASK & (n_rem >> MS5637_CRC_NIBBLE_SHIFT);
	prom[MS5637_PROM_WORD_COUNT - 1U] = crc_read;

	return n_rem == expected_crc;
}

/** Read a single 16-bit PROM word at the given address. */
static int ms5637_read_prom_word(const struct device *dev, uint8_t addr, uint16_t *val)
{
	const struct ms5637_config *cfg = dev->config;
	uint8_t buf[2];
	int ret;

	/*
	 * MS5637 requires: START, ADDR+W, cmd, RESTART, ADDR+R, MSB, LSB, STOP.
	 * Use i2c_write_read which generates this exact sequence.
	 */
	ret = i2c_write_read_dt(&cfg->bus, &addr, sizeof(addr), buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be16(buf);

	return 0;
}

/**
 * Trigger an ADC conversion and read the 24-bit result.
 *
 * Sends the conversion command, waits for the OSR-specific delay,
 * then reads the 24-bit ADC value via the READ_ADC command.
 */
static int ms5637_convert_and_read(const struct device *dev, uint8_t conv_cmd, uint8_t delay_ms,
				   uint32_t *adc_val)
{
	const struct ms5637_config *cfg = dev->config;
	uint8_t cmd;
	uint8_t buf[MS5637_ADC_READ_LEN];
	int ret;

	/* Start conversion */
	cmd = conv_cmd;
	ret = i2c_write_dt(&cfg->bus, &cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}

	/* Wait for ADC conversion to complete (datasheet Table 2) */
	k_msleep(delay_ms);

	/* Read 24-bit ADC result */
	uint8_t read_cmd = MS5637_CMD_READ_ADC;

	ret = i2c_write_read_dt(&cfg->bus, &read_cmd, sizeof(read_cmd), buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*adc_val = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[2];

	return 0;
}

/**
 * Apply temperature and pressure compensation per MS5637-02BA03 datasheet.
 *
 * First order:
 *   dT   = D2 - C5 * 2^8
 *   TEMP = 2000 + dT * C6 / 2^23
 *   OFF  = C2 * 2^17 + (C4 * dT) / 2^6
 *   SENS = C1 * 2^16 + (C3 * dT) / 2^7
 *   P    = (D1 * SENS / 2^21 - OFF) / 2^15
 *
 * Second order corrections are applied per datasheet flowchart.
 */
static void ms5637_compensate(struct ms5637_data *data, uint32_t adc_temperature,
			      uint32_t adc_pressure)
{
	int64_t dT;
	int64_t OFF;
	int64_t SENS;
	int64_t temp_sq;
	int64_t T2;
	int64_t OFF2;
	int64_t SENS2;

	/* First order compensation */
	dT = (int64_t)adc_temperature - ((int64_t)data->t_ref << 8);
	data->temperature = (int32_t)(2000 + (dT * data->tempsens) / (1LL << 23));
	OFF = ((int64_t)data->off_t1 << 17) + (dT * data->tco) / (1LL << 6);
	SENS = ((int64_t)data->sens_t1 << 16) + (dT * data->tcs) / (1LL << 7);

	/* Second order compensation */
	temp_sq = (int64_t)(data->temperature - 2000) * (int64_t)(data->temperature - 2000);

	if (data->temperature < 2000) {
		T2 = (3LL * dT * dT) / (1LL << 33);
		OFF2 = (61LL * temp_sq) / (1LL << 4);
		SENS2 = (29LL * temp_sq) / (1LL << 4);

		if (data->temperature < -1500) {
			temp_sq = (int64_t)(data->temperature + 1500) *
				  (int64_t)(data->temperature + 1500);
			OFF2 += 17LL * temp_sq;
			SENS2 += 9LL * temp_sq;
		}
	} else {
		T2 = (5LL * dT * dT) / (1LL << 38);
		OFF2 = 0;
		SENS2 = 0;
	}

	OFF -= OFF2;
	SENS -= SENS2;

	data->temperature -= (int32_t)T2;
	data->pressure =
		(int32_t)((SENS * (int64_t)adc_pressure / (1LL << 21) - OFF) / (1LL << 15));
}

static int ms5637_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ms5637_data *data = dev->data;
	uint32_t adc_pressure;
	uint32_t adc_temperature;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_PRESS) &&
	    (chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		return -ENOTSUP;
	}

	ret = ms5637_convert_and_read(dev, data->pressure_conv_cmd, data->pressure_conv_delay,
				      &adc_pressure);
	if (ret < 0) {
		LOG_ERR("Pressure conversion failed: %d", ret);
		return ret;
	}

	ret = ms5637_convert_and_read(dev, data->temperature_conv_cmd, data->temperature_conv_delay,
				      &adc_temperature);
	if (ret < 0) {
		LOG_ERR("Temperature conversion failed: %d", ret);
		return ret;
	}

	ms5637_compensate(data, adc_temperature, adc_pressure);

	return 0;
}

static int ms5637_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ms5637_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Compensated temperature is in centidegrees Celsius */
		sensor_value_from_micro(val,
					(int64_t)data->temperature * MS5637_MICRO_PER_CENTIDEG);
		break;
	case SENSOR_CHAN_PRESS:
		/* Compensated pressure is in Pa; channel unit is kPa */
		sensor_value_from_milli(val, (int64_t)data->pressure);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ms5637_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	struct ms5637_data *data = dev->data;
	uint8_t p_conv_cmd;
	uint8_t t_conv_cmd;
	uint8_t conv_delay;

	if (attr != SENSOR_ATTR_OVERSAMPLING) {
		return -ENOTSUP;
	}

	switch (val->val1) {
	case 8192:
		p_conv_cmd = MS5637_CMD_CONV_P_8192;
		t_conv_cmd = MS5637_CMD_CONV_T_8192;
		conv_delay = MS5637_CONV_DELAY_8192;
		break;
	case 4096:
		p_conv_cmd = MS5637_CMD_CONV_P_4096;
		t_conv_cmd = MS5637_CMD_CONV_T_4096;
		conv_delay = MS5637_CONV_DELAY_4096;
		break;
	case 2048:
		p_conv_cmd = MS5637_CMD_CONV_P_2048;
		t_conv_cmd = MS5637_CMD_CONV_T_2048;
		conv_delay = MS5637_CONV_DELAY_2048;
		break;
	case 1024:
		p_conv_cmd = MS5637_CMD_CONV_P_1024;
		t_conv_cmd = MS5637_CMD_CONV_T_1024;
		conv_delay = MS5637_CONV_DELAY_1024;
		break;
	case 512:
		p_conv_cmd = MS5637_CMD_CONV_P_512;
		t_conv_cmd = MS5637_CMD_CONV_T_512;
		conv_delay = MS5637_CONV_DELAY_512;
		break;
	case 256:
		p_conv_cmd = MS5637_CMD_CONV_P_256;
		t_conv_cmd = MS5637_CMD_CONV_T_256;
		conv_delay = MS5637_CONV_DELAY_256;
		break;
	default:
		LOG_ERR("Invalid oversampling rate %d", val->val1);
		return -EINVAL;
	}

	switch (chan) {
	case SENSOR_CHAN_ALL:
		data->pressure_conv_cmd = p_conv_cmd;
		data->pressure_conv_delay = conv_delay;
		data->temperature_conv_cmd = t_conv_cmd;
		data->temperature_conv_delay = conv_delay;
		break;
	case SENSOR_CHAN_PRESS:
		data->pressure_conv_cmd = p_conv_cmd;
		data->pressure_conv_delay = conv_delay;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		data->temperature_conv_cmd = t_conv_cmd;
		data->temperature_conv_delay = conv_delay;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ms5637_init(const struct device *dev)
{
	const struct ms5637_config *cfg = dev->config;
	struct ms5637_data *data = dev->data;
	uint16_t prom[MS5637_PROM_WORD_COUNT];
	struct sensor_value osr;
	uint8_t cmd;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* Reset sensor to load PROM into internal registers */
	cmd = MS5637_CMD_RESET;
	ret = i2c_write_dt(&cfg->bus, &cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Reset failed: %d", ret);
		return ret;
	}

	/* Wait for reset to complete per datasheet */
	k_msleep(MS5637_RESET_TIME_MS);

	/* Read all 8 PROM words for CRC validation.
	 * Some MS5637 variants do not respond to PROM[7] (0xAE);
	 * treat it as 0x0000 if the read fails.
	 */
	for (uint8_t i = 0U; i < MS5637_PROM_WORD_COUNT; i++) {
		ret = ms5637_read_prom_word(dev, MS5637_PROM_ADDR_BASE + (i * 2U), &prom[i]);
		if (ret < 0) {
			if (i == MS5637_PROM_WORD_COUNT - 1U) {
				LOG_WRN("PROM[%u] read failed, assuming 0x0000", i);
				prom[i] = 0x0000U;
			} else {
				LOG_ERR("PROM read failed at index %u: %d", i, ret);
				return ret;
			}
		}
		LOG_DBG("PROM[%u] = 0x%04x", i, prom[i]);
	}

	/* Validate PROM CRC-4 (skip if PROM[7] was not readable) */
	if (prom[MS5637_PROM_WORD_COUNT - 1U] != 0x0000U && !ms5637_prom_crc_valid(prom)) {
		LOG_ERR("PROM CRC-4 validation failed");
		return -EIO;
	}

	/* Store calibration coefficients */
	data->sens_t1 = prom[MS5637_PROM_C1_IDX];
	data->off_t1 = prom[MS5637_PROM_C2_IDX];
	data->tcs = prom[MS5637_PROM_C3_IDX];
	data->tco = prom[MS5637_PROM_C4_IDX];
	data->t_ref = prom[MS5637_PROM_C5_IDX];
	data->tempsens = prom[MS5637_PROM_C6_IDX];

	LOG_DBG("PROM: C1=%u C2=%u C3=%u C4=%u C5=%u C6=%u", data->sens_t1, data->off_t1, data->tcs,
		data->tco, data->t_ref, data->tempsens);

	/* Set default oversampling from Kconfig */
	osr.val1 = MS5637_PRES_OVER_DEFAULT;
	ret = ms5637_attr_set(dev, SENSOR_CHAN_PRESS, SENSOR_ATTR_OVERSAMPLING, &osr);
	if (ret < 0) {
		return ret;
	}

	osr.val1 = MS5637_TEMP_OVER_DEFAULT;
	ret = ms5637_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_OVERSAMPLING, &osr);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, ms5637_api) = {
	.attr_set = ms5637_attr_set,
	.sample_fetch = ms5637_sample_fetch,
	.channel_get = ms5637_channel_get,
};

#define MS5637_INIT(inst)                                                                          \
	static struct ms5637_data ms5637_data_##inst;                                              \
	static const struct ms5637_config ms5637_config_##inst = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ms5637_init, NULL, &ms5637_data_##inst,                 \
				     &ms5637_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &ms5637_api);

DT_INST_FOREACH_STATUS_OKAY(MS5637_INIT)
