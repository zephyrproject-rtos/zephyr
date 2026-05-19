/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm10xx_regulator

#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/npm10xx.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/linear_range.h>

LOG_MODULE_REGISTER(regulator_npm10xx, CONFIG_REGULATOR_LOG_LEVEL);

/* RESET ship task register offset and mask */
#define NPM10_RESET_TASKS    0xDDU
#define RESET_TASKS_SHIP_Msk BIT(5)

/* BUCK register offsets and bit masks */
/* TASKS (0x68) - write-one-to-trigger */
#define NPM10_BUCK_TASKS       0x68U
#define BUCK_TASKS_RELEASE_Msk BIT(0)

/* SET (0x69) - write-one-to-set, CLR (0x6A) - write-one-to-clear */
#define NPM10_BUCK_SET           0x69U
#define NPM10_BUCK_CLR           0x6AU
#define BUCK_SETCLR_START_Msk    BIT(0)
#define BUCK_SETCLR_VOUTSEL_Msk  BIT(1)
#define BUCK_SETCLR_PWRMODE_Msk  BIT(2)
#define BUCK_SETCLR_PTENA_Msk    BIT(3)
#define BUCK_SETCLR_SCPROT_Msk   BIT(4)
#define BUCK_SETCLR_ENASEL_Msk   BIT(5)
#define BUCK_SETCLR_AUTOPULL_Msk BIT(6)

/* VSET (0x6B) */
#define NPM10_BUCK_VSET   0x6BU
#define BUCK_VSET_SEL_Msk BIT(0)

/* PULLDN (0x6C) */
#define NPM10_BUCK_PULLDN   0x6CU
#define BUCK_PULLDN_ENA_Msk BIT(0)
#define BUCK_PULLDN_RES_Msk (BIT_MASK(2) << 1)

/* RIPPLE (0x6D) */
#define NPM10_BUCK_RIPPLE   0x6DU
#define BUCK_RIPPLE_LVL_Msk (BIT_MASK(2) << 0)

/* GPIO (0x6E) */
#define NPM10_BUCK_GPIO       0x6EU
#define BUCK_GPIO_ENABLE_Msk  BIT(0)
#define BUCK_GPIO_PWRMODE_Msk BIT(1)
#define BUCK_GPIO_ALTVOUT_Msk BIT(2)

/* STATUS (0x6F) */
#define NPM10_BUCK_STATUS            0x6FU
#define BUCK_STATUS_ENABLE_Msk       BIT(0)
#define BUCK_STATUS_SOFTSTART_Msk    BIT(1)
#define BUCK_STATUS_PWRMODE_Msk      BIT(2)
#define BUCK_STATUS_PTMODE_Msk       BIT(3)
#define BUCK_STATUS_TIMEOUT_Msk      BIT(4)
#define BUCK_STATUS_SHORTCIRCUIT_Msk BIT(5)

/* READVOUT (0x70), VOUT (0x71), VOUT2 (0x72) */
#define NPM10_BUCK_READVOUT 0x70U
#define NPM10_BUCK_VOUT     0x71U
#define NPM10_BUCK_VOUT2    0x72U
#define BUCK_VOUT_LVL_Msk   (BIT_MASK(6) << 0)

/* ILIM (0x73) */
#define NPM10_BUCK_ILIM        0x73U
#define BUCK_ILIM_LVL_Msk      (BIT_MASK(3) << 0)
#define BUCK_ILIM_STARTLVL_Msk (BIT_MASK(3) << 3)

/* BIAS (0x74) */
#define NPM10_BUCK_BIAS   0x74U
#define BUCK_BIAS_LP_Msk  (BIT_MASK(2) << 0)
#define BUCK_BIAS_ULP_Msk (BIT_MASK(2) << 2)

/* LOADSW1/LDO1 register offsets and bit masks */
/* SET (0x84) - write-one-to-set, CLR (0x85) - write-one-to-clear */
#define NPM10_LDO1_SET          0x84U
#define NPM10_LDO1_CLR          0x85U
#define LDO1_SETCLR_ENABLE_Msk  BIT(0)
#define LDO1_SETCLR_MODESEL_Msk BIT(1)
#define LDO1_SETCLR_ENASEL_Msk  BIT(2)

/* GPIO (0x86) */
#define NPM10_LDO1_GPIO      0x86U
#define LDO1_GPIO_ENABLE_Msk BIT(0)

/* CONFIG0 (0x87) */
#define NPM10_LDO1_CONFIG0             0x87U
#define LDO1_CONFIG0_SEL_Msk           BIT(0)
#define LDO1_CONFIG0_ILIMLVL_Msk       (BIT_MASK(2) << 1)
#define LDO1_CONFIG0_SOFTSTARTTIME_Msk (BIT_MASK(2) << 3)

/* CONFIG1 (0x88) */
#define NPM10_LDO1_CONFIG1          0x88U
#define LDO1_CONFIG1_SOFTSTART_Msk  BIT(0)
#define LDO1_CONFIG1_ILIM_Msk       BIT(1)
#define LDO1_CONFIG1_PULLDN_Msk     BIT(2)
#define LDO1_CONFIG1_WEAKPULLDN_Msk BIT(3)

