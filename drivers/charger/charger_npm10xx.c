/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm10xx_charger

#include <math.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/charger/npm10xx.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/linear_range.h>

LOG_MODULE_REGISTER(npm10xx_charger, CONFIG_CHARGER_LOG_LEVEL);

/* nPM10 MAIN register offsets */
#define NPM10_MAIN_STATUS 0x1EU

/* MAIN registers bit fields */
#define MAIN_STATUS_VBUSPRESENT_Msk (BIT_MASK(1U) << 1)
#define MAIN_STATUS_VBUSGOOD_Msk    (BIT_MASK(1U) << 2)
#define MAIN_STATUS_VBUSOVER_Msk    (BIT_MASK(1U) << 3)
#define MAIN_STATUS_VBUSUNDER_Msk   (BIT_MASK(1U) << 4)

/* nPM10 SYSREG register offsets and bit fields */
#define NPM10_SYS_VBUSILIM   0x28U
#define NPM10_SYS_VBUSDPM    0x29U
#define SYS_VBUSILIM_LVL_Msk (BIT_MASK(2) << 0)
#define SYS_VBUSDPM_ENABLE   BIT(0)
#define SYS_VBUSDPM_LVL_Msk  (BIT_MASK(2) << 1)

/* nPM10 CHARGER register offsets */
#define NPM10_CHRG_TASKS             0x38U
#define NPM10_CHRG_ENABLE            0x39U
#define NPM10_CHRG_SET               0x3AU
#define NPM10_CHRG_CLR               0x3BU
#define NPM10_CHRG_ISET              0x3CU
#define NPM10_CHRG_ITERM             0x40U
#define NPM10_CHRG_ITRICKLE          0x41U
#define NPM10_CHRG_VTERM             0x42U
#define NPM10_CHRG_VTRICKLE          0x45U
#define NPM10_CHRG_VWEAK             0x46U
#define NPM10_CHRG_THROTTLE          0x47U
#define NPM10_CHRG_TIMEOUT           0x48U
#define NPM10_CHRG_VBATOV            0x49U
#define NPM10_CHRG_COMPARATORSTATUS  0x4FU
#define NPM10_CHRG_STATUS            0x50U
#define NPM10_CHRG_ERRORREASON       0x53U
#define NPM10_CHRG_ERRORSENSOR       0x54U
#define NPM10_CHRG_NTCMSB            0x56U
#define NPM10_CHRG_DIETEMPREDUCEMSB  0x5EU
#define NPM10_CHRG_TEMPERATURESTATUS 0x62U

/* CHARGER registers bit field masks */
/* TASKS (0x38) */
#define CHRG_TASKS_RELEASE_Msk   (BIT_MASK(1U) << 0)
#define CHRG_TASKS_CLRERROR_Msk  (BIT_MASK(1U) << 1)
#define CHRG_TASKS_CLRSAFETY_Msk (BIT_MASK(1U) << 2)

/* ENABLE (0x39) */
#define CHRG_ENABLE_CHARGE_Msk     (BIT_MASK(1U) << 0)
#define CHRG_ENABLE_JEITA_Msk      (BIT_MASK(1U) << 1)
#define CHRG_ENABLE_VBATLOWCHG_Msk (BIT_MASK(1U) << 2)
#define CHRG_ENABLE_VBATLOWLVL_Msk (BIT_MASK(1U) << 3)
#define CHRG_ENABLE_WEAKCHG_Msk    (BIT_MASK(1U) << 4)
#define CHRG_ENABLE_COOLCHG_Msk    (BIT_MASK(1U) << 5)
#define CHRG_ENABLE_WARMCHG_Msk    (BIT_MASK(1U) << 6)
#define CHRG_ENABLE_ISETDOUBLE_Msk (BIT_MASK(1U) << 7)

/* SET (0x3A) */
#define CHRG_SET_RECHARGE_Msk    (BIT_MASK(1U) << 0)
#define CHRG_SET_NTC_Msk         (BIT_MASK(1U) << 1)
#define CHRG_SET_SINK_Msk        (BIT_MASK(1U) << 2)
#define CHRG_SET_ILIMBAT_Msk     (BIT_MASK(1U) << 3)
#define CHRG_SET_THROTTLECHG_Msk (BIT_MASK(1U) << 4)

/* CLR (0x3B) */
#define CHRG_CLR_RECHARGE_Msk    (BIT_MASK(1U) << 0)
#define CHRG_CLR_NTC_Msk         (BIT_MASK(1U) << 1)
#define CHRG_CLR_SINK_Msk        (BIT_MASK(1U) << 2)
#define CHRG_CLR_ILIMBAT_Msk     (BIT_MASK(1U) << 3)
#define CHRG_CLR_THROTTLECHG_Msk (BIT_MASK(1U) << 4)

/* ITERM (0x40) */
#define CHRG_ITERM_LVL_Msk (BIT_MASK(3U) << 0)

