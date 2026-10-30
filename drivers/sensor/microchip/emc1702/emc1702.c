/*
 * Copyright (c) 2026  Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Microchip EMC1702 high-side current sense and
 * dual temperature monitor.
 *
 * Implements the Zephyr sensor subsystem API. Exposes:
 *   SENSOR_CHAN_DIE_TEMP     - internal diode temperature (degC)
 *   SENSOR_CHAN_AMBIENT_TEMP - external diode temperature (degC)
 *   SENSOR_CHAN_VOLTAGE      - bus voltage at SENSE+ (V)
 *   SENSOR_CHAN_CURRENT      - current through Rsense (A, signed)
 *   SENSOR_CHAN_POWER        - power at the source side of Rsense (W)
 *
 * The driver has trigger support for temperature, current and voltage limit alerts.
 *
 * Datasheet can be found here:
 * https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/EMC1702-Data-Sheet-DS20006455A.pdf
 */

#define DT_DRV_COMPAT microchip_emc1702

#include <zephyr/drivers/sensor/emc1702.h>
#include "emc1702.h"

LOG_MODULE_REGISTER(emc1702, CONFIG_SENSOR_LOG_LEVEL);

/* --- Register helpers --------------------------------------------------- */

int emc1702_read8(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct emc1702_config *cfg = dev->config;
	struct emc1702_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->lock, K_FOREVER);
	rc = i2c_write_read_dt(&cfg->i2c, &reg, 1, val, 1);
	k_mutex_unlock(&data->lock);

	return rc;
}

int emc1702_write8(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct emc1702_config *cfg = dev->config;
	struct emc1702_data *data = dev->data;
	uint8_t buf[2] = {reg, val};
	int rc;

	k_mutex_lock(&data->lock, K_FOREVER);
	rc = i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
	k_mutex_unlock(&data->lock);

	return rc;
}

/*
 * Read two bytes from (reg_hi, reg_lo) using two separate I2C transactions.
 *
 * Reading a high byte register copies the corresponding low byte into an
 * internal shadow register (datasheet section 5.1). The lock is held across
 * both reads so a concurrent access cannot clobber that shadow latch between
 * the high and low-byte reads.
 */
int emc1702_read16(const struct device *dev, uint8_t reg_hi, uint8_t reg_lo, uint8_t *hi,
		   uint8_t *lo)
{
	const struct emc1702_config *cfg = dev->config;
	struct emc1702_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->lock, K_FOREVER);

	rc = i2c_write_read_dt(&cfg->i2c, &reg_hi, 1, hi, 1);
	if (rc) {
		goto out;
	}
	rc = i2c_write_read_dt(&cfg->i2c, &reg_lo, 1, lo, 1);

out:
	k_mutex_unlock(&data->lock);
	return rc;
}

/* --- Decode helpers ----------------------------------------------------- */

/* Convert raw temperature value from register into sensor_value */
static void emc1702_raw_to_real(uint16_t raw, struct sensor_value *val)
{
	uint8_t hi = (uint8_t)(raw >> 8);
	uint8_t lo = (uint8_t)(raw & 0xFF);

	val->val1 = (int32_t)((int8_t)hi);

	if (val->val1 < 0 && lo) {
		val->val1 += 1;
		val->val2 = (int32_t)((int8_t)0x08 - (int8_t)(lo >> 5)) * (-125000);
	} else {
		val->val2 = (int32_t)(lo >> 5) * 125000;
	}
}

/* Maps a devicetree/user value to its encoded register field value. */
struct emc1702_field_map {
	uint16_t key; /* value coming from devicetree */
	uint8_t val;  /* encoded register field       */
};

/*
 * Current-sense sampling time in ms -> CS_SAMP_TIME (datasheet Table 5.30).
 * Encodings 0 and 1 both select 82 ms; 0 is used.
 */
static const struct emc1702_field_map emc1702_cs_time_map[] = {
	{82, 0},
	{164, 2},
	{328, 3},
};

/* Current-sense range in mV -> CS_RNG (datasheet Table 5.32). */
static const struct emc1702_field_map emc1702_cs_range_map[] = {
	{10, 0},
	{20, 1},
	{40, 2},
	{80, 3},
};

/*
 * Conversion rate in ms (datasheet Table 5.7). e.g. 4000 = 4 conversions/s
 * (power-on default).
 */
static const struct emc1702_field_map emc1702_conv_rate_map[] = {
	{62, 0}, {125, 1}, {250, 2}, {500, 3}, {1000, 4}, {2000, 5}, {4000, 6}, {8000, 7},
};

/* Consecutive out-of-limit count -> CALRT/CTHRM (datasheet Table 5.14). */
static const struct emc1702_field_map emc1702_alert_consec_map[] = {
	{1, 0},
	{2, 1},
	{3, 3},
	{4, 7},
};

/* Consecutive out-of-limit count -> CS_QUEUE / V_QUEUE (Tables 5.25, 5.28). */
static const struct emc1702_field_map emc1702_queue_map[] = {
	{1, 0},
	{2, 1},
	{3, 2},
	{4, 3},
};

static int emc1702_map_lookup(const struct emc1702_field_map *map, size_t len, uint16_t key,
			      uint8_t *result)
{
	for (size_t i = 0; i < len; i++) {
		if (map[i].key == key) {
			*result = map[i].val;
			return 0;
		}
	}

	return -EINVAL;
}

