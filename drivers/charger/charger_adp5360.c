/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adp5360_charger

#include <errno.h>

#include <zephyr/drivers/charger.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

#include "mfd_adp5360.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(charger_adp5360, CONFIG_CHARGER_LOG_LEVEL);

#define ADP5360_REG_CHARGER_VBUS_ILIM     0x02u
#define ADP5360_REG_CHARGER_TERM_CFG      0x03u
#define ADP5360_REG_CHARGER_CURR_CFG      0x04u
#define ADP5360_REG_CHARGER_VOLT_CFG      0x05u
#define ADP5360_REG_CHARGER_TMR_CFG       0x06u
#define ADP5360_REG_CHARGER_FUNC_CFG      0x07u
#define ADP5360_REG_CHARGER_STATUS_1      0x08u
#define ADP5360_REG_CHARGER_STATUS_2      0x09u
#define ADP5360_REG_CHARGER_THERM_CTRL    0x0Au
#define ADP5360_REG_BATTERY_THERM_60C     0x0Bu
#define ADP5360_REG_BATTERY_THERM_45C     0x0Cu
#define ADP5360_REG_BATTERY_THERM_10C     0x0Du
#define ADP5360_REG_BATTERY_THERM_0C      0x0Eu
#define ADP5360_REG_BATPROT_CFG           0x11u
#define ADP5360_REG_BATPROT_UV_CFG        0x12u
#define ADP5360_REG_BATPROT_DISCHG_OC_CFG 0x13u
#define ADP5360_REG_BATPROT_OV_CFG        0x14u
#define ADP5360_REG_BATPROT_CHG_OC_CFG    0x15u
#define ADP5360_REG_FAULT_STATUS          0x2Eu
#define ADP5360_REG_PGOOD_STATUS          0x2Fu

/* VBUS and ILIM (0x02) */
#define ADP5360_ILIM_MASK     GENMASK(2, 0)
#define ADP5360_VSYS_5V_MASK  BIT(3)
#define ADP5360_VADPICHG_MASK GENMASK(7, 5)

/* VTERM and ITRK (0x03) */
#define ADP5360_ITRK_MASK  GENMASK(1, 0)
#define ADP5360_VTERM_MASK GENMASK(7, 2)

/* Charger current (0x04) */
#define ADP5360_ICHG_MASK GENMASK(4, 0)
#define ADP5360_IEND_MASK GENMASK(7, 5)

/* Voltage Thresholds (0x05) */
#define ADP5360_VWEAK_MASK        GENMASK(2, 0)
#define ADP5360_VTRK_DEAD_MASK    GENMASK(4, 3)
#define ADP5360_VRCH_MASK         GENMASK(6, 5)
#define ADP5360_DIS_RECHARGE_MASK BIT(7)

/* Charger Timer (0x06) */
#define ADP5360_TIMER_PERIOD_MASK GENMASK(1, 0)
#define ADP5360_EN_CHG_TIMER      BIT(2)
#define ADP5360_EN_T_END          BIT(3)

/* Charger Functions (0x07) */
#define ADP5360_FUNC_EN_CHG          BIT(0)
#define ADP5360_FUNC_EN_ADPICHG      BIT(1)
#define ADP5360_FUNC_EN_EOC          BIT(2)
#define ADP5360_FUNC_EN_LDO          BIT(3)
#define ADP5360_FUNC_OFF_ISOFET      BIT(4)
#define ADP5360_FUNC_ILIM_JEITA_COOL BIT(6)
#define ADP5360_FUNC_EN_JEITA        BIT(7)

/* Charger Status 1 (0x08) */
#define ADP5360_STATUS_1_CHARGER_MODE_MASK GENMASK(2, 0)
#define ADP5360_STATUS_1_VBUS_ILIM         BIT(5)
#define ADP5360_STATUS_1_ADPICHG           BIT(6)
#define ADP5360_STATUS_1_VBUS_OV           BIT(7)

enum {
	ADP5360_CHARGER_MODE_OFF = 0,
	ADP5360_CHARGER_MODE_TRICKLE,
	ADP5360_CHARGER_MODE_FAST_CONST_I,
	ADP5360_CHARGER_MODE_FAST_CONST_V,
	ADP5360_CHARGER_MODE_COMPLETE,
	ADP5360_CHARGER_MODE_LDO,
	ADP5360_CHARGER_MODE_TIMER_EXPIRED,
	ADP5360_CHARGER_MODE_BATTERY_DETECT,
};

/* Charger Status 2 (0x09) */
#define ADP5360_STATUS_2_BAT_CHG_MASK GENMASK(2, 0)
#define ADP5360_STATUS_2_BAT_UV       BIT(3)
#define ADP5360_STATUS_2_BAT_OV       BIT(4)
#define ADP5360_STATUS_2_THERM_MASK   GENMASK(7, 5)

enum {
	ADP5360_BATTERY_STATUS_NORMAL = 0,
	ADP5360_BATTERY_STATUS_NO_BATT,
	ADP5360_BATTERY_STATUS_CHG_DEAD,
	ADP5360_BATTERY_STATUS_CHG_WEAK,
	ADP5360_BATTERY_STATUS_CHG_OK,
};

enum {
	ADP5360_BATTERY_THERM_OFF = 0,
	ADP5360_BATTERY_THERM_COLD,
	ADP5360_BATTERY_THERM_COOL,
	ADP5360_BATTERY_THERM_WARM,
	ADP5360_BATTERY_THERM_HOT,
	ADP5360_BATTERY_THERM_OK = 7,
};

/* Thermistor Control (0x0A) */
#define ADP5360_THERM_CTRL_ITHR GENMASK(7, 6)
#define ADP5360_THERM_CTRL_EN   BIT(0)

/* Thermistor (0x0B-0x0E) */
#define ADP5360_THERM_THRESHOLD_MASK GENMASK(7, 0)

/* Battery Protection Config (0x11) */
#define ADP5360_BATPROT_CFG_EN_BATPROTECT    BIT(0)
#define ADP5360_BATPROT_CFG_EN_CHGLB         BIT(1)
#define ADP5360_BATPROT_CFG_OC_CHG_HICCUP    BIT(2)
#define ADP5360_BATPROT_CFG_OC_DISCHG_HICCUP BIT(3)
#define ADP5360_BATPROT_CFG_ISOFET_OVCHG     BIT(4)

/* Battery Protection Unvervoltage (0x12) */
#define ADP5360_BATPROT_UV_DEGLITCH_MASK   GENMASK(1, 0)
#define ADP5360_BATPROT_UV_HYSTERESIS_MASK GENMASK(3, 2)
#define ADP5360_BATPROT_UV_DISCHARGE_MASK  GENMASK(7, 4)

