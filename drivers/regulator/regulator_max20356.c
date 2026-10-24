/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max20356_regulator

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/max20356.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/regulator/max20356.h>
#include <zephyr/dt-bindings/mfd/max20356.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

#include "mfd_max20356.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(regulator_max20356, CONFIG_REGULATOR_LOG_LEVEL);

/* <reg>En[1:0] "Enabled" code (01). */
#define MAX20356_REG_EN_ENABLED 0x1U

/* <reg>En[1:0] "Controlled by MPC" code (10). */
#define MAX20356_REG_EN_MPC 0x2U

/* Buck<n>DvsCfg[4:0] DVS-mode codes. */
#define MAX20356_DVS_CFG_I2C      0x00U
#define MAX20356_DVS_CFG_GPIO_MIN 0x01U
#define MAX20356_DVS_CFG_GPIO_MAX 0x1CU
#define MAX20356_DVS_CFG_SPI      0x1DU

/* adi,dvs-mode enum indices (must match the binding's enum order). */
#define MAX20356_DVS_MODE_I2C  0
#define MAX20356_DVS_MODE_GPIO 1
#define MAX20356_DVS_MODE_SPI  2

/* SPI DVS command byte: {AD[1:0], VLT[5:0]}. */
#define MAX20356_DVS_SPI_ADD_MSK GENMASK(7, 6)
#define MAX20356_DVS_SPI_VLT_MSK GENMASK(5, 0)

/* SPI DVS repurposes MPC0/1/2 as the SPI interface. */
#define MAX20356_DVS_SPI_PIN_MSK (BIT(0) | BIT(1) | BIT(2))

/* Buck output: 0.5V base, per-rail step (Buck<n>VSet[5:0], codes 0x00..0x3F). */
static const struct linear_range __maybe_unused buck_range_10mv =
	LINEAR_RANGE_INIT(500000, 10000U, 0x00U, 0x3FU);
static const struct linear_range __maybe_unused buck_range_25mv =
	LINEAR_RANGE_INIT(500000, 25000U, 0x00U, 0x3FU);
static const struct linear_range __maybe_unused buck_range_50mv =
	LINEAR_RANGE_INIT(500000, 50000U, 0x00U, 0x3FU);

/* Buck-boost: 2.5V-5.5V, 50mV steps (BBstVSet[5:0]); codes above 0x3C not used. */
static const struct linear_range __maybe_unused bbst_range =
	LINEAR_RANGE_INIT(2500000, 50000U, 0x00U, 0x3CU);

/* LDO1/LDO2: 0.9V-4.0V, 100mV steps (LDO<n>VSet[4:0]). */
static const struct linear_range __maybe_unused ldo1_2_range =
	LINEAR_RANGE_INIT(900000, 100000U, 0x00U, 0x1FU);

/* LDO3: 0.9V-4.075V, 25mV steps (LDO3VSet[6:0]). */
static const struct linear_range __maybe_unused ldo3_range =
	LINEAR_RANGE_INIT(900000, 25000U, 0x00U, 0x7FU);

/* LDO4 (RTC): LDO4VSet base (1.2V/1.8V) plus LDO4VInc (0/25/50mV). */
static const int32_t ldo4_voltages[] = {
	1200000, 1225000, 1250000, 1800000, 1825000, 1850000,
};

/* Buck ISet / buck-boost BBstIPSet1: 0-375mA, 25mA steps (4-bit field). */
static const struct linear_range __maybe_unused iset_ua_range =
	LINEAR_RANGE_INIT(0, 25000U, 0x00U, 0x0FU);

/* One masked register write applied at init from devicetree. is_seq marks a
 * write to the rail's <rail>Ena.Seq[7:5] field, which is guarded by a separate
 * LockMsk2 domain (desc->seq_lock) rather than the rail's main lock domain.
 */
struct regulator_max20356_init_reg {
	uint8_t reg;
	uint8_t mask;
	uint8_t val;
	bool is_seq;
};

struct regulator_max20356_desc {
	uint8_t ena_reg;
	uint8_t ena_mask;
	uint8_t vset_reg;
	uint8_t vset_mask;
	uint8_t cfg_reg;
	uint8_t actdsc_mask;
	uint8_t mode_mask;
	uint8_t iset_reg;
	uint8_t iset_mask;
	/* DVS registers (bucks only; 0 elsewhere). */
	uint8_t dvscfg0_reg;
	uint8_t dvsvlt_reg[4];
	uint8_t dvsspi_reg;
	uint8_t spi_add;
	/* MPC enable-routing register (<rail>Ctr) and this rail's map selector. */
	uint8_t ctr_reg;
	uint8_t rail_sel;
	enum max20356_lock_domain lock;
	/* LockMsk2 domain guarding this rail's <rail>Ena.Seq field. Only valid
	 * for lockable rails (bucks, buck-boost, LDOs).
	 */
	enum max20356_lock_domain seq_lock;
	/* Fault-status sources (Status2/Status3). A zero mask means the rail has
	 * no source of that class.
	 */
	uint8_t ot_status_reg; /* REGULATOR_ERROR_OVER_TEMP */
	uint8_t ot_status_mask;
	uint8_t oc_status_reg; /* REGULATOR_ERROR_OVER_CURRENT */
	uint8_t oc_status_mask;
	bool lockable;
	bool is_ldo4;
	bool is_buck;
};

struct regulator_max20356_config {
	struct regulator_common_config common;
	const struct device *mfd_dev;
	const struct regulator_max20356_desc *desc;
	const struct linear_range *uv_range;
	const struct regulator_max20356_init_reg *init_regs;
	uint8_t num_init_regs;
	uint8_t mpc_ctr_mask;
	uint8_t mpc_all_mask;
	uint32_t dvs_voltages[4];
	uint32_t dvs_valley_ua;
	uint8_t dvs_mode;
	uint8_t dvs_mpc_pair[2];
	uint8_t dvs_mpc_pair_len;
	bool ldo4_rtc;
	/* DT adi,lock-enable: route this rail's writes through the password
	 * sequence. Only used for regulators whose desc->lockable is set.
	 */
	bool lock_enable;
};

struct regulator_max20356_data {
	struct regulator_common_data common;
};

struct regulator_max20356_common_config {
	const struct device *mfd_dev;
};

/* Route a register update through the password sequence for lockable rails
 * (bucks, buck-boost, LDOs) whose adi,lock-enable is set, or a plain update for
 * the load switches and for rails the devicetree marks as already unlocked.
 *
 * A write to the rail's <rail>Ena.Seq[7:5] field is guarded by a distinct
 * LockMsk2 domain (desc->seq_lock), so seq writes on a locked rail must unlock
 * that domain rather than the rail's main one.
 */
static int regulator_max20356_reg_update_dom(const struct device *dev, uint8_t reg, uint8_t mask,
					     uint8_t val, bool is_seq)
{
	const struct regulator_max20356_config *config = dev->config;

	if (config->desc->lockable && config->lock_enable) {
		enum max20356_lock_domain domain =
			is_seq ? config->desc->seq_lock : config->desc->lock;

		return mfd_max20356_reg_update_locked(config->mfd_dev, domain, reg, mask, val);
	}

	return mfd_max20356_reg_update(config->mfd_dev, reg, mask, val);
}

static int regulator_max20356_reg_update(const struct device *dev, uint8_t reg, uint8_t mask,
					 uint8_t val)
{
	return regulator_max20356_reg_update_dom(dev, reg, mask, val, false);
}