/* VOUT (0x89) */
#define NPM10_LDO1_VOUT   0x89U
#define LDO1_VOUT_LVL_Msk (BIT_MASK(6) << 0)

/* STATUS (0x8A) */
#define NPM10_LDO1_STATUS           0x8AU
#define LDO1_STATUS_PRWUPLS_Msk     BIT(0)
#define LDO1_STATUS_PWRUPLDO_Msk    BIT(1)
#define LDO1_STATUS_OVERCURRENT_Msk BIT(2)

/* LOADSW2 register offsets and bit masks */
/* SET (0x7C) - write-one-to-set, CLR (0x7D) - write-one-to-clear */
#define NPM10_LOADSW2_SET         0x7CU
#define NPM10_LOADSW2_CLR         0x7DU
#define LOADSW2_SETCLR_ENABLE_Msk BIT(0)
#define LOADSW2_SETCLR_PULLDN_Msk BIT(1)

/* CONFIG (0x7E) */
#define NPM10_LOADSW2_CONFIG             0x7EU
#define LOADSW2_CONFIG_LVL_Msk           (BIT_MASK(2) << 0)
#define LOADSW2_CONFIG_ILIM_Msk          BIT(2)
#define LOADSW2_CONFIG_SOFTSTARTTIME_Msk (BIT_MASK(2) << 3)
#define LOADSW2_CONFIG_SOFTSTART_Msk     BIT(5)

/* GPIO (0x7F) */
#define NPM10_LOADSW2_GPIO      0x7FU
#define LOADSW2_GPIO_ENABLE_Msk BIT(0)

/* STATUS (0x80) */
#define NPM10_LOADSW2_STATUS           0x80U
#define LOADSW2_STATUS_PWRUPLS_Msk     BIT(0)
#define LOADSW2_STATUS_OVERCURRENT_Msk BIT(1)

static const struct linear_range buck_vout_range = LINEAR_RANGE_INIT(1000000, 50000, 9, 55);
static const struct linear_range ldo_vout_range = LINEAR_RANGE_INIT(1000000, 50000, 6, 52);

#define LDO_LOADSW_ILIM_OFF -1
#define LDO_LOADSW_ILIM_UA  125000
#define BUCK_ILIM_MIN_IDX   4U
static const int32_t buck_ilim_ua[] = {66000,  91000,  117000, 142000,
				       167000, 192000, 217000, 291000};

/* GPIO enable control mask for all regulators is the same */
#define REG_GPIO_ENABLE_Msk BUCK_GPIO_ENABLE_Msk

enum npm10xx_source {
	NPM10XX_SOURCE_BUCK,
	NPM10XX_SOURCE_LDO,
	NPM10XX_SOURCE_LDSW,
};

struct regulator_npm10xx_gpio_config {
	bool in_use;
	uint8_t pin;
	gpio_dt_flags_t flags;
};

struct regulator_npm10xx_pconfig {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec dvs_state_pins[3];
};

struct regulator_npm10xx_config {
	struct regulator_common_config common;
	struct i2c_dt_spec i2c;
	enum npm10xx_source source;
	/* NOTE: gpio_config fields currently have no effect. To be implemented after GPIO driver */
	struct regulator_npm10xx_gpio_config enable_gpio_config;
	struct regulator_npm10xx_gpio_config pwr_mode_gpio_config;
	struct regulator_npm10xx_gpio_config alt_vout_gpio_config;
	int32_t alternate_uv;
	bool passthru;
	bool oc_protection;
	uint8_t soft_start_curr_idx;
	uint8_t soft_start_timeout_idx;
	uint8_t pull_down_idx;
	uint8_t ripple_idx;
};

struct regulator_npm10xx_data {
	struct regulator_common_data data;
};

/* Helper functions */
static int set_gpio_ctrl(const struct regulator_npm10xx_config *config, regulator_mode_t mode)
{
	uint8_t reg = 0U;

	if (!(mode & NPM10XX_REG_GPIO_Msk)) {
		return 0;
	}
	if (mode & NPM10XX_REG_GPIO_NONE &&
	    (mode & NPM10XX_REG_GPIO_Msk & ~NPM10XX_REG_GPIO_NONE)) {
		LOG_ERR("Cannot combine NPM10XX_REG_GPIO_NONE with other GPIO modes");
		return -EINVAL;
	}

	if (mode & NPM10XX_REG_GPIO_EN) {
		reg |= REG_GPIO_ENABLE_Msk;
	}
	if (mode & NPM10XX_BUCK_GPIO_PWRMODE) {
		reg |= BUCK_GPIO_PWRMODE_Msk;
	}
	if (mode & NPM10XX_BUCK_GPIO_ALTVOUT) {
		reg |= BUCK_GPIO_ALTVOUT_Msk;
	}

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_GPIO, reg);

	case NPM10XX_SOURCE_LDO:
		if (reg > 1U) {
			LOG_ERR("invalid mode for the LDO regulator");
			return -EINVAL;
		}
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_LDO1_GPIO, reg);

	case NPM10XX_SOURCE_LDSW:
		if (reg > 1U) {
			LOG_ERR("invalid mode for the LOADSW regulator");
			return -EINVAL;
		}
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_LOADSW2_GPIO, reg);

	default:
		return -ENODEV;
	}
}