/* Battery Protection Overdischarge (0x13) */
#define ADP5360_BATPROT_ODCHG_DEGLITCH_MASK GENMASK(3, 1)
#define ADP5360_BATPROT_ODCHG_OC_DISCH_MASK GENMASK(7, 5)

/* Battery Protection Overvoltage (0x14) */
#define ADP5360_BATPROT_OV_DEGLITCH        BIT(0)
#define ADP5360_BATPROT_OV_HYSTERESIS_MASK GENMASK(2, 1)
#define ADP5360_BATPROT_OV_CHARGE_MASK     GENMASK(7, 3)

/* Battery Protection Overcharge (0x15) */
#define ADP5360_BATPROT_OCHG_DEGLITCH_MASK GENMASK(4, 3)
#define ADP5360_BATPROT_OCHG_OC_CHG_MASK   GENMASK(7, 5)

/* Fault Bits (0x2E) */
#define ADP5360_FAULT_TEMP_SHUTDOWN        BIT(0)
#define ADP5360_FAULT_WATCHDOG_TIMEOUT     BIT(2)
#define ADP5360_FAULT_BAT_CHG_OVERVOLT     BIT(4)
#define ADP5360_FAULT_BAT_CHG_OVERCURR     BIT(5)
#define ADP5360_FAULT_BAT_DISCHG_OVERCURR  BIT(6)
#define ADP5360_FAULT_BAT_DISCHG_UNDERVOLT BIT(7)

/* PGOOD Status Bits (0x2F) */
#define ADP5360_PGOOD_VOUT1OK      BIT(0)
#define ADP5360_PGOOD_VOUT2OK      BIT(1)
#define ADP5360_PGOOD_BATOK        BIT(2)
#define ADP5360_PGOOD_VBUSOK       BIT(3)
#define ADP5360_PGOOD_CHG_COMPLETE BIT(4)
#define ADP5360_PGOOD_RESET_PUSHED BIT(5)

#define ADP5360_ILIM_MIN_VAL 50000
#define ADP5360_ILIM_MAX_VAL 500000
static const struct linear_range v_bus_i_limit_ua_range[] = {
	LINEAR_RANGE_INIT(50000, 50000, 0x0u, 0x5u),
	LINEAR_RANGE_INIT(400000, 100000, 0x6u, 0x7u),
};

#define ADP5360_VADPICHG_MIN_VAL 4400000
#define ADP5360_VADPICHG_MAX_VAL 4900000
static const struct linear_range v_bus_adaptive_thres_uv_range[] = {
	LINEAR_RANGE_INIT(4400000, 100000, 0x2u, 0x7u),
};

#define ADP5360_VTERM_MIN_VAL 3560000
#define ADP5360_VTERM_MAX_VAL 4660000
static const struct linear_range v_term_uv_range[] = {
	LINEAR_RANGE_INIT(3560000, 20000, 0x0u, 0x37u),
	LINEAR_RANGE_INIT(4660000, 0, 0x38u, 0x3Fu),
};

#define ADP5360_ITERM_MIN_VAL 5000
#define ADP5360_ITERM_MAX_VAL 32500
static const struct linear_range i_term_ua_range[] = {
	LINEAR_RANGE_INIT(5000, 2500, 0x1u, 0x02u),
	LINEAR_RANGE_INIT(12500, 5000, 0x3u, 0x7u),
};

#define ADP5360_ITRK_MIN_VAL 1000
#define ADP5360_ITRK_MAX_VAL 10000
static const struct linear_range i_trickle_charge_ua_range[] = {
	LINEAR_RANGE_INIT(1000, 1500, 0x0u, 0x1u),
	LINEAR_RANGE_INIT(5000, 5000, 0x2u, 0x3u),
};

#define ADP5360_ICHG_MIN_VAL 10000
#define ADP5360_ICHG_MAX_VAL 320000
static const struct linear_range i_fast_charge_ua_range[] = {
	LINEAR_RANGE_INIT(10000, 10000, 0x0u, 0x1Fu),
};

#define ADP5360_VRECHARGE_MIN_VAL 120000
#define ADP5360_VRECHARGE_MAX_VAL 240000
static const struct linear_range v_recharge_thres_uv_range[] = {
	LINEAR_RANGE_INIT(120000, 60000, 0x1u, 0x3u),
};

#define ADP5360_VWEAK_MIN_VAL 2700000
#define ADP5360_VWEAK_MAX_VAL 3400000
static const struct linear_range v_weak_thresh_uv_range[] = {
	LINEAR_RANGE_INIT(2700000, 100000, 0x0u, 0x7u),
};

#define ADP5360_VTEMP_MIN_VAL     0
#define ADP5360_VTEMP_HOT_MAX_VAL 510000
static const struct linear_range v_temp_hot_thresh_uv_range[] = {
	LINEAR_RANGE_INIT(0, 2000, 0x0u, 0xFFu),
};

#define ADP5360_VTEMP_COLD_MAX_VAL 2550000
static const struct linear_range v_temp_cold_thresh_uv_range[] = {
	LINEAR_RANGE_INIT(0, 10000, 0x0u, 0xFFu),
};

#define ADP5360_BAT_DISCHG_UV_THRESH_MIN_VAL 2050000
#define ADP5360_BAT_DISCHG_UV_THRESH_MAX_VAL 2800000
static const struct linear_range v_bat_dischg_thresh_uv_range[] = {
	LINEAR_RANGE_INIT(2050000, 50000, 0x0u, 0xFu),
};

#define ADP5360_BAT_DISCHG_OC_THRESH_MIN_VAL 50000
#define ADP5360_BAT_DISCHG_OC_THRESH_MAX_VAL 600000
static const struct linear_range i_bat_dischg_thresh_ua_range[] = {
	LINEAR_RANGE_INIT(50000, 50000, 0x0u, 0x3u),
	LINEAR_RANGE_INIT(300000, 100000, 0x4u, 0x7u),
};

#define ADP5360_BAT_CHG_OV_THRESH_MIN_VAL 3550000
#define ADP5360_BAT_CHG_OV_THRESH_MAX_VAL 4800000
static const struct linear_range v_bat_chg_thresh_ov_range[] = {
	LINEAR_RANGE_INIT(3550000, 50000, 0x0u, 0x19u),
	LINEAR_RANGE_INIT(4800000, 0, 0x1Au, 0x1Fu),
};

#define ADP5360_BAT_CHG_OC_THRESH_MIN_VAL 25000
#define ADP5360_BAT_CHG_OC_THRESH_MAX_VAL 400000
static const struct linear_range i_bat_chg_thresh_ua_range[] = {
	LINEAR_RANGE_INIT(25000, 25000, 0x0u, 0x1u),
	LINEAR_RANGE_INIT(100000, 50000, 0x2u, 0x5u),
	LINEAR_RANGE_INIT(300000, 100000, 0x6u, 0x7u),
};

