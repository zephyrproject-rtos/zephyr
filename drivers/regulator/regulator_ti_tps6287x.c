/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tps6287x

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/regulator/ti_tps6287x.h>

LOG_MODULE_REGISTER(tps6287x, CONFIG_REGULATOR_LOG_LEVEL);

#define TPS6287X_MIN_DIV_OUTPUT 1400000U /* Minimum difference between in- and output voltage. */
#define TPS6287X_MAX_INIT_RETRY 5U       /* Maximum retries during init (500µs) */

#define TPS6287X_REG_VSET 0x00U /* Output voltage setpoint, Reset = X */

#define TPS6287X_VSET_MASK GENMASK(7, 0)

#define TPS6287X_REG_CONTROL1 0x01U /* Control 1, Reset = 0x2A */

#define TPS6287X_CONTROL1_RESET      BIT(7)
#define TPS6287X_CONTROL1_SSCEN      BIT(6)
#define TPS6287X_CONTROL1_SWEN       BIT(5)
#define TPS6287X_CONTROL1_FPWMEN     BIT(4)
#define TPS6287X_CONTROL1_DISCHEN    BIT(3)
#define TPS6287X_CONTROL1_HICCUPEN   BIT(2)
#define TPS6287X_CONTROL1_VRAMP_MASK GENMASK(1, 0)

/* RESET (bit 7) */
#define TPS6287X_RESET_NO_EFFECT 0x0U
#define TPS6287X_RESET_ALL_REGS  0x1U /* Resets all registers to default. Read-back is always 0 */

/* SSCEN (bit 6) - Spread spectrum clocking enable */
#define TPS6287X_SSCEN_DISABLED 0x0U
#define TPS6287X_SSCEN_ENABLED  0x1U

/* SWEN (bit 5) - Software enable */
#define TPS6287X_SWEN_DISABLED 0x0U /* Switching disabled, register values retained */
#define TPS6287X_SWEN_ENABLED  0x1U /* Switching enabled (without the enable delay)  */

/* FPWMEN (bit 4) - Forced PWM enable (logically ORed with MODE/SYNC pin) */
#define TPS6287X_FPWMEN_POWER_SAVE 0x0U
#define TPS6287X_FPWMEN_FORCED_PWM 0x1U

/* DISCHEN (bit 3) - Output discharge enable */
#define TPS6287X_DISCHEN_DISABLED 0x0U
#define TPS6287X_DISCHEN_ENABLED  0x1U

/* HICCUPEN (bit 2) - Hiccup operation enable. Do not enable during stacked operation. */
#define TPS6287X_HICCUPEN_DISABLED 0x0U
#define TPS6287X_HICCUPEN_ENABLED  0x1U

/* VRAMP (bits 1-0) - Output voltage ramp speed */
#define TPS6287X_VRAMP_10000_UV_PER_US 0x0U
#define TPS6287X_VRAMP_5000_UV_PER_US  0x1U
#define TPS6287X_VRAMP_1250_UV_PER_US  0x2U
#define TPS6287X_VRAMP_500_UV_PER_US   0x3U

#define TPS6287X_VSET_RANGE0_BASE_UV  400000  /* 0.400 V */
#define TPS6287X_VSET_RANGE0_STEP_UV  1250    /* 1.25 mV/step  (0.400 V - 0.71875 V) */
#define TPS6287X_VSET_RANGE0_UPPER_UV 718750  /* 0.71875 V */
#define TPS6287X_VSET_RANGE1_BASE_UV  400000  /* 0.400 V */
#define TPS6287X_VSET_RANGE1_STEP_UV  2500    /* 2.5 mV/step   (0.400 V - 1.0375 V)  */
#define TPS6287X_VSET_RANGE1_UPPER_UV 1037500 /* 1.0375 V */
#define TPS6287X_VSET_RANGE2_BASE_UV  400000  /* 0.400 V */
#define TPS6287X_VSET_RANGE2_STEP_UV  5000    /* 5 mV/step     (0.400 V - 1.675 V)   */
#define TPS6287X_VSET_RANGE2_UPPER_UV 1675000 /* 1.675 V */
#define TPS6287X_VSET_RANGE3_BASE_UV  800000  /* 0.800 V */
#define TPS6287X_VSET_RANGE3_STEP_UV  10000   /* 10 mV/step    (0.800 V - 3.35 V)    */
#define TPS6287X_VSET_RANGE3_UPPER_UV 3350000 /* 3.35 V */