static int emc1702_decode_cs_time(uint16_t time, uint8_t *result)
{
	return emc1702_map_lookup(emc1702_cs_time_map, ARRAY_SIZE(emc1702_cs_time_map), time,
				  result);
}

static int emc1702_decode_cs_range(uint8_t range, uint8_t *result)
{
	return emc1702_map_lookup(emc1702_cs_range_map, ARRAY_SIZE(emc1702_cs_range_map), range,
				  result);
}

static int emc1702_decode_conv_rate(uint16_t conv, uint8_t *result)
{
	return emc1702_map_lookup(emc1702_conv_rate_map, ARRAY_SIZE(emc1702_conv_rate_map), conv,
				  result);
}

static int emc1702_decode_alert_consec(uint8_t count, uint8_t *result)
{
	return emc1702_map_lookup(emc1702_alert_consec_map, ARRAY_SIZE(emc1702_alert_consec_map),
				  count, result);
}

static int emc1702_decode_queue(uint8_t count, uint8_t *result)
{
	return emc1702_map_lookup(emc1702_queue_map, ARRAY_SIZE(emc1702_queue_map), count, result);
}

/* --- Sensor API --------------------------------------------------------- */

static int emc1702_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	uint16_t text = 0, tint, vsource, vsense, pratio;
	const struct emc1702_config *cfg = dev->config;
	struct emc1702_data *d = dev->data;
	uint8_t hi, lo;
	int rc;

	ARG_UNUSED(chan); /* Always fetch all channels at once. */

	rc = emc1702_read16(dev, EMC1702_REG_INTERNAL_DIODE_HIGH, EMC1702_REG_INTERNAL_DIODE_LOW,
			    &hi, &lo);
	if (rc) {
		LOG_ERR("read T_int: %d", rc);
		return rc;
	}
	tint = (int16_t)(((uint16_t)hi << 8) | lo);

	/* The external channel is only read when a diode is wired to DP/DN. */
	if (cfg->external_diode) {
		rc = emc1702_read16(dev, EMC1702_REG_EXTERNAL_DIODE_HIGH,
				    EMC1702_REG_EXTERNAL_DIODE_LOW, &hi, &lo);
		if (rc) {
			LOG_ERR("read T_ext: %d", rc);
			return rc;
		}
		text = (int16_t)(((uint16_t)hi << 8) | lo);
	}

	rc = emc1702_read16(dev, EMC1702_REG_SOURCE_VOLTAGE_HIGH, EMC1702_REG_SOURCE_VOLTAGE_LOW,
			    &hi, &lo);
	if (rc) {
		LOG_ERR("read V_src: %d", rc);
		return rc;
	}
	vsource = ((uint16_t)hi << 8) | lo;

	rc = emc1702_read16(dev, EMC1702_REG_SENSE_VOLTAGE_HIGH, EMC1702_REG_SENSE_VOLTAGE_LOW, &hi,
			    &lo);
	if (rc) {
		LOG_ERR("read V_sense: %d", rc);
		return rc;
	}
	vsense = (int16_t)(((uint16_t)hi << 8) | lo);

	rc = emc1702_read16(dev, EMC1702_REG_POWER_RATIO_HIGH, EMC1702_REG_POWER_RATIO_LOW, &hi,
			    &lo);
	if (rc) {
		LOG_ERR("read P_ratio: %d", rc);
		return rc;
	}
	pratio = ((uint16_t)hi << 8) | lo;

	d->t_internal_raw = tint;
	d->t_external_raw = text;
	d->vsource_raw = vsource;
	d->vsense_raw = vsense;
	d->pratio_raw = pratio;

	return 0;
}

static int emc1702_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct emc1702_config *cfg = dev->config;
	const struct emc1702_data *d = dev->data;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP: {
		emc1702_raw_to_real(d->t_internal_raw, val);

		return 0;
	}

	case SENSOR_CHAN_AMBIENT_TEMP: {
		if (!cfg->external_diode) {
			return -ENOTSUP;
		}
		emc1702_raw_to_real(d->t_external_raw, val);

		return 0;
	}

	/*
	 * In order to calculate the current and voltage values
	 * we will use the general formula:
	 * (Full_Scale_Measurement * Register_Value) / (2 ^ Number_Of_Used_Bits)
	 * instead of the formulas described in documentation.
	 */
	case SENSOR_CHAN_VOLTAGE: {
		uint32_t vsource_uv =
			(uint32_t)(((uint64_t)d->vsource_raw * EMC1702_FSV_UV) / 65536U);

		val->val1 = vsource_uv / 1000000;
		val->val2 = vsource_uv % 1000000;
		return 0;
	}

	case SENSOR_CHAN_CURRENT: {
		/*
		 * Isense_uA = raw * cs_range_mv * 1000000 / (32768 * Rsense_mohm),
		 * with 1000000 / 32768 pre-reduced to EMC1702_ISENSE_UA_NUM/_DEN.
		 */
		int64_t isense_ua =
			((int64_t)d->vsense_raw * cfg->cs_range_mv * EMC1702_ISENSE_UA_NUM) /
			((int64_t)cfg->sense_resistor_mohm * EMC1702_ISENSE_UA_DEN);
		val->val1 = (int32_t)(isense_ua / 1000000);
		val->val2 = (int32_t)(isense_ua % 1000000);
		return 0;
	}

	case SENSOR_CHAN_POWER: {
		/*
		 * P_uW = cs_range_mv * FSV_uV * PRATIO / (Rsense_mohm * PRATIO_FS),
		 * where FSP = FSC * FSV and FSC = cs_range_mv / Rsense_mohm.
		 */
		int64_t p_uw = ((int64_t)cfg->cs_range_mv * EMC1702_FSV_UV * d->pratio_raw) /
			       ((int64_t)cfg->sense_resistor_mohm * EMC1702_PRATIO_FULL_SCALE);

		val->val1 = (int32_t)(p_uw / 1000000);
		val->val2 = (int32_t)(p_uw % 1000000);
		return 0;
	}

	default:
		return -ENOTSUP;
	}
}

