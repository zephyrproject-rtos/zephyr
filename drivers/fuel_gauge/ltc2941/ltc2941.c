/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ltc2941

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ltc2941.h"

LOG_MODULE_REGISTER(LTC2941, CONFIG_FUEL_GAUGE_LOG_LEVEL);

struct ltc2941_config {
	struct i2c_dt_spec i2c;
	uint32_t rsense_milliohms;
	uint32_t design_capacity_uah;
	uint8_t prescaler;
};

static inline uint32_t u64_div_round_closest_u32_sat(uint64_t n, uint32_t d)
{
	uint64_t q;

	if (d == 0U) {
		return UINT32_MAX;
	}

	q = (n + (uint64_t)(d / 2U)) / d;

	return (q > UINT32_MAX) ? UINT32_MAX : (uint32_t)q;
}

static uint32_t ltc2941_counts_to_uah(uint16_t counts, const struct ltc2941_config *cfg)
{
	/* uAh = counts * 85 * 50 * M / (Rsense * 128) */
	uint64_t prod = (uint64_t)counts * (uint64_t)LTC2941_QLSB_UAH *
			(uint64_t)LTC2941_RSENSE_REF_MOHM * (uint64_t)cfg->prescaler;
	uint32_t den = cfg->rsense_milliohms * LTC2941_PRESCALER_REF;

	return u64_div_round_closest_u32_sat(prod, den);
}

static uint16_t ltc2941_uah_to_counts(uint32_t uah, const struct ltc2941_config *cfg)
{
	uint64_t prod;
	uint32_t den;
	uint32_t counts;

	/* counts = uAh * Rsense * 128 / (85 * 50 * M) */
	prod = (uint64_t)uah * (uint64_t)cfg->rsense_milliohms * (uint64_t)LTC2941_PRESCALER_REF;
	den = LTC2941_QLSB_UAH * LTC2941_RSENSE_REF_MOHM * cfg->prescaler;
	counts = u64_div_round_closest_u32_sat(prod, den);

	return (counts > LTC2941_ACC_CHARGE_FULL) ? (uint16_t)LTC2941_ACC_CHARGE_FULL
						  : (uint16_t)counts;
}

static uint32_t ltc2941_full_scale_uah(const struct ltc2941_config *cfg)
{
	return ltc2941_counts_to_uah(LTC2941_ACC_CHARGE_FULL, cfg);
}

static int ltc2941_read16(const struct device *dev, uint8_t reg, uint16_t *value)
{
	uint8_t buf[2];
	const struct ltc2941_config *cfg = dev->config;
	int ret = i2c_burst_read_dt(&cfg->i2c, reg, buf, sizeof(buf));

	if (ret < 0) {
		LOG_ERR("Failed to read register 0x%02X", reg);
		return ret;
	}

	*value = sys_get_be16(buf);
	return 0;
}

static int ltc2941_write16(const struct device *dev, uint8_t reg, uint16_t value)
{
	uint8_t buf[3];
	const struct ltc2941_config *cfg = dev->config;

	buf[0] = reg;
	sys_put_be16(value, &buf[1]);

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int ltc2941_write_acr(const struct device *dev, uint16_t counts)
{
	const struct ltc2941_config *cfg = dev->config;
	uint8_t ctrl;
	int ret;

	/* Datasheet: shut analog section down before writing accumulated charge. */
	ret = i2c_reg_read_byte_dt(&cfg->i2c, LTC2941_REG_CONTROL, &ctrl);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, LTC2941_REG_CONTROL, ctrl | LTC2941_CTRL_SHUTDOWN);
	if (ret < 0) {
		return ret;
	}

	ret = ltc2941_write16(dev, LTC2941_REG_ACC_CHARGE_MSB, counts);
	if (ret < 0) {
		(void)i2c_reg_write_byte_dt(&cfg->i2c, LTC2941_REG_CONTROL, ctrl);
		return ret;
	}

	return i2c_reg_write_byte_dt(&cfg->i2c, LTC2941_REG_CONTROL,
				     ctrl & (uint8_t)~LTC2941_CTRL_SHUTDOWN);
}

static uint8_t ltc2941_soc_pct(uint32_t remaining_uah, uint32_t full_uah)
{
	uint64_t pct;

	if (full_uah == 0U) {
		return 0U;
	}

	if (remaining_uah >= full_uah) {
		return 100U;
	}

	pct = ((uint64_t)remaining_uah * 100ULL + (full_uah / 2U)) / full_uah;

	return (pct > 100U) ? 100U : (uint8_t)pct;
}

