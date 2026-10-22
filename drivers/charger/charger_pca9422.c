/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9422_charger

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/pca9422.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/linear_range.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pca9422_charger, CONFIG_CHARGER_LOG_LEVEL);

/** Register memory map. See datasheet for more details. */
/** Interrupt registers */
/** @brief Interrupt registers for device operation */
#define PCA9422_REG_INT_DEVICE_0 0x5CU
#define PCA9422_REG_INT_DEVICE_1 0x5DU

/** @brief Interrupt registers for charger operation */
#define PCA9422_REG_INT_CHARGER_0 0x5EU
#define PCA9422_REG_INT_CHARGER_1 0x5FU
#define PCA9422_REG_INT_CHARGER_2 0x60U
#define PCA9422_REG_INT_CHARGER_3 0x61U

/** @brief Interrupt mask registers for device operation */
#define PCA9422_REG_INT_DEVICE_0_MASK 0x62U
#define PCA9422_REG_INT_DEVICE_1_MASK 0x63U

/** @brief Interrupt mask registers for charger operation */
#define PCA9422_REG_INT_CHARGER_0_MASK 0x64U
#define PCA9422_REG_INT_CHARGER_1_MASK 0x65U
#define PCA9422_REG_INT_CHARGER_2_MASK 0x66U
#define PCA9422_REG_INT_CHARGER_3_MASK 0x67U

/** Status registers */
/** @brief Status registers for device operation */
#define PCA9422_REG_DEVICE_0_STS 0x68U
#define PCA9422_BIT_VIN_SAFE_0V  BIT(2)
#define PCA9422_BIT_VIN_NOK      BIT(1)
#define PCA9422_BIT_VIN_OK       BIT(0)

#define PCA9422_REG_DEVICE_1_STS         0x69U
#define PCA9422_BIT_VIN_I_LIMIT_STS      BIT(7)
#define PCA9422_BIT_VSYS_SUPPLEMENT_EXIT BIT(6)
#define PCA9422_BIT_VSYS_SUPPLEMENT      BIT(5)
#define PCA9422_BIT_VSYS_OVER_LOAD       BIT(4)
#define PCA9422_BIT_VIN_AICL_RELEASE     BIT(3)
#define PCA9422_BIT_VIN_AICL             BIT(2)
#define PCA9422_BIT_VIN_OVP_EXIT         BIT(1)
#define PCA9422_BIT_VIN_OVP              BIT(0)

/** @brief Status registers for charger operation */
#define PCA9422_REG_CHARGER_0_STS 0x6AU
#define PCA9422_BIT_TOP_OFF       BIT(7)
#define PCA9422_BIT_CV_MODE       BIT(6)
#define PCA9422_BIT_FAST_CHARGE   BIT(5)
#define PCA9422_BIT_PRECHARGE     BIT(4)
#define PCA9422_BIT_CHARGER_OFF   BIT(3)
#define PCA9422_BIT_CHARGER_ON    BIT(2)
#define PCA9422_BIT_CHG_QUAL_NOK  BIT(1)
#define PCA9422_BIT_CHG_QUAL_OK   BIT(0)

#define PCA9422_REG_CHARGER_1_STS   0x6BU
#define PCA9422_BIT_THERM_HOT       BIT(7)
#define PCA9422_BIT_THERM_WARM_PLUS BIT(6)
#define PCA9422_BIT_THERM_WARM      BIT(5)
#define PCA9422_BIT_THERM_COOL      BIT(4)
#define PCA9422_BIT_THERM_COLD      BIT(3)
#define PCA9422_BIT_VBAT_OVP_EXIT   BIT(2)
#define PCA9422_BIT_VBAT_OVP        BIT(1)
#define PCA9422_BIT_NO_BATTERY      BIT(0)

#define PCA9422_REG_CHARGER_2_STS       0x6CU
#define PCA9422_BIT_RECHARGE            BIT(7)
#define PCA9422_BIT_CHARGE_DONE         BIT(6)
#define PCA9422_BIT_THERMAL_REGULATION  BIT(5)
#define PCA9422_BIT_TOP_OFF_TIMER_OUT   BIT(4)
#define PCA9422_BIT_FAST_CHG_TIMER_OUT  BIT(3)
#define PCA9422_BIT_PRECHARGE_TIMER_OUT BIT(2)
#define PCA9422_BIT_THERM_DISABLE       BIT(1)
#define PCA9422_BIT_THERM_OPEN          BIT(0)