/* Parent API implementation */
static int regulator_npm10xx_parent_dvs_state_set(const struct device *dev,
						  regulator_dvs_state_t state)
{
	const struct regulator_npm10xx_pconfig *pconfig = dev->config;
	const struct gpio_dt_spec *spec;
	int ret;

	ARRAY_FOR_EACH(pconfig->dvs_state_pins, idx) {
		spec = &pconfig->dvs_state_pins[idx];

		if (spec->port != NULL) {
			ret = gpio_pin_set_dt(spec, IS_BIT_SET(state, idx));
			if (ret != 0) {
				LOG_ERR("failed to set DVS pin #%d", idx);
				return ret;
			}
		}
	}

	return 0;
}

static int regulator_npm10xx_parent_ship_mode(const struct device *dev)
{
	const struct regulator_npm10xx_pconfig *pconfig = dev->config;

	return i2c_reg_write_byte_dt(&pconfig->i2c, NPM10_RESET_TASKS, RESET_TASKS_SHIP_Msk);
}

static int regulator_npm10xx_parent_init(const struct device *dev)
{
	const struct regulator_npm10xx_pconfig *pconfig = dev->config;
	const struct gpio_dt_spec *spec;
	int ret;

	ARRAY_FOR_EACH(pconfig->dvs_state_pins, idx) {
		spec = &pconfig->dvs_state_pins[idx];

		if (spec->port != NULL) {
			if (!gpio_is_ready_dt(spec)) {
				LOG_ERR("DVS pin #%d is not ready", idx);
				return -ENODEV;
			}

			ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
			if (ret != 0) {
				LOG_ERR("failed to configure DVS pin #%d", idx);
				return ret;
			}
		}
	}

	return 0;
}

static DEVICE_API(regulator_parent, regulator_npm10xx_parent_driver_api) = {
	.dvs_state_set = regulator_npm10xx_parent_dvs_state_set,
	.ship_mode = regulator_npm10xx_parent_ship_mode,
};

/* Child API implementation */
static int regulator_npm10xx_enable(const struct device *dev)
{
	const struct regulator_npm10xx_config *config = dev->config;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_SET,
					     BUCK_SETCLR_START_Msk | BUCK_SETCLR_ENASEL_Msk);

	case NPM10XX_SOURCE_LDO:
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_LDO1_SET,
					     LDO1_SETCLR_ENABLE_Msk | LDO1_SETCLR_ENASEL_Msk);

	case NPM10XX_SOURCE_LDSW:
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_LOADSW2_SET,
					     LOADSW2_SETCLR_ENABLE_Msk);

	default:
		return -ENODEV;
	}
}

static int regulator_npm10xx_disable(const struct device *dev)
{
	const struct regulator_npm10xx_config *config = dev->config;
	uint8_t regs[2];

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		regs[0] = BUCK_SETCLR_ENASEL_Msk;
		regs[1] = BUCK_SETCLR_START_Msk;
		return i2c_burst_write_dt(&config->i2c, NPM10_BUCK_SET, regs, ARRAY_SIZE(regs));

	case NPM10XX_SOURCE_LDO:
		regs[0] = LDO1_SETCLR_ENASEL_Msk;
		regs[1] = LDO1_SETCLR_ENABLE_Msk;
		return i2c_burst_write_dt(&config->i2c, NPM10_LDO1_SET, regs, ARRAY_SIZE(regs));

	case NPM10XX_SOURCE_LDSW:
		return i2c_reg_write_byte_dt(&config->i2c, NPM10_LOADSW2_CLR,
					     LOADSW2_SETCLR_ENABLE_Msk);

	default:
		return -ENODEV;
	}
}

static unsigned int regulator_npm10xx_count_voltages(const struct device *dev)
{
	const struct regulator_npm10xx_config *config = dev->config;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		return linear_range_values_count(&buck_vout_range);

	case NPM10XX_SOURCE_LDO:
		return linear_range_values_count(&ldo_vout_range);

	case NPM10XX_SOURCE_LDSW:
		/* no voltage selection on LOADSW - fall through */
	default:
		return 0U;
	}
}

static int regulator_npm10xx_list_voltage(const struct device *dev, unsigned int idx,
					  int32_t *volt_uv)
{
	const struct regulator_npm10xx_config *config = dev->config;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		return linear_range_get_value(&buck_vout_range, buck_vout_range.min_idx + idx,
					      volt_uv);

	case NPM10XX_SOURCE_LDO:
		return linear_range_get_value(&ldo_vout_range, ldo_vout_range.min_idx + idx,
					      volt_uv);

	case NPM10XX_SOURCE_LDSW:
		return -ENOTSUP;

	default:
		return -ENODEV;
	}
}