#define TPS6287X_REG_CONTROL2 0x02U /* Control 2, Reset = 0x09 */

#define TPS6287X_CONTROL2_RESERVED_MASK GENMASK(7, 4)
#define TPS6287X_CONTROL2_VRANGE_MASK   GENMASK(3, 2)
#define TPS6287X_CONTROL2_SSTIME_MASK   GENMASK(1, 0)

/* VRANGE (bits 3-2) - Output voltage range, applies to VSET register */
#define TPS6287X_VRANGE_0_400MV_TO_719MV_STEP_1P25MV 0x0U
#define TPS6287X_VRANGE_1_400MV_TO_1038MV_STEP_2P5MV 0x1U
#define TPS6287X_VRANGE_2_400MV_TO_1675MV_STEP_5MV   0x2U
#define TPS6287X_VRANGE_3_800MV_TO_3350MV_STEP_10MV  0x3U

/* SSTIME (bits 1-0) - Soft-start ramp time */
#define TPS6287X_SSTIME_0_5_MS 0x0U
#define TPS6287X_SSTIME_1_MS   0x1U
#define TPS6287X_SSTIME_2_MS   0x2U
#define TPS6287X_SSTIME_4_MS   0x3U

#define TPS6287X_REG_CONTROL3 0x03U /* Control 3, Reset = 0x00 */

#define TPS6287X_CONTROL3_RESERVED_MASK GENMASK(7, 2)
#define TPS6287X_CONTROL3_SINGLE        BIT(1)
#define TPS6287X_CONTROL3_PGBLNKDVS     BIT(0)

/* SINGLE (bit 1) - Controls internal EN pulldown and SYNC_OUT */
#define TPS6287X_SINGLE_EN_PULLDOWN_SYNCOUT_ENABLED  0x0U
#define TPS6287X_SINGLE_EN_PULLDOWN_SYNCOUT_DISABLED 0x1U

/* PGBLNKDVS (bit 0) - Power-good blanking during DVS */
#define TPS6287X_PGBLNKDVS_PG_REFLECTS_COMPARATOR 0x0U
#define TPS6287X_PGBLNKDVS_PG_HIGH_Z_DURING_DVS   0x1U

#define TPS6287X_REG_STATUS 0x04U /* Status, Reset = 0x02 */

#define TPS6287X_STATUS_RESERVED_MASK GENMASK(7, 6)
#define TPS6287X_STATUS_HICCUP        BIT(5) /* Hiccup event since last read */
#define TPS6287X_STATUS_ILIM          BIT(4) /* Current limit event since last read */
#define TPS6287X_STATUS_TWARN         BIT(3) /* Thermal warning event since last read */
#define TPS6287X_STATUS_TSHUT         BIT(2) /* Thermal shutdown event since last read */
#define TPS6287X_STATUS_PBUV          BIT(1) /* Power-bad undervolt event since last read */
#define TPS6287X_STATUS_PBOV          BIT(0) /* Power-bad overvoltage event since last read */

struct regulator_tps6287x_data {
	struct regulator_common_data data;
};

struct regulator_tps6287x_config {
	struct regulator_common_config common;
	struct i2c_dt_spec i2c;
	uint32_t input_voltage_uv;
	uint8_t ramp_delay;
	bool ssc;
	bool hiccup;
};

static const struct linear_range voltage_ranges[] = {
	LINEAR_RANGE_INIT(400000u, 1250u, 0, BIT_MASK(8)),
	LINEAR_RANGE_INIT(400000u, 2500u, 0, BIT_MASK(8)),
	LINEAR_RANGE_INIT(400000u, 5000u, 0, BIT_MASK(8)),
	LINEAR_RANGE_INIT(800000u, 10000u, 0, BIT_MASK(8)),
};

static unsigned int regulator_tps6287x_count_voltages(const struct device *dev)
{
	ARG_UNUSED(dev);

	return linear_range_group_values_count(voltage_ranges, ARRAY_SIZE(voltage_ranges));
}

static int regulator_tps6287x_list_voltage(const struct device *dev, unsigned int idx,
					   int32_t *volt_uv)
{
	ARG_UNUSED(dev);

	uint32_t count;

	for (uint8_t i = 0U; i < ARRAY_SIZE(voltage_ranges); i++) {
		count = linear_range_values_count(&voltage_ranges[i]);

		if (idx < count) {
			*volt_uv = voltage_ranges[i].min + (int32_t)(voltage_ranges[i].step * idx);
			return 0;
		}
		idx -= count;
	}

	return -EINVAL;
}