#define PCA9422_REG_CHARGER_3_STS 0x6DU
#define PCA9422_BIT_VBAT_OCP      BIT(0)

/** Control registers */
/** @brief Device control registers */
#define PCA9422_REG_VIN_CNTL_0 0x6EU
#define PCA9422_BIT_VIN_PD_EN  BIT(1)

#define PCA9422_REG_VIN_CNTL_1               0x6FU
#define PCA9422_BIT_FORCE_DISACHARGE_VSYS_EN BIT(3)
#define PCA9422_BIT_AICL_V                   GENMASK(2, 1)
#define PCA9422_BIT_AICL_EN                  BIT(0)

#define PCA9422_REG_VIN_CNTL_2  0x70U
#define PCA9422_BIT_VIN_I_LIMIT GENMASK(4, 0)

#define PCA9422_REG_VIN_CNTL_3 0x71U
#define PCA9422_BIT_VSYS_REG   GENMASK(7, 4)

/** @brief Charger control registers */
#define PCA9422_REG_CHARGER_CNTL_0 0x72U
#define PCA9422_BIT_CHARGER_LOCK   GENMASK(5, 4)

#define PCA9422_REG_CHARGER_CNTL_1           0x73U
#define PCA9422_BIT_BAT_PRESENCE_DET_DISABLE BIT(6)
#define PCA9422_BIT_AUTOSTOP_CHG_EN          BIT(5)
#define PCA9422_BIT_CHARGER_EN               BIT(4)
#define PCA9422_BIT_V_WARM_50C               GENMASK(3, 2)
#define PCA9422_BIT_PRECHG_CURRENT           BIT(1)
#define PCA9422_BIT_CHG_CURRENT_STEP         BIT(0)

#define PCA9422_REG_CHARGER_CNTL_2 0x74U
#define PCA9422_BIT_VBAT_REG       GENMASK(6, 0)

#define PCA9422_REG_CHARGER_CNTL_3 0x75U
#define PCA9422_BIT_I_FAST_CHG     GENMASK(6, 0)

#define PCA9422_REG_CHARGER_CNTL_4  0x76U
#define PCA9422_BIT_VBAT_OVP_DEB    GENMASK(7, 6)
#define PCA9422_BIT_RECHARGE_TH     GENMASK(5, 4)
#define PCA9422_BIT_TOP_OFF_CURRENT GENMASK(3, 2)
#define PCA9422_BIT_PRE_CHG_TIMER   GENMASK(1, 0)

#define PCA9422_REG_CHARGER_CNTL_5    0x77U
#define PCA9422_BIT_THERM_NTC_EN      BIT(6)
#define PCA9422_BIT_OCP_DISCHARGE_DEB GENMASK(5, 4)
#define PCA9422_BIT_OCP_DISCHARGE     GENMASK(1, 0)

#define PCA9422_REG_CHARGER_CNTL_6 0x78U
#define PCA9422_BIT_V_HOT_60C      GENMASK(7, 6)
#define PCA9422_BIT_V_WARM_45C     GENMASK(5, 4)
#define PCA9422_BIT_V_COOL_10C     GENMASK(3, 2)
#define PCA9422_BIT_V_COLD_0C      GENMASK(1, 0)

#define PCA9422_REG_CHARGER_CNTL_7             0x79U
#define PCA9422_BIT_FAST_CHG_TIMER             GENMASK(7, 6)
#define PCA9422_BIT_2X_ALL_TIMERS_EN           BIT(5)
#define PCA9422_BIT_CHG_DISABLE_AT_COLD_HOT_EN BIT(4)
#define PCA9422_BIT_NEW_I_VBAT_AT_10C          GENMASK(3, 2)
#define PCA9422_BIT_NEW_VBAT_AT_45C            GENMASK(1, 0)

#define PCA9422_REG_CHARGER_CNTL_8        0x7AU
#define PCA9422_BIT_THERMAL_REGULATION_TH GENMASK(5, 3)
#define PCA9422_BIT_TOP_OFF_TIMER_OUT_MIN GENMASK(2, 0)