static int regulator_npm10xx_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_npm10xx_config *config = dev->config;
	int ret;
	uint16_t idx;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		ret = linear_range_get_win_index(&buck_vout_range, min_uv, max_uv, &idx);
		if (ret == -EINVAL) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_VOUT,
					    FIELD_PREP(BUCK_VOUT_LVL_Msk, idx));
		if (ret < 0) {
			return ret;
		}

		return i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_VSET, BUCK_VSET_SEL_Msk);

	case NPM10XX_SOURCE_LDO:
		ret = linear_range_get_win_index(&ldo_vout_range, min_uv, max_uv, &idx);
		if (ret == -EINVAL) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_LDO1_VOUT,
					    FIELD_PREP(LDO1_VOUT_LVL_Msk, idx));
		if (ret < 0) {
			return ret;
		}

		return i2c_reg_update_byte_dt(&config->i2c, NPM10_LDO1_CONFIG0,
					      LDO1_CONFIG0_SEL_Msk, LDO1_CONFIG0_SEL_Msk);

	case NPM10XX_SOURCE_LDSW:
		return -ENOTSUP;
	default:
		return -ENODEV;
	}
}

static int regulator_npm10xx_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_npm10xx_config *config = dev->config;
	int ret;
	uint8_t reg;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_BUCK_READVOUT, &reg);
		if (ret < 0) {
			return ret;
		}
		return linear_range_get_value(&buck_vout_range, FIELD_GET(BUCK_VOUT_LVL_Msk, reg),
					      volt_uv);

	case NPM10XX_SOURCE_LDO:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LDO1_CONFIG0, &reg);
		if (ret < 0) {
			return ret;
		}
		if (!FIELD_GET(LDO1_CONFIG0_SEL_Msk, reg)) {
			LOG_WRN("LDO voltage is selected by VSET pin, cannot determine VOUT");
			return -ENOTSUP;
		}

		ret = regulator_get_mode(dev, &reg);
		if (ret < 0) {
			return ret;
		}
		if ((reg & NPM10XX_LDO_MODE_Msk) == NPM10XX_LDO_MODE_LS) {
			LOG_WRN("LDO is in LOADSW mode, cannot determine VOUT");
			return -ENOTSUP;
		}

		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LDO1_VOUT, &reg);
		if (ret < 0) {
			return ret;
		}
		return linear_range_get_value(&ldo_vout_range, FIELD_GET(LDO1_VOUT_LVL_Msk, reg),
					      volt_uv);

	case NPM10XX_SOURCE_LDSW:
		return -ENOTSUP;

	default:
		return 0;
	}
}

static unsigned int regulator_npm10xx_count_current_limits(const struct device *dev)
{
	const struct regulator_npm10xx_config *config = dev->config;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		return ARRAY_SIZE(buck_ilim_ua);

	case NPM10XX_SOURCE_LDO:
		/* fall through */
	case NPM10XX_SOURCE_LDSW:
		/* 2 "limits" - fixed 125 mA or off */
		return 2U;

	default:
		return 0U;
	}
}

static int regulator_npm10xx_list_current_limit(const struct device *dev, unsigned int idx,
						int32_t *current_ua)
{
	const struct regulator_npm10xx_config *config = dev->config;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		if (idx >= ARRAY_SIZE(buck_ilim_ua)) {
			return -EINVAL;
		}
		*current_ua = buck_ilim_ua[idx];
		return 0;

	case NPM10XX_SOURCE_LDO:
		/* fall through */
	case NPM10XX_SOURCE_LDSW:
		if (idx > 1) {
			return -EINVAL;
		}
		*current_ua = (idx == 0) ? LDO_LOADSW_ILIM_OFF : LDO_LOADSW_ILIM_UA;
		return 0;

	default:
		return -ENODEV;
	}
}

static int regulator_npm10xx_set_current_limit(const struct device *dev, int32_t min_ua,
					       int32_t max_ua)
{
	const struct regulator_npm10xx_config *config = dev->config;
	bool enable;

	if (max_ua < min_ua) {
		return -EINVAL;
	}

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		ARRAY_FOR_EACH(buck_ilim_ua, idx) {
			if (max_ua < buck_ilim_ua[idx]) {
				break;
			}
			if (min_ua > buck_ilim_ua[idx]) {
				continue;
			}
			return i2c_reg_update_byte_dt(&config->i2c, NPM10_BUCK_ILIM,
						      BUCK_ILIM_LVL_Msk,
						      FIELD_PREP(BUCK_ILIM_LVL_Msk, idx));
		}
		return -EINVAL;

	case NPM10XX_SOURCE_LDO:
		/* fall through */
	case NPM10XX_SOURCE_LDSW:
		if (min_ua == LDO_LOADSW_ILIM_OFF && max_ua == LDO_LOADSW_ILIM_OFF) {
			enable = false;
		} else if (min_ua <= LDO_LOADSW_ILIM_UA && max_ua >= LDO_LOADSW_ILIM_UA) {
			enable = true;
		} else {
			return -EINVAL;
		}
		break;

	default:
		return -ENODEV;
	}

	if (config->source == NPM10XX_SOURCE_LDO) {
		return i2c_reg_update_byte_dt(&config->i2c, NPM10_LDO1_CONFIG1,
					      LDO1_CONFIG1_ILIM_Msk,
					      FIELD_PREP(LDO1_CONFIG1_ILIM_Msk, enable));
	}
	return i2c_reg_update_byte_dt(&config->i2c, NPM10_LOADSW2_CONFIG, LOADSW2_CONFIG_ILIM_Msk,
				      FIELD_PREP(LOADSW2_CONFIG_ILIM_Msk, enable));
}

