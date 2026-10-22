/*
 * Copyright 2026 Sacra Systems Private Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(ti_bq2429x, CONFIG_CHARGER_LOG_LEVEL);

/* Register 0x00 - Input Source Control */
#define BQ2429X_REG_INPUT_SRC_CTRL 0x00

#define BQ2429X_IINLIM_MASK GENMASK(2, 0)
#define BQ2429X_VINDPM_MASK GENMASK(6, 3)
#define BQ2429X_EN_HIZ      BIT(7)

#define BQ2429X_IINLIM_SEL_100MA  0x0
#define BQ2429X_IINLIM_SEL_150MA  0x1
#define BQ2429X_IINLIM_SEL_500MA  0x2
#define BQ2429X_IINLIM_SEL_900MA  0x3
#define BQ2429X_IINLIM_SEL_1000MA 0x4
#define BQ2429X_IINLIM_SEL_1500MA 0x5
#define BQ2429X_IINLIM_SEL_2000MA 0x6
#define BQ2429X_IINLIM_SEL_3000MA 0x7
#define BQ2429X_VINDPM_OFFSET_UV  3880000
#define BQ2429X_VINDPM_STEP_UV    80000
#define BQ2429X_VINDPM_MIN_UV     3880000
#define BQ2429X_VINDPM_MAX_UV     5080000

/* Register 0x01 - Power-On Configuration */
#define BQ2429X_REG_POWER_ON_CONFIG 0x01

#define BQ2429X_REG_RESET       BIT(7)
#define BQ2429X_WDT_RESET       BIT(6)
#define BQ2429X_CHG_CONFIG_MASK GENMASK(5, 4)
#define BQ2429X_SYS_MIN_MASK    GENMASK(3, 1)

#define BQ2429X_CHG_DISABLE       0x0
#define BQ2429X_CHG_ENABLE        0x1
#define BQ2429X_OTG_ENABLE        0x2
#define BQ2429X_SYS_MIN_OFFSET_UV 3000000
#define BQ2429X_SYS_MIN_STEP_UV   100000
#define BQ2429X_SYS_MIN_MIN_UV    3000000
#define BQ2429X_SYS_MIN_MAX_UV    3700000

/* Register 0x02 - Charge Current Control */
#define BQ2429X_REG_CHARGE_CURRENT 0x02

#define BQ2429X_FORCE_20PCT BIT(0)
#define BQ2429X_BCOLD       BIT(1)
#define BQ2429X_ICHG_MASK   GENMASK(7, 2)

#define BQ2429X_BCOLD_SEL_76_PERCENT 0
#define BQ2429X_BCOLD_SEL_79_PERCENT 1
#define BQ2429X_ICHG_OFFSET_UA       512000
#define BQ2429X_ICHG_STEP_UA         64000
#define BQ2429X_ICHG_MIN_UA          512000
#define BQ2429X_ICHG_MAX_UA          3008000

/* Register 0x03 - Pre-Charge and Termination Current Control */
#define BQ2429X_REG_PRECHG_TERM_CURRENT 0x03

#define BQ2429X_IPRECHG_MASK GENMASK(7, 4)

#define BQ2429X_IPRECHG_OFFSET_UA 128000
#define BQ2429X_IPRECHG_STEP_UA   128000
#define BQ2429X_IPRECHG_MIN_UA    128000
#define BQ2429X_IPRECHG_MAX_UA    2048000
#define BQ2429X_ITERM_OFFSET_UA   128000
#define BQ2429X_ITERM_STEP_UA     128000

/* Register 0x04 - Charge Voltage Control */
#define BQ2429X_REG_CHARGE_VOLTAGE 0x04

#define BQ2429X_VREG_MASK GENMASK(7, 2)
#define BQ2429X_BATLOWV   BIT(1)
#define BQ2429X_VRECHG    BIT(0)

