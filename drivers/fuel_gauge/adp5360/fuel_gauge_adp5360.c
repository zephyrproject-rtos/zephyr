/*
 * Copyright (c) 2026 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adp5360_fuel_gauge

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/adp5360.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/linear_range.h>
#include <errno.h>

#include "mfd_adp5360.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fuel_gauge_adp5360, CONFIG_FUEL_GAUGE_LOG_LEVEL);

enum adp5360_fuel_gauge_reg_address {
	FUEL_GAUGE_ADP5360_REG_V_THERM = 0x0Fu,
	FUEL_GAUGE_ADP5360_REG_V_SOC_0 = 0x16u,
	FUEL_GAUGE_ADP5360_REG_V_SOC_5,
	FUEL_GAUGE_ADP5360_REG_V_SOC_11,
	FUEL_GAUGE_ADP5360_REG_V_SOC_19,
	FUEL_GAUGE_ADP5360_REG_V_SOC_28,
	FUEL_GAUGE_ADP5360_REG_V_SOC_41,
	FUEL_GAUGE_ADP5360_REG_V_SOC_55,
	FUEL_GAUGE_ADP5360_REG_V_SOC_69,
	FUEL_GAUGE_ADP5360_REG_V_SOC_84,
	FUEL_GAUGE_ADP5360_REG_V_SOC_100,
	FUEL_GAUGE_ADP5360_REG_BAT_CAP,
	FUEL_GAUGE_ADP5360_REG_BAT_SOC,
	FUEL_GAUGE_ADP5360_REG_BAT_SOC_ACM_CTL,
	FUEL_GAUGE_ADP5360_REG_BAT_SOC_ACM_H,
	FUEL_GAUGE_ADP5360_REG_BAT_SOC_ACM_L,
	FUEL_GAUGE_ADP5360_REG_VBAT_READ_H,
	FUEL_GAUGE_ADP5360_REG_VBAT_READ_L,
	FUEL_GAUGE_ADP5360_REG_FUEL_GAUGE_MODE,
	FUEL_GAUGE_ADP5360_REG_SOC_RESET,
	FUEL_GAUGE_ADP5360_REG_PGOOD_STATUS = 0x2Fu,
};

#define FUEL_GAUGE_ADP5360_SOC_SHIFT  4
#define FUEL_GAUGE_ADP5360_VBAT_SHIFT 3
#define FUEL_GAUGE_ADP5360_MV_TO_UV   1000
#define FUEL_GAUGE_ADP5360_SOC_FULL   100

#define FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL 2500
#define FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL 4540
static const struct linear_range v_soc_range[] = {
	LINEAR_RANGE_INIT(2500, 8, 0x0u, 0xFFu),
};

#define FUEL_GAUGE_ADP5360_BAT_CAP_MAX            510
#define FUEL_GAUGE_ADP5360_BAT_CAP_MAH_TO_CODE(x) (x / 2)
#define FUEL_GAUGE_ADP5360_BAT_CAP_CODE_TO_MAH(x) (x * 2)

/* BAT_SOC_ACM_CTL */
#define FUEL_GAUGE_ADP5360_BAT_CAP_AGE_MASK     GENMASK(7, 6)
#define FUEL_GAUGE_ADP5360_BAT_CAP_TEMP_MASK    GENMASK(5, 4)
#define FUEL_GAUGE_ADP5360_BAT_CAP_TEMP_EN_MASK BIT(1)
#define FUEL_GAUGE_ADP5360_BAT_CAP_AGE_EN_MASK  BIT(0)

/* BAT_SOC_ACM_H and BAT_SOC_ACM_L */
#define FUEL_GAUGE_ADP5360_BAT_SOC_ACM_H_MASK GENMASK(7, 0)
#define FUEL_GAUGE_ADP5360_BAT_SOC_ACM_L_MASK GENMASK(7, 4)

/* VBAT_READ_H and VBAT_READ_L */
#define FUEL_GAUGE_ADP5360_VBAT_READ_H_MASK GENMASK(7, 0)
#define FUEL_GAUGE_ADP5360_VBAT_READ_L_MASK GENMASK(7, 3)