static int regulator_npm10xx_get_current_limit(const struct device *dev, int32_t *curr_ua)
{
	const struct regulator_npm10xx_config *config = dev->config;
	int ret;
	uint8_t reg;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_BUCK_ILIM, &reg);
		if (ret < 0) {
			return ret;
		}

		*curr_ua = buck_ilim_ua[FIELD_GET(BUCK_ILIM_LVL_Msk, reg)];
		return 0;

	case NPM10XX_SOURCE_LDO:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LDO1_CONFIG1, &reg);
		if (ret < 0) {
			return ret;
		}

		*curr_ua = FIELD_GET(LDO1_CONFIG1_ILIM_Msk, reg) ? LDO_LOADSW_ILIM_UA
								 : LDO_LOADSW_ILIM_OFF;
		return 0;

	case NPM10XX_SOURCE_LDSW:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LOADSW2_CONFIG, &reg);
		if (ret < 0) {
			return ret;
		}

		*curr_ua = FIELD_GET(LOADSW2_CONFIG_ILIM_Msk, reg) ? LDO_LOADSW_ILIM_UA
								   : LDO_LOADSW_ILIM_OFF;
		return 0;

	default:
		return -ENODEV;
	}
}

static int regulator_npm10xx_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_npm10xx_config *config = dev->config;
	int ret = 0;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		if ((mode & NPM10XX_BUCK_PWRMODE_Msk) == NPM10XX_BUCK_PWRMODE_LP) {
			ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_CLR,
						    BUCK_SETCLR_PWRMODE_Msk);
		} else if ((mode & NPM10XX_BUCK_PWRMODE_Msk) == NPM10XX_BUCK_PWRMODE_ULP) {
			ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_SET,
						    BUCK_SETCLR_PWRMODE_Msk);
		}
		if (ret < 0) {
			return ret;
		}

		if ((mode & NPM10XX_BUCK_PASSTHRU_Msk) == NPM10XX_BUCK_PASSTHRU_OFF) {
			ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_CLR,
						    BUCK_SETCLR_PTENA_Msk);
		} else if ((mode & NPM10XX_BUCK_PASSTHRU_Msk) == NPM10XX_BUCK_PASSTHRU_ON) {
			ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_SET,
						    BUCK_SETCLR_PTENA_Msk);
		}
		if (ret < 0) {
			return ret;
		}

		break;

	case NPM10XX_SOURCE_LDO:
		if ((mode & NPM10XX_LDO_MODE_Msk) == NPM10XX_LDO_MODE_LDO) {
			ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_LDO1_CLR,
						    LDO1_SETCLR_MODESEL_Msk);
		} else if ((mode & NPM10XX_LDO_MODE_Msk) == NPM10XX_LDO_MODE_LS) {
			ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_LDO1_SET,
						    LDO1_SETCLR_MODESEL_Msk);
		}
		if (ret < 0) {
			return ret;
		}

		break;

	case NPM10XX_SOURCE_LDSW:
		break;

	default:
		return -ENODEV;
	}

	return set_gpio_ctrl(config, mode);
}

static int regulator_npm10xx_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_npm10xx_config *config = dev->config;
	uint8_t stat, gpio;
	int ret;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_BUCK_STATUS, &stat);
		if (ret < 0) {
			return ret;
		}
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_BUCK_GPIO, &gpio);
		if (ret < 0) {
			return ret;
		}

		*mode = FIELD_PREP(NPM10XX_BUCK_PWRMODE_Msk,
				   FIELD_GET(BUCK_STATUS_PWRMODE_Msk, stat) + 1) |
			FIELD_PREP(NPM10XX_BUCK_PASSTHRU_Msk,
				   FIELD_GET(BUCK_STATUS_PTMODE_Msk, stat) + 1);
		break;

	case NPM10XX_SOURCE_LDO:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LDO1_SET, &stat);
		if (ret < 0) {
			return ret;
		}
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LDO1_GPIO, &gpio);
		if (ret < 0) {
			return ret;
		}

		*mode = FIELD_PREP(NPM10XX_LDO_MODE_Msk,
				   FIELD_GET(LDO1_SETCLR_MODESEL_Msk, stat) + 1);
		break;

	case NPM10XX_SOURCE_LDSW:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LOADSW2_GPIO, &gpio);
		if (ret < 0) {
			return ret;
		}

		break;

	default:
		return -ENODEV;
	}

	*mode |= gpio ? FIELD_PREP(NPM10XX_REG_GPIO_Msk, gpio) : NPM10XX_REG_GPIO_NONE;
	return 0;
}