#define PCA9422_REG_CHARGER_CNTL_9          0x7BU
#define PCA9422_BIT_NEW_VBAT_AT_50C         GENMASK(7, 6)
#define PCA9422_BIT_NEW_I_VBAT_AT_50C       GENMASK(5, 4)
#define PCA9422_BIT_NEW_I_VBAT_AT_45C       GENMASK(3, 2)
#define PCA9422_BIT_FORCE_DISCHARGE_VBAT_EN BIT(1)
#define PCA9422_BIT_USB_SUSPEND             BIT(0)

#define PCA9422_REG_CHARGER_CNTL_10     0x7CU
#define PCA9422_BIT_AMUX_AUTO_OFF_WAIT  GENMASK(7, 6)
#define PCA9422_BIT_AMUX_MODE           BIT(5)
#define PCA9422_BIT_AMUX_VBAT_VSYS_GAIN BIT(4)
#define PCA9422_BIT_AMUX_THERM_GAIN     BIT(3)
#define PCA9422_BIT_AMUX_CHANNEL        GENMASK(2, 0)

/** brief Charger unlock value */
#define PCA9422_CHARGER_UNLOCK 0x30U
#define PCA9422_CHARGER_LOCK   0x00U

enum {
	RECHARGE_TH_100MV,
	RECHARGE_TH_150MV,
	RECHARGE_TH_200MV,
};

enum {
	ITOPOFF_2P5PCT,  /* 2.5% of Ifast_chg */
	ITOPOFF_5P0PCT,  /* 5.0% of Ifast_chg */
	ITOPOFF_7P5PCT,  /* 7.5% of Ifast_chg */
	ITOPOFF_10P0PCT, /* 10.0% of Ifast_chg */
};

enum {
	CHG_CURRENT_STEP_2P5MA,
	CHG_CURRENT_STEP_5P0MA,
};

#define CURRENT_STEP_2P5MA_MIN_UA 2500   /* 2.5mA */
#define CURRENT_STEP_2P5MA_MAX_UA 320000 /* 320mA */
#define CURRENT_STEP_5P0MA_MAX_UA 640000 /* 640mA */

#define VBAT_REG_MIN_UV 3600000 /* 3.6V */
#define VBAT_REG_MAX_UV 4600000 /* 4.6V */

#define VIN_I_LIMIT_MIN_UA 45000   /* 45mA */
#define VIN_I_LIMIT_MAX_UA 1195000 /* 1195mA */

#define VSYS_REG_MIN_UV 4425000 /* 4.425V */
#define VSYS_REG_MAX_UV 4800000 /* 4.8V */

struct charger_pca9422_config {
	const struct device *mfd;
	uint32_t vin_i_limit_ua;
	uint32_t vsys_reg_uv;
};

struct charger_pca9422_data {
	const struct device *dev;
	struct k_mutex mutex;
	uint32_t i_fast_chg_ua;
	uint8_t i_topoff_sel;
	uint8_t i_prechg_sel;
	uint32_t vbat_reg_uv;
	uint32_t recharge_th_sel;
	uint8_t chg_current_step;
	enum charger_status status;
	enum charger_online online;
	bool charger_enabled;
};

static const struct linear_range vin_i_limit_ua_range[] = {
	LINEAR_RANGE_INIT(45000, 25000, 0x0U, 0x1AU),
	LINEAR_RANGE_INIT(795000, 100000, 0x1BU, 0x1FU),
};

static const struct linear_range vsys_reg_uv_range[] = {
	LINEAR_RANGE_INIT(4425000, 25000, 0x0U, 0xFU),
};

static const struct linear_range vbat_reg_uv_range[] = {
	LINEAR_RANGE_INIT(3600000, 10000, 0x0U, 0x64U),
	LINEAR_RANGE_INIT(4600000, 0, 0x65U, 0x7FU),
};

static const struct linear_range i_fast_chg_ua_range[] = {
	LINEAR_RANGE_INIT(2500, 2500, 0x0U, 0x7FU),
};