#define BQ2429X_VREG_OFFSET_UV     3504000
#define BQ2429X_VREG_STEP_UV       16000
#define BQ2429X_VREG_MIN_UV        3504000
#define BQ2429X_VREG_MAX_UV        4400000
#define BQ2429X_BATLOWV_SEL_2800MV 0
#define BQ2429X_BATLOWV_SEL_3000MV 1
#define BQ2429X_VRECHG_SEL_100MV   0
#define BQ2429X_VRECHG_SEL_300MV   1

/* Register 0x05 - Charge Termination/Timer Control */
#define BQ2429X_REG_CHARGE_TERM_TIMER 0x05

#define BQ2429X_EN_TERM        BIT(7)
#define BQ2429X_WATCHDOG_MASK  GENMASK(5, 4)
#define BQ2429X_EN_TIMER       BIT(3)
#define BQ2429X_CHG_TIMER_MASK GENMASK(2, 1)

#define BQ2429X_WATCHDOG_SEL_DISABLE 0x0
#define BQ2429X_WATCHDOG_SEL_40S     0x1
#define BQ2429X_WATCHDOG_SEL_80S     0x2
#define BQ2429X_WATCHDOG_SEL_160S    0x3
#define BQ2429X_CHG_TIMER_SEL_5H     0x0
#define BQ2429X_CHG_TIMER_SEL_8H     0x1
#define BQ2429X_CHG_TIMER_SEL_12H    0x2
#define BQ2429X_CHG_TIMER_SEL_20H    0x3

/* Register 0x06 - Boost Voltage / Thermal Regulation Control */
#define BQ2429X_REG_BOOST_THERMAL 0x06

#define BQ2429X_BOOSTV_MASK GENMASK(7, 4)
#define BQ2429X_BHOT_MASK   GENMASK(3, 2)
#define BQ2429X_TREG_MASK   GENMASK(1, 0)

#define BQ2429X_BOOSTV_OFFSET_UV 4550000
#define BQ2429X_BOOSTV_STEP_UV   64000
#define BQ2429X_BOOSTV_MIN_UV    4550000
#define BQ2429X_BOOSTV_MAX_UV    5510000
#define BQ2429X_BHOT_SEL_VBHOT1  0x0
#define BQ2429X_BHOT_SEL_VBHOT0  0x1
#define BQ2429X_BHOT_SEL_VBHOT2  0x2
#define BQ2429X_BHOT_SEL_DISABLE 0x3
#define BQ2429X_TREG_SEL_60C     0x0
#define BQ2429X_TREG_SEL_80C     0x1
#define BQ2429X_TREG_SEL_100C    0x2
#define BQ2429X_TREG_SEL_120C    0x3

/* Register 0x07 - Misc Operation Control */
#define BQ2429X_REG_MISC_OPERATION 0x07

#define BQ2429X_DPDM_EN        BIT(7)
#define BQ2429X_TMR2X_EN       BIT(6)
#define BQ2429X_BATFET_DISABLE BIT(5)
#define BQ2429X_INT_MASK_MASK  GENMASK(1, 0)

#define BQ2429X_INT_MASK_NONE       0x0
#define BQ2429X_INT_MASK_BAT_FAULT  BIT(0)
#define BQ2429X_INT_MASK_CHRG_FAULT BIT(1)
#define BQ2429X_INT_MASK_ALL        GENMASK(1, 0)

/* Register 0x08 - System Status */
#define BQ2429X_REG_SYSTEM_STATUS 0x08

#define BQ2429X_VBUS_STAT_MASK GENMASK(7, 6)
#define BQ2429X_CHRG_STAT_MASK GENMASK(5, 4)
#define BQ2429X_DPM_STAT       BIT(3)
#define BQ2429X_PG_STAT        BIT(2)
#define BQ2429X_THERM_STAT     BIT(1)
#define BQ2429X_VSYS_STAT      BIT(0)