/* FUEL GAUGE MODE */
#define FUEL_GAUGE_ADP5360_SOC_LOW_THRESHOLD_MASK            GENMASK(7, 6)
#define FUEL_GAUGE_ADP5360_SLEEP_MODE_CURRENT_THRESHOLD_MASK GENMASK(5, 4)
#define FUEL_GAUGE_ADP5360_SLEEP_MODE_UPDATE_RATE_MASK       GENMASK(3, 2)
#define FUEL_GAUGE_ADP5360_FUEL_GAUGE_OPERATION_MODE_MASK    BIT(1)
#define FUEL_GAUGE_ADP5360_FUEL_GAUGE_ENABLE_MASK            BIT(0)

#define FUEL_GAUGE_ADP5360_SOC_ALARM_6PCT  6
#define FUEL_GAUGE_ADP5360_SOC_ALARM_11PCT 11
#define FUEL_GAUGE_ADP5360_SOC_ALARM_21PCT 21
#define FUEL_GAUGE_ADP5360_SOC_ALARM_31PCT 31

struct v_soc {
	uint16_t v_soc_0;
	uint16_t v_soc_5;
	uint16_t v_soc_11;
	uint16_t v_soc_19;
	uint16_t v_soc_28;
	uint16_t v_soc_41;
	uint16_t v_soc_55;
	uint16_t v_soc_69;
	uint16_t v_soc_84;
	uint16_t v_soc_100;
};

struct fuel_gauge_adp5360_config {
	const struct device *mfd_adp5360;
	uint32_t bat_cap;
	uint8_t bat_cap_age: 2;
	uint8_t bat_cap_temp: 2;
	uint8_t bat_cap_temp_en: 1;
	uint8_t bat_cap_age_en: 1;
	uint8_t soc_low_threshold: 2;
	uint8_t sleep_mode_current_threshold: 2;
	uint8_t sleep_mode_update_rate: 2;
	uint8_t fuel_gauge_operation_mode: 1;
	bool fuel_gauge_enable: 1;
	struct v_soc v_soc;
};

static int fuel_gauge_adp5360_enable(const struct device *dev, bool enable)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;

	ret = mfd_adp5360_reg_update(mfd_dev, FUEL_GAUGE_ADP5360_REG_FUEL_GAUGE_MODE,
				     FUEL_GAUGE_ADP5360_FUEL_GAUGE_ENABLE_MASK, enable ? 1 : 0);
	if (ret < 0) {
		LOG_ERR("Failed to %s fuel gauge in ADP5360 MFD: %d", enable ? "enable" : "disable",
			ret);
		return ret;
	}
	return 0;
}

static int fuel_gauge_adp5360_get_cycle_count(const struct device *dev,
					      union fuel_gauge_prop_val *val)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint16_t buf;

	/* Read the cycle count value from the corresponding registers in the ADP5360 MFD */
	ret = mfd_adp5360_reg_burst_read(mfd_dev, FUEL_GAUGE_ADP5360_REG_BAT_SOC_ACM_H,
					 (uint8_t *)&buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to read cycle count from ADP5360 MFD: %d", ret);
		return ret;
	}

	/* Register indicates 1/100ths of cycle count */
	val->cycle_count = sys_be16_to_cpu(buf) >> FUEL_GAUGE_ADP5360_SOC_SHIFT;

	return 0;
}

static int fuel_gauge_adp5360_get_vbat(const struct device *dev, union fuel_gauge_prop_val *val)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint16_t buf;

	/* Read the voltage value from the corresponding registers in the ADP5360 MFD */
	ret = mfd_adp5360_reg_burst_read(mfd_dev, FUEL_GAUGE_ADP5360_REG_VBAT_READ_H,
					 (uint8_t *)&buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to read voltage from ADP5360 MFD: %d", ret);
		return ret;
	}
	/* Register Voltage readout is in millivolts,
	 * multiply by 1000 to get it in terms of microvolts
	 */
	val->voltage = (sys_be16_to_cpu(buf) >> FUEL_GAUGE_ADP5360_VBAT_SHIFT) *
		       FUEL_GAUGE_ADP5360_MV_TO_UV;

	return 0;
}

