/*
 * Copyright (c) 2026 Federico Borgo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ltc3337

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ltc3337.h"

LOG_MODULE_REGISTER(LTC3337, CONFIG_FUEL_GAUGE_LOG_LEVEL);

struct ltc3337_config {
	struct i2c_dt_spec i2c;
	uint8_t ipeak_milliamp;
};

struct ltc3337_data {
	struct k_mutex lock;
};

/*
 * Charge of one counter LSB with the prescaler at 0, in nAh, for each IPEAK setting. Indexed
 * like the IPK[2:0] pins (datasheet Table 1); the charge is halved for every prescaler step.
 */
static const struct {
	uint8_t ipeak_milliamp;
	uint32_t qlsb_nah;
} ltc3337_ipeak[] = {
	{5U, 745700U},   {10U, 1491000U}, {15U, 2237000U},  {20U, 2983000U},
	{25U, 3728000U}, {50U, 7457000U}, {75U, 11180000U}, {100U, 14910000U},
};

static int ltc3337_ipeak_index(uint8_t ipeak_milliamp)
{
	for (size_t i = 0; i < ARRAY_SIZE(ltc3337_ipeak); i++) {
		if (ltc3337_ipeak[i].ipeak_milliamp == ipeak_milliamp) {
			return (int)i;
		}
	}

	return -EINVAL;
}