static int emc1702_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct emc1702_config *cfg = dev->config;
	int64_t temp, temp_micro, micro_value;
	struct emc1702_data *d = dev->data;
	uint64_t volt_micro_value, utemp;
	int32_t temp_val;
	int8_t high;
	uint8_t low, hyst;
	int rc;

	switch ((int)chan) {
	case SENSOR_CHAN_DIE_TEMP:
		/* Internal diode retains only integer part. */
		temp_val = val->val1;
		temp_val = CLAMP(temp_val, -128, 127);

		switch ((int)attr) {
		case SENSOR_ATTR_UPPER_THRESH:
			rc = emc1702_write8(dev, EMC1702_REG_INTERNAL_DIODE_H_LIMIT,
					    (int8_t)temp_val);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_LOWER_THRESH:
			rc = emc1702_write8(dev, EMC1702_REG_INTERNAL_DIODE_L_LIMIT,
					    (int8_t)temp_val);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_EMC1702_CRITICAL_LIMIT:
			rc = emc1702_write8(dev, EMC1702_REG_INT_DIODE_TEMP_CRIT_LIMIT,
					    (int8_t)temp_val);
			if (rc) {
				return rc;
			}

			return 0;
		default:
			return -ENOTSUP;
		}
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (!cfg->external_diode) {
			return -ENOTSUP;
		}
		temp_micro = sensor_value_to_micro(val);
		temp_micro = CLAMP(temp_micro, -128000000, 127875000);
		temp_micro = DIV_ROUND_CLOSEST(temp_micro, 125000);

		high = temp_micro >> 3;
		low = (temp_micro & 7) * 32;

		switch ((int)attr) {
		case SENSOR_ATTR_UPPER_THRESH:
			rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_H_LIMIT_H_BYTE, high);
			if (rc) {
				return rc;
			}
			rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_H_LIMIT_L_BYTE, low);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_LOWER_THRESH:
			rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_L_LIMIT_H_BYTE, high);
			if (rc) {
				return rc;
			}
			rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_L_LIMIT_L_BYTE, low);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_EMC1702_CRITICAL_LIMIT:
			/* This limit has no fractional byte, integer part only. */
			temp_val = CLAMP(val->val1, -128, 127);

			rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_TEMP_CRIT_LIMIT,
					    (int8_t)temp_val);
			if (rc) {
				return rc;
			}

			return 0;
		default:
			return -ENOTSUP;
		}
	/* Hysteresis is shared by the critical limits of both temp channels. */
	case SENSOR_CHAN_EMC1702_TEMP_HYSTERESIS:
		switch ((int)attr) {
		case SENSOR_ATTR_HYSTERESIS:
			hyst = (uint8_t)CLAMP(val->val1, 0, EMC1702_TEMP_HYST_MAX);

			rc = emc1702_write8(dev, EMC1702_REG_TEMP_HYST, hyst);
			if (rc) {
				return rc;
			}

			return 0;
		default:
			return -ENOTSUP;
		}
	case SENSOR_CHAN_CURRENT:
		micro_value = sensor_value_to_micro(val);
		micro_value = CLAMP(micro_value, d->current_min_valid, d->current_max_valid);

		temp = micro_value * cfg->sense_resistor_mohm;
		temp *= 128;
		temp = temp / (cfg->cs_range_mv * 1000000);
		/*
		 * This clamp is necessary to ensure that the value
		 * to be written in register is valid.
		 */
		temp = CLAMP(temp, -128, 127);

		switch ((int)attr) {
		case SENSOR_ATTR_UPPER_THRESH:
			rc = emc1702_write8(dev, EMC1702_REG_SENSE_VOLTAGE_HIGH_LIMIT,
					    (int8_t)temp);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_LOWER_THRESH:
			rc = emc1702_write8(dev, EMC1702_REG_SENSE_VOLTAGE_LOW_LIMIT, (int8_t)temp);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_EMC1702_CRITICAL_LIMIT:
			rc = emc1702_write8(dev, EMC1702_REG_SENSE_VOLTAGE_CRIT_LIMIT,
					    (int8_t)temp);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_HYSTERESIS:
			/* Hysteresis of the critical limit */
			temp = CLAMP(temp, 0, EMC1702_VOLT_CRIT_HYST_MAX);

			rc = emc1702_write8(dev, EMC1702_REG_SENSE_VOLTAGE_CRIT_HYST,
					    (uint8_t)temp);
			if (rc) {
				return rc;
			}

			return 0;
		default:
			return -ENOTSUP;
		}
	case SENSOR_CHAN_VOLTAGE:
		/* VSOURCE is unsigned: clamp negatives to 0 and cap at the 24 V max. */
		temp = sensor_value_to_micro(val);
		volt_micro_value = CLAMP((int64_t)temp, 0, 24000000);

		utemp = volt_micro_value << 8;
		utemp /= (uint64_t)EMC1702_FSV_UV;

		/* This clamp is necessary when trying to write maximum value */
		utemp = CLAMP(utemp, 0, 255);

		switch ((int)attr) {
		case SENSOR_ATTR_UPPER_THRESH:
			rc = emc1702_write8(dev, EMC1702_REG_SOURCE_VOLTAGE_HIGH_LIMIT,
					    (uint8_t)utemp);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_LOWER_THRESH:
			rc = emc1702_write8(dev, EMC1702_REG_SOURCE_VOLTAGE_LOW_LIMIT,
					    (uint8_t)utemp);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_EMC1702_CRITICAL_LIMIT:
			rc = emc1702_write8(dev, EMC1702_REG_SOURCE_VOLTAGE_CRIT_LIMIT,
					    (uint8_t)utemp);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_HYSTERESIS:
			/* Hysteresis of the critical limit. */
			utemp = CLAMP(utemp, 0, EMC1702_VOLT_CRIT_HYST_MAX);

			rc = emc1702_write8(dev, EMC1702_REG_SOURCE_VOLTAGE_CRIT_HYST,
					    (uint8_t)utemp);
			if (rc) {
				return rc;
			}

			return 0;
		default:
			return -ENOTSUP;
		}
	default:
		return -ENOTSUP;
	}
}