struct charger_adp5360_data {
	uint32_t v_adp_ichg_uv;
	uint32_t i_lim_ua;
	uint32_t i_fast_ua;
	uint32_t i_trk_ua;
	uint32_t i_end_ua;
	bool charger_enabled;
};

/* VSYS and ICHG configuration */
struct charger_adp5360_vsys_ichg_cfg {
	bool v_sys_5v;
};

/* Charger termination configuration */
struct charger_adp5360_charger_term_cfg {
	uint32_t v_trm_uv;
};

/* Charger voltage threshold configuration */
struct charger_adp5360_charger_volt_cfg {
	uint32_t v_weak_uv;
	uint32_t v_trk_fc_dead_idx;
	uint32_t v_recharge_th_uv;
	bool recharge_dis;
};

/* Timer configuration */
struct charger_adp5360_timer_cfg {
	uint8_t trk_chg_period_idx;
	bool en_tend_chg_tmr;
	bool en_tend;
};

/* Function configuration */
struct charger_adp5360_function_cfg {
	bool en_adpichg;
	bool en_eoc;
	bool en_ldo;
	bool off_isofet;
	bool jeita_ilim_cool;
	bool en_jeita_spec;
};

/* Battery thermistor configuration */
struct charger_adp5360_bat_therm_cfg {
	uint8_t ithr_idx;
	bool en_thr;
};

/* Thermistor voltage threshold configuration */
struct charger_adp5360_therm_volt_cfg {
	uint32_t v_temp_cold_thres_uv;
	uint32_t v_temp_cool_thres_uv;
	uint32_t v_temp_warm_thres_uv;
	uint32_t v_temp_hot_thres_uv;
};

/* Battery protection control configuration */
struct charger_adp5360_bat_prot_ctrl_cfg {
	bool en_batprot;
	bool en_chg_uv;
	bool en_isofet_oc_off;
	bool set_oc_dischg_hiccup;
	bool set_oc_chg_hiccup;
};

/* Battery protection undervoltage configuration */
struct charger_adp5360_bat_prot_uv_cfg {
	uint32_t bat_uv_dischg_th_uv;
	uint8_t bat_uv_dischg_hyst_idx;
	uint8_t bat_uv_dischg_dgl_idx;
};

/* Battery protection overcurrent discharge configuration */
struct charger_adp5360_bat_prot_oc_dischg_cfg {
	uint32_t bat_oc_dischg_ua;
	uint8_t bat_oc_dischg_dgl_idx;
};

/* Battery protection overvoltage configuration */
struct charger_adp5360_bat_prot_ov_cfg {
	uint32_t bat_ov_chg_th_uv;
	uint8_t bat_ov_chg_hyst_idx;
	uint8_t bat_ov_chg_dgl_idx;
};

/* Battery protection overcurrent charge configuration */
struct charger_adp5360_bat_prot_oc_chg_cfg {
	uint32_t bat_oc_chg_ua;
	uint8_t bat_oc_chg_dgl_idx;
};

/* Main charger configuration */
struct charger_adp5360_config {
	const struct device *mfd_dev;
	char *bat_name;
	char *bat_type;
	struct charger_adp5360_vsys_ichg_cfg vsys_ichg_cfg;
	struct charger_adp5360_charger_term_cfg charger_term_cfg;
	struct charger_adp5360_charger_volt_cfg charger_volt_cfg;
	struct charger_adp5360_timer_cfg timer_cfg;
	struct charger_adp5360_function_cfg function_cfg;
	struct charger_adp5360_bat_therm_cfg bat_therm_cfg;
	struct charger_adp5360_therm_volt_cfg therm_volt_cfg;
	struct charger_adp5360_bat_prot_ctrl_cfg bat_prot_ctrl_cfg;
	struct charger_adp5360_bat_prot_uv_cfg bat_prot_uv_cfg;
	struct charger_adp5360_bat_prot_oc_dischg_cfg bat_prot_oc_dischg_cfg;
	struct charger_adp5360_bat_prot_ov_cfg bat_prot_ov_cfg;
	struct charger_adp5360_bat_prot_oc_chg_cfg bat_prot_oc_chg_cfg;
};

/**
 * @brief Set a register value using a linear range mapping
 *
 * Helper function to convert a physical value (voltage, current, etc.) to
 * a register code using a linear range mapping table and write it to the
 * specified register field.
 *
 * @param dev Charger device
 * @param range Array of linear range definitions
 * @param num_ranges Number of ranges in the array
 * @param reg Register address to write
 * @param mask Bit mask for the register field
 * @param input Physical value to convert and write (in appropriate units)
 * @return 0 on success, negative errno on failure
 */