static int ltc3337_read_reg(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct ltc3337_config *cfg = dev->config;
	uint8_t buf[2];
	int ret;

	ret = i2c_write_read_dt(&cfg->i2c, &reg, sizeof(reg), buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	*val = sys_get_le16(buf);

	return 0;
}

static int ltc3337_write_reg(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct ltc3337_config *cfg = dev->config;
	uint8_t buf[3] = {reg};

	sys_put_le16(val, &buf[1]);

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

/* The strobe bits are never part of the configuration, even if they read back as set */
static int ltc3337_read_reg_a(const struct device *dev, uint16_t *val)
{
	int ret = ltc3337_read_reg(dev, LTC3337_REG_A, val);

	if (ret == 0) {
		*val &= ~(LTC3337_A_CLEAR_INT | LTC3337_A_ADC_TRIGGER);
	}

	return ret;
}

static inline uint32_t u64_div_round_closest_u32_sat(uint64_t n, uint32_t d)
{
	/* round-to-nearest: (n + d/2) / d, with saturation to u32 */
	uint64_t q = (n + (uint64_t)(d / 2U)) / d;

	return (q > UINT32_MAX) ? UINT32_MAX : (uint32_t)q;
}

static uint32_t ltc3337_charge_to_uah(const struct device *dev, uint16_t counts, uint8_t prescaler)
{
	const struct ltc3337_config *cfg = dev->config;
	uint64_t prod =
		(uint64_t)counts * ltc3337_ipeak[ltc3337_ipeak_index(cfg->ipeak_milliamp)].qlsb_nah;

	/* uAh = counts * qLSB (nAh) / 2^prescaler / 1000 */
	return u64_div_round_closest_u32_sat(prod, 1000U << prescaler);
}

static int ltc3337_uah_to_charge(const struct device *dev, uint32_t uah, uint8_t prescaler,
				 uint16_t *counts)
{
	const struct ltc3337_config *cfg = dev->config;
	uint32_t qlsb_nah = ltc3337_ipeak[ltc3337_ipeak_index(cfg->ipeak_milliamp)].qlsb_nah;
	uint32_t raw;

	/* counts = uAh * 1000 * 2^prescaler / qLSB (nAh) */
	raw = u64_div_round_closest_u32_sat(((uint64_t)uah * 1000U) << prescaler, qlsb_nah);
	if (raw > UINT16_MAX) {
		return -ERANGE;
	}

	*counts = (uint16_t)raw;

	return 0;
}

static uint16_t ltc3337_raw_to_temp_dk(uint8_t raw)
{
	int32_t mdegc = ((int32_t)raw * LTC3337_TEMP_LSB_MDEGC) + LTC3337_TEMP_ZERO_MDEGC;

	/* Round to the nearest 0.1 K */
	return (uint16_t)((mdegc - LTC3337_KELVIN_ZERO_MDEGC + (LTC3337_MDEGC_PER_DK / 2)) /
			  LTC3337_MDEGC_PER_DK);
}

static int ltc3337_temp_dk_to_raw(uint16_t dk, uint8_t *raw)
{
	int32_t mdegc = ((int32_t)dk * LTC3337_MDEGC_PER_DK) + LTC3337_KELVIN_ZERO_MDEGC;
	int32_t counts = (mdegc - LTC3337_TEMP_ZERO_MDEGC + (LTC3337_TEMP_LSB_MDEGC / 2)) /
			 LTC3337_TEMP_LSB_MDEGC;

	if ((mdegc < LTC3337_TEMP_ZERO_MDEGC) || (counts > LTC3337_DIE_TEMP_FULLSCALE)) {
		return -ERANGE;
	}

	*raw = (uint8_t)counts;

	return 0;
}

static int ltc3337_get_status(const struct device *dev, uint16_t *status)
{
	uint16_t reg_c;
	int ret;

	ret = ltc3337_read_reg(dev, LTC3337_REG_C, &reg_c);
	if (ret != 0) {
		return ret;
	}

	*status = FIELD_GET(LTC3337_C_STATUS, reg_c);

	/*
	 * The alarm flags are latched and hold IRQ low until acknowledged. Reading them
	 * acknowledges them, as it does on the other fuel gauges.
	 */
	if ((reg_c & LTC3337_C_ALARMS) != 0U) {
		uint16_t reg_a;

		ret = ltc3337_read_reg_a(dev, &reg_a);
		if (ret == 0) {
			ret = ltc3337_write_reg(dev, LTC3337_REG_A, reg_a | LTC3337_A_CLEAR_INT);
		}
	}

	return ret;
}

static int ltc3337_get_temp_alarm(const struct device *dev, bool high, uint16_t *dk)
{
	uint16_t mask = high ? LTC3337_H_HOT_MASK : LTC3337_H_COLD_MASK;
	uint16_t reg_h;
	int ret;

	ret = ltc3337_read_reg(dev, LTC3337_REG_H, &reg_h);
	if (ret == 0) {
		*dk = ltc3337_raw_to_temp_dk(FIELD_GET(mask, reg_h));
	}

	return ret;
}

static int ltc3337_set_temp_alarm(const struct device *dev, bool high, uint16_t dk)
{
	uint16_t mask = high ? LTC3337_H_HOT_MASK : LTC3337_H_COLD_MASK;
	uint16_t reg_h;
	uint8_t raw;
	int ret;

	ret = ltc3337_temp_dk_to_raw(dk, &raw);
	if (ret != 0) {
		return ret;
	}

	ret = ltc3337_read_reg(dev, LTC3337_REG_H, &reg_h);
	if (ret != 0) {
		return ret;
	}

	return ltc3337_write_reg(dev, LTC3337_REG_H, (reg_h & ~mask) | FIELD_PREP(mask, raw));
}

/*
 * With the coulomb counter shut down (A[6]) the voltage and temperature results are only updated
 * by a conversion started through A[7], which the driver does not start. The stored results are
 * then stale, so report that no measurement is available instead of returning them.
 */
static int ltc3337_check_measurement(const struct device *dev)
{
	uint16_t reg_a;
	int ret;

	ret = ltc3337_read_reg_a(dev, &reg_a);
	if (ret != 0) {
		return ret;
	}

	return ((reg_a & LTC3337_A_SHUTDOWN) != 0U) ? -ENODATA : 0;
}

static int ltc3337_get_prop_locked(const struct device *dev, fuel_gauge_prop_t prop,
				   union fuel_gauge_prop_val *val)
{
	uint16_t raw;
	uint16_t reg_a;
	int ret;

	switch (prop) {
	case FUEL_GAUGE_STATUS:
		return ltc3337_get_status(dev, &val->fg_status);
	case FUEL_GAUGE_VOLTAGE_UV:
		ret = ltc3337_check_measurement(dev);
		if (ret != 0) {
			return ret;
		}
		/* BAT_IN sampled while the current source is off: the unloaded battery voltage */
		ret = ltc3337_read_reg(dev, LTC3337_REG_E, &raw);
		if (ret == 0) {
			val->voltage_uv =
				FIELD_GET(LTC3337_VOLTAGE_MASK, raw) * LTC3337_VOLTAGE_LSB_UV;
		}
		return ret;
	case FUEL_GAUGE_TEMPERATURE_DK:
		ret = ltc3337_check_measurement(dev);
		if (ret != 0) {
			return ret;
		}
		ret = ltc3337_read_reg(dev, LTC3337_REG_C, &raw);
		if (ret == 0) {
			val->temperature_dk =
				ltc3337_raw_to_temp_dk(FIELD_GET(LTC3337_C_TEMP_MASK, raw));
		}
		return ret;
	case FUEL_GAUGE_CONSUMED_CAPACITY_UAH:
		ret = ltc3337_read_reg_a(dev, &reg_a);
		if (ret != 0) {
			return ret;
		}
		ret = ltc3337_read_reg(dev, LTC3337_REG_B, &raw);
		if (ret == 0) {
			val->consumed_capacity_uah = ltc3337_charge_to_uah(
				dev, raw, FIELD_GET(LTC3337_A_PRESCALER_MASK, reg_a));
		}
		return ret;
	case FUEL_GAUGE_CC_CONFIG:
		ret = ltc3337_read_reg_a(dev, &reg_a);
		if (ret == 0) {
			val->cc_config = reg_a & LTC3337_CC_MASK;
		}
		return ret;
	case FUEL_GAUGE_LOW_TEMPERATURE_ALARM_DK:
		return ltc3337_get_temp_alarm(dev, false, &val->low_temperature_alarm_dk);
	case FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK:
		return ltc3337_get_temp_alarm(dev, true, &val->high_temperature_alarm_dk);
	default:
		return -ENOTSUP;
	}
}

static int ltc3337_set_prop_locked(const struct device *dev, fuel_gauge_prop_t prop,
				   union fuel_gauge_prop_val val)
{
	uint16_t counts;
	uint16_t reg_a;
	int ret;

	switch (prop) {
	case FUEL_GAUGE_CONSUMED_CAPACITY_UAH:
		ret = ltc3337_read_reg_a(dev, &reg_a);
		if (ret != 0) {
			return ret;
		}
		/* Only the upper byte of the counter can be preloaded */
		ret = ltc3337_uah_to_charge(dev, val.consumed_capacity_uah,
					    FIELD_GET(LTC3337_A_PRESCALER_MASK, reg_a), &counts);
		if (ret != 0) {
			return ret;
		}
		return ltc3337_write_reg(dev, LTC3337_REG_B, counts & LTC3337_B_WRITABLE_MASK);
	case FUEL_GAUGE_CC_CONFIG:
		if (((val.cc_config & ~LTC3337_CC_MASK) != 0U) ||
		    (FIELD_GET(LTC3337_CC_PRESCALER_MASK, val.cc_config) > LTC3337_PRESCALER_MAX)) {
			return -EINVAL;
		}
		ret = ltc3337_read_reg_a(dev, &reg_a);
		if (ret != 0) {
			return ret;
		}
		return ltc3337_write_reg(dev, LTC3337_REG_A,
					 (reg_a & ~LTC3337_CC_MASK) | val.cc_config);
	case FUEL_GAUGE_LOW_TEMPERATURE_ALARM_DK:
		return ltc3337_set_temp_alarm(dev, false, val.low_temperature_alarm_dk);
	case FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK:
		return ltc3337_set_temp_alarm(dev, true, val.high_temperature_alarm_dk);
	default:
		return -ENOTSUP;
	}
}

static int ltc3337_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val *val)
{
	struct ltc3337_data *data = dev->data;
	int ret;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	ret = ltc3337_get_prop_locked(dev, prop, val);
	(void)k_mutex_unlock(&data->lock);

	return ret;
}

static int ltc3337_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val val)
{
	struct ltc3337_data *data = dev->data;
	int ret;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	ret = ltc3337_set_prop_locked(dev, prop, val);
	(void)k_mutex_unlock(&data->lock);

	return ret;
}