static int emc1702_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	const struct emc1702_config *cfg = dev->config;
	uint16_t vsource_raw;
	int8_t signed_value;
	uint32_t vsource_uv;
	int16_t vsense_raw;
	int64_t isense_ua;
	uint8_t high, low;
	uint8_t crit, hyst, status;
	int rc;

	switch ((int)chan) {
	case SENSOR_CHAN_DIE_TEMP:
		/* Internal diode retains only the integer part */
		switch ((int)attr) {
		case SENSOR_ATTR_UPPER_THRESH:
			rc = emc1702_read8(dev, EMC1702_REG_INTERNAL_DIODE_H_LIMIT, &high);
			if (rc) {
				return rc;
			}

			emc1702_raw_to_real((uint16_t)((uint8_t)high << 8), val);

			return 0;
		case SENSOR_ATTR_LOWER_THRESH:
			rc = emc1702_read8(dev, EMC1702_REG_INTERNAL_DIODE_L_LIMIT, &low);
			if (rc) {
				return rc;
			}

			emc1702_raw_to_real((uint16_t)((uint8_t)low << 8), val);

			return 0;
		case SENSOR_ATTR_EMC1702_CRITICAL_LIMIT:
			rc = emc1702_read8(dev, EMC1702_REG_INT_DIODE_TEMP_CRIT_LIMIT, &crit);
			if (rc) {
				return rc;
			}

			emc1702_raw_to_real((uint16_t)((uint8_t)crit << 8), val);

			return 0;
		default:
			return -ENOTSUP;
		}
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (!cfg->external_diode) {
			return -ENOTSUP;
		}
		switch ((int)attr) {
		case SENSOR_ATTR_UPPER_THRESH:
			rc = emc1702_read8(dev, EMC1702_REG_EXT_DIODE_H_LIMIT_H_BYTE, &high);
			if (rc) {
				return rc;
			}

			rc = emc1702_read8(dev, EMC1702_REG_EXT_DIODE_H_LIMIT_L_BYTE, &low);
			if (rc) {
				return rc;
			}

			emc1702_raw_to_real((uint16_t)(((uint8_t)high << 8) | low), val);

			return 0;
		case SENSOR_ATTR_LOWER_THRESH:
			rc = emc1702_read8(dev, EMC1702_REG_EXT_DIODE_L_LIMIT_H_BYTE, &high);
			if (rc) {
				return rc;
			}

			rc = emc1702_read8(dev, EMC1702_REG_EXT_DIODE_L_LIMIT_L_BYTE, &low);
			if (rc) {
				return rc;
			}

			emc1702_raw_to_real((uint16_t)(((uint8_t)high << 8) | low), val);

			return 0;
		case SENSOR_ATTR_EMC1702_CRITICAL_LIMIT:
			rc = emc1702_read8(dev, EMC1702_REG_EXT_DIODE_TEMP_CRIT_LIMIT, &crit);
			if (rc) {
				return rc;
			}

			emc1702_raw_to_real((uint16_t)((uint8_t)crit << 8), val);

			return 0;
		default:
			return -ENOTSUP;
		}
	case SENSOR_CHAN_EMC1702_TEMP_HYSTERESIS:
		switch ((int)attr) {
		case SENSOR_ATTR_HYSTERESIS:
			rc = emc1702_read8(dev, EMC1702_REG_TEMP_HYST, &hyst);
			if (rc) {
				return rc;
			}

			val->val1 = hyst;
			val->val2 = 0;

			return 0;
		default:
			return -ENOTSUP;
		}
	case SENSOR_CHAN_CURRENT:
		switch ((int)attr) {
		case SENSOR_ATTR_UPPER_THRESH:
			rc = emc1702_read8(dev, EMC1702_REG_SENSE_VOLTAGE_HIGH_LIMIT, &high);
			if (rc) {
				return rc;
			}

			signed_value = (int8_t)high;
			vsense_raw = (int16_t)signed_value * 256;
			isense_ua =
				((int64_t)vsense_raw * cfg->cs_range_mv * EMC1702_ISENSE_UA_NUM) /
				((int64_t)cfg->sense_resistor_mohm * EMC1702_ISENSE_UA_DEN);

			rc = sensor_value_from_micro(val, isense_ua);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_LOWER_THRESH:
			rc = emc1702_read8(dev, EMC1702_REG_SENSE_VOLTAGE_LOW_LIMIT, &low);
			if (rc) {
				return rc;
			}

			signed_value = (int8_t)low;
			vsense_raw = (int16_t)signed_value * 256;
			isense_ua =
				((int64_t)vsense_raw * cfg->cs_range_mv * EMC1702_ISENSE_UA_NUM) /
				((int64_t)cfg->sense_resistor_mohm * EMC1702_ISENSE_UA_DEN);

			rc = sensor_value_from_micro(val, isense_ua);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_EMC1702_CRITICAL_LIMIT:
			rc = emc1702_read8(dev, EMC1702_REG_SENSE_VOLTAGE_CRIT_LIMIT, &crit);
			if (rc) {
				return rc;
			}

			signed_value = (int8_t)crit;
			vsense_raw = (int16_t)signed_value * 256;
			isense_ua =
				((int64_t)vsense_raw * cfg->cs_range_mv * EMC1702_ISENSE_UA_NUM) /
				((int64_t)cfg->sense_resistor_mohm * EMC1702_ISENSE_UA_DEN);

			rc = sensor_value_from_micro(val, isense_ua);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_HYSTERESIS:
			rc = emc1702_read8(dev, EMC1702_REG_SENSE_VOLTAGE_CRIT_HYST, &hyst);
			if (rc) {
				return rc;
			}

			vsense_raw = (int16_t)(hyst & EMC1702_VOLT_CRIT_HYST_MAX) * 256;
			isense_ua =
				((int64_t)vsense_raw * cfg->cs_range_mv * EMC1702_ISENSE_UA_NUM) /
				((int64_t)cfg->sense_resistor_mohm * EMC1702_ISENSE_UA_DEN);

			rc = sensor_value_from_micro(val, isense_ua);
			if (rc) {
				return rc;
			}

			return 0;
		default:
			return -ENOTSUP;
		}
	case SENSOR_CHAN_VOLTAGE:
		/* Voltage limits have only integer parts */
		switch ((int)attr) {
		case SENSOR_ATTR_UPPER_THRESH:
			rc = emc1702_read8(dev, EMC1702_REG_SOURCE_VOLTAGE_HIGH_LIMIT, &high);
			if (rc) {
				return rc;
			}

			vsource_raw = high << 8;
			vsource_uv = (uint32_t)(((uint64_t)vsource_raw * EMC1702_FSV_UV) / 65536U);
			rc = sensor_value_from_micro(val, vsource_uv);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_LOWER_THRESH:
			rc = emc1702_read8(dev, EMC1702_REG_SOURCE_VOLTAGE_LOW_LIMIT, &low);
			if (rc) {
				return rc;
			}

			vsource_raw = low << 8;
			vsource_uv = (uint32_t)(((uint64_t)vsource_raw * EMC1702_FSV_UV) / 65536U);
			rc = sensor_value_from_micro(val, vsource_uv);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_EMC1702_CRITICAL_LIMIT:
			rc = emc1702_read8(dev, EMC1702_REG_SOURCE_VOLTAGE_CRIT_LIMIT, &crit);
			if (rc) {
				return rc;
			}

			vsource_raw = crit << 8;
			vsource_uv = (uint32_t)(((uint64_t)vsource_raw * EMC1702_FSV_UV) / 65536U);
			rc = sensor_value_from_micro(val, vsource_uv);
			if (rc) {
				return rc;
			}

			return 0;
		case SENSOR_ATTR_HYSTERESIS:
			rc = emc1702_read8(dev, EMC1702_REG_SOURCE_VOLTAGE_CRIT_HYST, &hyst);
			if (rc) {
				return rc;
			}

			vsource_raw = (hyst & EMC1702_VOLT_CRIT_HYST_MAX) << 8;
			vsource_uv = (uint32_t)(((uint64_t)vsource_raw * EMC1702_FSV_UV) / 65536U);
			rc = sensor_value_from_micro(val, vsource_uv);
			if (rc) {
				return rc;
			}

			return 0;
		default:
			return -ENOTSUP;
		}
	/* Raw High/Low/Critical Limit Status registers (bitmask of channels). */
	case SENSOR_CHAN_EMC1702_LIMIT_STATUS:
		switch ((int)attr) {
		/* Reading this register does not clear it. */
		case SENSOR_ATTR_EMC1702_CRIT_LIMIT_STATUS:
			rc = emc1702_read8(dev, EMC1702_REG_CRIT_LIMIT_STATUS, &status);
			if (rc) {
				return rc;
			}
			val->val1 = status;
			val->val2 = 0;

			return 0;
#ifndef CONFIG_EMC1702_TRIGGER
		/* These are cleared by reading: only exposed when the trigger
		 * path is not consuming them.
		 */
		case SENSOR_ATTR_EMC1702_HIGH_LIMIT_STATUS:
			rc = emc1702_read8(dev, EMC1702_REG_HIGH_LIMIT_STATUS, &status);
			if (rc) {
				return rc;
			}
			val->val1 = status;
			val->val2 = 0;

			return 0;
		case SENSOR_ATTR_EMC1702_LOW_LIMIT_STATUS:
			rc = emc1702_read8(dev, EMC1702_REG_LOW_LIMIT_STATUS, &status);
			if (rc) {
				return rc;
			}
			val->val1 = status;
			val->val2 = 0;

			return 0;
#endif
		default:
			return -ENOTSUP;
		}
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, emc1702_driver_api) = {
	.sample_fetch = emc1702_sample_fetch,
	.channel_get = emc1702_channel_get,
	.attr_set = emc1702_attr_set,
	.attr_get = emc1702_attr_get,
#ifdef CONFIG_EMC1702_TRIGGER
	.trigger_set = emc1702_trigger_set,
#endif
};