static int down_from_range3(const struct device *dev, uint8_t new_vrange, uint16_t vset)
{
	int rc = 0;
	uint8_t vreg = 0;
	int32_t req_voltage = 0;
	int32_t current_voltage = 0;
	uint8_t current_vset = 0;
	bool vreg_set = false; /* Use this to check if vreg has been set. */
	const struct regulator_tps6287x_config *cfg = dev->config;

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, &current_vset);
	if (rc < 0) {
		return rc;
	}

	current_voltage =
		TPS6287X_VSET_RANGE3_BASE_UV + current_vset * TPS6287X_VSET_RANGE3_STEP_UV;
	(void)linear_range_get_value(&voltage_ranges[new_vrange], vset, &req_voltage);

	/* The reason to step down from vrange 3 is either a requested output voltage below 0.8V or
	 * a requested voltage which requires a better resolution than the 10mV that vrange 3 can
	 * provide.
	 *
	 * The step down from vrange 3 is executed here, the final value in the selected vrange will
	 * be set in regulator_tps6287x_set_voltage().
	 */

	if (req_voltage < TPS6287X_VSET_RANGE3_BASE_UV) {
		LOG_DBG("%s: Requested voltage below 0.8V. Set common voltage of 0.8V in vrange 1",
			dev->name);
		/* Set voltage to 0.8V in vrange 3. */
		rc = i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, 0U);
		/* Set vreg for 0.8V in vrange 1 (vrange 1 chosen, could also be vrange 2). */
		vreg = 160U;
		vreg_set = true;
	} else if (new_vrange == TPS6287X_VRANGE_1_400MV_TO_1038MV_STEP_2P5MV) {
		if (current_voltage > TPS6287X_VSET_RANGE1_UPPER_UV) {
			LOG_DBG("%s: Current voltage above %d uV. Set common voltage of 1.030V "
				"in vrange 1",
				dev->name, TPS6287X_VSET_RANGE1_UPPER_UV);
			rc = i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, 23U);
			vreg = 252U;
			vreg_set = true;
		} else {
			LOG_DBG("%s: Current voltage %d uV within vrange 1 limit. Find "
				"representation in vrange 1",
				dev->name, current_voltage);
			/* Current voltage within vrange 1 limit:
			 * Calculate representation in vrange 1. vrange 1 has a
			 * better resolution, exact match must be possible.
			 */
			vreg = (uint8_t)((current_voltage - TPS6287X_VSET_RANGE1_BASE_UV) /
					 TPS6287X_VSET_RANGE1_STEP_UV);
			vreg_set = true;
		}
	} else if (new_vrange == TPS6287X_VRANGE_2_400MV_TO_1675MV_STEP_5MV) {
		if (current_voltage > TPS6287X_VSET_RANGE2_UPPER_UV) {
			LOG_DBG("%s: Current voltage above %d uV. Set common voltage of 1.670V "
				"in vrange 2",
				dev->name, TPS6287X_VSET_RANGE2_UPPER_UV);
			rc = i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, 87U);
			/* Set vreg for 1.670V in vrange 2. */
			vreg = 254U;
			vreg_set = true;
		} else {
			LOG_DBG("%s: Current voltage %d uV within vrange 2 limit. Find "
				"representation in vrange 2",
				dev->name, current_voltage);
			vreg = (uint8_t)((current_voltage - TPS6287X_VSET_RANGE2_BASE_UV) /
					 TPS6287X_VSET_RANGE2_STEP_UV);
			vreg_set = true;
		}
	}

	if (rc == 0 && vreg_set == true) {
		uint8_t control2 = 0;

		/* Set vrange. If new_range == 0, set new_range = 1. */
		control2 = FIELD_PREP(TPS6287X_CONTROL2_VRANGE_MASK,
				      new_vrange == TPS6287X_VRANGE_2_400MV_TO_1675MV_STEP_5MV
					      ? TPS6287X_VRANGE_2_400MV_TO_1675MV_STEP_5MV
					      : TPS6287X_VRANGE_1_400MV_TO_1038MV_STEP_2P5MV);
		rc = i2c_reg_update_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL2,
					    TPS6287X_CONTROL2_VRANGE_MASK, control2);
		if (rc < 0) {
			return rc;
		}

		rc = i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, vreg);
	}

	return rc;
}