static int regulator_max20356_enable(const struct device *dev)
{
	const struct regulator_max20356_config *config = dev->config;

	return regulator_max20356_reg_update(dev, config->desc->ena_reg, config->desc->ena_mask,
					     MAX20356_REG_EN_ENABLED);
}

static int regulator_max20356_disable(const struct device *dev)
{
	const struct regulator_max20356_config *config = dev->config;

	return regulator_max20356_reg_update(dev, config->desc->ena_reg, config->desc->ena_mask,
					     0U);
}

static int regulator_max20356_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_max20356_config *config = dev->config;

	if (config->desc->mode_mask == 0U) {
		return -ENOTSUP;
	}

	if (mode > MAX20356_MODE_LOAD_SWITCH) {
		return -ENOTSUP;
	}

	return regulator_max20356_reg_update(dev, config->desc->cfg_reg, config->desc->mode_mask,
					     (mode == MAX20356_MODE_LOAD_SWITCH)
						     ? config->desc->mode_mask
						     : 0U);
}

static int regulator_max20356_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_max20356_config *config = dev->config;
	uint8_t val;
	int ret;

	if (config->desc->mode_mask == 0U) {
		return -ENOTSUP;
	}

	ret = mfd_max20356_reg_read(config->mfd_dev, config->desc->cfg_reg, &val);
	if (ret != 0) {
		return ret;
	}

	*mode = ((val & config->desc->mode_mask) != 0U) ? MAX20356_MODE_LOAD_SWITCH
							: MAX20356_MODE_LDO;
	return 0;
}

static int regulator_max20356_set_active_discharge(const struct device *dev, bool active_discharge)
{
	const struct regulator_max20356_config *config = dev->config;

	if (config->desc->actdsc_mask == 0U) {
		return -ENOTSUP;
	}

	return regulator_max20356_reg_update(dev, config->desc->cfg_reg, config->desc->actdsc_mask,
					     active_discharge ? config->desc->actdsc_mask : 0U);
}

static int regulator_max20356_get_active_discharge(const struct device *dev, bool *active_discharge)
{
	const struct regulator_max20356_config *config = dev->config;
	uint8_t val;
	int ret;

	if (config->desc->actdsc_mask == 0U) {
		return -ENOTSUP;
	}

	ret = mfd_max20356_reg_read(config->mfd_dev, config->desc->cfg_reg, &val);
	if (ret != 0) {
		return ret;
	}

	*active_discharge = (val & config->desc->actdsc_mask) != 0U;
	return 0;
}

static unsigned int regulator_max20356_count_voltages(const struct device *dev)
{
	const struct regulator_max20356_config *config = dev->config;

	if (config->desc->is_ldo4) {
		return ARRAY_SIZE(ldo4_voltages);
	}

	if (config->uv_range == NULL) {
		return 0;
	}

	return linear_range_values_count(config->uv_range);
}

static int regulator_max20356_list_voltage(const struct device *dev, unsigned int idx,
					   int32_t *volt_uv)
{
	const struct regulator_max20356_config *config = dev->config;

	if (config->desc->is_ldo4) {
		if (idx >= ARRAY_SIZE(ldo4_voltages)) {
			return -EINVAL;
		}

		*volt_uv = ldo4_voltages[idx];
		return 0;
	}

	if (config->uv_range == NULL) {
		return -EINVAL;
	}

	return linear_range_get_value(config->uv_range, idx, volt_uv);
}

static int regulator_max20356_ldo4_set_voltage(const struct device *dev, int32_t min_uv,
					       int32_t max_uv)
{
	uint8_t mask = MAX20356_LDO4CFG_LDO4VSET_MSK | MAX20356_LDO4CFG_LDO4VINC_MSK;

	ARRAY_FOR_EACH(ldo4_voltages, i) {
		if ((ldo4_voltages[i] >= min_uv) && (ldo4_voltages[i] <= max_uv)) {
			uint8_t vset = (i >= 3U) ? 1U : 0U;
			uint8_t vinc = i % 3U;
			uint8_t val = FIELD_PREP(MAX20356_LDO4CFG_LDO4VSET_MSK, vset) |
				      FIELD_PREP(MAX20356_LDO4CFG_LDO4VINC_MSK, vinc);

			return regulator_max20356_reg_update(dev, MAX20356_REG_LDO4CFG, mask, val);
		}
	}

	return -EINVAL;
}

static int regulator_max20356_ldo4_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_max20356_config *config = dev->config;
	uint8_t val, vinc;
	int ret;

	ret = mfd_max20356_reg_read(config->mfd_dev, MAX20356_REG_LDO4CFG, &val);
	if (ret != 0) {
		return ret;
	}

	vinc = FIELD_GET(MAX20356_LDO4CFG_LDO4VINC_MSK, val);
	if (vinc > 2U) {
		vinc = 2U;
	}

	*volt_uv = ((val & MAX20356_LDO4CFG_LDO4VSET_MSK) != 0U ? 1800000 : 1200000) +
		   ((int32_t)vinc * 25000);
	return 0;
}

/* SPI DVS: clock one {AD[1:0], VLT[5:0]} command byte to set the buck voltage.
 * Public: see regulator_max20356_spi_set_voltage() in the driver's public header.
 */
int regulator_max20356_spi_set_voltage(const struct device *dev, int min_uv, int max_uv)
{
	const struct regulator_max20356_config *config = dev->config;
	uint16_t idx;
	uint8_t cmd;
	int ret;

	if (!config->desc->is_buck || (config->dvs_mode != MAX20356_DVS_MODE_SPI) ||
	    (config->uv_range == NULL)) {
		return -ENOTSUP;
	}

	ret = linear_range_get_win_index(config->uv_range, min_uv, max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	cmd = FIELD_PREP(MAX20356_DVS_SPI_ADD_MSK, config->desc->spi_add) |
	      FIELD_PREP(MAX20356_DVS_SPI_VLT_MSK, idx);

	return mfd_max20356_dvs_spi_write(config->mfd_dev, cmd);
}

static int regulator_max20356_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_max20356_config *config = dev->config;
	uint16_t idx;
	int ret;

	if (config->desc->is_ldo4) {
		return regulator_max20356_ldo4_set_voltage(dev, min_uv, max_uv);
	}

	if (config->uv_range == NULL) {
		return -ENOTSUP;
	}

	if (config->desc->is_buck) {
		switch (config->dvs_mode) {
		case MAX20356_DVS_MODE_SPI:
			return regulator_max20356_spi_set_voltage(dev, min_uv, max_uv);
		case MAX20356_DVS_MODE_GPIO:
			/* Live voltage is chosen by the external MPC pins from the
			 * presets programmed at init; there is no single setpoint.
			 */
			return -ENOTSUP;
		default:
			break;
		}
	}

	ret = linear_range_get_win_index(config->uv_range, min_uv, max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return regulator_max20356_reg_update(dev, config->desc->vset_reg, config->desc->vset_mask,
					     FIELD_PREP(config->desc->vset_mask, idx));
}

static int regulator_max20356_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_max20356_config *config = dev->config;
	uint8_t reg, val;
	int ret;

	if (config->desc->is_ldo4) {
		return regulator_max20356_ldo4_get_voltage(dev, volt_uv);
	}

	if (config->uv_range == NULL) {
		return -ENOTSUP;
	}

	/* SPI-set voltage is mirrored in Buck<n>DvsSpi and read back over I2C. */
	reg = (config->desc->is_buck && (config->dvs_mode == MAX20356_DVS_MODE_SPI))
		      ? config->desc->dvsspi_reg
		      : config->desc->vset_reg;

	ret = mfd_max20356_reg_read(config->mfd_dev, reg, &val);
	if (ret != 0) {
		return ret;
	}

	return linear_range_get_value(config->uv_range,
				      FIELD_GET(config->desc->vset_mask, val), volt_uv);
}