static int pca9422_charger_get_status(const struct device *dev, enum charger_status *status)
{
	const struct charger_pca9422_config *const config = dev->config;
	uint8_t val;
	int ret;

	ret = mfd_pca9422_reg_read_byte(config->mfd, PCA9422_REG_CHARGER_0_STS, &val);
	if (ret < 0) {
		return ret;
	}

	if (val & PCA9422_BIT_CHG_QUAL_OK) {
		if (val & (PCA9422_BIT_PRECHARGE | PCA9422_BIT_FAST_CHARGE | PCA9422_BIT_CV_MODE |
			   PCA9422_BIT_TOP_OFF)) {
			*status = CHARGER_STATUS_CHARGING;
		} else {
			*status = CHARGER_STATUS_NOT_CHARGING;
		}

		/* Check charging done status */
		ret = mfd_pca9422_reg_read_byte(config->mfd, PCA9422_REG_CHARGER_2_STS, &val);
		if (ret < 0) {
			return ret;
		}
		if (val & PCA9422_BIT_CHARGE_DONE) {
			*status = CHARGER_STATUS_FULL;
		}
	} else {
		*status = CHARGER_STATUS_NOT_CHARGING;
	}

	return 0;
}

static int pca9422_charger_get_online(const struct device *dev, enum charger_online *online)
{
	const struct charger_pca9422_config *const config = dev->config;
	uint8_t val;
	int ret;

	ret = mfd_pca9422_reg_read_byte(config->mfd, PCA9422_REG_DEVICE_0_STS, &val);
	if (ret < 0) {
		return ret;
	}

	if (val & PCA9422_BIT_VIN_OK) {
		*online = CHARGER_ONLINE_FIXED;
	} else {
		*online = CHARGER_ONLINE_OFFLINE;
	}

	return 0;
}

static int pca9422_charger_set_constant_charge_current(const struct device *dev,
						       uint32_t current_ua)
{
	const struct charger_pca9422_config *const config = dev->config;
	struct charger_pca9422_data *data = dev->data;
	uint16_t idx;
	uint8_t val;
	uint8_t num_ranges;
	int ret;

	/* Lock mutex */
	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Unlock charger control register */
	val = PCA9422_CHARGER_UNLOCK;
	ret = mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);
	if (ret < 0) {
		goto lock;
	}

	current_ua = CLAMP(current_ua, CURRENT_STEP_2P5MA_MIN_UA, CURRENT_STEP_5P0MA_MAX_UA);

	if ((data->chg_current_step == CHG_CURRENT_STEP_2P5MA) &&
	    (current_ua > CURRENT_STEP_2P5MA_MAX_UA)) {
		val = CHG_CURRENT_STEP_5P0MA;
		val = FIELD_PREP(PCA9422_BIT_CHG_CURRENT_STEP, val);
		ret = mfd_pca9422_reg_update_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_1,
						  PCA9422_BIT_CHG_CURRENT_STEP, val);
		if (ret < 0) {
			goto lock;
		}
		data->chg_current_step = CHG_CURRENT_STEP_5P0MA;
	}

	num_ranges = ARRAY_SIZE(i_fast_chg_ua_range);
	ret = linear_range_group_get_win_index(i_fast_chg_ua_range, num_ranges, current_ua,
					       current_ua, &idx);
	if (ret == -EINVAL) {
		goto lock;
	}
	ret = mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_3, (uint8_t)idx);
	if (ret == 0) {
		data->i_fast_chg_ua = current_ua;
	}

lock:
	/* Lock charger control register */
	val = PCA9422_CHARGER_LOCK;
	(void)mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);

	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int pca9422_charger_set_constant_charge_voltage(const struct device *dev,
						       uint32_t voltage_uv)
{
	const struct charger_pca9422_config *const config = dev->config;
	struct charger_pca9422_data *data = dev->data;
	uint16_t idx;
	uint8_t val;
	uint8_t num_ranges;
	int ret;

	/* Lock mutex */
	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Unlock charger control register */
	val = PCA9422_CHARGER_UNLOCK;
	ret = mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);
	if (ret < 0) {
		goto lock;
	}

	voltage_uv = CLAMP(voltage_uv, VBAT_REG_MIN_UV, VBAT_REG_MAX_UV);
	num_ranges = ARRAY_SIZE(vbat_reg_uv_range);
	ret = linear_range_group_get_win_index(vbat_reg_uv_range, num_ranges, voltage_uv,
					       voltage_uv, &idx);
	if (ret == -EINVAL) {
		goto lock;
	}

	ret = mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_2, (uint8_t)idx);
	if (ret == 0) {
		data->vbat_reg_uv = voltage_uv;
	}