static int up_to_range3(const struct device *dev, uint8_t current_vrange)
{
	int rc = 0;
	uint8_t vreg = 0;
	uint8_t control2 = 0;
	const struct regulator_tps6287x_config *cfg = dev->config;

	/* The only reason to step up to range 3 is a requested output voltage above 1.675V.
	 * Voltages below 1.675V can be covered (with better resolution) in ranges 0..2.
	 * Select the highest common voltage in the lower ranges (range 0 has to choose range 1 in
	 * order to reach a common voltage: Range 0 ends at 0.71875V and range 3 starts at 0.8V.
	 */
	switch (current_vrange) {
	case TPS6287X_VRANGE_0_400MV_TO_719MV_STEP_1P25MV:
		LOG_DBG("%s: Set common voltage of 0.8V in vrange 1", dev->name);

		/* Set voltage to 0.8V in vrange 1. */
		control2 = FIELD_PREP(TPS6287X_CONTROL2_VRANGE_MASK,
				      TPS6287X_VRANGE_1_400MV_TO_1038MV_STEP_2P5MV);
		rc = i2c_reg_update_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL2,
					    TPS6287X_CONTROL2_VRANGE_MASK, control2);
		if (rc < 0) {
			return rc;
		}
		rc = i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, 160U);
		/* Set vreg for 0.8V in vrange 3. */
		vreg = 0U;
		break;
	case TPS6287X_VRANGE_1_400MV_TO_1038MV_STEP_2P5MV:
		LOG_DBG("%s: Set common voltage of 1.030V in range 1", dev->name);
		rc = i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, 252U);
		/* Set vreg for 1.030V in vrange 3. */
		vreg = 23U;
		break;
	case TPS6287X_VRANGE_2_400MV_TO_1675MV_STEP_5MV:
		LOG_DBG("%s: Set common voltage of 1.670V in range 2", dev->name);
		rc = i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, 254U);
		/* Set vreg for 1.670V in vrange 3. */
		vreg = 87U;
		break;
	default:
		rc = -EINVAL;
		break;
	}

	if (rc < 0) {
		return rc;
	}

	/* Set vrange to TPS6287X_VRANGE_3_800MV_TO_3350MV_STEP_10MV. */
	control2 = FIELD_PREP(TPS6287X_CONTROL2_VRANGE_MASK,
			      TPS6287X_VRANGE_3_800MV_TO_3350MV_STEP_10MV);
	rc = i2c_reg_update_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL2, TPS6287X_CONTROL2_VRANGE_MASK,
				    control2);
	if (rc < 0) {
		return rc;
	}
	/* Set output voltage. */
	return i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, vreg);
}

static int regulator_tps6287x_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_tps6287x_config *cfg = dev->config;
	int rc = 0;
	uint8_t control2 = 0;
	int32_t max_input_voltage_uv = (int32_t)(cfg->input_voltage_uv - TPS6287X_MIN_DIV_OUTPUT);

	if (min_uv > max_input_voltage_uv) {
		return -EINVAL;
	}

	max_uv = MIN(max_uv, max_input_voltage_uv);

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL2, &control2);
	if (rc < 0) {
		return rc;
	}

	uint8_t vrange = FIELD_GET(TPS6287X_CONTROL2_VRANGE_MASK, control2);
	uint16_t vset = 0;

	rc = linear_range_get_win_index(&voltage_ranges[vrange], min_uv, max_uv, &vset);

	/* If we cannot find a matching voltage in the current range, check the other ranges
	 */
	if (rc < 0) {
		uint8_t new_vrange;

		/* We start with the lowest voltage range and work our way up to avoid switching to
		 * range 3 when not necessary.
		 */
		for (new_vrange = 0; new_vrange < ARRAY_SIZE(voltage_ranges); new_vrange++) {
			rc = linear_range_get_win_index(&voltage_ranges[new_vrange], min_uv, max_uv,
							&vset);
			if (rc == 0) {
				break;
			}
		}

		if (rc < 0) {
			return rc;
		}

		/* When changing from or to vrange3 the TPS6287x requires to set a common voltage
		 * level before changing the vrange. See chapter 7.3.6.1 in the datasheet.
		 * "When changing to or from the 0.8V to 3.35V range, the device switches the
		 * internal reference between 0.4V and 0.8V. To avoid any output voltage over or
		 * undershoot that can occur during the change, the VRANGE change must be done at an
		 * output voltage that occurs in both the new range and old range and the VSET[7:0]
		 * bits must set the same output voltage in both the new range and old range."
		 *
		 * This setting of a common voltage is done in down_from_range3 and up_to_range3.
		 * Both do not set the final output voltage, they only ensure that the requirement
		 * is fulfilled. Both access the TPS6287x via I2C to read and write registers.
		 */
		if (vrange == TPS6287X_VRANGE_3_800MV_TO_3350MV_STEP_10MV) {
			/* Step down from 0.8V setpoint range. */
			rc = down_from_range3(dev, new_vrange, vset);
		} else if (new_vrange == TPS6287X_VRANGE_3_800MV_TO_3350MV_STEP_10MV) {
			/* Step up to 0.8V setpoint range. */
			rc = up_to_range3(dev, vrange);
		}
		if (rc < 0) {
			return rc;
		}

		control2 = FIELD_PREP(TPS6287X_CONTROL2_VRANGE_MASK, new_vrange);
		rc = i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL2, control2);
		if (rc < 0) {
			return rc;
		}

		vrange = new_vrange;
	}

	LOG_DBG("%s: Setting voltage to range %u, index %u", dev->name, vrange, vset);

	return i2c_reg_write_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, (uint8_t)vset);
}