#define BQ2429X_VBUS_STAT_UNKNOWN      0x0
#define BQ2429X_VBUS_STAT_USB_HOST     0x1
#define BQ2429X_VBUS_STAT_ADAPTER      0x2
#define BQ2429X_VBUS_STAT_OTG          0x3
#define BQ2429X_CHRG_STAT_NOT_CHARGING 0x0
#define BQ2429X_CHRG_STAT_PRECHARGE    0x1
#define BQ2429X_CHRG_STAT_FASTCHARGE   0x2
#define BQ2429X_CHRG_STAT_DONE         0x3

/* Register 0x09 - Fault Register */
#define BQ2429X_REG_FAULT 0x09

#define BQ2429X_WATCHDOG_FAULT  BIT(7)
#define BQ2429X_OTG_FAULT       BIT(6)
#define BQ2429X_CHRG_FAULT_MASK GENMASK(5, 4)
#define BQ2429X_BAT_FAULT       BIT(3)
#define BQ2429X_NTC_FAULT_MASK  GENMASK(1, 0)

#define BQ2429X_CHRG_FAULT_NORMAL  0x0
#define BQ2429X_CHRG_FAULT_INPUT   0x1
#define BQ2429X_CHRG_FAULT_THERMAL 0x2
#define BQ2429X_CHRG_FAULT_TIMER   0x3

#define BQ2429X_NTC_FAULT_NORMAL   0x0
#define BQ2429X_NTC_FAULT_COLD     0x1
#define BQ2429X_NTC_FAULT_HOT      0x2
#define BQ2429X_NTC_FAULT_RESERVED 0x3

/* Register 0x0A - Vendor / Part / Revision Status */
#define BQ2429X_REG_VENDOR 0x0A

#define BQ2429X_REVISION_MASK    GENMASK(2, 0)
#define BQ2429X_PART_NUMBER_MASK GENMASK(7, 5)

#define BQ24295_PART_NUMBER  0x6
#define BQ24296_PART_NUMBER  0x1
#define BQ24296M_PART_NUMBER 0x1
#define BQ24297_PART_NUMBER  0x3
#define BQ24298_PART_NUMBER  0x1

struct bq2429x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec ce_gpio;
	uint32_t ichg_ua;
	uint32_t vreg_uv;
	uint32_t watchdog_timeout_ms;
	uint32_t iterm_min_ua;
	uint32_t iterm_max_ua;
	uint8_t iterm_mask;
	uint8_t part_no;
};

static const uint32_t bq2429x_iinlim_table[] = {
	100000, 150000, 500000, 900000, 1000000, 1500000, 2000000, 3000000,
};

static int bq2429x_test_bit(const struct device *dev, uint8_t reg, const uint8_t bit, bool *set)
{
	const struct bq2429x_config *config = dev->config;
	uint8_t value;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, reg, &value);
	if (ret < 0) {
		return ret;
	}

	*set = (value & bit) != 0U;

	return 0;
}

static int bq2429x_field_read(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t *value)
{
	const struct bq2429x_config *config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, reg, &tmp);
	if (ret < 0) {
		return ret;
	}

	*value = FIELD_GET(mask, tmp);

	return 0;
}

static int bq2429x_field_write(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
	const struct bq2429x_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, reg, mask, FIELD_PREP(mask, value));
}

static int bq2429x_identify(const struct device *dev)
{
	const struct bq2429x_config *config = dev->config;
	uint8_t reg, part_no;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2429X_REG_VENDOR, &reg);
	if (ret < 0) {
		LOG_ERR("Failed to read vendor register (%d)", ret);
		return ret;
	}

	part_no = FIELD_GET(BQ2429X_PART_NUMBER_MASK, reg);

	if (part_no != config->part_no) {
		LOG_ERR("Unexpected part number: 0x%x", part_no);
		return -ENODEV;
	}

	LOG_INF("Detected BQ2429X (PN=0x%x)", part_no);

	return 0;
}