/* ITRICKLE (0x41) */
#define CHRG_ITRICKLE_LVL_Msk (BIT_MASK(3U) << 0)

/* VTERM (0x42) */
#define CHRG_VTERM_LVL_Msk (BIT_MASK(7U) << 0)

/* VTRICKLE (0x45) */
#define CHRG_VTRICKLE_LVL_Msk (BIT_MASK(2U) << 0)

/* THROTTLE (0x47) */
#define CHRG_THROTTLE_VLVL_Msk (BIT_MASK(4U) << 0)
#define CHRG_THROTTLE_ILVL_Msk (BIT_MASK(3U) << 4)

/* TIMEOUT (0x48) */
#define CHRG_TIMEOUT_TRICKLE_Msk (BIT_MASK(4U) << 0)
#define CHRG_TIMEOUT_CHARGE_Msk  (BIT_MASK(4U) << 4)

/* VBATOV (0x49) */
#define CHRG_VBATOV_EN_Msk (BIT_MASK(1U) << 0)

/* COMPARATORSTATUS (0x4F) */
#define CHRG_COMPSTAT_VBATLOW_Msk   (BIT_MASK(1U) << 0)
#define CHRG_COMPSTAT_VTRICKLE_Msk  (BIT_MASK(1U) << 1)
#define CHRG_COMPSTAT_VTHROTTLE_Msk (BIT_MASK(1U) << 2)
#define CHRG_COMPSTAT_VRECHARGE_Msk (BIT_MASK(1U) << 3)
#define CHRG_COMPSTAT_VTERM_Msk     (BIT_MASK(1U) << 4)
#define CHRG_COMPSTAT_VWEAK_Msk     (BIT_MASK(1U) << 5)
#define CHRG_COMPSTAT_ITHROTTLE_Msk (BIT_MASK(1U) << 6)
#define CHRG_COMPSTAT_ITERM_Msk     (BIT_MASK(1U) << 7)

/* STATUS (0x50) */
#define CHRG_STATUS_STATE_Msk         (BIT_MASK(3U) << 0)
#define CHRG_STATUS_STATE_IDLE        0U
#define CHRG_STATUS_STATE_TRICKLE     1U
#define CHRG_STATUS_STATE_FAST        2U
#define CHRG_STATUS_STATE_THROTTLE    3U
#define CHRG_STATUS_STATE_COMPLETED   4U
#define CHRG_STATUS_STATE_LOWVTERM    5U
#define CHRG_STATUS_STATE_DISCHARGING 6U
#define CHRG_STATUS_STATE_ERROR       7U
#define CHRG_STATUS_DIETEMP_Msk       (BIT_MASK(1U) << 3)
#define CHRG_STATUS_SUPPLEMENT_Msk    (BIT_MASK(1U) << 4)
#define CHRG_STATUS_DROPOUT_Msk       (BIT_MASK(1U) << 5)
#define CHRG_STATUS_ILIMDISCHARGE_Msk (BIT_MASK(1U) << 7)

/* ERRORREASON (0x53) */
#define CHRG_ERRORREASON_VBATLOW_Msk        (BIT_MASK(1U) << 0)
#define CHRG_ERRORREASON_CHARGETIMEOUT_Msk  (BIT_MASK(1U) << 1)
#define CHRG_ERRORREASON_TRICKLETIMEOUT_Msk (BIT_MASK(1U) << 2)
#define CHRG_ERRORREASON_VBATOV_Msk         (BIT_MASK(1U) << 3)

/* ERRORSENSOR (0x54) */
#define CHRG_ERRORSENSOR_NTCCOLD_Msk  (BIT_MASK(1U) << 0)
#define CHRG_ERRORSENSOR_NTCCOOL_Msk  (BIT_MASK(1U) << 1)
#define CHRG_ERRORSENSOR_NTCWARM_Msk  (BIT_MASK(1U) << 2)
#define CHRG_ERRORSENSOR_NTCHOT_Msk   (BIT_MASK(1U) << 3)
#define CHRG_ERRORSENSOR_VTERM_Msk    (BIT_MASK(1U) << 4)
#define CHRG_ERRORSENSOR_RECHARGE_Msk (BIT_MASK(1U) << 5)
#define CHRG_ERRORSENSOR_VTRICKLE_Msk (BIT_MASK(1U) << 6)
#define CHRG_ERRORSENSOR_VBATLOW_Msk  (BIT_MASK(1U) << 7)

/* Temperature masks (10-bit) */
#define CHRG_TEMPMSB_Msk (BIT_MASK(8U) << 2)
#define CHRG_TEMPLSB_Msk (BIT_MASK(2U) << 0)

/* TEMPERATURESTATUS (0x62) */
#define CHRG_TEMPSTAT_NTCCOLD_Msk (BIT_MASK(1U) << 0)
#define CHRG_TEMPSTAT_NTCCOOL_Msk (BIT_MASK(1U) << 1)
#define CHRG_TEMPSTAT_NTCWARM_Msk (BIT_MASK(1U) << 2)
#define CHRG_TEMPSTAT_NTCHOT_Msk  (BIT_MASK(1U) << 3)
#define CHRG_TEMPSTAT_DIETEMP_Msk (BIT_MASK(1U) << 4)