static int fuel_gauge_adp5360_get_status(const struct device *dev, union fuel_gauge_prop_val *val)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint8_t buf;
	/* Read the status value from the corresponding register in the ADP5360 MFD */

	ret = mfd_adp5360_reg_read(mfd_dev, FUEL_GAUGE_ADP5360_REG_PGOOD_STATUS, &buf);
	if (ret < 0) {
		LOG_ERR("Failed to read status from ADP5360 MFD: %d", ret);
		return ret;
	}
	val->fg_status = buf;

	return 0;
}

static int fuel_gauge_adp5360_get_capacity(const struct device *dev, union fuel_gauge_prop_val *val)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint8_t buf;

	/* Read the capacity value from the corresponding register in the ADP5360 MFD */
	ret = mfd_adp5360_reg_read(mfd_dev, FUEL_GAUGE_ADP5360_REG_BAT_CAP, &buf);
	if (ret < 0) {
		LOG_ERR("Failed to read capacity from ADP5360 MFD: %d", ret);
		return ret;
	}
	val->design_cap = FUEL_GAUGE_ADP5360_BAT_CAP_CODE_TO_MAH(buf);

	return 0;
}

static int fuel_gauge_adp5360_get_bat_soc(const struct device *dev, union fuel_gauge_prop_val *val)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint8_t buf;

	/* Read the state of charge value from the corresponding register in the ADP5360 MFD */
	ret = mfd_adp5360_reg_read(mfd_dev, FUEL_GAUGE_ADP5360_REG_BAT_SOC, &buf);
	if (ret < 0) {
		LOG_ERR("Failed to read state of charge from ADP5360 MFD: %d", ret);
		return ret;
	}
	/* Clamp the state of charge value to 100% */
	if (buf > FUEL_GAUGE_ADP5360_SOC_FULL) {
		LOG_WRN("State of charge value read from ADP5360 MFD is greater than 100%%: %d",
			buf);
		buf = FUEL_GAUGE_ADP5360_SOC_FULL;
	}

	val->absolute_state_of_charge = buf;

	return 0;
}

static int fuel_gauge_adp5360_get_soc_alarm(const struct device *dev,
					    union fuel_gauge_prop_val *val)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint8_t buf;
	uint8_t soc_low_th_setting;

	/* Read the state of charge alarm value from the corresponding register in the ADP5360 MFD
	 */
	ret = mfd_adp5360_reg_read(mfd_dev, FUEL_GAUGE_ADP5360_REG_FUEL_GAUGE_MODE, &buf);
	if (ret < 0) {
		LOG_ERR("Failed to read state of charge alarm from ADP5360 MFD: %d", ret);
		return ret;
	}

	soc_low_th_setting = FIELD_GET(FUEL_GAUGE_ADP5360_SOC_LOW_THRESHOLD_MASK, buf);

	switch (soc_low_th_setting) {
	case 0:
		val->state_of_charge_alarm = FUEL_GAUGE_ADP5360_SOC_ALARM_6PCT;
		break;
	case 1:
		val->state_of_charge_alarm = FUEL_GAUGE_ADP5360_SOC_ALARM_11PCT;
		break;
	case 2:
		val->state_of_charge_alarm = FUEL_GAUGE_ADP5360_SOC_ALARM_21PCT;
		break;
	case 3:
		val->state_of_charge_alarm = FUEL_GAUGE_ADP5360_SOC_ALARM_31PCT;
		break;
	default:
		LOG_ERR("Invalid state of charge alarm setting read from ADP5360 MFD: %d",
			soc_low_th_setting);
		return -EIO;
	}
	return 0;
}

static int fuel_gauge_adp5360_get_thr_uv(const struct device *dev, union fuel_gauge_prop_val *val)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint32_t buf;

	ret = mfd_adp5360_reg_burst_read(mfd_dev, FUEL_GAUGE_ADP5360_REG_V_THERM, (uint8_t *)&buf,
					 2);
	if (ret < 0) {
		LOG_ERR("Failed to read Thermistor Voltage from ADP5360: %d", ret);
		return ret;
	}

	buf = (uint32_t)sys_le16_to_cpu(buf);

	buf = buf * FUEL_GAUGE_ADP5360_MV_TO_UV;

	val->therm_voltage_uv = buf;

	return 0;
}