static int ltc3337_init(const struct device *dev)
{
	const struct ltc3337_config *cfg = dev->config;
	struct ltc3337_data *data = dev->data;
	uint16_t reg_c;
	uint8_t strap_milliamp;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	ret = ltc3337_read_reg(dev, LTC3337_REG_C, &reg_c);
	if (ret != 0) {
		LOG_ERR("Failed to read the status register (%d)", ret);
		return ret;
	}

	/* The IPK[2:0] pins read back with the same encoding as the table index */
	strap_milliamp = ltc3337_ipeak[FIELD_GET(LTC3337_C_IPK_MASK, reg_c)].ipeak_milliamp;
	if (strap_milliamp != cfg->ipeak_milliamp) {
		LOG_WRN("ipeak-milliamp is %u mA but the IPK pins select %u mA",
			cfg->ipeak_milliamp, strap_milliamp);
	}

	/*
	 * The chip is powered by the battery, so its configuration, including the prescaler that
	 * gives the accumulated charge its meaning, survives a reset of the host: leave it alone.
	 */
	return 0;
}

static DEVICE_API(fuel_gauge, ltc3337_driver_api) = {
	.get_property = ltc3337_get_prop,
	.set_property = ltc3337_set_prop,
};

#define LTC3337_DEFINE(inst)                                                                       \
	static struct ltc3337_data ltc3337_data_##inst;                                            \
	static const struct ltc3337_config ltc3337_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.ipeak_milliamp = DT_INST_PROP(inst, ipeak_milliamp),                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ltc3337_init, NULL, &ltc3337_data_##inst,                      \
			      &ltc3337_config_##inst, POST_KERNEL,                                 \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &ltc3337_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LTC3337_DEFINE)