/* --- Init --------------------------------------------------------------- */
static int emc1702_init(const struct device *dev)
{
	uint8_t cs_time, cs_range, conv_rate, id, mfr, rev, ext_diode_fault, therm_consec,
		alert_consec, tmp, vol_que, cur_que;
	const struct emc1702_config *cfg = dev->config;
	struct emc1702_data *d = dev->data;
	uint8_t val;
	int rc;

	k_mutex_init(&d->lock);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	rc = emc1702_read8(dev, EMC1702_REG_PRODUCT_ID, &id);
	if (rc) {
		LOG_ERR("Failed to read product ID: %d", rc);
		return rc;
	}
	rc = emc1702_read8(dev, EMC1702_REG_MANUFACTURER_ID, &mfr);
	if (rc) {
		LOG_ERR("Failed to read manufacturer ID: %d", rc);
		return rc;
	}
	rc = emc1702_read8(dev, EMC1702_REG_REVISION, &rev);
	if (rc) {
		LOG_ERR("Failed to read revision: %d", rc);
		return rc;
	}

	if (id != EMC1702_PRODUCT_ID || mfr != EMC1702_MANUFACTURER_ID) {
		LOG_ERR("Wrong chip: pid=0x%02x mfr=0x%02x (expected 0x%02x/0x%02x)", id, mfr,
			EMC1702_PRODUCT_ID, EMC1702_MANUFACTURER_ID);
		return -ENODEV;
	}

	if (cfg->sense_resistor_mohm == 0) {
		LOG_ERR("Rsense is 0");
		return -EINVAL;
	}

	LOG_INF("EMC1702 found (PID=0x%02x MFR=0x%02x REV=0x%02x) @ 0x%02x, "
		"Rsense=%u mOhm, FSR=%u mV, external diode %s",
		id, mfr, rev, cfg->i2c.addr, cfg->sense_resistor_mohm, cfg->cs_range_mv,
		cfg->external_diode ? "enabled" : "disabled");

	if (cfg->rec && !cfg->external_diode) {
		LOG_WRN("resistance-error-correction ignored: no external diode");
	}

	/* Drain any stale status from power-up. */
	rc = emc1702_read8(dev, EMC1702_REG_HIGH_LIMIT_STATUS, &tmp);
	if (rc) {
		LOG_ERR("Failed to drain High Limit Status: %d", rc);
		return rc;
	}
	rc = emc1702_read8(dev, EMC1702_REG_LOW_LIMIT_STATUS, &tmp);
	if (rc) {
		LOG_ERR("Failed to drain Low Limit Status: %d", rc);
		return rc;
	}

	/*
	 * Check the external diode for fault, but only when the board actually
	 * has one. Without a diode DP/DN are open, so the fault bit is always
	 * set and would otherwise abort the whole init.
	 */
	if (cfg->external_diode) {
		rc = emc1702_read8(dev, EMC1702_REG_EXT_DIODE_FAULT, &ext_diode_fault);
		if (rc) {
			return rc;
		}
		if (ext_diode_fault) {
			LOG_ERR("External diode fault detected");
			return -ENODEV;
		}
	}

	d->current_min_valid = -1000000 * DIV_ROUND_UP(cfg->cs_range_mv, cfg->sense_resistor_mohm);
	d->current_max_valid = (-1 * d->current_min_valid) - 1000;

	rc = emc1702_decode_cs_time(cfg->cs_samp_time, &cs_time);
	if (rc) {
		LOG_ERR("Invalid current-sense-sampling-time: %u", cfg->cs_samp_time);
		return rc;
	}
	rc = emc1702_decode_cs_range(cfg->cs_range_mv, &cs_range);
	if (rc) {
		LOG_ERR("Invalid current-sense-range-mv: %u", cfg->cs_range_mv);
		return rc;
	}
	rc = emc1702_decode_conv_rate(cfg->conv_rate, &conv_rate);
	if (rc) {
		LOG_ERR("Invalid conversion-rate: %u", cfg->conv_rate);
		return rc;
	}

	/*
	 * Write Current Sense Configuration register:
	 * - number of consecutive out of limit measurements is taken from dt
	 * - averaging is disabled
	 * - current sense sampling time is taken from dt
	 * - current sense range is taken from dt
	 */
	rc = emc1702_decode_queue(cfg->cs_queue, &cur_que);
	if (rc) {
		LOG_ERR("Invalid Current queue setting: %u", cfg->cs_queue);
		return rc;
	}
	val = FIELD_PREP(EMC1702_CS_SAMPLING_CONFIG_QUEUE, cur_que) |
	      FIELD_PREP(EMC1702_CS_SAMPLING_CONFIG_AVG, 0) |
	      FIELD_PREP(EMC1702_CS_SAMPLING_CONFIG_TIME, cs_time) |
	      FIELD_PREP(EMC1702_CS_SAMPLING_CONFIG_RANGE, cs_range);
	rc = emc1702_write8(dev, EMC1702_REG_CS_SAMPLING_CONFIG, val);
	if (rc) {
		return rc;
	}

	/* Set conversion rate from devicetree */
	rc = emc1702_write8(dev, EMC1702_REG_CONVERSION_RATE, conv_rate);
	if (rc) {
		return rc;
	}

	/*
	 * Write Configuration register:
	 * - all measurements run continuously
	 * - ALERT pin is in Interrupt mode
	 * - REC is taken from dt (only meaningful with an external diode)
	 * - dynamic averaging is disabled
	 */
	val = FIELD_PREP(EMC1702_CONFIG_MASK_ALL, 0) | FIELD_PREP(EMC1702_CONFIG_TMEAS_STOP, 0) |
	      FIELD_PREP(EMC1702_CONFIG_ALERT_COMP, 0) |
	      FIELD_PREP(EMC1702_CONFIG_DIS_REC1, !(cfg->rec && cfg->external_diode)) |
	      FIELD_PREP(EMC1702_CONFIG_IMEAS_STOP, 0) | FIELD_PREP(EMC1702_CONFIG_DAVG_DIS, 1);
	rc = emc1702_write8(dev, EMC1702_REG_CONFIG, val);
	if (rc) {
		return rc;
	}

	/*
	 * Write the Channel Mask register: enable every channel, except when no
	 * external diode is wired. In that case the external channel is masked
	 * so an open diode cannot keep asserting ALERT.
	 */
	rc = emc1702_write8(dev, EMC1702_REG_CHANNEL_MASK,
			    cfg->external_diode ? 0 : EMC1702_MASK_EXT_CHAN);
	if (rc) {
		return rc;
	}

	/*
	 * Write Consecutive Alert register:
	 * - SMBus timeout feature from dt
	 * - Number of consecutive readings for THERM pin to be asserted from dt
	 * - Number of consecutive readings for ALERT pin to be asserted from dt
	 */
	rc = emc1702_decode_alert_consec(cfg->therm_consec, &therm_consec);
	if (rc) {
		LOG_ERR("Invalid THERM consecutive measurements: %u", cfg->therm_consec);
		return rc;
	}
	rc = emc1702_decode_alert_consec(cfg->alert_consec, &alert_consec);
	if (rc) {
		LOG_ERR("Invalid ALERT consecutive measurements: %u", cfg->alert_consec);
		return rc;
	}
	val = FIELD_PREP(EMC1702_CONSEC_ALERT_TIMEOUT, cfg->smb_timeout) |
	      FIELD_PREP(EMC1702_CONSEC_ALERT_CTHRM, therm_consec) |
	      FIELD_PREP(EMC1702_CONSEC_ALERT_CALRT, alert_consec);
	rc = emc1702_write8(dev, EMC1702_REG_CONSECUTIVE_ALERT, val);
	if (rc) {
		return rc;
	}

	/* External-diode-only settings. */
	if (cfg->external_diode) {
		/* Enable beta autodetection in Beta Configuration register*/
		rc = emc1702_write8(dev, EMC1702_REG_BETA_CONFIGURATION,
				    EMC1702_BETA_CONFIGURATION_DEFAULT);
		if (rc) {
			return rc;
		}

		/* Write default value in External Diode Ideality Factor register */
		rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_IDEALITY,
				    EMC1702_EXT_DIODE_IDEALITY_DEFAULT);
		if (rc) {
			return rc;
		}

		/* Disable digital averaging on external diode */
		rc = emc1702_write8(dev, EMC1702_REG_AVERAGING_CONTROL, 0);
		if (rc) {
			return rc;
		}
	}

	/*
	 * Write Voltage Sampling Configuration register:
	 * - peak detector circuitry asserts THERM pin
	 * - number of consecutive out of limit measurements is taken from dt
	 * - voltage averaging is disabled
	 */
	rc = emc1702_decode_queue(cfg->volt_queue, &vol_que);
	if (rc) {
		LOG_ERR("Invalid Voltage queue setting: %u", cfg->volt_queue);
		return rc;
	}
	val = FIELD_PREP(EMC1702_V_SAMPLING_CONFIG_PK_ALERT, 1) |
	      FIELD_PREP(EMC1702_V_SAMPLING_CONFIG_QUEUE, vol_que) |
	      FIELD_PREP(EMC1702_V_SAMPLING_CONFIG_AVG, 0);
	rc = emc1702_write8(dev, EMC1702_REG_V_SAMPLING_CONFIG, val);
	if (rc) {
		return rc;
	}

	/* Write default values in temperature limit registers */
	rc = emc1702_write8(dev, EMC1702_REG_INTERNAL_DIODE_H_LIMIT,
			    EMC1702_HIGH_LIMIT_TEMP_VALUE_DEFAULT);
	if (rc) {
		return rc;
	}
	rc = emc1702_write8(dev, EMC1702_REG_INTERNAL_DIODE_L_LIMIT,
			    EMC1702_LOW_LIMIT_TEMP_VALUE_DEFAULT);
	if (rc) {
		return rc;
	}

	/* Critical (THERM) limit and the hysteresis shared by both channels */
	rc = emc1702_write8(dev, EMC1702_REG_INT_DIODE_TEMP_CRIT_LIMIT,
			    EMC1702_CRIT_LIMIT_TEMP_VALUE_DEFAULT);
	if (rc) {
		return rc;
	}
	rc = emc1702_write8(dev, EMC1702_REG_TEMP_HYST, EMC1702_TEMP_CRIT_HYST_DEFAULT);
	if (rc) {
		return rc;
	}

	if (cfg->external_diode) {
		rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_TEMP_CRIT_LIMIT,
				    EMC1702_CRIT_LIMIT_TEMP_VALUE_DEFAULT);
		if (rc) {
			return rc;
		}
		rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_H_LIMIT_H_BYTE,
				    EMC1702_HIGH_LIMIT_TEMP_VALUE_DEFAULT);
		if (rc) {
			return rc;
		}
		rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_H_LIMIT_L_BYTE, 0);
		if (rc) {
			return rc;
		}

		rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_L_LIMIT_H_BYTE,
				    EMC1702_LOW_LIMIT_TEMP_VALUE_DEFAULT);
		if (rc) {
			return rc;
		}
		rc = emc1702_write8(dev, EMC1702_REG_EXT_DIODE_L_LIMIT_L_BYTE, 0);
		if (rc) {
			return rc;
		}
	}

	/* Write default values in current limit registers */
	rc = emc1702_write8(dev, EMC1702_REG_SENSE_VOLTAGE_HIGH_LIMIT,
			    EMC1702_HIGH_LIMIT_CURRENT_DEFAULT);
	if (rc) {
		return rc;
	}

	rc = emc1702_write8(dev, EMC1702_REG_SENSE_VOLTAGE_LOW_LIMIT,
			    EMC1702_LOW_LIMIT_CURRENT_DEFAULT);
	if (rc) {
		return rc;
	}

	rc = emc1702_write8(dev, EMC1702_REG_SENSE_VOLTAGE_CRIT_LIMIT,
			    EMC1702_CRIT_LIMIT_CURRENT_DEFAULT);
	if (rc) {
		return rc;
	}
	rc = emc1702_write8(dev, EMC1702_REG_SENSE_VOLTAGE_CRIT_HYST,
			    EMC1702_VOLT_CRIT_HYST_DEFAULT);
	if (rc) {
		return rc;
	}

	/* Write default values in voltage limit registers */
	rc = emc1702_write8(dev, EMC1702_REG_SOURCE_VOLTAGE_HIGH_LIMIT,
			    EMC1702_HIGH_LIMIT_VOLTAGE_DEFAULT);
	if (rc) {
		return rc;
	}

	rc = emc1702_write8(dev, EMC1702_REG_SOURCE_VOLTAGE_LOW_LIMIT, 0);
	if (rc) {
		return rc;
	}

	rc = emc1702_write8(dev, EMC1702_REG_SOURCE_VOLTAGE_CRIT_LIMIT,
			    EMC1702_CRIT_LIMIT_VOLTAGE_DEFAULT);
	if (rc) {
		return rc;
	}
	rc = emc1702_write8(dev, EMC1702_REG_SOURCE_VOLTAGE_CRIT_HYST,
			    EMC1702_VOLT_CRIT_HYST_DEFAULT);
	if (rc) {
		return rc;
	}