static int regulator_npm10xx_set_active_discharge(const struct device *dev, bool active_discharge)
{
	const struct regulator_npm10xx_config *config = dev->config;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		return i2c_reg_update_byte_dt(&config->i2c, NPM10_BUCK_PULLDN, BUCK_PULLDN_ENA_Msk,
					      FIELD_PREP(BUCK_PULLDN_ENA_Msk, active_discharge));

	case NPM10XX_SOURCE_LDO:
		return i2c_reg_update_byte_dt(
			&config->i2c, NPM10_LDO1_CONFIG1, LDO1_CONFIG1_PULLDN_Msk,
			FIELD_PREP(LDO1_CONFIG1_PULLDN_Msk, active_discharge));

	case NPM10XX_SOURCE_LDSW:
		return i2c_reg_write_byte_dt(
			&config->i2c, active_discharge ? NPM10_LOADSW2_SET : NPM10_LOADSW2_CLR,
			LOADSW2_SETCLR_PULLDN_Msk);

	default:
		return -ENODEV;
	}
}

static int regulator_npm10xx_get_active_discharge(const struct device *dev, bool *active_discharge)
{
	const struct regulator_npm10xx_config *config = dev->config;
	uint8_t reg;
	int ret;

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_BUCK_PULLDN, &reg);
		if (ret < 0) {
			return ret;
		}
		*active_discharge = FIELD_GET(BUCK_PULLDN_ENA_Msk, reg);
		return 0;

	case NPM10XX_SOURCE_LDO:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LDO1_CONFIG1, &reg);
		if (ret < 0) {
			return ret;
		}
		*active_discharge = FIELD_GET(LDO1_CONFIG1_PULLDN_Msk, reg);
		return 0;

	case NPM10XX_SOURCE_LDSW:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LOADSW2_SET, &reg);
		if (ret < 0) {
			return ret;
		}
		*active_discharge = FIELD_GET(LOADSW2_SETCLR_PULLDN_Msk, reg);
		return 0;

	default:
		return -ENODEV;
	}
}

/* Initializers */
static int configure_active_discharge(const struct regulator_npm10xx_config *config)
{
	uint8_t active_discharge = REGULATOR_ACTIVE_DISCHARGE_GET_BITS(config->common.flags);
	uint8_t reg = UINT8_MAX;

	if (config->pull_down_idx != UINT8_MAX) {
		if (config->source != NPM10XX_SOURCE_BUCK) {
			LOG_ERR("Pull-down resistance setting is supported on BUCK only");
			return -ENOTSUP;
		}

		reg = FIELD_PREP(BUCK_PULLDN_RES_Msk, config->pull_down_idx);
	}

	if (active_discharge == REGULATOR_ACTIVE_DISCHARGE_DEFAULT) {
		if (reg != UINT8_MAX) {
			return i2c_reg_update_byte_dt(&config->i2c, NPM10_BUCK_PULLDN,
						      BUCK_PULLDN_RES_Msk, reg);
		}
		return 0;
	}

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		if (reg != UINT8_MAX) {
			return i2c_reg_write_byte_dt(
				&config->i2c, NPM10_BUCK_PULLDN,
				reg | FIELD_PREP(BUCK_PULLDN_ENA_Msk, active_discharge));
		} else {
			return i2c_reg_update_byte_dt(
				&config->i2c, NPM10_BUCK_PULLDN, BUCK_PULLDN_ENA_Msk,
				FIELD_PREP(BUCK_PULLDN_ENA_Msk, active_discharge));
		}

	case NPM10XX_SOURCE_LDO:
		return i2c_reg_update_byte_dt(&config->i2c, NPM10_LDO1_CONFIG1,
					      LDO1_CONFIG1_PULLDN_Msk,
					      FIELD_PREP(LDO1_CONFIG1_PULLDN_Msk, reg));

	case NPM10XX_SOURCE_LDSW:
		return i2c_reg_write_byte_dt(
			&config->i2c, active_discharge ? NPM10_LOADSW2_SET : NPM10_LOADSW2_CLR,
			LOADSW2_SETCLR_PULLDN_Msk);

	default:
		return -ENODEV;
	}
}