static int charger_adp5360_set_linear_range(const struct device *dev,
					    const struct linear_range *range, uint8_t num_ranges,
					    uint8_t reg, uint8_t mask, uint32_t input)
{
	uint16_t code;
	int ret;

	ret = linear_range_group_get_index(range, num_ranges, input, &code);
	if (ret < 0) {
		LOG_ERR("Failed to get index in linear range: %d", ret);
		return ret;
	}

	ret = mfd_adp5360_reg_update(dev, reg, mask, (uint8_t)code);
	if (ret < 0) {
		LOG_ERR("Failed to update mask 0x%2X in reg 0x%2X: %d", mask, reg, ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Prepare a register value using linear range with clamping
 *
 * Helper function to convert a physical value to a register field value
 * using a linear range mapping. The input value is clamped to the valid
 * range before conversion. The result is masked and returned via reg_val.
 *
 * @param range Array of linear range definitions
 * @param nrange Number of ranges in the array
 * @param min_range Minimum valid value for clamping
 * @param max_range Maximum valid value for clamping
 * @param mask Bit mask for the register field
 * @param value Physical value to convert (will be clamped)
 * @param reg_val Pointer to store the resulting masked register value
 * @return 0 on success, negative errno on failure
 */
static int charger_adp5360_prep_linear_range(const struct linear_range *range, const uint8_t nrange,
					     const uint32_t min_range, const uint32_t max_range,
					     const uint8_t mask, uint32_t value, uint8_t *reg_val)
{
	uint16_t lin_idx;
	int ret;

	value = CLAMP(value, min_range, max_range);
	ret = linear_range_group_get_index(range, nrange, value, &lin_idx);
	if (ret) {
		LOG_ERR("Failed to get index in linear range %d", ret);
		return ret;
	}

	*reg_val |= FIELD_PREP(mask, lin_idx);

	return 0;
}

static int charger_adp5360_charge_enable(const struct device *dev, const bool enable)
{
	const struct charger_adp5360_config *cfg = dev->config;
	struct charger_adp5360_data *data = dev->data;
	int ret;

	if (!cfg->function_cfg.en_ldo) {
		LOG_ERR("Charger is not enabled due to LDO disabled");
		return -ENOTSUP;
	}

	ret = mfd_adp5360_reg_update(cfg->mfd_dev, ADP5360_REG_CHARGER_FUNC_CFG,
				     ADP5360_FUNC_EN_CHG, enable);
	if (ret < 0) {
		LOG_ERR("Failed to update mask 0x%2X in charger function: %d",
			(uint32_t)ADP5360_FUNC_EN_CHG, ret);
		return ret;
	}

	data->charger_enabled = enable;
	return 0;
}

static int charger_adp5360_get_online(const struct device *dev, enum charger_online *online)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = mfd_adp5360_reg_read(cfg->mfd_dev, ADP5360_REG_CHARGER_FUNC_CFG, &val);
	if (ret) {
		LOG_ERR("Failed to read charger function - ret %d", ret);
		return ret;
	}

	if (val & ADP5360_FUNC_EN_CHG) {
		*online = CHARGER_ONLINE_FIXED;
	} else {
		*online = CHARGER_ONLINE_OFFLINE;
	}

	return 0;
}

static int charger_adp5360_get_battery_present(const struct device *dev, bool *present)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = mfd_adp5360_reg_read(cfg->mfd_dev, ADP5360_REG_CHARGER_STATUS_2, &val);
	if (ret) {
		LOG_ERR("Failed to read charger status 2 - ret %d", ret);
		return ret;
	}

	if ((val & ADP5360_STATUS_2_BAT_CHG_MASK) == ADP5360_BATTERY_STATUS_NO_BATT) {
		*present = false;
	} else {
		*present = true;
	}

	return 0;
}

static int charger_adp5360_get_status(const struct device *dev, enum charger_status *status)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = mfd_adp5360_reg_read(cfg->mfd_dev, ADP5360_REG_CHARGER_STATUS_1, &val);
	if (ret) {
		LOG_ERR("Failed to read charger status 1 - ret %d", ret);
		return ret;
	}

	switch (val & ADP5360_STATUS_1_CHARGER_MODE_MASK) {
	case ADP5360_CHARGER_MODE_OFF:
		*status = CHARGER_STATUS_NOT_CHARGING;
		return 0;
	case ADP5360_CHARGER_MODE_TRICKLE:
	case ADP5360_CHARGER_MODE_FAST_CONST_I:
	case ADP5360_CHARGER_MODE_FAST_CONST_V:
		*status = CHARGER_STATUS_CHARGING;
		return 0;
	case ADP5360_CHARGER_MODE_COMPLETE:
		*status = CHARGER_STATUS_FULL;
		return 0;
	case ADP5360_CHARGER_MODE_LDO:
	case ADP5360_CHARGER_MODE_TIMER_EXPIRED:
	case ADP5360_CHARGER_MODE_BATTERY_DETECT:
		*status = CHARGER_STATUS_DISCHARGING;
		return 0;
	default:
		return 0;
	}
}

static int charger_adp5360_get_charge_type(const struct device *dev,
					   enum charger_charge_type *charge_type)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = mfd_adp5360_reg_read(cfg->mfd_dev, ADP5360_REG_CHARGER_STATUS_1, &val);
	if (ret) {
		LOG_ERR("Failed to read charger status 1 - ret %d", ret);
		return ret;
	}

	switch (val & ADP5360_STATUS_1_CHARGER_MODE_MASK) {
	case ADP5360_CHARGER_MODE_OFF:
	case ADP5360_CHARGER_MODE_COMPLETE:
		*charge_type = CHARGER_CHARGE_TYPE_NONE;
		return 0;
	case ADP5360_CHARGER_MODE_TRICKLE:
		*charge_type = CHARGER_CHARGE_TYPE_TRICKLE;
		return 0;
	case ADP5360_CHARGER_MODE_FAST_CONST_I:
		*charge_type = CHARGER_CHARGE_TYPE_FAST;
		return 0;
	case ADP5360_CHARGER_MODE_FAST_CONST_V:
		*charge_type = CHARGER_CHARGE_TYPE_LONGLIFE;
		return 0;
	default:
		*charge_type = CHARGER_CHARGE_TYPE_UNKNOWN;
		return 0;
	}
}

static int charger_adp5360_get_health(const struct device *dev, enum charger_health *health)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t val;
	bool bat_present;
	int ret;

	ret = mfd_adp5360_reg_read(cfg->mfd_dev, ADP5360_REG_CHARGER_STATUS_2, &val);
	if (ret) {
		LOG_ERR("Failed to read charger status 2 - ret %d", ret);
		return ret;
	}

	/* battery thermals */
	val = (val & ADP5360_STATUS_2_THERM_MASK) >> 5;
	switch (val) {
	case ADP5360_BATTERY_THERM_COLD:
		*health = CHARGER_HEALTH_COLD;
		return 0;
	case ADP5360_BATTERY_THERM_COOL:
		*health = CHARGER_HEALTH_COOL;
		return 0;
	case ADP5360_BATTERY_THERM_WARM:
		*health = CHARGER_HEALTH_WARM;
		return 0;
	case ADP5360_BATTERY_THERM_HOT:
		*health = CHARGER_HEALTH_HOT;
		return 0;
	default:
		/* if thermistor is OK/battery is off, continue to other statuses */
		LOG_DBG("Battery status is OK.");
		break;
	}

	/* device watchdog (if set) */
	ret = mfd_adp5360_reg_read(cfg->mfd_dev, ADP5360_REG_FAULT_STATUS, &val);
	if (ret) {
		LOG_ERR("Failed to read fault status - ret %d", ret);
		return ret;
	}

	if (val & ADP5360_FAULT_WATCHDOG_TIMEOUT) {
		*health = CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE;
		return 0;
	} else if (val & ADP5360_FAULT_BAT_CHG_OVERVOLT) {
		*health = CHARGER_HEALTH_OVERVOLTAGE;
		return 0;
	} else if (val & ADP5360_FAULT_TEMP_SHUTDOWN) {
		*health = CHARGER_HEALTH_OVERHEAT;
		return 0;
	}

	/* safety timeout */
	ret = mfd_adp5360_reg_read(cfg->mfd_dev, ADP5360_REG_CHARGER_STATUS_1, &val);
	if (ret) {
		LOG_ERR("Failed to read charger status 1 - ret %d", ret);
		return ret;
	}

	if ((val & ADP5360_STATUS_1_CHARGER_MODE_MASK) == ADP5360_CHARGER_MODE_TIMER_EXPIRED) {
		*health = CHARGER_HEALTH_SAFETY_TIMER_EXPIRE;
		return 0;
	}

	/* check if battery is present */
	ret = charger_adp5360_get_battery_present(dev, &bat_present);
	if (ret) {
		return ret;
	}

	if (!bat_present) {
		*health = CHARGER_HEALTH_NO_BATTERY;
		return 0;
	}

	*health = CHARGER_HEALTH_GOOD;
	return 0;
}