static int fuel_gauge_adp5360_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
				       union fuel_gauge_prop_val *val)
{
	switch (prop) {
	case FUEL_GAUGE_CYCLE_COUNT:
		return fuel_gauge_adp5360_get_cycle_count(dev, val);
	case FUEL_GAUGE_VOLTAGE:
		return fuel_gauge_adp5360_get_vbat(dev, val);
	case FUEL_GAUGE_STATUS:
		return fuel_gauge_adp5360_get_status(dev, val);
	case FUEL_GAUGE_DESIGN_CAPACITY:
		return fuel_gauge_adp5360_get_capacity(dev, val);
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE:
		return fuel_gauge_adp5360_get_bat_soc(dev, val);
	case FUEL_GAUGE_STATE_OF_CHARGE_ALARM:
		return fuel_gauge_adp5360_get_soc_alarm(dev, val);
	case FUEL_GAUGE_THERM_VOLTAGE_UV:
		return fuel_gauge_adp5360_get_thr_uv(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int fuel_gauge_adp5360_set_soc_alarm(const struct device *dev, union fuel_gauge_prop_val val)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint8_t buf;

	switch (val.state_of_charge_alarm) {
	case 6:
		buf = 0;
		break;
	case 11:
		buf = 1;
		break;
	case 21:
		buf = 2;
		break;
	case 31:
		buf = 3;
		break;
	default:
		LOG_ERR("Invalid state of charge alarm value: %d", val.state_of_charge_alarm);
		LOG_ERR("Valid values are: 6, 11, 21, 31");
		return -EINVAL;
	}
	/* Write the state of charge alarm value to the corresponding register in the ADP5360 MFD */
	ret = mfd_adp5360_reg_update(mfd_dev, FUEL_GAUGE_ADP5360_REG_FUEL_GAUGE_MODE,
				     FUEL_GAUGE_ADP5360_SOC_LOW_THRESHOLD_MASK, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write state of charge alarm to ADP5360 MFD: %d", ret);
		return ret;
	}

	return 0;
}

static int fuel_gauge_adp5360_set_bat_cap(const struct device *dev, union fuel_gauge_prop_val val)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint8_t buf;

	/* Write the battery capacity value to the corresponding register in the ADP5360 MFD */
	val.design_cap = CLAMP(val.design_cap, 0, FUEL_GAUGE_ADP5360_BAT_CAP_MAX);
	buf = FUEL_GAUGE_ADP5360_BAT_CAP_MAH_TO_CODE(val.design_cap);
	ret = mfd_adp5360_reg_write(mfd_dev, FUEL_GAUGE_ADP5360_REG_BAT_CAP, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write battery capacity to ADP5360 MFD: %d", ret);
		return ret;
	}
	return 0;
}

static int fuel_gauge_adp5360_set_bat_cap_probe(const struct device *dev)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	uint16_t clamp_bat_cap;
	int ret;
	uint8_t buf;

	/* Write the battery capacity value to the corresponding register in the ADP5360 MFD */
	clamp_bat_cap = CLAMP(config->bat_cap, 0, FUEL_GAUGE_ADP5360_BAT_CAP_MAX);
	buf = FUEL_GAUGE_ADP5360_BAT_CAP_MAH_TO_CODE(clamp_bat_cap);
	ret = mfd_adp5360_reg_write(mfd_dev, FUEL_GAUGE_ADP5360_REG_BAT_CAP, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write battery capacity to ADP5360 MFD: %d", ret);
		return ret;
	}
	return 0;
}