static int bq2429x_get_online(const struct device *dev, enum charger_online *online)
{
	bool pg_stat, batfet_disable;
	int ret;

	ret = bq2429x_test_bit(dev, BQ2429X_REG_SYSTEM_STATUS, BQ2429X_PG_STAT, &pg_stat);
	if (ret < 0) {
		return ret;
	}

	ret = bq2429x_test_bit(dev, BQ2429X_REG_MISC_OPERATION, BQ2429X_BATFET_DISABLE,
			       &batfet_disable);
	if (ret < 0) {
		return ret;
	}

	if (pg_stat && !batfet_disable) {
		*online = CHARGER_ONLINE_FIXED;
	} else {
		*online = CHARGER_ONLINE_OFFLINE;
	}

	return 0;
}

static int bq2429x_get_status(const struct device *dev, enum charger_status *status)
{
	uint8_t chrg;
	int ret;

	ret = bq2429x_field_read(dev, BQ2429X_REG_SYSTEM_STATUS, BQ2429X_CHRG_STAT_MASK, &chrg);
	if (ret < 0) {
		return ret;
	}

	switch (chrg) {
	case BQ2429X_CHRG_STAT_NOT_CHARGING:
		*status = CHARGER_STATUS_NOT_CHARGING;
		break;
	case BQ2429X_CHRG_STAT_PRECHARGE:
	case BQ2429X_CHRG_STAT_FASTCHARGE:
		*status = CHARGER_STATUS_CHARGING;
		break;
	case BQ2429X_CHRG_STAT_DONE:
		*status = CHARGER_STATUS_FULL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bq2429x_charger_get_charge_type(const struct device *dev,
					   enum charger_charge_type *charge_type)
{
	*charge_type = CHARGER_CHARGE_TYPE_UNKNOWN;
	return 0;
}

static int bq2429x_get_health(const struct device *dev, enum charger_health *health)
{
	const struct bq2429x_config *config = dev->config;
	uint8_t fault, ntc_fault, chrg_fault;
	int ret;

	/*
	 * REG09 is a latched fault register.
	 * Reading it acknowledges previously latched faults.
	 */
	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2429X_REG_FAULT, &fault);
	if (ret < 0) {
		return ret;
	}

	if ((fault & BQ2429X_BAT_FAULT) != 0U) {
		*health = CHARGER_HEALTH_OVERVOLTAGE;
		return 0;
	}

	ntc_fault = FIELD_GET(BQ2429X_NTC_FAULT_MASK, fault);

	switch (ntc_fault) {
	case BQ2429X_NTC_FAULT_NORMAL:
		break;
	case BQ2429X_NTC_FAULT_COLD:
		*health = CHARGER_HEALTH_COLD;
		return 0;
	case BQ2429X_NTC_FAULT_HOT:
		*health = CHARGER_HEALTH_HOT;
		return 0;
	default:
		LOG_WRN("Reserved NTC fault value: %u", ntc_fault);
		*health = CHARGER_HEALTH_UNSPEC_FAILURE;
		return 0;
	}

	if (fault & BQ2429X_WATCHDOG_FAULT) {
		*health = CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE;
		return 0;
	}

	chrg_fault = FIELD_GET(BQ2429X_CHRG_FAULT_MASK, fault);

	switch (chrg_fault) {
	case BQ2429X_CHRG_FAULT_NORMAL:
		break;
	case BQ2429X_CHRG_FAULT_INPUT:
		*health = CHARGER_HEALTH_UNSPEC_FAILURE;
		return 0;
	case BQ2429X_CHRG_FAULT_THERMAL:
		*health = CHARGER_HEALTH_OVERHEAT;
		return 0;
	case BQ2429X_CHRG_FAULT_TIMER:
		*health = CHARGER_HEALTH_SAFETY_TIMER_EXPIRE;
		return 0;
	default:
		LOG_ERR("Invalid CHRG_FAULT value: %u", chrg_fault);
		return -EINVAL;
	}

	*health = CHARGER_HEALTH_GOOD;

	return 0;
}

static int bq2429x_get_constant_charge_current(const struct device *dev, uint32_t *current_ua)
{
	uint8_t ichg;
	int ret;

	ret = bq2429x_field_read(dev, BQ2429X_REG_CHARGE_CURRENT, BQ2429X_ICHG_MASK, &ichg);
	if (ret < 0) {
		return ret;
	}

	*current_ua = BQ2429X_ICHG_OFFSET_UA + (ichg * BQ2429X_ICHG_STEP_UA);

	return 0;
}

static int bq2429x_get_precharge_current(const struct device *dev, uint32_t *current_ua)
{
	uint8_t iprechg;
	int ret;

	ret = bq2429x_field_read(dev, BQ2429X_REG_PRECHG_TERM_CURRENT, BQ2429X_IPRECHG_MASK,
				 &iprechg);
	if (ret < 0) {
		return ret;
	}

	*current_ua = BQ2429X_IPRECHG_OFFSET_UA + (iprechg * BQ2429X_IPRECHG_STEP_UA);

	return 0;
}

static int bq2429x_get_termination_current(const struct device *dev, uint32_t *current_ua)
{
	const struct bq2429x_config *config = dev->config;
	uint8_t iterm;
	int ret;

	ret = bq2429x_field_read(dev, BQ2429X_REG_PRECHG_TERM_CURRENT, config->iterm_mask, &iterm);
	if (ret < 0) {
		return ret;
	}

	*current_ua = BQ2429X_ITERM_OFFSET_UA + (iterm * BQ2429X_ITERM_STEP_UA);

	return 0;
}

static int bq2429x_get_constant_charge_voltage(const struct device *dev, uint32_t *voltage_uv)
{
	uint8_t vreg;
	int ret;

	ret = bq2429x_field_read(dev, BQ2429X_REG_CHARGE_VOLTAGE, BQ2429X_VREG_MASK, &vreg);
	if (ret < 0) {
		return ret;
	}

	*voltage_uv = BQ2429X_VREG_OFFSET_UV + (vreg * BQ2429X_VREG_STEP_UV);

	return 0;
}

static int bq2429x_get_input_current_limit(const struct device *dev, uint32_t *current_ua)
{
	uint8_t iinlim;
	int ret;

	ret = bq2429x_field_read(dev, BQ2429X_REG_INPUT_SRC_CTRL, BQ2429X_IINLIM_MASK, &iinlim);
	if (ret < 0) {
		return ret;
	}

	if (iinlim >= ARRAY_SIZE(bq2429x_iinlim_table)) {
		return -EINVAL;
	}

	*current_ua = bq2429x_iinlim_table[iinlim];

	return 0;
}

static int bq2429x_get_vindpm(const struct device *dev, uint32_t *voltage_uv)
{
	uint8_t vindpm;
	int ret;

	ret = bq2429x_field_read(dev, BQ2429X_REG_INPUT_SRC_CTRL, BQ2429X_VINDPM_MASK, &vindpm);
	if (ret < 0) {
		return ret;
	}

	*voltage_uv = BQ2429X_VINDPM_OFFSET_UV + (vindpm * BQ2429X_VINDPM_STEP_UV);

	return 0;
}

static int bq2429x_set_constant_charge_current(const struct device *dev, uint32_t current_ua)
{
	uint8_t ichg;

	if (!IN_RANGE(current_ua, BQ2429X_ICHG_MIN_UA, BQ2429X_ICHG_MAX_UA)) {
		LOG_WRN("Charge current %u uA out of range, clamping", current_ua);
	}

	current_ua = CLAMP(current_ua, BQ2429X_ICHG_MIN_UA, BQ2429X_ICHG_MAX_UA);

	ichg = (current_ua - BQ2429X_ICHG_OFFSET_UA) / BQ2429X_ICHG_STEP_UA;

	return bq2429x_field_write(dev, BQ2429X_REG_CHARGE_CURRENT, BQ2429X_ICHG_MASK, ichg);
}

static int bq2429x_set_precharge_current(const struct device *dev, uint32_t current_ua)
{
	uint8_t iprechg;

	if (!IN_RANGE(current_ua, BQ2429X_IPRECHG_MIN_UA, BQ2429X_IPRECHG_MAX_UA)) {
		LOG_WRN("Precharge current %u uA out of range, clamping", current_ua);
	}

	current_ua = CLAMP(current_ua, BQ2429X_IPRECHG_MIN_UA, BQ2429X_IPRECHG_MAX_UA);

	iprechg = (current_ua - BQ2429X_IPRECHG_OFFSET_UA) / BQ2429X_IPRECHG_STEP_UA;

	return bq2429x_field_write(dev, BQ2429X_REG_PRECHG_TERM_CURRENT, BQ2429X_IPRECHG_MASK,
				   iprechg);
}

static int bq2429x_set_termination_current(const struct device *dev, uint32_t current_ua)
{
	const struct bq2429x_config *config = dev->config;
	uint8_t iterm;

	if (!IN_RANGE(current_ua, config->iterm_min_ua, config->iterm_max_ua)) {
		LOG_WRN("Termination current %u uA out of range, clamping", current_ua);
	}

	current_ua = CLAMP(current_ua, config->iterm_min_ua, config->iterm_max_ua);

	iterm = (current_ua - BQ2429X_ITERM_OFFSET_UA) / BQ2429X_ITERM_STEP_UA;

	return bq2429x_field_write(dev, BQ2429X_REG_PRECHG_TERM_CURRENT, config->iterm_mask, iterm);
}

static int bq2429x_set_constant_charge_voltage(const struct device *dev, uint32_t voltage_uv)
{
	uint8_t vreg;

	if (!IN_RANGE(voltage_uv, BQ2429X_VREG_MIN_UV, BQ2429X_VREG_MAX_UV)) {
		LOG_WRN("Charge voltage %u uV out of range, clamping", voltage_uv);
	}

	voltage_uv = CLAMP(voltage_uv, BQ2429X_VREG_MIN_UV, BQ2429X_VREG_MAX_UV);

	vreg = (voltage_uv - BQ2429X_VREG_OFFSET_UV) / BQ2429X_VREG_STEP_UV;

	return bq2429x_field_write(dev, BQ2429X_REG_CHARGE_VOLTAGE, BQ2429X_VREG_MASK, vreg);
}

static int bq2429x_set_watchdog_timeout(const struct device *dev, uint32_t timeout_ms)
{
	uint8_t wdt;

	if (timeout_ms == 0) {
		wdt = BQ2429X_WATCHDOG_SEL_DISABLE;
	} else if (timeout_ms <= 40000) {
		wdt = BQ2429X_WATCHDOG_SEL_40S;
	} else if (timeout_ms <= 80000) {
		wdt = BQ2429X_WATCHDOG_SEL_80S;
	} else if (timeout_ms <= 160000) {
		wdt = BQ2429X_WATCHDOG_SEL_160S;
	} else {
		LOG_WRN("Watchdog timeout %u ms out of range, clamping", timeout_ms);
		wdt = BQ2429X_WATCHDOG_SEL_160S;
	}

	return bq2429x_field_write(dev, BQ2429X_REG_CHARGE_TERM_TIMER, BQ2429X_WATCHDOG_MASK, wdt);
}

static int bq2429x_set_input_current_limit(const struct device *dev, uint32_t current_ua)
{
	uint8_t i;

	if (!IN_RANGE(current_ua, bq2429x_iinlim_table[0],
		      bq2429x_iinlim_table[ARRAY_SIZE(bq2429x_iinlim_table) - 1])) {
		LOG_WRN("Input current %u uA out of range, clamping", current_ua);
	}

	current_ua = CLAMP(current_ua, bq2429x_iinlim_table[0],
			   bq2429x_iinlim_table[ARRAY_SIZE(bq2429x_iinlim_table) - 1]);

	for (i = 0; i < ARRAY_SIZE(bq2429x_iinlim_table); i++) {
		if (current_ua <= bq2429x_iinlim_table[i]) {
			break;
		}
	}

	return bq2429x_field_write(dev, BQ2429X_REG_INPUT_SRC_CTRL, BQ2429X_IINLIM_MASK, i);
}

static int bq2429x_set_vindpm(const struct device *dev, uint32_t voltage_uv)
{
	uint8_t vindpm;

	if (!IN_RANGE(voltage_uv, BQ2429X_VINDPM_MIN_UV, BQ2429X_VINDPM_MAX_UV)) {
		LOG_WRN("Input regulation voltage %u uV out of range, clamping", voltage_uv);
	}

	voltage_uv = CLAMP(voltage_uv, BQ2429X_VINDPM_MIN_UV, BQ2429X_VINDPM_MAX_UV);

	vindpm = (voltage_uv - BQ2429X_VINDPM_OFFSET_UV) / BQ2429X_VINDPM_STEP_UV;

	return bq2429x_field_write(dev, BQ2429X_REG_INPUT_SRC_CTRL, BQ2429X_VINDPM_MASK, vindpm);
}

static int bq2429x_gpio_init(const struct device *dev)
{
	const struct bq2429x_config *config = dev->config;
	int ret;

	/* CE GPIO (optional) */
	if (config->ce_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->ce_gpio)) {
			LOG_ERR("CE GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->ce_gpio, GPIO_OUTPUT_INACTIVE);

		if (ret < 0) {
			LOG_ERR("Failed to configure CE GPIO (%d)", ret);
			return ret;
		}
	} else {
		LOG_INF("CE GPIO not configured");
	}

	return 0;
}

static int bq2429x_get_property(const struct device *dev, const charger_prop_t prop,
				union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return bq2429x_get_online(dev, &val->online);
	case CHARGER_PROP_STATUS:
		return bq2429x_get_status(dev, &val->status);
	case CHARGER_PROP_CHARGE_TYPE:
		return bq2429x_charger_get_charge_type(dev, &val->charge_type);
	case CHARGER_PROP_HEALTH:
		return bq2429x_get_health(dev, &val->health);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq2429x_get_constant_charge_current(dev, &val->const_charge_current_ua);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return bq2429x_get_precharge_current(dev, &val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return bq2429x_get_termination_current(dev, &val->charge_term_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq2429x_get_constant_charge_voltage(dev, &val->const_charge_voltage_uv);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq2429x_get_input_current_limit(dev,
						       &val->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq2429x_get_vindpm(dev, &val->input_voltage_regulation_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int bq2429x_set_property(const struct device *dev, const charger_prop_t prop,
				const union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq2429x_set_constant_charge_current(dev, val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq2429x_set_constant_charge_voltage(dev, val->const_charge_voltage_uv);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq2429x_set_input_current_limit(dev,
						       val->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq2429x_set_vindpm(dev, val->input_voltage_regulation_voltage_uv);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return bq2429x_set_precharge_current(dev, val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return bq2429x_set_termination_current(dev, val->charge_term_current_ua);
	default:
		return -ENOTSUP;
	}
}

static int bq2429x_charge_enable(const struct device *dev, bool enable)
{
	const struct bq2429x_config *config = dev->config;
	uint8_t value;

	if (config->ce_gpio.port != NULL) {
		if (enable == true) {
			return gpio_pin_set_dt(&config->ce_gpio, 1);
		} else {
			return gpio_pin_set_dt(&config->ce_gpio, 0);
		}
	}

	value = enable ? BQ2429X_CHG_ENABLE : BQ2429X_CHG_DISABLE;

	return bq2429x_field_write(dev, BQ2429X_REG_POWER_ON_CONFIG, BQ2429X_CHG_CONFIG_MASK,
				   value);
}

static int bq2429x_set_config(const struct device *dev)
{
	const struct bq2429x_config *config = dev->config;
	int ret;

	ret = bq2429x_set_constant_charge_current(dev, config->ichg_ua);
	if (ret < 0) {
		return ret;
	}

	ret = bq2429x_set_constant_charge_voltage(dev, config->vreg_uv);
	if (ret < 0) {
		return ret;
	}

	ret = bq2429x_set_watchdog_timeout(dev, config->watchdog_timeout_ms);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int bq2429x_init(const struct device *dev)
{
	const struct bq2429x_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = bq2429x_identify(dev);
	if (ret < 0) {
		LOG_ERR("Failed to identify BQ2429X (%d)", ret);
		return ret;
	}

	ret = bq2429x_gpio_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize GPIOs (%d)", ret);
		return ret;
	}

	ret = bq2429x_set_config(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set configuration (%d)", ret);
		return ret;
	}

	LOG_DBG("BQ2429X initialized");

	return 0;
}

static DEVICE_API(charger, bq2429x_api) = {
	.get_property = bq2429x_get_property,
	.set_property = bq2429x_set_property,
	.charge_enable = bq2429x_charge_enable,
};

#define CHARGER_BQ2429X_CONFIG(inst, name, id, term_mask, term_min, term_max)                      \
	static const struct bq2429x_config name##_config_##inst = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.ce_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ce_gpios, {}),                           \
		.ichg_ua = DT_INST_PROP(inst, constant_charge_current_max_microamp),               \
		.vreg_uv = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),              \
		.watchdog_timeout_ms = DT_INST_PROP(inst, watchdog_timeout_ms),                    \
		.part_no = id,                                                                     \
		.iterm_mask = term_mask,                                                           \
		.iterm_min_ua = term_min,                                                          \
		.iterm_max_ua = term_max,                                                          \
	}

#define CHARGER_BQ2429X_INIT(inst, name)                                                           \
	DEVICE_DT_INST_DEFINE(inst, bq2429x_init, NULL, NULL, &name##_config_##inst, POST_KERNEL,  \
			      CONFIG_CHARGER_INIT_PRIORITY, &bq2429x_api)

#define CHARGER_BQ2429X_DEFINE(inst, name, id, term_mask, term_min, term_max)                      \
	CHARGER_BQ2429X_CONFIG(inst, name, id, term_mask, term_min, term_max);                     \
	CHARGER_BQ2429X_INIT(inst, name);

#define DT_DRV_COMPAT ti_bq24295
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(CHARGER_BQ2429X_DEFINE, bq24295, BQ24295_PART_NUMBER,
				  GENMASK(3, 0), 128000, 2048000)
#endif /*DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)*/
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_bq24296
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(CHARGER_BQ2429X_DEFINE, bq24296, BQ24296_PART_NUMBER,
				  GENMASK(3, 0), 128000, 2048000)
#endif /*DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)*/
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_bq24296m
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(CHARGER_BQ2429X_DEFINE, bq24296m, BQ24296M_PART_NUMBER,
				  GENMASK(2, 0), 128000, 1024000)
#endif /*DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)*/
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_bq24297
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(CHARGER_BQ2429X_DEFINE, bq24297, BQ24297_PART_NUMBER,
				  GENMASK(3, 0), 128000, 2048000)
#endif /*DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)*/
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_bq24298
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(CHARGER_BQ2429X_DEFINE, bq24298, BQ24298_PART_NUMBER,
				  GENMASK(2, 0), 128000, 1024000)
#endif /*DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)*/
#undef DT_DRV_COMPAT