static int charger_adp5360_set_trickle_current(const struct device *dev, uint32_t const_current)
{
	const struct charger_adp5360_config *cfg = dev->config;
	struct charger_adp5360_data *data = dev->data;
	int ret;

	const_current = CLAMP(const_current, ADP5360_ITRK_MIN_VAL, ADP5360_ITRK_MAX_VAL);
	ret = charger_adp5360_set_linear_range(
		cfg->mfd_dev, i_trickle_charge_ua_range, ARRAY_SIZE(i_trickle_charge_ua_range),
		ADP5360_REG_CHARGER_TERM_CFG, ADP5360_ITRK_MASK, const_current);
	if (ret) {
		LOG_ERR("Failed to set trickle current %d", ret);
		return ret;
	}

	data->i_trk_ua = const_current;
	return 0;
}

static int charger_adp5360_set_constant_current(const struct device *dev, uint32_t const_current)
{
	const struct charger_adp5360_config *cfg = dev->config;
	struct charger_adp5360_data *data = dev->data;
	int ret;

	const_current = CLAMP(const_current, ADP5360_ICHG_MIN_VAL, ADP5360_ICHG_MAX_VAL);
	ret = charger_adp5360_set_linear_range(
		cfg->mfd_dev, i_fast_charge_ua_range, ARRAY_SIZE(i_fast_charge_ua_range),
		ADP5360_REG_CHARGER_CURR_CFG, ADP5360_ICHG_MASK, const_current);
	if (ret) {
		LOG_ERR("Failed to set constant current %d", ret);
		return ret;
	}

	data->i_fast_ua = const_current;
	return 0;
}

static int charger_adp5360_set_termination_current(const struct device *dev, uint32_t term_current)
{
	const struct charger_adp5360_config *cfg = dev->config;
	struct charger_adp5360_data *data = dev->data;
	int ret;

	term_current = CLAMP(term_current, ADP5360_ITERM_MIN_VAL, ADP5360_ITERM_MAX_VAL);
	ret = charger_adp5360_set_linear_range(
		cfg->mfd_dev, i_term_ua_range, ARRAY_SIZE(i_term_ua_range),
		ADP5360_REG_CHARGER_CURR_CFG, ADP5360_IEND_MASK, term_current);
	if (ret) {
		LOG_ERR("Failed to set Termination current %d", ret);
		return ret;
	}

	data->i_end_ua = term_current;
	return 0;
}

static int charger_adp5360_set_clear_faults(const struct device *dev, enum charger_health health)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t val;

	switch (health) {
	case CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE:
		val = ADP5360_FAULT_WATCHDOG_TIMEOUT;
		break;
	case CHARGER_HEALTH_OVERVOLTAGE:
		val = ADP5360_FAULT_BAT_CHG_OVERVOLT;
		break;
	case CHARGER_HEALTH_OVERHEAT:
		val = ADP5360_FAULT_TEMP_SHUTDOWN;
		break;
	default:
		return -EINVAL;
	}

	return mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_FAULT_STATUS, val);
}

static int charger_adp5360_set_regulation_current(const struct device *dev, uint32_t reg_current)
{
	const struct charger_adp5360_config *cfg = dev->config;
	struct charger_adp5360_data *data = dev->data;
	int ret;

	reg_current = CLAMP(reg_current, ADP5360_ILIM_MIN_VAL, ADP5360_ILIM_MAX_VAL);

	ret = charger_adp5360_set_linear_range(
		cfg->mfd_dev, v_bus_i_limit_ua_range, ARRAY_SIZE(v_bus_i_limit_ua_range),
		ADP5360_REG_CHARGER_VBUS_ILIM, ADP5360_ILIM_MASK, reg_current);
	if (ret) {
		LOG_ERR("Failed to set regulation current %d", ret);
		return ret;
	}

	data->i_lim_ua = reg_current;
	return 0;
}

static int charger_adp5360_set_regulation_voltage(const struct device *dev, uint32_t reg_voltage)
{
	const struct charger_adp5360_config *cfg = dev->config;
	struct charger_adp5360_data *data = dev->data;
	int ret;

	reg_voltage = CLAMP(reg_voltage, ADP5360_VADPICHG_MIN_VAL, ADP5360_VADPICHG_MAX_VAL);

	ret = charger_adp5360_set_linear_range(cfg->mfd_dev, v_bus_adaptive_thres_uv_range,
					       ARRAY_SIZE(v_bus_adaptive_thres_uv_range),
					       ADP5360_REG_CHARGER_VBUS_ILIM, ADP5360_VADPICHG_MASK,
					       reg_voltage);
	if (ret) {
		LOG_ERR("Failed to set regulation voltage %d", ret);
		return ret;
	}

	data->v_adp_ichg_uv = reg_voltage;
	return 0;
}