lock:
	/* Lock charger control register */
	val = PCA9422_CHARGER_LOCK;
	(void)mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);

	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int pca9422_charger_set_topoff_current(const struct device *dev, uint32_t current_ua)
{
	const struct charger_pca9422_config *const config = dev->config;
	struct charger_pca9422_data *data = dev->data;
	uint8_t topoff_pct[4] = {25, 50, 75, 100}; /* 2.5%, 5.0%, 7.5%, 10.0% - permilliage */
	uint16_t cur_pct;
	uint8_t val;
	int ret;

	/* Lock mutex */
	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Unlock charger control register */
	val = PCA9422_CHARGER_UNLOCK;
	ret = mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);
	if (ret < 0) {
		goto lock;
	}

	cur_pct = current_ua * 1000U / data->i_fast_chg_ua; /* 0.1% - permilliage */

	data->i_topoff_sel = 4;
	for (int i = 0; i < 4; i++) {
		if (cur_pct <= topoff_pct[i]) {
			data->i_topoff_sel = i;
			break;
		}
	}

	/* Topoff current */
	val = FIELD_PREP(PCA9422_BIT_TOP_OFF_CURRENT, data->i_topoff_sel);
	ret = mfd_pca9422_reg_update_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_4,
					  PCA9422_BIT_TOP_OFF_CURRENT, val);
lock:
	/* Lock charger control register */
	val = PCA9422_CHARGER_LOCK;
	(void)mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);

	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int pca9422_charger_set_vsys_regulation_voltage(const struct device *dev,
						       uint32_t voltage_uv)
{
	const struct charger_pca9422_config *const config = dev->config;
	uint16_t idx;
	uint8_t num_ranges;
	int ret;

	voltage_uv = CLAMP(voltage_uv, VSYS_REG_MIN_UV, VSYS_REG_MAX_UV);
	num_ranges = ARRAY_SIZE(vsys_reg_uv_range);
	ret = linear_range_group_get_win_index(vsys_reg_uv_range, num_ranges, voltage_uv,
					       voltage_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}
	idx = FIELD_PREP(PCA9422_BIT_VSYS_REG, idx);

	return mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_VIN_CNTL_3, (uint8_t)idx);
}

static int pca9422_charger_set_input_regulation_current(const struct device *dev,
							uint32_t current_ua)
{
	const struct charger_pca9422_config *const config = dev->config;
	uint16_t idx;
	uint8_t num_ranges;
	int ret;

	current_ua = CLAMP(current_ua, VIN_I_LIMIT_MIN_UA, VIN_I_LIMIT_MAX_UA);
	num_ranges = ARRAY_SIZE(vin_i_limit_ua_range);
	ret = linear_range_group_get_win_index(vin_i_limit_ua_range, num_ranges, current_ua,
					       current_ua, &idx);
	if (ret == -EINVAL) {
		return ret;
	}
	return mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_VIN_CNTL_2, (uint8_t)idx);
}

static int pca9422_charger_set_enabled(const struct device *dev, bool enable)
{
	const struct charger_pca9422_config *const config = dev->config;
	struct charger_pca9422_data *data = dev->data;
	uint8_t val;
	int ret;

	/* Lock mutex */
	k_mutex_lock(&data->mutex, K_FOREVER);

	/* Unlock charger control register */
	val = PCA9422_CHARGER_UNLOCK;
	ret = mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);
	if (ret < 0) {
		goto lock;
	}

	data->charger_enabled = enable;

	val = FIELD_PREP(PCA9422_BIT_CHARGER_EN, enable);
	ret = mfd_pca9422_reg_update_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_1,
					  PCA9422_BIT_CHARGER_EN, val);