#define INV_T0              (1.f / 298.15f)
#define C_TO_K(t)           ((t) + 273)
#define DIE_TEMP_TO_CODE(t) ((390490 - (t) * 1000) / 796)

/* CHARGER voltage and current ranges */
/* 3.5 - 4.65 V in 10 mV steps */
static const struct linear_range chrg_vterm_range = LINEAR_RANGE_INIT(3500000, 10000, 0, 115);
/* (ISETDOUBLE=0) 0.5 - 128 mA in 0.5 mA steps */
static const struct linear_range chrg_current_range1 = LINEAR_RANGE_INIT(500, 500, 0, 255);
/* (ISETDOUBLE=1) 1 - 256 mA in 1 mA steps */
static const struct linear_range chrg_current_range2 = LINEAR_RANGE_INIT(1000, 1000, 0, 255);
static const int32_t vbus_ilim_ua[] = {100000, 10000, 225000, 300000};
static const struct linear_range vbusdpm_range = LINEAR_RANGE_INIT(4350000, 150000, 0, 3);

struct npm10xx_charger_config {
	struct i2c_dt_spec i2c;
	int32_t iset[3];      /* normal, warm, cool */
	int32_t vterm[3];     /* normal, warm, cool */
	int32_t ntc_temps[4]; /* cold, cool, warm, hot */
	int32_t thermistor_beta;
	int32_t dietemp_reduce;
	int32_t dietemp_resume;
	uint8_t term_current;
	uint8_t throttle_current;
	uint8_t trickle_current;
	uint8_t vtrickle;
	uint8_t vthrottle;
	uint8_t trickle_timeout;
	uint8_t charge_timeout;
	uint8_t vbatlow_threshold;
	uint8_t vbusilim;
	uint8_t vbusdpm;
	bool enable;
	bool disable_battmon;
	bool disable_recharge;
	bool disable_warm_charging;
	bool disable_cool_charging;
	bool enable_advanced;
	bool enable_throttle;
	bool disable_lowbatt;
};

/* API implementation functions */
static int npm10xx_charger_enable(const struct device *dev, const bool enable)
{
	const struct npm10xx_charger_config *config = dev->config;
	int ret;

	if (enable) {
		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_CHRG_TASKS,
					    CHRG_TASKS_RELEASE_Msk | CHRG_TASKS_CLRERROR_Msk |
						    CHRG_TASKS_CLRSAFETY_Msk);
		if (ret < 0) {
			return ret;
		}
	}

	return i2c_reg_update_byte_dt(&config->i2c, NPM10_CHRG_ENABLE, CHRG_ENABLE_CHARGE_Msk,
				      FIELD_PREP(CHRG_ENABLE_CHARGE_Msk, enable));
}

static int npm10xx_charger_get_prop(const struct device *dev, const charger_prop_t prop,
				    union charger_propval *val)
{
	int ret;
	uint8_t reg;
	uint8_t addr = 0;
	bool iset_double;

	const struct npm10xx_charger_config *config = dev->config;

	switch (prop) {

	case CHARGER_PROP_ONLINE:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_MAIN_STATUS, &reg);
		if (ret < 0) {
			return ret;
		}
		val->online = FIELD_GET(MAIN_STATUS_VBUSPRESENT_Msk, reg) ? CHARGER_ONLINE_FIXED
									  : CHARGER_ONLINE_OFFLINE;
		return 0;