static int fuel_gauge_adp5360_set_v_soc_probe(const struct device *dev)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	uint16_t temp_val, temp_idx;
	uint8_t buf[10] = {0};
	int ret;

	/* Parse the v_soc values from the device tree and store them in the buffer */
	temp_val = CLAMP(config->v_soc.v_soc_0, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 0 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[0] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	temp_val = CLAMP(config->v_soc.v_soc_5, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 5 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[1] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	temp_val = CLAMP(config->v_soc.v_soc_11, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 11 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[2] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	temp_val = CLAMP(config->v_soc.v_soc_19, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 19 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[3] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	temp_val = CLAMP(config->v_soc.v_soc_28, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 28 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[4] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	temp_val = CLAMP(config->v_soc.v_soc_41, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 41 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[5] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	temp_val = CLAMP(config->v_soc.v_soc_55, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 55 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[6] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	temp_val = CLAMP(config->v_soc.v_soc_69, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 69 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[7] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	temp_val = CLAMP(config->v_soc.v_soc_84, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 84 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[8] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	temp_val = CLAMP(config->v_soc.v_soc_100, FUEL_GAUGE_ADP5360_V_SOC_MIN_VAL,
			 FUEL_GAUGE_ADP5360_V_SOC_MAX_VAL);
	ret = linear_range_group_get_win_index(v_soc_range, ARRAY_SIZE(v_soc_range), temp_val,
					       temp_val, &temp_idx);
	if (ret) {
		LOG_ERR("Failed to retrieve SOC Value for 100 percent!");
		return ret;
	}

	if (IN_RANGE(temp_idx, 0, UINT8_MAX)) {
		buf[9] = temp_idx;
	} else {
		LOG_ERR("Value is out of range!");
		return -EINVAL;
	}

	/* Write the v_soc values to the corresponding registers in the ADP5360 MFD */
	ret = mfd_adp5360_reg_burst_write(mfd_dev, FUEL_GAUGE_ADP5360_REG_V_SOC_0, buf,
					  ARRAY_SIZE(buf)-1);
	if (ret < 0) {
		LOG_ERR("Failed to write v_soc values to ADP5360 MFD: %d", ret);
		return ret;
	}
	return 0;
}

static int fuel_gauge_adp5360_set_bat_cap_acm_ctl_probe(const struct device *dev)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint8_t buf;

	buf = FIELD_PREP(FUEL_GAUGE_ADP5360_BAT_CAP_AGE_MASK, config->bat_cap_age) |
	      FIELD_PREP(FUEL_GAUGE_ADP5360_BAT_CAP_TEMP_MASK, config->bat_cap_temp) |
	      FIELD_PREP(FUEL_GAUGE_ADP5360_BAT_CAP_TEMP_EN_MASK, config->bat_cap_temp_en) |
	      FIELD_PREP(FUEL_GAUGE_ADP5360_BAT_CAP_AGE_EN_MASK, config->bat_cap_age_en);

	/* Write the battery capacity ACM control value to the corresponding
	 * register in the ADP5360 MFD
	 */
	ret = mfd_adp5360_reg_write(mfd_dev, FUEL_GAUGE_ADP5360_REG_BAT_SOC_ACM_CTL, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write battery capacity ACM control to ADP5360 MFD: %d", ret);
		return ret;
	}
	return 0;
}

static int fuel_gauge_adp5360_set_fuel_gauge_mode_probe(const struct device *dev)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	const struct device *mfd_dev = config->mfd_adp5360;
	int ret;
	uint8_t buf;

	buf = FIELD_PREP(FUEL_GAUGE_ADP5360_SOC_LOW_THRESHOLD_MASK, config->soc_low_threshold) |
	      FIELD_PREP(FUEL_GAUGE_ADP5360_SLEEP_MODE_CURRENT_THRESHOLD_MASK,
			 config->sleep_mode_current_threshold) |
	      FIELD_PREP(FUEL_GAUGE_ADP5360_SLEEP_MODE_UPDATE_RATE_MASK,
			 config->sleep_mode_update_rate) |
	      FIELD_PREP(FUEL_GAUGE_ADP5360_FUEL_GAUGE_OPERATION_MODE_MASK,
			 config->fuel_gauge_operation_mode) |
	      FIELD_PREP(FUEL_GAUGE_ADP5360_FUEL_GAUGE_ENABLE_MASK, config->fuel_gauge_enable);
	ret = mfd_adp5360_reg_write(mfd_dev, FUEL_GAUGE_ADP5360_REG_FUEL_GAUGE_MODE, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write fuel gauge mode to ADP5360 MFD: %d", ret);
		return ret;
	}
	return 0;
}

static int fuel_gauge_adp5360_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
				       union fuel_gauge_prop_val val)
{
	switch (prop) {
	case FUEL_GAUGE_STATE_OF_CHARGE_ALARM:
		return fuel_gauge_adp5360_set_soc_alarm(dev, val);
	case FUEL_GAUGE_DESIGN_CAPACITY:
		return fuel_gauge_adp5360_set_bat_cap(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int fuel_gauge_adp5360_probe(const struct device *dev)
{
	int ret;

	ret = fuel_gauge_adp5360_set_v_soc_probe(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set v_soc values during probe: %d", ret);
		return ret;
	}
	ret = fuel_gauge_adp5360_set_bat_cap_probe(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set battery capacity during probe: %d", ret);
		return ret;
	}
	ret = fuel_gauge_adp5360_set_bat_cap_acm_ctl_probe(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set battery capacity ACM control during probe: %d", ret);
		return ret;
	}
	ret = fuel_gauge_adp5360_set_fuel_gauge_mode_probe(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set fuel gauge mode during probe: %d", ret);
		return ret;
	}
	return 0;
}

static int fuel_gauge_adp5360_init(const struct device *dev)
{
	const struct fuel_gauge_adp5360_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->mfd_adp5360)) {
		LOG_ERR("ADP5360 MFD device / Fuel Gauge is not ready");
		return -ENODEV;
	}

	ret = fuel_gauge_adp5360_probe(dev);
	if (ret < 0) {
		LOG_ERR("Failed to probe fuel gauge during initialization: %d", ret);
		return ret;
	}

	ret = fuel_gauge_adp5360_enable(dev, true);
	if (ret < 0) {
		LOG_ERR("Failed to enable fuel gauge during initialization: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(fuel_gauge, fuel_gauge_adp5360_driver_api) = {
	.get_property = &fuel_gauge_adp5360_get_prop,
	.set_property = &fuel_gauge_adp5360_set_prop,
};

#define FUEL_GAUGE_ADP5360_DEFINE(inst)                                                            \
	static struct fuel_gauge_adp5360_config fuel_gauge_adp5360_config_##inst = {               \
		.mfd_adp5360 = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                \
		.v_soc = {.v_soc_0 = DT_INST_PROP(inst, v_soc_0),                                  \
			  .v_soc_5 = DT_INST_PROP(inst, v_soc_5),                                  \
			  .v_soc_11 = DT_INST_PROP(inst, v_soc_11),                                \
			  .v_soc_19 = DT_INST_PROP(inst, v_soc_19),                                \
			  .v_soc_28 = DT_INST_PROP(inst, v_soc_28),                                \
			  .v_soc_41 = DT_INST_PROP(inst, v_soc_41),                                \
			  .v_soc_55 = DT_INST_PROP(inst, v_soc_55),                                \
			  .v_soc_69 = DT_INST_PROP(inst, v_soc_69),                                \
			  .v_soc_84 = DT_INST_PROP(inst, v_soc_84),                                \
			  .v_soc_100 = DT_INST_PROP(inst, v_soc_100)},                             \
		.bat_cap = DT_INST_PROP(inst, bat_cap),                                            \
		.bat_cap_age = DT_INST_PROP(inst, bat_cap_age),                                    \
		.bat_cap_temp = DT_INST_PROP(inst, bat_cap_temp),                                  \
		.bat_cap_temp_en = DT_INST_PROP(inst, bat_cap_temp_en),                            \
		.bat_cap_age_en = DT_INST_PROP(inst, bat_cap_age_en),                              \
		.soc_low_threshold = DT_INST_PROP(inst, soc_low_threshold),                        \
		.sleep_mode_current_threshold = DT_INST_PROP(inst, sleep_mode_current_threshold),  \
		.sleep_mode_update_rate = DT_INST_PROP(inst, sleep_mode_update_rate),              \
		.fuel_gauge_operation_mode = DT_INST_PROP(inst, fuel_gauge_operation_mode),        \
		.fuel_gauge_enable = DT_INST_PROP(inst, fuel_gauge_enable),                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &fuel_gauge_adp5360_init, NULL, NULL,                          \
			      &fuel_gauge_adp5360_config_##inst, POST_KERNEL,                      \
			      CONFIG_FUEL_GAUGE_ADP5360_INIT_PRIORITY,                             \
			      &fuel_gauge_adp5360_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FUEL_GAUGE_ADP5360_DEFINE)