static unsigned int regulator_max20356_count_current_limits(const struct device *dev)
{
	const struct regulator_max20356_config *config = dev->config;

	if (config->desc->iset_mask == 0U) {
		return 0;
	}

	return linear_range_values_count(&iset_ua_range);
}

static int regulator_max20356_list_current_limit(const struct device *dev, unsigned int idx,
						 int32_t *current_ua)
{
	const struct regulator_max20356_config *config = dev->config;

	if (config->desc->iset_mask == 0U) {
		return -ENOTSUP;
	}

	return linear_range_get_value(&iset_ua_range, idx, current_ua);
}

static int regulator_max20356_set_current_limit(const struct device *dev, int32_t min_ua,
						int32_t max_ua)
{
	const struct regulator_max20356_config *config = dev->config;
	uint16_t idx;
	int ret;

	if (config->desc->iset_mask == 0U) {
		return -ENOTSUP;
	}

	ret = linear_range_get_win_index(&iset_ua_range, min_ua, max_ua, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return regulator_max20356_reg_update(dev, config->desc->iset_reg, config->desc->iset_mask,
					     FIELD_PREP(config->desc->iset_mask, idx));
}

static int regulator_max20356_get_current_limit(const struct device *dev, int32_t *curr_ua)
{
	const struct regulator_max20356_config *config = dev->config;
	uint8_t val;
	int ret;

	if (config->desc->iset_mask == 0U) {
		return -ENOTSUP;
	}

	ret = mfd_max20356_reg_read(config->mfd_dev, config->desc->iset_reg, &val);
	if (ret != 0) {
		return ret;
	}

	return linear_range_get_value(&iset_ua_range, FIELD_GET(config->desc->iset_mask, val),
				      curr_ua);
}

static int regulator_max20356_get_error_flags(const struct device *dev,
					      regulator_error_flags_t *flags)
{
	const struct regulator_max20356_config *config = dev->config;
	const struct regulator_max20356_desc *desc = config->desc;
	uint8_t val;
	int ret;

	*flags = 0U;

	if (desc->ot_status_mask != 0U) {
		ret = mfd_max20356_reg_read(config->mfd_dev, desc->ot_status_reg, &val);
		if (ret != 0) {
			return ret;
		}

		if ((val & desc->ot_status_mask) != 0U) {
			*flags |= REGULATOR_ERROR_OVER_TEMP;
		}
	}

	if (desc->oc_status_mask != 0U) {
		if ((desc->oc_status_reg != desc->ot_status_reg) || (desc->ot_status_mask == 0U)) {
			ret = mfd_max20356_reg_read(config->mfd_dev, desc->oc_status_reg, &val);
			if (ret != 0) {
				return ret;
			}
		}

		if ((val & desc->oc_status_mask) != 0U) {
			*flags |= REGULATOR_ERROR_OVER_CURRENT;
		}
	}

	return 0;
}

/* MPC pin pair (a<b over MPC0..7) to the Buck<n>DvsCfg[4:0] GPIO code. */
static int regulator_max20356_dvs_gpio_code(uint8_t a, uint8_t b, uint8_t *code)
{
	uint8_t val = MAX20356_DVS_CFG_GPIO_MIN;

	if ((a >= b) || (b > 7U)) {
		return -EINVAL;
	}

	for (uint8_t i = 0U; i < 8U; i++) {
		for (uint8_t j = i + 1U; j < 8U; j++) {
			if ((i == a) && (j == b)) {
				*code = val;
				return 0;
			}
			val++;
		}
	}

	return -EINVAL;
}

static int regulator_max20356_dvs_init(const struct device *dev)
{
	const struct regulator_max20356_config *config = dev->config;
	const struct regulator_max20356_desc *desc = config->desc;
	uint8_t cfg;
	int ret;

	switch (config->dvs_mode) {
	case MAX20356_DVS_MODE_I2C:
		cfg = MAX20356_DVS_CFG_I2C;
		break;
	case MAX20356_DVS_MODE_SPI:
		if ((config->mpc_all_mask & MAX20356_DVS_SPI_PIN_MSK) != 0U) {
			LOG_ERR("spi DVS needs MPC0/1/2, but adi,mpc-enable-rails routes one of "
				"them to a rail enable");
			return -EINVAL;
		}

		cfg = MAX20356_DVS_CFG_SPI;
		break;
	case MAX20356_DVS_MODE_GPIO:
		if (config->dvs_mpc_pair_len != 2U) {
			LOG_ERR("gpio DVS needs adi,dvs-mpc-pair of two MPC pins");
			return -EINVAL;
		}

		if ((config->mpc_all_mask & (BIT(config->dvs_mpc_pair[0]) |
					     BIT(config->dvs_mpc_pair[1]))) != 0U) {
			LOG_ERR("gpio DVS pin also routed to a rail enable via "
				"adi,mpc-enable-rails");
			return -EINVAL;
		}

		ret = regulator_max20356_dvs_gpio_code(config->dvs_mpc_pair[0],
						       config->dvs_mpc_pair[1], &cfg);
		if (ret != 0) {
			LOG_ERR("invalid adi,dvs-mpc-pair");
			return ret;
		}

		/* Program the four preset voltages selected by the MPC pin states. */
		for (uint8_t i = 0U; i < 4U; i++) {
			uint16_t idx;

			ret = linear_range_get_index(config->uv_range, config->dvs_voltages[i],
						     &idx);
			if (ret != 0) {
				LOG_ERR("adi,dvs-voltages[%u] out of range", i);
				return -EINVAL;
			}

			ret = regulator_max20356_reg_update(dev, desc->dvsvlt_reg[i],
							    desc->vset_mask,
							    FIELD_PREP(desc->vset_mask, idx));
			if (ret != 0) {
				return ret;
			}
		}
		break;
	default:
		return -EINVAL;
	}

	/* Program DvsCfg[4:0] and the valley current (DvsCur). */
	cfg = FIELD_PREP(MAX20356_BUCK1DVSCFG0_BUCK1DVSCFG_MSK, cfg);
	if (config->dvs_valley_ua == 1000000U) {
		cfg |= MAX20356_BUCK1DVSCFG0_BUCK1DVSCUR_MSK;
	}

	return regulator_max20356_reg_update(dev, desc->dvscfg0_reg,
					     MAX20356_BUCK1DVSCFG0_BUCK1DVSCFG_MSK |
						     MAX20356_BUCK1DVSCFG0_BUCK1DVSCUR_MSK,
					     cfg);
}

static int regulator_max20356_init_regs(const struct device *dev)
{
	const struct regulator_max20356_config *config = dev->config;
	int ret;

	for (uint8_t i = 0U; i < config->num_init_regs; i++) {
		const struct regulator_max20356_init_reg *e = &config->init_regs[i];

		if (e->mask == 0U) {
			continue;
		}

		ret = regulator_max20356_reg_update_dom(dev, e->reg, e->mask, e->val, e->is_seq);
		if (ret != 0) {
			LOG_ERR("Failed to init reg 0x%02x: %d", e->reg, ret);
			return ret;
		}
	}

	return 0;
}

/* Route the DT-selected MPC pins to this rail's enable control (<rail>Ctr) and
 * switch the rail's enable field to controlled-by-MPC (<rail>En = 10). No-op for
 * a rail with no pin mapped to it. Effective in hardware only when the rail is
 * also in sequencing slot 7 (adi,sequence-slot = <7>).
 */
static int regulator_max20356_mpc_init(const struct device *dev)
{
	const struct regulator_max20356_config *config = dev->config;
	int ret;

	if (config->mpc_ctr_mask == 0U) {
		return 0;
	}

	ret = regulator_max20356_reg_update(dev, config->desc->ctr_reg, config->mpc_ctr_mask,
					    config->mpc_ctr_mask);
	if (ret != 0) {
		return ret;
	}

	return regulator_max20356_reg_update(dev, config->desc->ena_reg, config->desc->ena_mask,
					     MAX20356_REG_EN_MPC);
}

static int regulator_max20356_init(const struct device *dev)
{
	const struct regulator_max20356_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->mfd_dev)) {
		LOG_ERR("MFD parent device not ready");
		return -ENODEV;
	}

	regulator_common_data_init(dev);

	/* Apply the DT-configured register block before the rail is brought up
	 * and before DVS init.
	 */
	ret = regulator_max20356_init_regs(dev);
	if (ret != 0) {
		return ret;
	}

	if (config->desc->is_buck) {
		ret = regulator_max20356_dvs_init(dev);
		if (ret != 0) {
			return ret;
		}
	}

	if (config->desc->is_ldo4 && config->ldo4_rtc) {
		if (mfd_max20356_get_variant(config->mfd_dev) != MAX20356_VARIANT_MAX20358) {
			LOG_ERR("LDO4RTC is only supported on the MAX20358");
			return -ENOTSUP;
		}

		ret = regulator_max20356_reg_update(dev, MAX20356_REG_LDO4CFG,
						    MAX20356_LDO4CFG_LDO4RTC_MSK,
						    MAX20356_LDO4CFG_LDO4RTC_MSK);
		if (ret != 0) {
			return ret;
		}
	}

	ret = regulator_common_init(dev, false);
	if (ret != 0) {
		return ret;
	}

	/* Program MPC enable routing last: regulator_common_init() enables a
	 * boot-on rail via the enable op (En = 01), which would conflict
	 * the controlled-by-MPC code (En = 10) written here.
	 */
	return regulator_max20356_mpc_init(dev);
}