	case CHARGER_PROP_STATUS:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_CHRG_STATUS, &reg);
		if (ret < 0) {
			return ret;
		}

		switch (FIELD_GET(CHRG_STATUS_STATE_Msk, reg)) {
		case CHRG_STATUS_STATE_IDLE:
			/* fall through */
		case CHRG_STATUS_STATE_ERROR:
			val->status = CHARGER_STATUS_NOT_CHARGING;
			break;
		case CHRG_STATUS_STATE_COMPLETED:
			/* fall through */
		case CHRG_STATUS_STATE_LOWVTERM:
			val->status = CHARGER_STATUS_FULL;
			break;
		case CHRG_STATUS_STATE_DISCHARGING:
			val->status = CHARGER_STATUS_DISCHARGING;
			break;
		default:
			val->status = CHARGER_STATUS_CHARGING;
			break;
		}
		return 0;

	case CHARGER_PROP_CHARGE_TYPE:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_CHRG_STATUS, &reg);
		if (ret < 0) {
			return ret;
		}

		switch (FIELD_GET(CHRG_STATUS_STATE_Msk, reg)) {
		case CHRG_STATUS_STATE_TRICKLE:
			val->charge_type = CHARGER_CHARGE_TYPE_TRICKLE;
			break;
		case CHRG_STATUS_STATE_FAST:
			val->charge_type = CHARGER_CHARGE_TYPE_FAST;
			break;
		case CHRG_STATUS_STATE_THROTTLE:
			val->charge_type = CHARGER_CHARGE_TYPE_LONGLIFE;
			break;
		default:
			val->charge_type = CHARGER_CHARGE_TYPE_NONE;
			break;
		}
		return 0;

	case CHARGER_PROP_HEALTH:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_CHRG_STATUS, &reg);
		if (ret < 0) {
			return ret;
		}

		if (FIELD_GET(CHRG_STATUS_STATE_Msk, reg) == CHRG_STATUS_STATE_ERROR) {
			ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_CHRG_ERRORREASON, &reg);
			if (ret < 0) {
				return ret;
			}

			if (FIELD_GET(CHRG_ERRORREASON_VBATOV_Msk, reg)) {
				val->health = CHARGER_HEALTH_OVERVOLTAGE;
			} else if (FIELD_GET(CHRG_ERRORREASON_CHARGETIMEOUT_Msk |
						     CHRG_TIMEOUT_TRICKLE_Msk,
					     reg)) {
				val->health = CHARGER_HEALTH_SAFETY_TIMER_EXPIRE;
			} else {
				val->health = CHARGER_HEALTH_UNSPEC_FAILURE;
			}

			return 0;
		}

		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_CHRG_TEMPERATURESTATUS, &reg);
		if (ret < 0) {
			return ret;
		}

		if (reg == 0U) {
			val->health = CHARGER_HEALTH_GOOD;
			return 0;
		}

		if (reg & CHRG_TEMPSTAT_DIETEMP_Msk) {
			val->health = CHARGER_HEALTH_OVERHEAT;
		} else if (reg & CHRG_TEMPSTAT_NTCCOLD_Msk) {
			val->health = CHARGER_HEALTH_COLD;
		} else if (reg & CHRG_TEMPSTAT_NTCCOOL_Msk) {
			val->health = CHARGER_HEALTH_COOL;
		} else if (reg & CHRG_TEMPSTAT_NTCHOT_Msk) {
			val->health = CHARGER_HEALTH_HOT;
		} else if (reg & CHRG_TEMPSTAT_NTCWARM_Msk) {
			val->health = CHARGER_HEALTH_WARM;
		} else {
			val->health = CHARGER_HEALTH_UNKNOWN;
		}
		return 0;

	case NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_CURRENT_COOL_UA:
		addr += 1U;
		/* fall-through */
	case NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_CURRENT_WARM_UA:
		addr += 1U;
		/* fall-through */
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		addr += NPM10_CHRG_ISET;
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_CHRG_ENABLE, &reg);
		if (ret < 0) {
			return ret;
		}
		iset_double = !!(reg & CHRG_ENABLE_ISETDOUBLE_Msk);

		ret = i2c_reg_read_byte_dt(&config->i2c, addr, &reg);
		if (ret < 0) {
			return ret;
		}

		return linear_range_get_value(iset_double ? &chrg_current_range2
							  : &chrg_current_range1,
					      reg, &val->const_charge_current_ua);

	case NPM10XX_CHARGER_PROP_TRICKLE_CURRENT:
		addr += 1U;
		/* fall through */
	case NPM10XX_CHARGER_PROP_TERM_CURRENT:
		addr += NPM10_CHRG_ITERM;
		ret = i2c_reg_read_byte_dt(&config->i2c, addr, &reg);
		if (ret < 0) {
			return ret;
		}

		val->custom_uint = FIELD_GET(CHRG_ITERM_LVL_Msk, reg);
		return 0;

	case NPM10XX_CHARGER_PROP_THROTTLE_LVL:
		val->custom_uint = 0U;
		return i2c_reg_read_byte_dt(&config->i2c, NPM10_CHRG_THROTTLE,
					    (uint8_t *)&val->custom_uint);

	case NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_COOL_UV:
		addr += 1U;
		/* fall-through */
	case NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_WARM_UV:
		addr += 1U;
		/* fall-through */
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		addr += NPM10_CHRG_VTERM;
		ret = i2c_reg_read_byte_dt(&config->i2c, addr, &reg);
		if (ret < 0) {
			return ret;
		}

		return linear_range_get_value(&chrg_vterm_range, FIELD_GET(CHRG_VTERM_LVL_Msk, reg),
					      &val->const_charge_voltage_uv);

	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_SYS_VBUSILIM, &reg);
		if (ret < 0) {
			return ret;
		}

		val->input_current_regulation_current_ua =
			vbus_ilim_ua[FIELD_GET(SYS_VBUSILIM_LVL_Msk, reg)];
		return 0;

	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_SYS_VBUSDPM, &reg);
		if (ret < 0) {
			return ret;
		}

		if (FIELD_GET(SYS_VBUSDPM_ENABLE, reg)) {
			return linear_range_get_value(&vbusdpm_range,
						      FIELD_GET(SYS_VBUSDPM_LVL_Msk, reg),
						      &val->input_voltage_regulation_voltage_uv);
		}
		val->input_voltage_regulation_voltage_uv = 0;
		return 0;

	default:
		LOG_ERR("Unsupported property %u", prop);
		return -ENOTSUP;
	}

	return 0;
}