static int charger_adp5360_get_prop(const struct device *dev, charger_prop_t prop,
				    union charger_propval *val)
{
	const struct charger_adp5360_data *data = dev->data;

	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return charger_adp5360_get_online(dev, &val->online);
	case CHARGER_PROP_PRESENT:
		return charger_adp5360_get_battery_present(dev, &val->present);
	case CHARGER_PROP_STATUS:
		return charger_adp5360_get_status(dev, &val->status);
	case CHARGER_PROP_CHARGE_TYPE:
		return charger_adp5360_get_charge_type(dev, &val->charge_type);
	case CHARGER_PROP_HEALTH:
		return charger_adp5360_get_health(dev, &val->health);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		val->const_charge_current_ua = data->i_fast_ua;
		break;
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		val->precharge_current_ua = data->i_trk_ua;
		break;
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		val->charge_term_current_ua = data->i_end_ua;
		break;
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		val->input_current_regulation_current_ua = data->i_lim_ua;
		break;
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		val->input_voltage_regulation_voltage_uv = data->v_adp_ichg_uv;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int charger_adp5360_set_prop(const struct device *dev, charger_prop_t prop,
				    const union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_HEALTH:
		return charger_adp5360_set_clear_faults(dev, val->health);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return charger_adp5360_set_constant_current(dev, val->const_charge_current_ua);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return charger_adp5360_set_trickle_current(dev, val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return charger_adp5360_set_termination_current(dev, val->charge_term_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return charger_adp5360_set_regulation_current(
			dev, val->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return charger_adp5360_set_regulation_voltage(
			dev, val->input_voltage_regulation_voltage_uv);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline int charger_adp5360_init_vsys_ichg(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	struct charger_adp5360_data *data = dev->data;
	uint8_t reg_val = 0u;
	int ret;

	ret = charger_adp5360_prep_linear_range(
		v_bus_adaptive_thres_uv_range, ARRAY_SIZE(v_bus_adaptive_thres_uv_range),
		ADP5360_VADPICHG_MIN_VAL, ADP5360_VADPICHG_MAX_VAL, ADP5360_VADPICHG_MASK,
		data->v_adp_ichg_uv, &reg_val);
	if (ret) {
		return ret;
	}

	reg_val |= FIELD_PREP(ADP5360_VSYS_5V_MASK, cfg->vsys_ichg_cfg.v_sys_5v);

	ret = charger_adp5360_prep_linear_range(
		v_bus_i_limit_ua_range, ARRAY_SIZE(v_bus_i_limit_ua_range), ADP5360_ILIM_MIN_VAL,
		ADP5360_ILIM_MAX_VAL, ADP5360_ILIM_MASK, data->i_lim_ua, &reg_val);
	if (ret) {
		return ret;
	}

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_CHARGER_VBUS_ILIM, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to VBUS ILIM setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_charger_term(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	struct charger_adp5360_data *data = dev->data;
	uint8_t reg_val = 0u;
	int ret;

	ret = charger_adp5360_prep_linear_range(v_term_uv_range, ARRAY_SIZE(v_term_uv_range),
						ADP5360_VTERM_MIN_VAL, ADP5360_VTERM_MAX_VAL,
						ADP5360_VTERM_MASK, cfg->charger_term_cfg.v_trm_uv,
						&reg_val);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_prep_linear_range(i_trickle_charge_ua_range,
						ARRAY_SIZE(i_trickle_charge_ua_range),
						ADP5360_ITRK_MIN_VAL, ADP5360_ITRK_MAX_VAL,
						ADP5360_ITRK_MASK, data->i_trk_ua, &reg_val);
	if (ret) {
		return ret;
	}

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_CHARGER_TERM_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Charger Termination setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_charger_current(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	struct charger_adp5360_data *data = dev->data;
	uint8_t reg_val = 0u;
	int ret;

	ret = charger_adp5360_prep_linear_range(i_term_ua_range, ARRAY_SIZE(i_term_ua_range),
						ADP5360_ITERM_MIN_VAL, ADP5360_ITERM_MAX_VAL,
						ADP5360_IEND_MASK, data->i_end_ua, &reg_val);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_prep_linear_range(
		i_fast_charge_ua_range, ARRAY_SIZE(i_fast_charge_ua_range), ADP5360_ICHG_MIN_VAL,
		ADP5360_ICHG_MAX_VAL, ADP5360_ICHG_MASK, data->i_fast_ua, &reg_val);
	if (ret) {
		return ret;
	}

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_CHARGER_CURR_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Charger Current setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_charger_volt_thres(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	reg_val = FIELD_PREP(ADP5360_VTRK_DEAD_MASK, cfg->charger_volt_cfg.v_trk_fc_dead_idx) |
		  FIELD_PREP(ADP5360_DIS_RECHARGE_MASK, cfg->charger_volt_cfg.recharge_dis);

	ret = charger_adp5360_prep_linear_range(
		v_recharge_thres_uv_range, ARRAY_SIZE(v_recharge_thres_uv_range),
		ADP5360_VRECHARGE_MIN_VAL, ADP5360_VRECHARGE_MAX_VAL, ADP5360_VRCH_MASK,
		cfg->charger_volt_cfg.v_recharge_th_uv, &reg_val);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_prep_linear_range(
		v_weak_thresh_uv_range, ARRAY_SIZE(v_weak_thresh_uv_range), ADP5360_VWEAK_MIN_VAL,
		ADP5360_VWEAK_MAX_VAL, ADP5360_VWEAK_MASK, cfg->charger_volt_cfg.v_weak_uv,
		&reg_val);
	if (ret) {
		return ret;
	}

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_CHARGER_VOLT_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Charger Voltage setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_timer(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	reg_val = FIELD_PREP(ADP5360_TIMER_PERIOD_MASK, cfg->timer_cfg.trk_chg_period_idx) |
		  FIELD_PREP(ADP5360_EN_CHG_TIMER, cfg->timer_cfg.en_tend_chg_tmr) |
		  FIELD_PREP(ADP5360_EN_T_END, cfg->timer_cfg.en_tend);

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_CHARGER_TMR_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Charger Timer setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_function(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	reg_val = FIELD_PREP(ADP5360_FUNC_EN_ADPICHG, cfg->function_cfg.en_adpichg) |
		  FIELD_PREP(ADP5360_FUNC_EN_EOC, cfg->function_cfg.en_eoc) |
		  FIELD_PREP(ADP5360_FUNC_EN_LDO, cfg->function_cfg.en_ldo) |
		  FIELD_PREP(ADP5360_FUNC_OFF_ISOFET, cfg->function_cfg.off_isofet) |
		  FIELD_PREP(ADP5360_FUNC_ILIM_JEITA_COOL, cfg->function_cfg.jeita_ilim_cool) |
		  FIELD_PREP(ADP5360_FUNC_EN_JEITA, cfg->function_cfg.en_jeita_spec);

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_CHARGER_FUNC_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Charger Function setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_bat_therm(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	reg_val = FIELD_PREP(ADP5360_THERM_CTRL_ITHR, cfg->bat_therm_cfg.ithr_idx) |
		  FIELD_PREP(ADP5360_THERM_CTRL_EN, cfg->bat_therm_cfg.en_thr);

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_CHARGER_THERM_CTRL, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Charger Thermistor setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_therm_voltage(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	ret = charger_adp5360_prep_linear_range(
		v_temp_cold_thresh_uv_range, ARRAY_SIZE(v_temp_cold_thresh_uv_range),
		ADP5360_VTEMP_MIN_VAL, ADP5360_VTEMP_COLD_MAX_VAL, ADP5360_THERM_THRESHOLD_MASK,
		cfg->therm_volt_cfg.v_temp_cold_thres_uv, &reg_val);
	if (ret) {
		return ret;
	}

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_BATTERY_THERM_0C, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Cold Battery Voltage setting:%d", ret);
		return ret;
	}

	reg_val = 0u;

	ret = charger_adp5360_prep_linear_range(
		v_temp_cold_thresh_uv_range, ARRAY_SIZE(v_temp_cold_thresh_uv_range),
		ADP5360_VTEMP_MIN_VAL, ADP5360_VTEMP_COLD_MAX_VAL, ADP5360_THERM_THRESHOLD_MASK,
		cfg->therm_volt_cfg.v_temp_cool_thres_uv, &reg_val);
	if (ret) {
		return ret;
	}

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_BATTERY_THERM_10C, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Cool Battery Voltage setting:%d", ret);
		return ret;
	}

	reg_val = 0u;

	ret = charger_adp5360_prep_linear_range(
		v_temp_hot_thresh_uv_range, ARRAY_SIZE(v_temp_hot_thresh_uv_range),
		ADP5360_VTEMP_MIN_VAL, ADP5360_VTEMP_HOT_MAX_VAL, ADP5360_THERM_THRESHOLD_MASK,
		cfg->therm_volt_cfg.v_temp_warm_thres_uv, &reg_val);
	if (ret) {
		return ret;
	}

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_BATTERY_THERM_45C, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Warm Battery Voltage setting:%d", ret);
		return ret;
	}

	reg_val = 0u;

	ret = charger_adp5360_prep_linear_range(
		v_temp_hot_thresh_uv_range, ARRAY_SIZE(v_temp_hot_thresh_uv_range),
		ADP5360_VTEMP_MIN_VAL, ADP5360_VTEMP_HOT_MAX_VAL, ADP5360_THERM_THRESHOLD_MASK,
		cfg->therm_volt_cfg.v_temp_hot_thres_uv, &reg_val);
	if (ret) {
		return ret;
	}

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_BATTERY_THERM_60C, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Hot Battery Voltage setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_bat_prot_ctrl(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	reg_val = FIELD_PREP(ADP5360_BATPROT_CFG_EN_BATPROTECT, cfg->bat_prot_ctrl_cfg.en_batprot) |
		  FIELD_PREP(ADP5360_BATPROT_CFG_EN_CHGLB, cfg->bat_prot_ctrl_cfg.en_chg_uv) |
		  FIELD_PREP(ADP5360_BATPROT_CFG_ISOFET_OVCHG,
			     cfg->bat_prot_ctrl_cfg.en_isofet_oc_off) |
		  FIELD_PREP(ADP5360_BATPROT_CFG_OC_DISCHG_HICCUP,
			     cfg->bat_prot_ctrl_cfg.set_oc_dischg_hiccup) |
		  FIELD_PREP(ADP5360_BATPROT_CFG_OC_CHG_HICCUP,
			     cfg->bat_prot_ctrl_cfg.set_oc_chg_hiccup);

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_BATPROT_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Battery Protection setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_bat_prot_uv(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	ret = charger_adp5360_prep_linear_range(
		v_bat_dischg_thresh_uv_range, ARRAY_SIZE(v_bat_dischg_thresh_uv_range),
		ADP5360_BAT_DISCHG_UV_THRESH_MIN_VAL, ADP5360_BAT_DISCHG_UV_THRESH_MAX_VAL,
		ADP5360_BATPROT_UV_DISCHARGE_MASK, cfg->bat_prot_uv_cfg.bat_uv_dischg_th_uv,
		&reg_val);
	if (ret) {
		return ret;
	}

	reg_val |= FIELD_PREP(ADP5360_BATPROT_UV_HYSTERESIS_MASK,
			      cfg->bat_prot_uv_cfg.bat_uv_dischg_hyst_idx) |
		   FIELD_PREP(ADP5360_BATPROT_UV_DEGLITCH_MASK,
			      cfg->bat_prot_uv_cfg.bat_uv_dischg_dgl_idx);

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_BATPROT_UV_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Battery Undervoltage Protection setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_bat_prot_oc_dischg(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	ret = charger_adp5360_prep_linear_range(
		i_bat_dischg_thresh_ua_range, ARRAY_SIZE(i_bat_dischg_thresh_ua_range),
		ADP5360_BAT_DISCHG_OC_THRESH_MIN_VAL, ADP5360_BAT_DISCHG_OC_THRESH_MAX_VAL,
		ADP5360_BATPROT_ODCHG_OC_DISCH_MASK, cfg->bat_prot_oc_dischg_cfg.bat_oc_dischg_ua,
		&reg_val);
	if (ret) {
		return ret;
	}

	reg_val |= FIELD_PREP(ADP5360_BATPROT_ODCHG_DEGLITCH_MASK,
			      cfg->bat_prot_oc_dischg_cfg.bat_oc_dischg_dgl_idx);

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_BATPROT_DISCHG_OC_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Battery Undercurrent Protection setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_bat_prot_ov(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	ret = charger_adp5360_prep_linear_range(
		v_bat_chg_thresh_ov_range, ARRAY_SIZE(v_bat_chg_thresh_ov_range),
		ADP5360_BAT_CHG_OV_THRESH_MIN_VAL, ADP5360_BAT_CHG_OV_THRESH_MAX_VAL,
		ADP5360_BATPROT_OV_CHARGE_MASK, cfg->bat_prot_ov_cfg.bat_ov_chg_th_uv, &reg_val);
	if (ret) {
		return ret;
	}

	reg_val |= FIELD_PREP(ADP5360_BATPROT_OV_HYSTERESIS_MASK,
			      cfg->bat_prot_ov_cfg.bat_ov_chg_hyst_idx) |
		   FIELD_PREP(ADP5360_BATPROT_OV_DEGLITCH, cfg->bat_prot_ov_cfg.bat_ov_chg_dgl_idx);

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_BATPROT_OV_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Battery Overvoltage Protection setting:%d", ret);
		return ret;
	}

	return 0;
}

static inline int charger_adp5360_init_bat_prot_oc_chg(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	uint8_t reg_val = 0u;
	int ret;

	ret = charger_adp5360_prep_linear_range(
		i_bat_chg_thresh_ua_range, ARRAY_SIZE(i_bat_chg_thresh_ua_range),
		ADP5360_BAT_CHG_OC_THRESH_MIN_VAL, ADP5360_BAT_CHG_OC_THRESH_MAX_VAL,
		ADP5360_BATPROT_OCHG_OC_CHG_MASK, cfg->bat_prot_oc_chg_cfg.bat_oc_chg_ua, &reg_val);
	if (ret) {
		return ret;
	}

	reg_val |= FIELD_PREP(ADP5360_BATPROT_OCHG_DEGLITCH_MASK,
			      cfg->bat_prot_oc_chg_cfg.bat_oc_chg_dgl_idx);

	ret = mfd_adp5360_reg_write(cfg->mfd_dev, ADP5360_REG_BATPROT_CHG_OC_CFG, reg_val);
	if (ret) {
		LOG_ERR("Failed to write to Battery Overcurrent Protection setting:%d", ret);
		return ret;
	}

	return 0;
}

static int charger_adp5360_init(const struct device *dev)
{
	const struct charger_adp5360_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->mfd_dev)) {
		LOG_ERR("MFD device is not ready");
		return -ENODEV;
	}

	ret = charger_adp5360_init_vsys_ichg(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_charger_term(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_charger_current(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_charger_volt_thres(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_timer(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_function(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_bat_therm(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_therm_voltage(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_bat_prot_ctrl(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_bat_prot_uv(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_bat_prot_oc_dischg(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_bat_prot_ov(dev);
	if (ret) {
		return ret;
	}

	ret = charger_adp5360_init_bat_prot_oc_chg(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

static DEVICE_API(charger, adp5360_charger_driver_api) = {
	.get_property = charger_adp5360_get_prop,
	.set_property = charger_adp5360_set_prop,
	.charge_enable = charger_adp5360_charge_enable,
};

#define CHARGER_ADP5360_DEFINE(inst)                                                               \
	static struct charger_adp5360_data charger_adp5360_data_##inst = {                         \
		.i_fast_ua = DT_INST_PROP(inst, constant_charge_current_max_microamp),             \
		.i_trk_ua = DT_INST_PROP(inst, precharge_current_microamp),                        \
		.i_lim_ua = DT_INST_PROP(inst, charger_init_input_i_limit_ua),                     \
		.i_end_ua = DT_INST_PROP(inst, charge_term_current_microamp),                      \
		.v_adp_ichg_uv = DT_INST_PROP(inst, charger_init_adpichg_limit_uv),                \
	};                                                                                         \
                                                                                                   \
	static const struct charger_adp5360_config charger_adp5360_config_##inst = {               \
		.mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
		.bat_name = DT_INST_PROP(inst, battery_name),                                      \
		.bat_type = DT_INST_PROP(inst, device_chemistry),                                  \
		.vsys_ichg_cfg =                                                                   \
			{                                                                          \
				.v_sys_5v = DT_INST_PROP(inst, set_vsys_5v),                       \
			},                                                                         \
		.charger_term_cfg =                                                                \
			{                                                                          \
				.v_trm_uv =                                                        \
					DT_INST_PROP(inst, constant_charge_voltage_max_microvolt), \
			},                                                                         \
		.charger_volt_cfg =                                                                \
			{                                                                          \
				.v_weak_uv =                                                       \
					DT_INST_PROP(inst, battery_voltage_weak_threshold_uv),     \
				.v_trk_fc_dead_idx =                                               \
					DT_INST_ENUM_IDX(inst, charger_voltage_trk_to_fast_uv),    \
				.v_recharge_th_uv =                                                \
					DT_INST_PROP(inst, re_charge_voltage_microvolt),           \
				.recharge_dis = DT_INST_PROP(inst, disable_recharge),              \
			},                                                                         \
		.timer_cfg =                                                                       \
			{                                                                          \
				.trk_chg_period_idx =                                              \
					DT_INST_ENUM_IDX(inst, trickle_fast_charge_timer_period),  \
				.en_tend_chg_tmr =                                                 \
					DT_INST_PROP(inst, enable_trickle_fast_charge_timer),      \
				.en_tend = DT_INST_PROP(inst, enable_charge_complete_timer),       \
			},                                                                         \
		.function_cfg =                                                                    \
			{                                                                          \
				.en_adpichg = DT_INST_PROP(inst, enable_voltage_adpichg),          \
				.en_eoc = !DT_INST_PROP(inst, disable_end_of_charge),              \
				.en_ldo = !DT_INST_PROP(inst, disable_low_dropout),                \
				.off_isofet = DT_INST_PROP(inst, disable_isofet),                  \
				.jeita_ilim_cool = DT_INST_PROP(inst, ilim_jeita_cool_10pct),      \
				.en_jeita_spec = DT_INST_PROP(inst, enable_jeita_temp_spec),       \
			},                                                                         \
		.bat_therm_cfg =                                                                   \
			{                                                                          \
				.ithr_idx = DT_INST_ENUM_IDX(inst, thermistor_current_limit_ua),   \
				.en_thr = DT_INST_PROP(inst, enable_thermistor_current),           \
			},                                                                         \
		.therm_volt_cfg =                                                                  \
			{                                                                          \
				.v_temp_cold_thres_uv =                                            \
					DT_INST_PROP(inst, battery_voltage_cold_threshold_uv),     \
				.v_temp_cool_thres_uv =                                            \
					DT_INST_PROP(inst, battery_voltage_cool_threshold_uv),     \
				.v_temp_warm_thres_uv =                                            \
					DT_INST_PROP(inst, battery_voltage_warm_threshold_uv),     \
				.v_temp_hot_thres_uv =                                             \
					DT_INST_PROP(inst, battery_voltage_hot_threshold_uv),      \
			},                                                                         \
		.bat_prot_ctrl_cfg =                                                               \
			{                                                                          \
				.en_batprot = DT_INST_PROP(inst, enable_battery_protection),       \
				.en_chg_uv =                                                       \
					!DT_INST_PROP(inst, disable_charge_with_undervoltage),     \
				.en_isofet_oc_off = DT_INST_PROP(inst, set_isofet_overcharge_off), \
				.set_oc_dischg_hiccup =                                            \
					DT_INST_PROP(inst, set_overcurrent_discharge_hiccup),      \
				.set_oc_chg_hiccup =                                               \
					DT_INST_PROP(inst, set_overcurrent_charge_hiccup),         \
			},                                                                         \
		.bat_prot_uv_cfg =                                                                 \
			{                                                                          \
				.bat_uv_dischg_th_uv = DT_INST_PROP(                               \
					inst, battery_undervolt_discharge_threshold_uv),           \
				.bat_uv_dischg_hyst_idx = DT_INST_ENUM_IDX(                        \
					inst, battery_undervolt_discharge_hysteresis),             \
				.bat_uv_dischg_dgl_idx = DT_INST_ENUM_IDX(                         \
					inst, battery_undervolt_discharge_deglitch_ms),            \
			},                                                                         \
		.bat_prot_oc_dischg_cfg =                                                          \
			{                                                                          \
				.bat_oc_dischg_ua =                                                \
					DT_INST_PROP(inst, battery_overcurrent_discharge_ua),      \
				.bat_oc_dischg_dgl_idx =                                           \
					DT_INST_ENUM_IDX(                                          \
						inst, battery_overcurrent_discharge_deglitch_us) + \
					1,                                                         \
			},                                                                         \
		.bat_prot_ov_cfg =                                                                 \
			{                                                                          \
				.bat_ov_chg_th_uv =                                                \
					DT_INST_PROP(inst, battery_overvolt_charge_threshold_uv),  \
				.bat_ov_chg_hyst_idx = DT_INST_ENUM_IDX(                           \
					inst, battery_overvolt_charge_hysteresis),                 \
				.bat_ov_chg_dgl_idx = DT_INST_ENUM_IDX(                            \
					inst, battery_overvolt_charge_deglitch_ms),                \
			},                                                                         \
		.bat_prot_oc_chg_cfg =                                                             \
			{                                                                          \
				.bat_oc_chg_ua =                                                   \
					DT_INST_PROP(inst, battery_overcurrent_charge_ua),         \
				.bat_oc_chg_dgl_idx = DT_INST_ENUM_IDX(                            \
					inst, battery_overcurrent_charge_deglitch_ms),             \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &charger_adp5360_init, NULL, &charger_adp5360_data_##inst,     \
			      &charger_adp5360_config_##inst, POST_KERNEL,                         \
			      CONFIG_CHARGER_ADP5360_INIT_PRIORITY, &adp5360_charger_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CHARGER_ADP5360_DEFINE)