static int regulator_tps6287x_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_tps6287x_config *cfg = dev->config;
	int rc = 0;
	uint8_t control2 = 0;

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL2, &control2);
	if (rc < 0) {
		return rc;
	}

	uint8_t vrange = FIELD_GET(TPS6287X_CONTROL2_VRANGE_MASK, control2);
	uint8_t vset = 0;

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TPS6287X_REG_VSET, &vset);
	if (rc < 0) {
		return rc;
	}

	rc = linear_range_get_value(&voltage_ranges[vrange], (uint16_t)vset, volt_uv);

	LOG_DBG("%s: Got voltage: %d uV (range %u, index %u)", dev->name, *volt_uv, vrange, vset);

	return rc;
}

static int regulator_tps6287x_set_active_discharge(const struct device *dev, bool active_discharge)
{
	const struct regulator_tps6287x_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL1, TPS6287X_CONTROL1_DISCHEN,
				      active_discharge ? TPS6287X_CONTROL1_DISCHEN : 0);
}

static int regulator_tps6287x_get_active_discharge(const struct device *dev, bool *active_discharge)
{
	const struct regulator_tps6287x_config *cfg = dev->config;
	uint8_t control1;
	int rc;

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL1, &control1);
	if (rc == 0) {
		*active_discharge = control1 & TPS6287X_CONTROL1_DISCHEN;
	}

	return rc;
}

static int regulator_tps6287x_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_tps6287x_config *cfg = dev->config;
	bool enable_forced_pwm = false;

	switch (mode) {
	case TI_TPS6287X_MODE_PWM:
		enable_forced_pwm = true;
		break;
	case TI_TPS6287X_MODE_PFM:
		break;
	default:
		return -EINVAL;
	}

	return i2c_reg_update_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL1, TPS6287X_CONTROL1_FPWMEN,
				      FIELD_PREP(TPS6287X_CONTROL1_FPWMEN, enable_forced_pwm));
}

static int regulator_tps6287x_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_tps6287x_config *cfg = dev->config;
	uint8_t control1 = 0;
	int rc = 0;

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL1, &control1);
	if (rc < 0) {
		return rc;
	}

	*mode = (FIELD_GET(TPS6287X_CONTROL1_FPWMEN, control1) ? TI_TPS6287X_MODE_PWM
							       : TI_TPS6287X_MODE_PFM);

	return 0;
}

static int regulator_tps6287x_enable(const struct device *dev)
{
	const struct regulator_tps6287x_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL1, TPS6287X_CONTROL1_SWEN,
				      FIELD_PREP(TPS6287X_CONTROL1_SWEN, TPS6287X_SWEN_ENABLED));
}

static int regulator_tps6287x_disable(const struct device *dev)
{
	const struct regulator_tps6287x_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL1, TPS6287X_CONTROL1_SWEN,
				      FIELD_PREP(TPS6287X_CONTROL1_SWEN, TPS6287X_SWEN_DISABLED));
}