static int configure_soft_start(const struct regulator_npm10xx_config *config)
{
	uint8_t addr = 0U;
	uint8_t mask = 0U;
	uint8_t value = 0U;

	if (config->soft_start_curr_idx != UINT8_MAX) {
		switch (config->source) {
		case NPM10XX_SOURCE_BUCK:
			if (config->soft_start_curr_idx < BUCK_ILIM_MIN_IDX) {
				LOG_ERR("invalid regulator-soft-start-microamp for BUCK");
				return -EINVAL;
			}
			addr = NPM10_BUCK_ILIM;
			mask = BUCK_ILIM_STARTLVL_Msk;
			value = FIELD_PREP(BUCK_ILIM_STARTLVL_Msk,
					   config->soft_start_curr_idx - BUCK_ILIM_MIN_IDX);
			break;

		case NPM10XX_SOURCE_LDO:
			if (config->soft_start_curr_idx >= BUCK_ILIM_MIN_IDX) {
				LOG_ERR("invalid regulator-soft-start-microamp for LDO");
				return -EINVAL;
			}
			addr = NPM10_LDO1_CONFIG0;
			mask = LDO1_CONFIG0_ILIMLVL_Msk;
			value = FIELD_PREP(LDO1_CONFIG0_ILIMLVL_Msk, config->soft_start_curr_idx);
			break;

		case NPM10XX_SOURCE_LDSW:
			if (config->soft_start_curr_idx >= BUCK_ILIM_MIN_IDX) {
				LOG_ERR("invalid regulator-soft-start-microamp for LOADSW");
				return -EINVAL;
			}
			addr = NPM10_LOADSW2_CONFIG;
			mask = LOADSW2_CONFIG_LVL_Msk;
			value = FIELD_PREP(LOADSW2_CONFIG_LVL_Msk, config->soft_start_curr_idx);
			break;

		default:
			return -ENODEV;
		}
	}

	if (config->soft_start_timeout_idx != UINT8_MAX) {
		switch (config->source) {
		case NPM10XX_SOURCE_BUCK:
			LOG_ERR("Soft start timeout configuration is not supported on BUCK");
			return -ENOTSUP;

		case NPM10XX_SOURCE_LDO:
			addr = NPM10_LDO1_CONFIG0;
			mask |= LDO1_CONFIG0_SOFTSTARTTIME_Msk;
			value |= FIELD_PREP(LDO1_CONFIG0_SOFTSTARTTIME_Msk,
					    config->soft_start_timeout_idx);
			break;

		case NPM10XX_SOURCE_LDSW:
			addr = NPM10_LOADSW2_CONFIG;
			mask |= LOADSW2_CONFIG_SOFTSTARTTIME_Msk;
			value |= FIELD_PREP(LOADSW2_CONFIG_SOFTSTARTTIME_Msk,
					    config->soft_start_timeout_idx);
			break;

		default:
			return -ENODEV;
		}
	}

	return mask ? i2c_reg_update_byte_dt(&config->i2c, addr, mask, value) : 0;
}

static int regulator_npm10xx_init(const struct device *dev)
{
	const struct regulator_npm10xx_config *config = dev->config;
	int ret;
	uint16_t idx;
	uint8_t reg;
	bool enabled;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	if (config->alternate_uv > INT32_MIN) {
		if (config->source != NPM10XX_SOURCE_BUCK) {
			LOG_ERR("Alternate VOUT is supported on BUCK only");
			return -ENOTSUP;
		}

		ret = linear_range_get_index(&buck_vout_range, config->alternate_uv, &idx);
		if (ret == -EINVAL) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_VOUT2,
					    FIELD_PREP(BUCK_VOUT_LVL_Msk, idx));
		if (ret < 0) {
			return ret;
		}
	}

	if (config->passthru) {
		if (config->source != NPM10XX_SOURCE_BUCK) {
			LOG_ERR("Pass-through mode is supported on BUCK only");
			return -ENOTSUP;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_SET, BUCK_SETCLR_PTENA_Msk);
		if (ret < 0) {
			return ret;
		}
	}

	ret = configure_active_discharge(config);
	if (ret < 0) {
		return ret;
	}

	ret = configure_soft_start(config);
	if (ret < 0) {
		return ret;
	}

	if (config->oc_protection) {
		switch (config->source) {
		case NPM10XX_SOURCE_BUCK:
			ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_SET,
						    BUCK_SETCLR_SCPROT_Msk);
			break;
		case NPM10XX_SOURCE_LDO:
			ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_LDO1_CONFIG1,
						     LDO1_CONFIG1_ILIM_Msk, LDO1_CONFIG1_ILIM_Msk);
			break;
		case NPM10XX_SOURCE_LDSW:
			ret = i2c_reg_update_byte_dt(&config->i2c, NPM10_LOADSW2_CONFIG,
						     LOADSW2_CONFIG_ILIM_Msk,
						     LOADSW2_CONFIG_ILIM_Msk);
			break;
		default:
			return -ENODEV;
		}

		if (ret < 0) {
			return ret;
		}
	}

	if (config->ripple_idx != UINT8_MAX) {
		if (config->source != NPM10XX_SOURCE_BUCK) {
			LOG_ERR("Ripple level setting is supported on BUCK only");
			return -ENOTSUP;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_BUCK_RIPPLE,
					    FIELD_PREP(BUCK_RIPPLE_LVL_Msk, config->ripple_idx));
		if (ret < 0) {
			return ret;
		}
	}

	switch (config->source) {
	case NPM10XX_SOURCE_BUCK:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_BUCK_STATUS, &reg);
		enabled = FIELD_GET(BUCK_STATUS_ENABLE_Msk, reg);
		break;

	case NPM10XX_SOURCE_LDO:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LDO1_STATUS, &reg);
		enabled = FIELD_GET(LDO1_STATUS_PRWUPLS_Msk | LDO1_STATUS_PWRUPLDO_Msk, reg);
		break;

	case NPM10XX_SOURCE_LDSW:
		ret = i2c_reg_read_byte_dt(&config->i2c, NPM10_LOADSW2_STATUS, &reg);
		enabled = FIELD_GET(LOADSW2_STATUS_PWRUPLS_Msk, reg);
		break;

	default:
		return -ENODEV;
	}
	if (ret < 0) {
		return ret;
	}

	return regulator_common_init(dev, enabled);
}