static int npm10xx_charger_set_prop(const struct device *dev, const charger_prop_t prop,
				    const union charger_propval *val)
{
	int ret;
	uint8_t addr = 0;
	uint16_t idx, reg;

	const struct npm10xx_charger_config *config = dev->config;

	switch (prop) {

	/* Iset for diffent NTC regions */
	case NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_CURRENT_COOL_UA:
		addr += 1U;
		/* fall-through */
	case NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_CURRENT_WARM_UA:
		addr += 1U;
		/* fall-through */
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		addr += NPM10_CHRG_ISET;
		idx = val->const_charge_current_ua >
		      linear_range_get_max_value(&chrg_current_range1);
		ret = linear_range_get_index(idx ? &chrg_current_range2 : &chrg_current_range1,
					     val->const_charge_current_ua, &reg);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_CHRG_ENABLE,
					     CHRG_ENABLE_ISETDOUBLE_Msk,
					     FIELD_PREP(CHRG_ENABLE_ISETDOUBLE_Msk, idx));
		if (ret < 0) {
			return ret;
		}

		return i2c_reg_write_byte_dt(&config->i2c, addr, reg);

	case NPM10XX_CHARGER_PROP_TRICKLE_CURRENT:
		addr += 1U;
		/* fall through */
	case NPM10XX_CHARGER_PROP_TERM_CURRENT:
		addr += NPM10_CHRG_ITERM;

		if (val->custom_uint >= NPM10XX_CHARGER_CURRENT_PERCENT_MAX) {
			LOG_ERR("Current percentage index (%u) out of range", val->custom_uint);
			return -EINVAL;
		}

		return i2c_reg_write_byte_dt(&config->i2c, addr, (uint8_t)val->custom_uint);

	case NPM10XX_CHARGER_PROP_THROTTLE_LVL:
		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_CHRG_THROTTLE,
					    (uint8_t)val->custom_uint);
		if (ret < 0) {
			return ret;
		}

		return i2c_reg_write_byte_dt(&config->i2c, NPM10_CHRG_SET,
					     CHRG_SET_THROTTLECHG_Msk);

	case NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_COOL_UV:
		addr += 1U;
		/* fall-through */
	case NPM10XX_CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_WARM_UV:
		addr += 1U;
		/* fall-through */
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		addr += NPM10_CHRG_VTERM;
		ret = linear_range_get_index(&chrg_vterm_range, val->const_charge_voltage_uv, &reg);
		if (ret < 0) {
			return ret;
		}

		return i2c_reg_write_byte_dt(&config->i2c, addr, reg);

	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		for (reg = 0; reg < ARRAY_SIZE(vbus_ilim_ua); reg++) {
			if (vbus_ilim_ua[reg] == val->input_current_regulation_current_ua) {
				return i2c_reg_write_byte_dt(&config->i2c, NPM10_SYS_VBUSILIM, reg);
			}
		}
		LOG_ERR("invalid input_current_regulation_current_ua value %d",
			val->input_current_regulation_current_ua);
		return -EINVAL;

	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		if (val->input_voltage_regulation_voltage_uv) {
			ret = linear_range_get_index(
				&vbusdpm_range, val->input_voltage_regulation_voltage_uv, &idx);
			if (ret < 0) {
				return ret;
			}
			return i2c_reg_write_byte_dt(&config->i2c, NPM10_SYS_VBUSDPM,
						     SYS_VBUSDPM_ENABLE |
							     FIELD_PREP(SYS_VBUSDPM_LVL_Msk, idx));
		}
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_SYS_VBUSDPM, 0U);

	default:
		LOG_ERR("Unsupported property %u", prop);
		return -ENOTSUP;
	}

	return 0;
}

/* Initializers */
static inline int set_iset(const struct npm10xx_charger_config *config)
{
	int ret;
	bool iset_double;
	const struct linear_range *i_range;
	uint16_t idx;
	uint8_t iset_regs[ARRAY_SIZE(config->iset)];

	if (config->iset[0] == INT32_MAX) {
		return 0;
	}

	iset_double = config->iset[0] > linear_range_get_max_value(&chrg_current_range1);
	i_range = iset_double ? &chrg_current_range2 : &chrg_current_range1;

	ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_CHRG_ENABLE, CHRG_ENABLE_ISETDOUBLE_Msk,
				     FIELD_PREP(CHRG_ENABLE_ISETDOUBLE_Msk, iset_double));
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(config->iset); i++) {
		/* default to normal region current if not provided */
		ret = linear_range_get_index(
			i_range, config->iset[i] == INT32_MAX ? config->iset[0] : config->iset[i],
			&idx);
		if (ret < 0) {
			LOG_ERR("Charging current (%d) out of range [%d, %d]", config->iset[i],
				i_range->min, linear_range_get_max_value(i_range));
			return ret;
		}
		iset_regs[i] = idx;
	}

	return i2c_burst_write_dt(&config->i2c, NPM10_CHRG_ISET, iset_regs, ARRAY_SIZE(iset_regs));
}