static int regulator_tps6287x_get_error_flags(const struct device *dev,
					      regulator_error_flags_t *flags)
{
	const struct regulator_tps6287x_config *cfg = dev->config;
	uint8_t status = 0U;
	int rc = 0;

	*flags = 0U;

	rc = i2c_reg_read_byte_dt(&cfg->i2c, TPS6287X_REG_STATUS, &status);
	if (rc == 0) {
		if (FIELD_GET(TPS6287X_STATUS_ILIM, status)) {
			*flags |= REGULATOR_ERROR_OVER_CURRENT;
		}

		if (FIELD_GET(TPS6287X_STATUS_TWARN, status) ||
		    FIELD_GET(TPS6287X_STATUS_TSHUT, status)) {
			*flags |= REGULATOR_ERROR_OVER_TEMP;
		}

		if (FIELD_GET(TPS6287X_STATUS_PBOV, status)) {
			*flags |= REGULATOR_ERROR_OVER_VOLTAGE;
		}
	}

	return rc;
}
static int regulator_tps6287x_init(const struct device *dev)
{
	const struct regulator_tps6287x_config *cfg = dev->config;
	int rc = -EAGAIN;
	uint8_t status = 0;

	/* Try to access device. */
	for (int i = 0; i < TPS6287X_MAX_INIT_RETRY && rc != 0; i++) {
		rc = i2c_reg_read_byte_dt(&cfg->i2c, TPS6287X_REG_STATUS, &status);
		if (rc < 0) {
			k_busy_wait(100);
		}
	}

	if (rc < 0) {
		return rc;
	}

	/* Access successful, continue with initialization. */
	regulator_common_data_init(dev);

	/* Reset register. */
	rc = i2c_reg_update_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL1, TPS6287X_CONTROL1_RESET,
				    FIELD_PREP(TPS6287X_CONTROL1_RESET, TPS6287X_RESET_ALL_REGS));
	if (rc < 0) {
		return rc;
	}
	k_busy_wait(100);

	/* Disable output. */
	rc = regulator_tps6287x_disable(dev);
	if (rc < 0) {
		return rc;
	}

	/* Configure non-common properties. */
	uint8_t control1 = 0;

	control1 = FIELD_PREP(TPS6287X_CONTROL1_SSCEN, cfg->ssc);
	control1 |= FIELD_PREP(TPS6287X_CONTROL1_HICCUPEN, cfg->hiccup);
	control1 |= FIELD_PREP(TPS6287X_CONTROL1_VRAMP_MASK, cfg->ramp_delay);
	rc = i2c_reg_update_byte_dt(&cfg->i2c, TPS6287X_REG_CONTROL1,
				    TPS6287X_CONTROL1_SSCEN | TPS6287X_CONTROL1_HICCUPEN |
					    TPS6287X_CONTROL1_VRAMP_MASK,
				    control1);
	if (rc < 0) {
		return rc;
	}

	rc = regulator_common_init(dev, false);

	return rc;
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_tps6287x_enable,
	.disable = regulator_tps6287x_disable,
	.count_voltages = regulator_tps6287x_count_voltages,
	.list_voltage = regulator_tps6287x_list_voltage,
	.set_voltage = regulator_tps6287x_set_voltage,
	.get_voltage = regulator_tps6287x_get_voltage,
	.set_active_discharge = regulator_tps6287x_set_active_discharge,
	.get_active_discharge = regulator_tps6287x_get_active_discharge,
	.set_mode = regulator_tps6287x_set_mode,
	.get_mode = regulator_tps6287x_get_mode,
	.get_error_flags = regulator_tps6287x_get_error_flags,
};

/* clang-format off */
#define SANITY_CHECK_INIT_MICROVOLT(inst)	\
	BUILD_ASSERT(DT_PROP_OR(DT_DRV_INST(inst), regulator_init_microvolt, 0) <	\
				(DT_INST_PROP(inst, input_voltage_microvolt) -	\
				TPS6287X_MIN_DIV_OUTPUT),	\
				"input-voltage-microvolt must be at least 1.4V greater"	\
				"than regulator-init-microvolt")

#define REGULATOR_TPS6287X_DEFINE_ALL(inst)	\
	SANITY_CHECK_INIT_MICROVOLT(inst);	\
	\
	static struct regulator_tps6287x_data data_##inst;	\
	\
	static const struct regulator_tps6287x_config config_##inst = {	\
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),	\
		.input_voltage_uv = DT_INST_PROP(inst, input_voltage_microvolt),	\
		.ramp_delay = DT_INST_ENUM_IDX(inst, regulator_ramp_delay),	\
		.ssc = DT_INST_PROP(inst, enable_ssc),	\
		.hiccup = DT_INST_PROP(inst, enable_hiccup_mode)	\
	};	\
	\
	DEVICE_DT_INST_DEFINE(inst, regulator_tps6287x_init, NULL, &data_##inst,	\
						  &config_##inst, POST_KERNEL,	\
						  CONFIG_REGULATOR_TPS6287X_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_TPS6287X_DEFINE_ALL)
/* clang-format on */