static DEVICE_API(regulator, regulator_npm10xx_driver_api) = {
	.enable = regulator_npm10xx_enable,
	.disable = regulator_npm10xx_disable,
	.count_voltages = regulator_npm10xx_count_voltages,
	.list_voltage = regulator_npm10xx_list_voltage,
	.set_voltage = regulator_npm10xx_set_voltage,
	.get_voltage = regulator_npm10xx_get_voltage,
	.count_current_limits = regulator_npm10xx_count_current_limits,
	.list_current_limit = regulator_npm10xx_list_current_limit,
	.set_current_limit = regulator_npm10xx_set_current_limit,
	.get_current_limit = regulator_npm10xx_get_current_limit,
	.set_mode = regulator_npm10xx_set_mode,
	.get_mode = regulator_npm10xx_get_mode,
	.set_active_discharge = regulator_npm10xx_set_active_discharge,
	.get_active_discharge = regulator_npm10xx_get_active_discharge,
};

#define GPIO_CONFIG_DEFINE(node_id, prop)                                                          \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop),                                               \
		({1, DT_PROP_BY_IDX(node_id, prop, 0), DT_PROP_BY_IDX(node_id, prop, 1)}), ({0}))

#define REGULATOR_NPM10XX_DEFINE(node_id, id, src)                                                 \
	static struct regulator_npm10xx_data regulator_data_##id;                                  \
	static const struct regulator_npm10xx_config regulator_config_##id = {                     \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.i2c = I2C_DT_SPEC_GET(DT_GPARENT(node_id)),                                       \
		.source = src,                                                                     \
		.enable_gpio_config = GPIO_CONFIG_DEFINE(node_id, enable_gpio_config),             \
		.pwr_mode_gpio_config = GPIO_CONFIG_DEFINE(node_id, pwr_mode_gpio_config),         \
		.alt_vout_gpio_config = GPIO_CONFIG_DEFINE(node_id, alternate_vout_gpio_config),   \
		.alternate_uv = DT_PROP_OR(node_id, regulator_alternate_microvolt, INT32_MIN),     \
		.passthru = DT_PROP(node_id, regulator_allow_bypass),                              \
		.oc_protection = DT_PROP(node_id, regulator_over_current_protection),              \
		.soft_start_curr_idx =                                                             \
			DT_ENUM_IDX_OR(node_id, regulator_soft_start_microamp, UINT8_MAX),         \
		.soft_start_timeout_idx =                                                          \
			DT_ENUM_IDX_OR(node_id, regulator_soft_start_us, UINT8_MAX),               \
		.pull_down_idx = DT_ENUM_IDX_OR(node_id, regulator_pull_down_ohms, UINT8_MAX),     \
		.ripple_idx = DT_ENUM_IDX_OR(node_id, regulator_ripple, UINT8_MAX),                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, regulator_npm10xx_init, NULL, &regulator_data_##id,              \
			 &regulator_config_##id, POST_KERNEL,                                      \
			 CONFIG_REGULATOR_NPM10XX_INIT_PRIORITY, &regulator_npm10xx_driver_api);

#define REGULATOR_NPM10XX_DEFINE_COND(n, child, src)                                               \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(n, child)),                                       \
		    (REGULATOR_NPM10XX_DEFINE(DT_INST_CHILD(n, child), child##n, src)),            \
		    ())

#define REGULATOR_NPM10XX_DEFINE_ALL(n)                                                            \
	static const struct regulator_npm10xx_pconfig regulator_config##n = {                      \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
		.dvs_state_pins = {GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, dvs_gpios, 0, {0}),          \
				   GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, dvs_gpios, 1, {0}),          \
				   GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, dvs_gpios, 2, {0})},         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, regulator_npm10xx_parent_init, NULL, NULL, &regulator_config##n,  \
			      POST_KERNEL, CONFIG_REGULATOR_NPM10XX_COMMON_INIT_PRIORITY,          \
			      &regulator_npm10xx_parent_driver_api);                               \
                                                                                                   \
	REGULATOR_NPM10XX_DEFINE_COND(n, buck, NPM10XX_SOURCE_BUCK)                                \
	REGULATOR_NPM10XX_DEFINE_COND(n, ldo, NPM10XX_SOURCE_LDO)                                  \
	REGULATOR_NPM10XX_DEFINE_COND(n, ldsw, NPM10XX_SOURCE_LDSW)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_NPM10XX_DEFINE_ALL)