lock:
	/* Lock charger control register */
	val = PCA9422_CHARGER_LOCK;
	(void)mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);

	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int pca9422_charger_set_config(const struct device *dev)
{
	const struct charger_pca9422_config *const config = dev->config;
	struct charger_pca9422_data *data = dev->data;
	uint8_t val;
	int ret;

	/* Input limit current */
	ret = pca9422_charger_set_input_regulation_current(dev, config->vin_i_limit_ua);
	if (ret < 0) {
		return ret;
	}
	/* System regulation voltage */
	ret = pca9422_charger_set_vsys_regulation_voltage(dev, config->vsys_reg_uv);
	if (ret < 0) {
		return ret;
	}
	/* Battery regulation voltage */
	ret = pca9422_charger_set_constant_charge_voltage(dev, data->vbat_reg_uv);
	if (ret < 0) {
		return ret;
	}
	/* Fast charge current */
	ret = pca9422_charger_set_constant_charge_current(dev, data->i_fast_chg_ua);
	if (ret < 0) {
		return ret;
	}

	/* Unlock charger control register */
	val = PCA9422_CHARGER_UNLOCK;
	ret = mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);
	if (ret < 0) {
		goto lock;
	}

	/* Precharge current */
	val = FIELD_PREP(PCA9422_BIT_PRECHG_CURRENT, data->i_prechg_sel);
	ret = mfd_pca9422_reg_update_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_1,
					  PCA9422_BIT_PRECHG_CURRENT, val);
	if (ret < 0) {
		goto lock;
	}
	/* Topoff current */
	val = FIELD_PREP(PCA9422_BIT_TOP_OFF_CURRENT, data->i_topoff_sel);
	ret = mfd_pca9422_reg_update_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_4,
					  PCA9422_BIT_TOP_OFF_CURRENT, val);
	if (ret < 0) {
		goto lock;
	}
	/* Recharge threshold */
	val = FIELD_PREP(PCA9422_BIT_RECHARGE_TH, data->recharge_th_sel);
	ret = mfd_pca9422_reg_update_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_4,
					  PCA9422_BIT_RECHARGE_TH, val);

lock:
	/* Lock charger control register */
	val = PCA9422_CHARGER_LOCK;
	(void)mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_CHARGER_CNTL_0, val);
	return ret;
}

static int pca9422_charger_get_prop(const struct device *dev, charger_prop_t prop,
				    union charger_propval *val)
{
	struct charger_pca9422_data *data = dev->data;
	uint8_t precharge_pct[2] = {7, 16};        /* 7%, 16% */
	uint8_t topoff_pct[4] = {25, 50, 75, 100}; /* 2.5%, 5.0%, 7.5%, 10.0% - permilliage */
	int ret;

	switch (prop) {
	case CHARGER_PROP_ONLINE:
		val->online = data->online;
		return 0;
	case CHARGER_PROP_STATUS:
		ret = pca9422_charger_get_status(dev, &data->status);
		if (ret < 0) {
			LOG_ERR("Failed to read charger status %d", ret);
		} else {
			val->status = data->status;
		}
		return ret;
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		val->const_charge_current_ua = data->i_fast_chg_ua;
		return 0;
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		val->const_charge_voltage_uv = data->vbat_reg_uv;
		return 0;
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		val->precharge_current_ua =
			data->i_fast_chg_ua * precharge_pct[data->i_prechg_sel] / 100U;
		return 0;
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		val->charge_term_current_ua =
			data->i_fast_chg_ua * topoff_pct[data->i_topoff_sel] / 1000U;
		return 0;
	default:
		return -ENOTSUP;
	}
	return ret;
}