#ifdef CONFIG_EMC1702_TRIGGER
	rc = emc1702_trigger_init(dev);
	if (rc) {
		return rc;
	}
#endif

	return 0;
}

/* --- Devicetree instantiation ------------------------------------------ */

#ifdef CONFIG_EMC1702_TRIGGER
#define EMC1702_GPIO_DECL(inst) .alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),
#else
#define EMC1702_GPIO_DECL(inst)
#endif

#define EMC1702_INIT(inst)                                                                         \
	static struct emc1702_data emc1702_data_##inst;                                            \
	static const struct emc1702_config emc1702_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.sense_resistor_mohm = DT_INST_PROP(inst, sense_resistor_milliohms),               \
		.cs_range_mv = DT_INST_PROP(inst, current_sense_range_mv),                         \
		.cs_samp_time = DT_INST_PROP(inst, current_sense_sampling_time),                   \
		.cs_queue = DT_INST_PROP(inst, current_sense_queue),                               \
		.volt_queue = DT_INST_PROP(inst, voltage_source_queue),                            \
		.rec = DT_INST_PROP(inst, resistance_error_correction),                            \
		.conv_rate = DT_INST_PROP(inst, conversion_rate),                                  \
		.therm_consec = DT_INST_PROP(inst, therm_consecutive),                             \
		.alert_consec = DT_INST_PROP(inst, alert_consecutive),                             \
		.smb_timeout = DT_INST_PROP(inst, smbus_timeout),                                  \
		.external_diode = DT_INST_PROP(inst, external_diode_is_present),                   \
		EMC1702_GPIO_DECL(inst)};                                                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, emc1702_init, NULL, &emc1702_data_##inst,               \
				     &emc1702_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &emc1702_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EMC1702_INIT)