static int regulator_max20356_ship_mode(const struct device *dev)
{
	const struct regulator_max20356_common_config *config = dev->config;

	return mfd_max20356_power_command(config->mfd_dev, MAX20356_PWR_OFF);
}

static int regulator_max20356_common_init(const struct device *dev)
{
	const struct regulator_max20356_common_config *config = dev->config;

	if (!device_is_ready(config->mfd_dev)) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(regulator_parent, parent_api) = {
	.ship_mode = regulator_max20356_ship_mode,
};

static DEVICE_API(regulator, api) = {
	.enable = regulator_max20356_enable,
	.disable = regulator_max20356_disable,
	.set_mode = regulator_max20356_set_mode,
	.get_mode = regulator_max20356_get_mode,
	.set_voltage = regulator_max20356_set_voltage,
	.get_voltage = regulator_max20356_get_voltage,
	.count_voltages = regulator_max20356_count_voltages,
	.list_voltage = regulator_max20356_list_voltage,
	.set_active_discharge = regulator_max20356_set_active_discharge,
	.get_active_discharge = regulator_max20356_get_active_discharge,
	.count_current_limits = regulator_max20356_count_current_limits,
	.list_current_limit = regulator_max20356_list_current_limit,
	.set_current_limit = regulator_max20356_set_current_limit,
	.get_current_limit = regulator_max20356_get_current_limit,
	.get_error_flags = regulator_max20356_get_error_flags,
};

/* Devicetree field-contribution helpers for the init register block. */
#define MAX20356_DT_BOOL(node_id, prop, msk) (DT_PROP(node_id, prop) ? (uint8_t)(msk) : 0U)
#define MAX20356_DT_ENUM_MASK(node_id, prop, msk)                                                  \
	(DT_NODE_HAS_PROP(node_id, prop) ? (uint8_t)(msk) : 0U)
#define MAX20356_DT_ENUM_VALUE(node_id, prop, msk)                                                 \
	((uint8_t)FIELD_PREP(msk, DT_ENUM_IDX_OR(node_id, prop, 0)))

#define MAX20356_REG_ENTRY(regmac, m_expr, v_expr) {(regmac), (m_expr), (v_expr)}

/* MPC enable-routing map fold. adi,mpc-enable-rails lives on the parent
 * (regulators) node; cell @pin holds a MAX20356_MPC_* selector. MAX20356_MPC_CELL
 * yields BIT(pin) when that cell selects this rail (@sel), else 0.
 */
#define MAX20356_MPC_CELL(node_id, pin, sel)                                                       \
	COND_CODE_1(DT_PROP_HAS_IDX(DT_PARENT(node_id), adi_mpc_enable_rails, pin),                \
		    ((DT_PROP_BY_IDX(DT_PARENT(node_id), adi_mpc_enable_rails, pin) == (sel))      \
			     ? BIT(pin)                                                            \
			     : 0U),                                                                \
		    (0U))

#define MAX20356_MPC_CTR_MASK(node_id, sel)                                                        \
	((uint8_t)(MAX20356_MPC_CELL(node_id, 0, sel) | MAX20356_MPC_CELL(node_id, 1, sel) |       \
		   MAX20356_MPC_CELL(node_id, 2, sel) | MAX20356_MPC_CELL(node_id, 3, sel) |       \
		   MAX20356_MPC_CELL(node_id, 4, sel) | MAX20356_MPC_CELL(node_id, 5, sel) |       \
		   MAX20356_MPC_CELL(node_id, 6, sel) | MAX20356_MPC_CELL(node_id, 7, sel)))

/* BIT(pin) when the parent map routes pin to any rail (cell != NONE), else 0. */
#define MAX20356_MPC_ANY_CELL(node_id, pin)                                                        \
	COND_CODE_1(DT_PROP_HAS_IDX(DT_PARENT(node_id), adi_mpc_enable_rails, pin),                \
		    ((DT_PROP_BY_IDX(DT_PARENT(node_id), adi_mpc_enable_rails, pin) !=             \
		      MAX20356_MPC_NONE)                                                           \
			     ? BIT(pin)                                                            \
			     : 0U),                                                                \
		    (0U))

/* Mask of every MPC pin routed to a rail's enable, across the whole map. */
#define MAX20356_MPC_ALL_MASK(node_id)                                                             \
	((uint8_t)(MAX20356_MPC_ANY_CELL(node_id, 0) | MAX20356_MPC_ANY_CELL(node_id, 1) |         \
		   MAX20356_MPC_ANY_CELL(node_id, 2) | MAX20356_MPC_ANY_CELL(node_id, 3) |         \
		   MAX20356_MPC_ANY_CELL(node_id, 4) | MAX20356_MPC_ANY_CELL(node_id, 5) |         \
		   MAX20356_MPC_ANY_CELL(node_id, 6) | MAX20356_MPC_ANY_CELL(node_id, 7)))

/* Sequencing slot entry: masked update on <rail>Ena[7:5], leaving En[1:0]. The
 * seq field lives in a separate LockMsk2 domain, so is_seq is set.
 */
#define MAX20356_SEQ_ENTRY(ena_reg, seq_msk, node_id)                                              \
	{(ena_reg), MAX20356_DT_ENUM_MASK(node_id, adi_sequence_slot, seq_msk),                    \
	 MAX20356_DT_ENUM_VALUE(node_id, adi_sequence_slot, seq_msk), true}

/* Buck Cfg0 bits. ActDsc is excluded (owned by set_active_discharge()).
 */
#define MAX20356_BUCK_CFG0_BITS(n, cfg_reg_id, node_id)                                            \
	(MAX20356_DT_BOOL(node_id, adi_buck_enb_integrator,                                        \
			    MAX20356_##cfg_reg_id##_BUCK##n##ENBINTGR_MSK) |                       \
	 MAX20356_DT_BOOL(node_id, adi_buck_pgood_enable,                                          \
			    MAX20356_##cfg_reg_id##_BUCK##n##PGOODENA_MSK) |                       \
	 MAX20356_DT_BOOL(node_id, adi_buck_fast, MAX20356_##cfg_reg_id##_BUCK##n##FAST_MSK) |     \
	 MAX20356_DT_BOOL(node_id, adi_buck_passive_discharge,                                     \
			    MAX20356_##cfg_reg_id##_BUCK##n##PSVDSC_MSK) |                         \
	 MAX20356_DT_BOOL(node_id, adi_buck_low_emi,                                               \
			    MAX20356_##cfg_reg_id##_BUCK##n##LOWEMI_MSK) |                         \
	 MAX20356_DT_BOOL(node_id, adi_buck_force_fet_scaling,                                     \
			    MAX20356_##cfg_reg_id##_BUCK##n##FET_MSK) |                            \
	 MAX20356_DT_BOOL(node_id, adi_buck_enable_lx_sense,                                       \
			    MAX20356_##cfg_reg_id##_BUCK##n##ENLXSNS_MSK))

#define MAX20356_BUCK_CFG1_BITS(n, node_id)                                                        \
	(MAX20356_DT_BOOL(node_id, adi_buck_low_bandwidth,                                         \
			    MAX20356_BUCK##n##CFG1_BUCK##n##LOWBW_MSK) |                           \
	 MAX20356_DT_BOOL(node_id, adi_buck_force_dcm,                                             \
			    MAX20356_BUCK##n##CFG1_BUCK##n##FRCDCM_MSK) |                          \
	 MAX20356_DT_BOOL(node_id, adi_buck_mpc2_fast,                                             \
			    MAX20356_BUCK##n##CFG1_BUCK##n##MPCFAST_MSK) |                         \
	 MAX20356_DT_BOOL(node_id, adi_buck_forced_pwm,                                            \
			    MAX20356_BUCK##n##CFG1_BUCK##n##FPWM_MSK) |                            \
	 MAX20356_DT_BOOL(node_id, adi_buck_enb_adaptive_peak,                                     \
			    MAX20356_BUCK##n##CFG1_BUCK##n##ENBIADPT_MSK))

#define MAX20356_BUCK_INIT_REGS(id, n, cfg_reg_id, node_id)                                        \
	static const struct regulator_max20356_init_reg regulator_max20356_init_regs_##id[] = {    \
		MAX20356_SEQ_ENTRY(MAX20356_REG_BUCK##n##ENA,                                      \
				   MAX20356_BUCK##n##ENA_BUCK##n##SEQ_MSK, node_id),               \
		MAX20356_REG_ENTRY(MAX20356_REG_##cfg_reg_id,                                      \
				   MAX20356_BUCK_CFG0_BITS(n, cfg_reg_id, node_id),                \
				   MAX20356_BUCK_CFG0_BITS(n, cfg_reg_id, node_id)),               \
		MAX20356_REG_ENTRY(MAX20356_REG_BUCK##n##CFG1,                                     \
				   MAX20356_BUCK_CFG1_BITS(n, node_id),                            \
				   MAX20356_BUCK_CFG1_BITS(n, node_id)),                           \
		MAX20356_REG_ENTRY(MAX20356_REG_BUCK##n##ISET,                                     \
				MAX20356_DT_BOOL(node_id, adi_buck_iset_lookup_disable,            \
						MAX20356_BUCK##n##ISET_BUCK##n##ISETLOOKUPB_MSK),  \
				MAX20356_DT_BOOL(node_id, adi_buck_iset_lookup_disable,            \
						MAX20356_BUCK##n##ISET_BUCK##n##ISETLOOKUPB_MSK)), \
		MAX20356_REG_ENTRY(MAX20356_REG_BUCK##n##DVSCFG0,                                  \
				MAX20356_DT_BOOL(node_id, adi_buck_dvs_ipmax,                      \
						MAX20356_BUCK##n##DVSCFG0_BUCK##n##DVSIPMAX_MSK),  \
				MAX20356_DT_BOOL(node_id, adi_buck_dvs_ipmax,                      \
						MAX20356_BUCK##n##DVSCFG0_BUCK##n##DVSIPMAX_MSK)), \
	}

#define MAX20356_BBST_CFG_BITS(node_id)                                                            \
	(MAX20356_DT_BOOL(node_id, adi_bbst_ipset_lookup_disable,                                  \
			    MAX20356_BBSTCFG_BBSTIPSETLOOKUPB_MSK) |                               \
	 MAX20356_DT_BOOL(node_id, adi_bbst_low_emi, MAX20356_BBSTCFG_BBSTLOWEMI_MSK) |            \
	 MAX20356_DT_BOOL(node_id, adi_bbst_ramp_enable, MAX20356_BBSTCFG_BBSTRAMPENA_MSK) |       \
	 MAX20356_DT_BOOL(node_id, adi_bbst_mode, MAX20356_BBSTCFG_BBSTMODE_MSK) |                 \
	 MAX20356_DT_BOOL(node_id, adi_bbst_passive_discharge,                                     \
			    MAX20356_BBSTCFG_BBSTPSVDISC_MSK))

#define MAX20356_BBST_CFG1_BOOLS(node_id)                                                          \
	(MAX20356_DT_BOOL(node_id, adi_bbst_ippad_adaptive_enable,                                 \
			    MAX20356_BBSTCFG1_BBSTIPPADPENB_MSK) |                                 \
	 MAX20356_DT_BOOL(node_id, adi_bbst_fast, MAX20356_BBSTCFG1_BBSTFAST_MSK) |                \
	 MAX20356_DT_BOOL(node_id, adi_bbst_zero_cross_comp_disable,                               \
			    MAX20356_BBSTCFG1_BBZCCMPENB_MSK) |                                    \
	 MAX20356_DT_BOOL(node_id, adi_bbst_force_fet_scaling, MAX20356_BBSTCFG1_BBSTFFET_MSK) |   \
	 MAX20356_DT_BOOL(node_id, adi_bbst_mpc1_fast, MAX20356_BBSTCFG1_BBSTMPC1FCT_MSK))

#define MAX20356_BBST_INIT_REGS(id, node_id)                                                       \
	static const struct regulator_max20356_init_reg regulator_max20356_init_regs_##id[] = {    \
		MAX20356_SEQ_ENTRY(MAX20356_REG_BBSTENA, MAX20356_BBSTENA_BBSTSEQ_MSK, node_id),   \
		MAX20356_REG_ENTRY(MAX20356_REG_BBSTCFG, MAX20356_BBST_CFG_BITS(node_id),          \
				   MAX20356_BBST_CFG_BITS(node_id)),                               \
		MAX20356_REG_ENTRY(MAX20356_REG_BBSTCFG1,                                          \
				   (MAX20356_BBST_CFG1_BOOLS(node_id) |                            \
				    MAX20356_DT_ENUM_MASK(node_id,                                 \
							  adi_bbst_fhigh_threshold_kilohertz,      \
							  MAX20356_BBSTCFG1_BBFHIGHSH_MSK)),       \
				   (MAX20356_BBST_CFG1_BOOLS(node_id) |                            \
				    MAX20356_DT_ENUM_VALUE(node_id,                                \
							   adi_bbst_fhigh_threshold_kilohertz,     \
							   MAX20356_BBSTCFG1_BBFHIGHSH_MSK))),     \
	}

#define MAX20356_LDO1_CFG_BITS(node_id)                                                            \
	(MAX20356_DT_BOOL(node_id, adi_ldo1_internal_supply,                                       \
			    MAX20356_LDO1CFG_LDO1INTSUP_MSK) |                                     \
	 MAX20356_DT_BOOL(node_id, adi_ldo1_passive_discharge,                                     \
			    MAX20356_LDO1CFG_LDO1PSVDSC_MSK))

#define MAX20356_LDO2_CFG_BITS(node_id)                                                            \
	(MAX20356_DT_BOOL(node_id, adi_ldo2_mpc0_config, MAX20356_LDO2CFG_LDO2_MPC0CNF_MSK) |      \
	 MAX20356_DT_BOOL(node_id, adi_ldo2_mpc0_control, MAX20356_LDO2CFG_LDO2_MPC0CNT_MSK) |     \
	 MAX20356_DT_BOOL(node_id, adi_ldo2_passive_discharge,                                     \
			    MAX20356_LDO2CFG_LDO2PSVDSC_MSK))

#define MAX20356_LDO3_CFG_BITS(node_id)                                                            \
	(MAX20356_DT_BOOL(node_id, adi_ldo3_mpc_config, MAX20356_LDO3CFG_LDO3_MPC_CNF_MSK) |       \
	 MAX20356_DT_BOOL(node_id, adi_ldo3_no_clip_protection,                                    \
			    MAX20356_LDO3CFG_LDO3_NOCLP_MSK) |                                     \
	 MAX20356_DT_BOOL(node_id, adi_ldo3_high_current_cout,                                     \
			    MAX20356_LDO3CFG_LDO3_HICOUT_MSK) |                                    \
	 MAX20356_DT_BOOL(node_id, adi_ldo3_force_high_current,                                    \
			    MAX20356_LDO3CFG_LDO3_FRC_HIC_MSK) |                                   \
	 MAX20356_DT_BOOL(node_id, adi_ldo3_proportional_mode,                                     \
			    MAX20356_LDO3CFG_LDO3_PMOD_MSK) |                                      \
	 MAX20356_DT_BOOL(node_id, adi_ldo3_passive_discharge,                                     \
			    MAX20356_LDO3CFG_LDO3PSVDSC_MSK))

#define MAX20356_LDO1_INIT_REGS(id, node_id)                                                       \
	static const struct regulator_max20356_init_reg regulator_max20356_init_regs_##id[] = {    \
		MAX20356_SEQ_ENTRY(MAX20356_REG_LDO1ENA, MAX20356_LDO1ENA_LDO1SEQ_MSK, node_id),   \
		MAX20356_REG_ENTRY(MAX20356_REG_LDO1CFG, MAX20356_LDO1_CFG_BITS(node_id),          \
				   MAX20356_LDO1_CFG_BITS(node_id)),                               \
	}

#define MAX20356_LDO2_INIT_REGS(id, node_id)                                                       \
	static const struct regulator_max20356_init_reg regulator_max20356_init_regs_##id[] = {    \
		MAX20356_SEQ_ENTRY(MAX20356_REG_LDO2ENA, MAX20356_LDO2ENA_LDO2SEQ_MSK, node_id),   \
		MAX20356_REG_ENTRY(MAX20356_REG_LDO2CFG, MAX20356_LDO2_CFG_BITS(node_id),          \
				   MAX20356_LDO2_CFG_BITS(node_id)),                               \
	}

#define MAX20356_LDO3_INIT_REGS(id, node_id)                                                       \
	static const struct regulator_max20356_init_reg regulator_max20356_init_regs_##id[] = {    \
		MAX20356_SEQ_ENTRY(MAX20356_REG_LDO3ENA, MAX20356_LDO3ENA_LDO3SEQ_MSK, node_id),   \
		MAX20356_REG_ENTRY(MAX20356_REG_LDO3CFG, MAX20356_LDO3_CFG_BITS(node_id),          \
				   MAX20356_LDO3_CFG_BITS(node_id)),                               \
	}

#define MAX20356_LDO4_INIT_REGS(id, node_id)                                                       \
	static const struct regulator_max20356_init_reg regulator_max20356_init_regs_##id[] = {    \
		MAX20356_SEQ_ENTRY(MAX20356_REG_LDO4ENA, MAX20356_LDO4ENA_LDO4SEQ_MSK, node_id),   \
		MAX20356_REG_ENTRY(MAX20356_REG_LDO4CFG,                                           \
				   MAX20356_DT_BOOL(node_id, adi_ldo4_passive_discharge,           \
						      MAX20356_LDO4CFG_LDO4PSVDSC_MSK),            \
				   MAX20356_DT_BOOL(node_id, adi_ldo4_passive_discharge,           \
						      MAX20356_LDO4CFG_LDO4PSVDSC_MSK)),           \
	}

#define MAX20356_LSW_CFG_BITS(n, node_id)                                                          \
	(MAX20356_DT_BOOL(node_id, adi_lsw_low_iq, MAX20356_LSW##n##CFG_LSW##n##LOWIQ_MSK) |       \
	 MAX20356_DT_BOOL(node_id, adi_lsw_passive_discharge,                                      \
			    MAX20356_LSW##n##CFG_LSW##n##PSVDSC_MSK))

#define MAX20356_LSW_INIT_REGS(id, n, node_id)                                                     \
	static const struct regulator_max20356_init_reg regulator_max20356_init_regs_##id[] = {    \
		MAX20356_SEQ_ENTRY(MAX20356_REG_LSW##n##ENA,                                       \
				   MAX20356_LSW##n##ENA_LSW##n##SEQ_MSK, node_id),                 \
		MAX20356_REG_ENTRY(MAX20356_REG_LSW##n##CFG, MAX20356_LSW_CFG_BITS(n, node_id),    \
				   MAX20356_LSW_CFG_BITS(n, node_id)),                             \
	}

#define MAX20356_BUCK_DESC(n, cfg_reg_id)                                                          \
	static const struct regulator_max20356_desc __maybe_unused buck##n##_desc = {              \
		.ena_reg = MAX20356_REG_BUCK##n##ENA,                                              \
		.ena_mask = MAX20356_BUCK##n##ENA_BUCK##n##EN_MSK,                                 \
		.vset_reg = MAX20356_REG_BUCK##n##VSET,                                            \
		.vset_mask = MAX20356_BUCK##n##VSET_BUCK##n##VSET_MSK,                             \
		.cfg_reg = MAX20356_REG_##cfg_reg_id,                                              \
		.actdsc_mask = MAX20356_##cfg_reg_id##_BUCK##n##ACTDSC_MSK,                        \
		.iset_reg = MAX20356_REG_BUCK##n##ISET,                                            \
		.iset_mask = MAX20356_BUCK##n##ISET_BUCK##n##ISET_MSK,                             \
		.dvscfg0_reg = MAX20356_REG_BUCK##n##DVSCFG0,                                      \
		.dvsvlt_reg = {MAX20356_REG_BUCK##n##DVSCFG1, MAX20356_REG_BUCK##n##DVSCFG2,       \
			       MAX20356_REG_BUCK##n##DVSCFG3, MAX20356_REG_BUCK##n##DVSCFG4},      \
		.dvsspi_reg = MAX20356_REG_BUCK##n##DVSSPI,                                        \
		.spi_add = (n) - 1U,                                                               \
		.ctr_reg = MAX20356_REG_BUCK##n##CTR,                                              \
		.rail_sel = MAX20356_MPC_BUCK##n,                                                  \
		.lock = MAX20356_LOCK_BUCK##n,                                                     \
		.seq_lock = MAX20356_LOCK_BUCK##n##_SEQ,                                           \
		.ot_status_reg = MAX20356_REG_STATUS3,                                             \
		.ot_status_mask = MAX20356_STATUS3_THMBK##n##_MSK,                                 \
		.lockable = true,                                                                  \
		.is_buck = true,                                                                   \
	}

MAX20356_BUCK_DESC(1, BUCK1CFG0);
MAX20356_BUCK_DESC(2, BUCK2CFG);
MAX20356_BUCK_DESC(3, BUCK3CFG);

static const struct regulator_max20356_desc __maybe_unused bbst_desc = {
	.ena_reg = MAX20356_REG_BBSTENA,
	.ena_mask = MAX20356_BBSTENA_BBSTEN_MSK,
	.vset_reg = MAX20356_REG_BBSTVSET,
	.vset_mask = MAX20356_BBSTVSET_BBSTVSET_MSK,
	.cfg_reg = MAX20356_REG_BBSTCFG,
	.actdsc_mask = MAX20356_BBSTCFG_BBSTACTDSC_MSK,
	.iset_reg = MAX20356_REG_BBSTISET,
	.iset_mask = MAX20356_BBSTISET_BBSTIPSET1_MSK,
	.ctr_reg = MAX20356_REG_BBSTCTR0,
	.rail_sel = MAX20356_MPC_BUCKBOOST,
	.lock = MAX20356_LOCK_BBST,
	.seq_lock = MAX20356_LOCK_BBST_SEQ,
	.oc_status_reg = MAX20356_REG_STATUS3,
	.oc_status_mask = MAX20356_STATUS3_BBSTFAULT_MSK,
	.lockable = true,
};

#define MAX20356_LDO12_DESC(n)                                                                     \
	static const struct regulator_max20356_desc __maybe_unused ldo##n##_desc = {               \
		.ena_reg = MAX20356_REG_LDO##n##ENA,                                               \
		.ena_mask = MAX20356_LDO##n##ENA_LDO##n##EN_MSK,                                   \
		.vset_reg = MAX20356_REG_LDO##n##VSET,                                             \
		.vset_mask = MAX20356_LDO##n##VSET_LDO##n##VSET_MSK,                               \
		.cfg_reg = MAX20356_REG_LDO##n##CFG,                                               \
		.actdsc_mask = MAX20356_LDO##n##CFG_LDO##n##ACTDSC_MSK,                            \
		.mode_mask = MAX20356_LDO##n##CFG_LDO##n##MODE_MSK,                                \
		.ctr_reg = MAX20356_REG_LDO##n##CTR,                                               \
		.rail_sel = MAX20356_MPC_LDO##n,                                                   \
		.lock = MAX20356_LOCK_LDO##n,                                                      \
		.seq_lock = MAX20356_LOCK_LDO##n##_SEQ,                                            \
		.ot_status_reg = MAX20356_REG_STATUS2,                                             \
		.ot_status_mask = MAX20356_STATUS2_THMLDO##n##_MSK,                                \
		.oc_status_reg = MAX20356_REG_STATUS2,                                             \
		.oc_status_mask = MAX20356_STATUS2_UVLOLDO##n##_MSK,                               \
		.lockable = true,                                                                  \
	}

MAX20356_LDO12_DESC(1);
MAX20356_LDO12_DESC(2);

static const struct regulator_max20356_desc __maybe_unused ldo3_desc = {
	.ena_reg = MAX20356_REG_LDO3ENA,
	.ena_mask = MAX20356_LDO3ENA_LDO3EN_MSK,
	.vset_reg = MAX20356_REG_LDO3VSET,
	.vset_mask = MAX20356_LDO3VSET_LDO3VSET_MSK,
	.cfg_reg = MAX20356_REG_LDO3CFG,
	.actdsc_mask = MAX20356_LDO3CFG_LDO3ACTDSC_MSK,
	.ctr_reg = MAX20356_REG_LDO3CTR,
	.rail_sel = MAX20356_MPC_LDO3,
	.lock = MAX20356_LOCK_LDO3,
	.seq_lock = MAX20356_LOCK_LDO3_SEQ,
	.ot_status_reg = MAX20356_REG_STATUS2,
	.ot_status_mask = MAX20356_STATUS2_THMLDO3_MSK,
	.oc_status_reg = MAX20356_REG_STATUS2,
	.oc_status_mask = MAX20356_STATUS2_UVLOLDO3_MSK | MAX20356_STATUS2_SCLDO3_MSK |
			  MAX20356_STATUS2_DRPLDO3_MSK,
	.lockable = true,
};

static const struct regulator_max20356_desc __maybe_unused ldo4_desc = {
	.ena_reg = MAX20356_REG_LDO4ENA,
	.ena_mask = MAX20356_LDO4ENA_LDO4EN_MSK,
	.cfg_reg = MAX20356_REG_LDO4CFG,
	.ctr_reg = MAX20356_REG_LDO4CTR,
	.rail_sel = MAX20356_MPC_LDO4,
	.lock = MAX20356_LOCK_LDO4,
	.seq_lock = MAX20356_LOCK_LDO4_SEQ,
	.lockable = true,
	.is_ldo4 = true,
};

#define MAX20356_LSW_DESC(n)                                                                       \
	static const struct regulator_max20356_desc __maybe_unused lsw##n##_desc = {               \
		.ena_reg = MAX20356_REG_LSW##n##ENA,                                               \
		.ena_mask = MAX20356_LSW##n##ENA_LSW##n##EN_MSK,                                   \
		.cfg_reg = MAX20356_REG_LSW##n##CFG,                                               \
		.actdsc_mask = MAX20356_LSW##n##CFG_LSW##n##ACTDSC_MSK,                            \
		.ctr_reg = MAX20356_REG_LSW##n##CTR,                                               \
		.rail_sel = MAX20356_MPC_LSW##n,                                                   \
		.lock = MAX20356_LOCK_BUCK1,                                                       \
		.ot_status_reg = MAX20356_REG_STATUS3,                                             \
		.ot_status_mask = MAX20356_STATUS3_THMLSW_MSK,                                     \
		.oc_status_reg = MAX20356_REG_STATUS3,                                             \
		.oc_status_mask = MAX20356_STATUS3_LSW##n##TMO_MSK,                                \
	}

MAX20356_LSW_DESC(1);
MAX20356_LSW_DESC(2);
MAX20356_LSW_DESC(3);

/* Buck output range from the per-rail voltage-step property. */
#define MAX20356_BUCK_RANGE(node_id)                                                               \
	((DT_ENUM_IDX(node_id, adi_buck_voltage_step_microvolt) == 0)                              \
		 ? &buck_range_10mv                                                                \
		 : (DT_ENUM_IDX(node_id, adi_buck_voltage_step_microvolt) == 1) ? &buck_range_25mv \
									       : &buck_range_50mv)

/* SPI spec only for a buck node carrying adi,dvs-spi; empty otherwise. */
#define MAX20356_DVS_SPI_INIT(node_id)                                                             \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, adi_dvs_spi),                                        \
		    (.dvs_spi = SPI_DT_SPEC_GET(DT_PHANDLE(node_id, adi_dvs_spi),                  \
						SPI_WORD_SET(8) | SPI_TRANSFER_MSB),), ())

#define REGULATOR_MAX20356_DEFINE(node_id, id, _desc, _range, _regs, _sel)                         \
	_regs;                                                                                     \
                                                                                                   \
	static const struct regulator_max20356_config regulator_max20356_config_##id = {           \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.mfd_dev = DEVICE_DT_GET(DT_GPARENT(node_id)),                                     \
		.desc = &(_desc),                                                                  \
		.uv_range = (_range),                                                              \
		.init_regs = regulator_max20356_init_regs_##id,                                    \
		.num_init_regs = ARRAY_SIZE(regulator_max20356_init_regs_##id),                    \
		.mpc_ctr_mask = MAX20356_MPC_CTR_MASK(node_id, _sel),                              \
		.mpc_all_mask = MAX20356_MPC_ALL_MASK(node_id),                                    \
		.ldo4_rtc = DT_PROP_OR(node_id, adi_ldo4_always_on_off_on_pfn1, 0),                \
		.lock_enable = DT_PROP(node_id, adi_lock_enable),                                  \
		.dvs_mode = DT_ENUM_IDX_OR(node_id, adi_dvs_mode, MAX20356_DVS_MODE_I2C),          \
		.dvs_mpc_pair = DT_PROP_OR(node_id, adi_dvs_mpc_pair, {0}),                        \
		.dvs_mpc_pair_len = DT_PROP_LEN_OR(node_id, adi_dvs_mpc_pair, 0),                  \
		.dvs_voltages = DT_PROP_OR(node_id, adi_dvs_voltages, {0}),                        \
		.dvs_valley_ua = DT_PROP_OR(node_id, adi_dvs_valley_current_microamp, 500000),     \
		MAX20356_DVS_SPI_INIT(node_id)                                                     \
	};                                                                                         \
                                                                                                   \
	static struct regulator_max20356_data regulator_max20356_data_##id;                        \
	DEVICE_DT_DEFINE(node_id, regulator_max20356_init, NULL, &regulator_max20356_data_##id,    \
			 &regulator_max20356_config_##id, POST_KERNEL,                             \
			 CONFIG_REGULATOR_MAX20356_INIT_PRIORITY, &api);

/* COND rails whose init-table builder takes (id, node_id): buck-boost, LDOs. */
#define REGULATOR_MAX20356_DEFINE_COND(inst, child, desc, range, regs_builder, sel)                \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_MAX20356_DEFINE(DT_INST_CHILD(inst, child), child##inst, desc,      \
					       range,                                              \
					       regs_builder(child##inst,                           \
							    DT_INST_CHILD(inst, child)),           \
					       sel)),                                              \
		    ())

/* Bucks: builder takes (id, n, cfg_reg_id, node_id) and the range is derived. */
#define REGULATOR_MAX20356_DEFINE_BUCK(inst, child, desc, n, cfg_reg_id, sel)                      \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_MAX20356_DEFINE(DT_INST_CHILD(inst, child), child##inst, desc,      \
					       MAX20356_BUCK_RANGE(DT_INST_CHILD(inst, child)),    \
					       MAX20356_BUCK_INIT_REGS(child##inst, n, cfg_reg_id, \
								       DT_INST_CHILD(inst,         \
										     child)),      \
					       sel)),                                              \
		    ())

/* Load switches: builder takes (id, n, node_id); no voltage range. */
#define REGULATOR_MAX20356_DEFINE_LSW(inst, child, desc, n, sel)                                   \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_MAX20356_DEFINE(DT_INST_CHILD(inst, child), child##inst, desc, NULL,\
					       MAX20356_LSW_INIT_REGS(child##inst, n,              \
								      DT_INST_CHILD(inst,          \
										    child)),       \
					       sel)),                                              \
		    ())

#define REGULATOR_MAX20356_DEFINE_ALL(inst)                                                        \
	static const struct regulator_max20356_common_config common_config_##inst = {              \
		.mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_max20356_common_init, NULL, NULL,                    \
			      &common_config_##inst, POST_KERNEL,                                  \
			      CONFIG_REGULATOR_MAX20356_COMMON_INIT_PRIORITY, &parent_api);        \
                                                                                                   \
	REGULATOR_MAX20356_DEFINE_BUCK(inst, buck1, buck1_desc, 1, BUCK1CFG0, MAX20356_MPC_BUCK1)  \
	REGULATOR_MAX20356_DEFINE_BUCK(inst, buck2, buck2_desc, 2, BUCK2CFG, MAX20356_MPC_BUCK2)   \
	REGULATOR_MAX20356_DEFINE_BUCK(inst, buck3, buck3_desc, 3, BUCK3CFG, MAX20356_MPC_BUCK3)   \
	REGULATOR_MAX20356_DEFINE_COND(inst, buckboost, bbst_desc, &bbst_range,                    \
				       MAX20356_BBST_INIT_REGS, MAX20356_MPC_BUCKBOOST)            \
	REGULATOR_MAX20356_DEFINE_COND(inst, ldo1, ldo1_desc, &ldo1_2_range,                       \
				       MAX20356_LDO1_INIT_REGS, MAX20356_MPC_LDO1)                 \
	REGULATOR_MAX20356_DEFINE_COND(inst, ldo2, ldo2_desc, &ldo1_2_range,                       \
				       MAX20356_LDO2_INIT_REGS, MAX20356_MPC_LDO2)                 \
	REGULATOR_MAX20356_DEFINE_COND(inst, ldo3, ldo3_desc, &ldo3_range,                         \
				       MAX20356_LDO3_INIT_REGS, MAX20356_MPC_LDO3)                 \
	REGULATOR_MAX20356_DEFINE_COND(inst, ldo4, ldo4_desc, NULL, MAX20356_LDO4_INIT_REGS,       \
				       MAX20356_MPC_LDO4)                                          \
	REGULATOR_MAX20356_DEFINE_LSW(inst, lsw1, lsw1_desc, 1, MAX20356_MPC_LSW1)                 \
	REGULATOR_MAX20356_DEFINE_LSW(inst, lsw2, lsw2_desc, 2, MAX20356_MPC_LSW2)                 \
	REGULATOR_MAX20356_DEFINE_LSW(inst, lsw3, lsw3_desc, 3, MAX20356_MPC_LSW3)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_MAX20356_DEFINE_ALL)