static int pca9422_charger_set_prop(const struct device *dev, charger_prop_t prop,
				    const union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return pca9422_charger_set_constant_charge_current(dev,
								   val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return pca9422_charger_set_constant_charge_voltage(dev,
								   val->const_charge_voltage_uv);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return pca9422_charger_set_input_regulation_current(
			dev, val->input_current_regulation_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return pca9422_charger_set_topoff_current(dev, val->charge_term_current_ua);
	default:
		return -ENOTSUP;
	}
}

static void pca9422_charger_isr(const struct device *dev)
{
	const struct charger_pca9422_config *const config = dev->config;
	struct charger_pca9422_data *data = dev->data;
	uint8_t int_val[6];
	uint8_t mask_val[6];
	int ret;

	/* Read charger interrupt register */
	/* Read int_device interrupt 0, 1, int_charger 0, 1, 2, and 3 */
	ret = mfd_pca9422_reg_burst_read(config->mfd, PCA9422_REG_INT_DEVICE_0, int_val, 6);
	if (ret == 0) {
		/* Check interrupt event */
		LOG_DBG("%s: int_device[0]=0x%x,  [1]=0x%x\n", __func__, int_val[0], int_val[1]);
		LOG_DBG("%s: int_charger[0]=0x%x, [1]=0x%x, [2]=0x%x, [3]=0x%x\n", __func__,
			int_val[2], int_val[3], int_val[4], int_val[5]);
	} else {
		LOG_ERR("%s: INT_DEVICE_0 ~ INT_CHARGER_3 read fail(%d)\n", __func__, ret);
	}
	/* Read mask registers */
	ret = mfd_pca9422_reg_burst_read(config->mfd, PCA9422_REG_INT_DEVICE_0_MASK, mask_val, 6);
	if (ret == 0) {
		/* Check interrupt event */
		LOG_DBG("%s: int_device_mask[0]=0x%x,  [1]=0x%x\n", __func__, mask_val[0],
			mask_val[1]);
		LOG_DBG("%s: int_charger_mask[0]=0x%x, [1]=0x%x, [2]=0x%x, [3]=0x%x\n", __func__,
			mask_val[2], mask_val[3], mask_val[4], mask_val[5]);
	} else {
		LOG_ERR("%s: INT_DEVICE_0_MASK ~ INT_CHARGER_3_MASK read fail(%d)\n", __func__,
			ret);
	}
	/* Set event */
	if ((int_val[0] & PCA9422_BIT_VIN_OK) && (~mask_val[0] & PCA9422_BIT_VIN_OK)) {
		/* Check status register */
		(void)pca9422_charger_get_online(dev, &data->online);
		LOG_DBG("%s: VIN_OK INT - online=%d\n", __func__, data->online);
	}
	if ((int_val[0] & PCA9422_BIT_VIN_NOK) && (~mask_val[0] & PCA9422_BIT_VIN_NOK)) {
		/* Check status register */
		(void)pca9422_charger_get_online(dev, &data->online);
		LOG_DBG("%s: VIN_NOK INT - online=%d\n", __func__, data->online);
	}
}

static int pca9422_charger_init(const struct device *dev)
{
	const struct charger_pca9422_config *const config = dev->config;
	struct charger_pca9422_data *data = dev->data;
	int ret;
	uint8_t val;
	uint8_t int_val[6];

	k_mutex_init(&data->mutex);

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	ret = pca9422_charger_set_config(dev);
	if (ret < 0) {
		return ret;
	}

	/* Get initial properties */
	ret = pca9422_charger_get_status(dev, &data->status);
	if (ret < 0) {
		return ret;
	}

	ret = pca9422_charger_get_online(dev, &data->online);
	if (ret < 0) {
		return ret;
	}

	/* Set interrupt handler */
	mfd_pca9422_set_irqhandler(config->mfd, dev, PCA9422_DEV_CHG, pca9422_charger_isr);

	/* Clear pending interrupt */
	ret = mfd_pca9422_reg_burst_read(config->mfd, PCA9422_REG_INT_DEVICE_0, int_val, 6);
	if (ret < 0) {
		return ret;
	}

	/* Set interrupt mask register */
	val = (uint8_t)~(PCA9422_BIT_VIN_OK | PCA9422_BIT_VIN_NOK);
	return mfd_pca9422_reg_write_byte(config->mfd, PCA9422_REG_INT_DEVICE_0_MASK, val);
}

static DEVICE_API(charger, pca9422_charger_driver_api) = {
	.get_property = pca9422_charger_get_prop,
	.set_property = pca9422_charger_set_prop,
	.charge_enable = pca9422_charger_set_enabled,
};

#define CHARGER_PCA9422_DEFINE(inst)                                                               \
	static struct charger_pca9422_data charger_pca9422_data_##inst = {                         \
		.i_fast_chg_ua = DT_INST_PROP(inst, constant_charge_current_max_microamp),         \
		.vbat_reg_uv = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),          \
		.recharge_th_sel = DT_INST_ENUM_IDX(inst, re_charge_threshold_microvolt),          \
		.i_prechg_sel = DT_INST_ENUM_IDX(inst, precharge_current_percent),                 \
		.i_topoff_sel = DT_INST_ENUM_IDX(inst, charge_termination_current_percent),        \
	};                                                                                         \
	static const struct charger_pca9422_config charger_pca9422_config_##inst = {               \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.vin_i_limit_ua = DT_INST_PROP(inst, input_current_limit_microamp),                \
		.vsys_reg_uv = DT_INST_PROP(inst, system_voltage_min_threshold_microvolt),         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &pca9422_charger_init, NULL, &charger_pca9422_data_##inst,     \
			      &charger_pca9422_config_##inst, POST_KERNEL,                         \
			      CONFIG_CHARGER_INIT_PRIORITY, &pca9422_charger_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CHARGER_PCA9422_DEFINE)