static inline int set_vterm(const struct npm10xx_charger_config *config)
{
	int ret;
	uint16_t idx;
	uint8_t vterm_regs[ARRAY_SIZE(config->vterm)];

	if (config->vterm[0] == INT32_MAX) {
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(config->vterm); i++) {
		ret = linear_range_get_index(
			&chrg_vterm_range,
			config->vterm[i] == INT32_MAX ? config->vterm[0] : config->vterm[i], &idx);
		if (ret < 0) {
			LOG_ERR("Termination voltage (%d) out of range [%d, %d]", config->vterm[i],
				chrg_vterm_range.min,
				linear_range_get_max_value(&chrg_vterm_range));
			return ret;
		}
		vterm_regs[i] = idx;
	}

	return i2c_burst_write_dt(&config->i2c, NPM10_CHRG_VTERM, vterm_regs,
				  ARRAY_SIZE(vterm_regs));
}

static inline int set_ntc_thresholds(const struct npm10xx_charger_config *config)
{
	int ret;
	float exp_t1;
	uint16_t ntc;
	uint8_t ntc_regs[2];

	for (int i = 0; i < ARRAY_SIZE(config->ntc_temps); i++) {
		if (config->ntc_temps[i] == INT32_MAX) {
			continue;
		}
		if (config->ntc_temps[i] < -20 || config->ntc_temps[i] > 60) {
			LOG_WRN("Thermistor temperature threshold (%d) out of range [-20, +60]",
				config->ntc_temps[i]);
			continue;
		}
		if (config->thermistor_beta == INT32_MAX) {
			LOG_ERR("A thermistor threshold is set, but beta is not provided");
			return -ENOTSUP;
		}

		/**
		 * Equations used:
		 *   1. NTC resistance at a given temperature T1 (R0 = resistance at T0 (25°C))
		 *      R1 = R0 * exp(β * (1/T1 - 1/T0))
		 *   2. Datasheet: CHARGER - Battery temperature monitoring
		 *      Kntc = round(1023 * R1 / (R1 + R0))
		 * Combining the two, the resulting equation can be simplified:
		 *   Let exp_T1 = exp(β * (1/T1 - 1/T0))
		 *   Kntc = 1023 * exp_T1 / (exp_T1 + 1)
		 */

		exp_t1 = expf(config->thermistor_beta *
			      (1.f / (float)C_TO_K(config->ntc_temps[i]) - INV_T0));
		ntc = lroundf(1023.f * exp_t1 / (exp_t1 + 1.f));

		ntc_regs[0] = FIELD_GET(CHRG_TEMPMSB_Msk, ntc);
		ntc_regs[1] = FIELD_GET(CHRG_TEMPLSB_Msk, ntc);

		ret = i2c_burst_write_dt(&config->i2c, NPM10_CHRG_NTCMSB + 2 * i, ntc_regs,
					 ARRAY_SIZE(ntc_regs));
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static inline int set_dietemp_thresholds(const struct npm10xx_charger_config *config)
{
	int ret;
	uint16_t dietemp[2];
	uint8_t dietemp_regs[4];

	if (config->dietemp_reduce == INT32_MAX) {
		return 0;
	}
	/* Ref. Datasheet CHARGER - Charger Thermal Regulation */
	dietemp[0] = DIE_TEMP_TO_CODE(config->dietemp_reduce);

	if (config->dietemp_resume == INT32_MAX) {
		/* Set to recommended 10°C lower than the reduce temperature */
		dietemp[1] = DIE_TEMP_TO_CODE(config->dietemp_reduce - 10);
	} else {
		dietemp[1] = DIE_TEMP_TO_CODE(config->dietemp_resume);
	}

	for (int i = 0; i < ARRAY_SIZE(dietemp); i++) {
		dietemp_regs[2 * i] = FIELD_GET(CHRG_TEMPMSB_Msk, dietemp[i]);
		dietemp_regs[2 * i + 1] = FIELD_GET(CHRG_TEMPLSB_Msk, dietemp[i]);
	}

	ret = i2c_burst_write_dt(&config->i2c, NPM10_CHRG_DIETEMPREDUCEMSB, dietemp_regs,
				 ARRAY_SIZE(dietemp_regs));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int npm10xx_charger_init(const struct device *dev)
{
	const struct npm10xx_charger_config *config = dev->config;
	int ret;
	uint8_t reg, mask;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	/* Configuring charging properties requires the charger to be disabled */
	ret = npm10xx_charger_enable(dev, false);
	if (ret < 0) {
		LOG_ERR("failed to disable charger");
		return ret;
	}

	/* Set voltages and currents */
	ret = set_iset(config);
	if (ret < 0) {
		LOG_ERR("failed to set charging currents");
		return ret;
	}

	ret = set_vterm(config);
	if (ret < 0) {
		LOG_ERR("failed to set termination voltages");
		return ret;
	}

	if (config->term_current != UINT8_MAX) {
		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_CHRG_ITERM, config->term_current);
		if (ret < 0) {
			LOG_ERR("failed to set termination current");
			return ret;
		}
	}

	if (config->trickle_current != UINT8_MAX) {
		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_CHRG_ITRICKLE,
					    config->trickle_current);
		if (ret < 0) {
			LOG_ERR("failed to set trickle current");
			return ret;
		}
	}
	if (config->vtrickle != UINT8_MAX) {
		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_CHRG_VTRICKLE, config->vtrickle);
		if (ret < 0) {
			LOG_ERR("failed to set trickle voltage threshold");
			return ret;
		}
	}

	mask = 0U;
	reg = 0U;
	if (config->throttle_current != UINT8_MAX) {
		mask |= CHRG_THROTTLE_ILVL_Msk;
		reg |= FIELD_PREP(CHRG_THROTTLE_ILVL_Msk, config->throttle_current);
	}
	if (config->vthrottle != UINT8_MAX) {
		mask |= CHRG_THROTTLE_VLVL_Msk;
		reg |= FIELD_PREP(CHRG_THROTTLE_VLVL_Msk, config->vthrottle);
	}
	if (mask) {
		ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_CHRG_THROTTLE, mask, reg);
		if (ret < 0) {
			LOG_ERR("failed to set throttle configuration");
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_CHRG_SET, CHRG_SET_THROTTLECHG_Msk);
		if (ret < 0) {
			LOG_ERR("failed to enable charge throttling");
			return ret;
		}
	}
	/* End of voltages and currents */

	/* Set temperature thresholds for battery thermistor and die */
	ret = set_ntc_thresholds(config);
	if (ret < 0) {
		LOG_ERR("failed to set NTC region thresholds");
		return ret;
	}
	ret = set_dietemp_thresholds(config);
	if (ret < 0) {
		LOG_ERR("failed to set charge reduce/resume die temperature thresholds");
		return ret;
	}

	/* Set timeouts */
	mask = 0U;
	reg = 0U;
	if (config->trickle_timeout != UINT8_MAX) {
		mask |= CHRG_TIMEOUT_TRICKLE_Msk;
		reg |= FIELD_PREP(CHRG_TIMEOUT_TRICKLE_Msk, config->trickle_timeout);
	}
	if (config->charge_timeout != UINT8_MAX) {
		mask |= CHRG_TIMEOUT_CHARGE_Msk;
		reg |= FIELD_PREP(CHRG_TIMEOUT_CHARGE_Msk, config->charge_timeout);
	}
	if (mask) {
		ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_CHRG_TIMEOUT, mask, reg);
		if (ret < 0) {
			LOG_ERR("failed to set timeout values");
			return ret;
		}
	}

	/* Writing 0s to the SET and CLR registers has no effect, no need to call update */
	reg = FIELD_PREP(CHRG_CLR_NTC_Msk, config->disable_battmon) |
	      FIELD_PREP(CHRG_CLR_RECHARGE_Msk, config->disable_recharge);
	if (reg) {
		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_CHRG_CLR, reg);
		if (ret < 0) {
			LOG_ERR("failed to update the CLR register");
			return ret;
		}
	}
	if (config->enable_throttle) {
		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_CHRG_SET, CHRG_SET_THROTTLECHG_Msk);
		if (ret < 0) {
			LOG_ERR("failed to update the SET register");
			return ret;
		}
	}

	/* Combine all properties going to the ENABLE register in one write. Do this last as this
	 * potentially enables charging.
	 */
	reg = FIELD_PREP(CHRG_ENABLE_CHARGE_Msk, config->enable) |
	      FIELD_PREP(CHRG_ENABLE_JEITA_Msk, config->enable_advanced) |
	      FIELD_PREP(CHRG_ENABLE_VBATLOWCHG_Msk, config->disable_lowbatt) |
	      FIELD_PREP(CHRG_ENABLE_COOLCHG_Msk, config->disable_cool_charging) |
	      FIELD_PREP(CHRG_ENABLE_WARMCHG_Msk, config->disable_warm_charging);
	mask = reg;
	if (config->vbatlow_threshold != UINT8_MAX) {
		mask |= CHRG_ENABLE_VBATLOWLVL_Msk;
		reg |= FIELD_PREP(CHRG_ENABLE_VBATLOWLVL_Msk, config->vbatlow_threshold);
	}
	if (mask) {
		ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_CHRG_ENABLE, mask, reg);
		if (ret < 0) {
			LOG_ERR("failed to update the ENABLE register");
			return ret;
		}
	}

	/* Additinal SYSREG configuration */
	if (config->vbusilim != UINT8_MAX) {
		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_SYS_VBUSILIM,
					    FIELD_PREP(SYS_VBUSILIM_LVL_Msk, config->vbusilim));
		if (ret < 0) {
			LOG_ERR("failed to configure VBUS current limit");
			return ret;
		}
	}
	if (config->vbusdpm != UINT8_MAX) {
		ret = i2c_reg_write_byte_dt(
			&config->i2c, NPM10_SYS_VBUSDPM,
			SYS_VBUSDPM_ENABLE | FIELD_PREP(SYS_VBUSDPM_LVL_Msk, config->vbusdpm));
		if (ret < 0) {
			LOG_ERR("failed to configure VBUSDPM");
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(charger, npm10xx_charger_driver_api) = {
	.get_property = &npm10xx_charger_get_prop,
	.set_property = &npm10xx_charger_set_prop,
	.charge_enable = &npm10xx_charger_enable,
};

#define NPM10XX_CHARGER_INIT(inst)                                                                 \
	static const struct npm10xx_charger_config npm10xx_charger_config_##inst = {               \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.iset = {DT_INST_PROP_OR(inst, charge_current_microamp, INT32_MAX),                \
			 DT_INST_PROP_OR(inst, charge_current_warm_microamp, INT32_MAX),           \
			 DT_INST_PROP_OR(inst, charge_current_cool_microamp, INT32_MAX)},          \
		.vterm = {DT_INST_PROP_OR(inst, term_voltage_microvolt, INT32_MAX),                \
			  DT_INST_PROP_OR(inst, term_voltage_warm_microvolt, INT32_MAX),           \
			  DT_INST_PROP_OR(inst, term_voltage_cool_microvolt, INT32_MAX)},          \
		.ntc_temps = {DT_INST_PROP_OR(inst, thermistor_cold_degrees, INT32_MAX),           \
			      DT_INST_PROP_OR(inst, thermistor_cool_degrees, INT32_MAX),           \
			      DT_INST_PROP_OR(inst, thermistor_warm_degrees, INT32_MAX),           \
			      DT_INST_PROP_OR(inst, thermistor_hot_degrees, INT32_MAX)},           \
		.enable = DT_INST_PROP(inst, charging_enable),                                     \
		.disable_recharge = DT_INST_PROP(inst, disable_recharge),                          \
		.disable_battmon = DT_INST_PROP(inst, disable_batt_monitoring),                    \
		.disable_warm_charging = DT_INST_PROP(inst, disable_warm_charging),                \
		.disable_cool_charging = DT_INST_PROP(inst, disable_cool_charging),                \
		.enable_advanced = DT_INST_PROP(inst, enable_advanced_profile),                    \
		.enable_throttle = DT_INST_PROP(inst, enable_throttle_charging),                   \
		.disable_lowbatt = DT_INST_PROP(inst, disable_lowbatt_charging),                   \
		.vbatlow_threshold = DT_INST_ENUM_IDX_OR(inst, vbatlow_microvolt, UINT8_MAX),      \
		.vbusilim = DT_INST_ENUM_IDX_OR(inst, vbus_limit_microamp, UINT8_MAX),             \
		.vbusdpm = DT_INST_ENUM_IDX_OR(inst, vbusdpm_microvolt, UINT8_MAX),                \
		.term_current = DT_INST_ENUM_IDX_OR(inst, term_current_percent, UINT8_MAX),        \
		.throttle_current =                                                                \
			DT_INST_ENUM_IDX_OR(inst, throttle_current_percent, UINT8_MAX),            \
		.trickle_current = DT_INST_ENUM_IDX_OR(inst, trickle_current_percent, UINT8_MAX),  \
		.vtrickle = DT_INST_ENUM_IDX_OR(inst, trickle_voltage_microvolt, UINT8_MAX),       \
		.vthrottle = DT_INST_ENUM_IDX_OR(inst, throttle_level_microvolt, UINT8_MAX),       \
		.trickle_timeout = DT_INST_ENUM_IDX_OR(inst, trickle_timeout_minute, UINT8_MAX),   \
		.charge_timeout = DT_INST_ENUM_IDX_OR(inst, charge_timeout_hour, UINT8_MAX),       \
		.thermistor_beta = DT_INST_PROP_OR(inst, thermistor_beta, INT32_MAX),              \
		.dietemp_reduce = DT_INST_PROP_OR(inst, dietemp_reduce_degrees, INT32_MAX),        \
		.dietemp_resume = DT_INST_PROP_OR(inst, dietemp_resume_degrees, INT32_MAX),        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &npm10xx_charger_init, NULL, NULL,                             \
			      &npm10xx_charger_config_##inst, POST_KERNEL,                         \
			      CONFIG_CHARGER_INIT_PRIORITY, &npm10xx_charger_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPM10XX_CHARGER_INIT)