static int ltc2941_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val *val)
{
	const struct ltc2941_config *cfg = dev->config;
	int ret;

	switch (prop) {
	case FUEL_GAUGE_STATUS:
	case FUEL_GAUGE_FLAGS: {
		uint8_t status;

		ret = i2c_reg_read_byte_dt(&cfg->i2c, LTC2941_REG_STATUS, &status);
		if (ret < 0) {
			return ret;
		}

		if (prop == FUEL_GAUGE_STATUS) {
			val->fg_status = status;
		} else {
			val->flags = status;
		}
		break;
	}
	case FUEL_GAUGE_CC_CONFIG:
		ret = i2c_reg_read_byte_dt(&cfg->i2c, LTC2941_REG_CONTROL, &val->cc_config);
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY_UAH: {
		uint16_t counts;

		ret = ltc2941_read16(dev, LTC2941_REG_ACC_CHARGE_MSB, &counts);
		if (ret < 0) {
			return ret;
		}

		val->remaining_capacity_uah = ltc2941_counts_to_uah(counts, cfg);
		break;
	}
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH:
		if (cfg->design_capacity_uah != 0U) {
			val->full_charge_capacity_uah = cfg->design_capacity_uah;
		} else {
			val->full_charge_capacity_uah = ltc2941_full_scale_uah(cfg);
		}
		ret = 0;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		if (cfg->design_capacity_uah == 0U) {
			return -ENOTSUP;
		}

		val->design_cap = (uint16_t)(cfg->design_capacity_uah / 1000U);
		ret = 0;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT:
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT: {
		uint16_t counts;
		uint32_t remaining;
		uint32_t full;

		ret = ltc2941_read16(dev, LTC2941_REG_ACC_CHARGE_MSB, &counts);
		if (ret < 0) {
			return ret;
		}

		remaining = ltc2941_counts_to_uah(counts, cfg);
		full = (cfg->design_capacity_uah != 0U) ? cfg->design_capacity_uah
							: ltc2941_full_scale_uah(cfg);

		if (prop == FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT) {
			val->relative_state_of_charge_pct = ltc2941_soc_pct(remaining, full);
		} else {
			val->absolute_state_of_charge_pct = ltc2941_soc_pct(remaining, full);
		}
		break;
	}
	default:
		return -ENOTSUP;
	}

	return ret;
}

static int ltc2941_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val val)
{
	const struct ltc2941_config *cfg = dev->config;
	int ret;

	switch (prop) {
	case FUEL_GAUGE_CC_CONFIG: {
		uint8_t alcc = val.cc_config & LTC2941_CTRL_ALCC_MASK;

		if (alcc == (LTC2941_ALCC_ALERT | LTC2941_ALCC_CHARGE_COMPLETE)) {
			return -EINVAL;
		}

		ret = i2c_reg_write_byte_dt(&cfg->i2c, LTC2941_REG_CONTROL, val.cc_config);
		break;
	}
	case FUEL_GAUGE_REMAINING_CAPACITY_UAH: {
		uint16_t counts = ltc2941_uah_to_counts(val.remaining_capacity_uah, cfg);

		ret = ltc2941_write_acr(dev, counts);
		break;
	}
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int ltc2941_init(const struct device *dev)
{
	const struct ltc2941_config *cfg = dev->config;
	uint8_t status;
	uint8_t ctrl;
	uint8_t prescaler_field;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, LTC2941_REG_STATUS, &status);
	if (ret < 0) {
		LOG_ERR("Failed to read status");
		return ret;
	}

	if ((status & LTC2941_STATUS_CHIP_ID) == 0U) {
		LOG_ERR("Chip ID is not LTC2941 (status=0x%02x)", status);
		return -ENODEV;
	}

	if ((status & LTC2941_STATUS_UVLO) != 0U) {
		uint16_t acr;

		ret = ltc2941_read16(dev, LTC2941_REG_ACC_CHARGE_MSB, &acr);
		if (ret < 0) {
			return ret;
		}

		/*
		 * A rising SENSE+ supply can latch UVLO. ACR at the datasheet
		 * POR value 0x7FFF is expected. Warn only when a brown-out
		 * may have corrupted a previously programmed count.
		 */
		if (acr != LTC2941_ACC_CHARGE_POR) {
			LOG_WRN("UVLO latched; accumulated charge 0x%04x may be uncertain", acr);
		} else {
			LOG_DBG("UVLO after POR (ACR=0x7FFF)");
		}
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, LTC2941_REG_CONTROL, &ctrl);
	if (ret < 0) {
		return ret;
	}

	/* M = 2^(B[5:3]); field is log2(prescaler). */
	prescaler_field = (uint8_t)(find_msb_set(cfg->prescaler) - 1);
	ctrl &= (uint8_t)~(LTC2941_CTRL_PRESCALER_MASK | LTC2941_CTRL_SHUTDOWN);
	ctrl |= FIELD_PREP(LTC2941_CTRL_PRESCALER_MASK, prescaler_field);

	ret = i2c_reg_write_byte_dt(&cfg->i2c, LTC2941_REG_CONTROL, ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to write control register");
		return ret;
	}

	return 0;
}

static DEVICE_API(fuel_gauge, ltc2941_driver_api) = {
	.get_property = &ltc2941_get_prop,
	.set_property = &ltc2941_set_prop,
};

#define LTC2941_DEFINE(inst)                                                                       \
	BUILD_ASSERT(DT_INST_PROP(inst, rsense_milliohms) > 0);                                    \
	BUILD_ASSERT(IS_POWER_OF_TWO(DT_INST_PROP(inst, prescaler)));                              \
	BUILD_ASSERT(DT_INST_PROP(inst, prescaler) <= 128);                                        \
	static const struct ltc2941_config ltc2941_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.rsense_milliohms = DT_INST_PROP(inst, rsense_milliohms),                          \
		.prescaler = DT_INST_PROP(inst, prescaler),                                        \
		.design_capacity_uah = DT_INST_PROP_OR(inst, design_capacity_microamp_hours, 0),   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ltc2941_init, NULL, NULL, &ltc2941_config_##inst, POST_KERNEL, \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &ltc2941_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LTC2941_DEFINE)
