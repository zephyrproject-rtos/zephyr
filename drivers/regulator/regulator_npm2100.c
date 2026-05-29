/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm2100_regulator

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/regulator/npm2100.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

enum npm2100_sources {
	NPM2100_SOURCE_BOOST,
	NPM2100_SOURCE_LDOSW,
};

#define BOOST_VOUT     0x22U
#define BOOST_VOUTSEL  0x23U
#define BOOST_OPER     0x24U
#define BOOST_LIMIT    0x26U
#define BOOST_GPIO     0x28U
#define BOOST_PIN      0x29U
#define BOOST_CTRLSET  0x2AU
#define BOOST_CTRLCLR  0x2BU
#define BOOST_IBATLIM  0x2DU
#define BOOST_VBATMINL 0x2FU
#define BOOST_VBATMINH 0x30U
#define BOOST_VOUTMIN  0x31U
#define BOOST_VOUTWRN  0x32U
#define BOOST_VOUTDPS  0x33U
#define BOOST_STATUS0  0x34U
#define BOOST_STATUS1  0x35U
#define BOOST_VSET0    0x36U
#define BOOST_VSET1    0x37U

#define LDOSW_VOUT   0x68U
#define LDOSW_ENABLE 0x69U
#define LDOSW_SEL    0x6AU
#define LDOSW_GPIO   0x6BU
#define LDOSW_STATUS 0x6EU
#define LDOSW_PRGOCP 0x6FU

#define SHIP_TASK_SHIP 0xC0U

#define RESET_ALTCONFIG    0xD6U
#define RESET_WRITESTICKY  0xDBU
#define RESET_STROBESTICKY 0xDCU

#define BOOST_OPER_MODE_MASK     0x07U
#define BOOST_OPER_MODE_AUTO     0x00U
#define BOOST_OPER_MODE_HP       0x01U
#define BOOST_OPER_MODE_LP       0x02U
#define BOOST_OPER_MODE_PASS     0x03U
#define BOOST_OPER_MODE_NOHP     0x04U
#define BOOST_OPER_DPS_MASK      0x18U
#define BOOST_OPER_DPS_DISABLE   0x00U
#define BOOST_OPER_DPS_ALLOW     0x01U
#define BOOST_OPER_DPS_ALLOWLP   0x02U
#define BOOST_OPER_DPSTIMER_MASK 0x60U

#define BOOST_PIN_FORCE_HP   0x00U
#define BOOST_PIN_FORCE_LP   0x01U
#define BOOST_PIN_FORCE_PASS 0x02U
#define BOOST_PIN_FORCE_NOHP 0x03U

#define BOOST_STATUS0_MODE_MASK 0x07U
#define BOOST_STATUS0_MODE_HP   0x00U
#define BOOST_STATUS0_MODE_LP   0x01U
#define BOOST_STATUS0_MODE_ULP  0x02U
#define BOOST_STATUS0_MODE_PT   0x03U
#define BOOST_STATUS0_MODE_DPS  0x04U

#define BOOST_STATUS1_VSET_MASK 0x40U

#define LDOSW_SEL_OPER_MASK 0x06U
#define LDOSW_SEL_OPER_AUTO 0x00U
#define LDOSW_SEL_OPER_ULP  0x02U
#define LDOSW_SEL_OPER_HP   0x04U
#define LDOSW_SEL_OPER_PIN  0x06U

#define LDOSW_GPIO_PIN_MASK     0x07U
#define LDOSW_GPIO_PINACT_MASK  0x18U
#define LDOSW_GPIO_PINACT_HP    0x00U
#define LDOSW_GPIO_PINACT_ULP   0x08U
#define LDOSW_GPIO_PININACT_OFF 0x00U
#define LDOSW_GPIO_PININACT_ULP 0x10U

#define LDOSW_STATUS_LDO 0x01U
#define LDOSW_STATUS_SW  0x02U
#define LDOSW_STATUS_HP  0x04U
#define LDOSW_STATUS_ULP 0x08U
#define LDOSW_STATUS_OCP 0x10U

#define RESET_ALTCONFIG_LDOSW_OFF 0x01U

struct regulator_npm2100_pconfig {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec dvs_state_pins[2];
};

struct regulator_npm2100_config {
	struct regulator_common_config common;
	struct i2c_dt_spec i2c;
	uint8_t source;
	struct gpio_dt_spec mode_gpios;
	bool ldosw_wd_reset;
	uint8_t dps_timer;
	uint8_t dps_pulse_limit;
};

struct regulator_npm2100_data {
	struct regulator_common_data data;
	bool ldsw_mode;
};

static const struct linear_range boost_range = LINEAR_RANGE_INIT(1800000, 50000, 0U, 30U);
static const struct linear_range ldosw_range = LINEAR_RANGE_INIT(800000, 50000, 8U, 52U);
static const struct linear_range vset0_range = LINEAR_RANGE_INIT(1800000, 100000, 0U, 6U);
static const struct linear_range vset1_ranges[] = {LINEAR_RANGE_INIT(3000000, 0, 0U, 0U),
						   LINEAR_RANGE_INIT(2700000, 100000, 1U, 3U),
						   LINEAR_RANGE_INIT(3100000, 100000, 4U, 6U)};
static const struct linear_range boost_ocp_range = LINEAR_RANGE_INIT(0, 300000, 0U, 1U);

static const struct linear_range ldsw_ocp_ranges[] = {LINEAR_RANGE_INIT(40000, 0, 0U, 0U),
						      LINEAR_RANGE_INIT(70000, 5000, 1U, 3U),
						      LINEAR_RANGE_INIT(110000, 0, 4U, 4U)};
static const uint8_t ldo_ocp_lookup[] = {13, 7, 6, 4, 1};

static const struct linear_range ldo_ocp_ranges[] = {LINEAR_RANGE_INIT(25000, 13000, 0U, 1U),
						     LINEAR_RANGE_INIT(50000, 25000, 2U, 3U),
						     LINEAR_RANGE_INIT(150000, 0, 4U, 4U)};
static const uint8_t ldsw_ocp_lookup[] = {1, 7, 8, 9, 15};

static unsigned int regulator_npm2100_count_voltages(const struct device *dev)
{
	const struct regulator_npm2100_config *config = dev->config;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		return linear_range_values_count(&boost_range);
	case NPM2100_SOURCE_LDOSW:
		return linear_range_values_count(&ldosw_range);
	default:
		return 0;
	}
}

static int regulator_npm2100_list_voltage(const struct device *dev, unsigned int idx,
					  int32_t *volt_uv)
{
	const struct regulator_npm2100_config *config = dev->config;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		return linear_range_get_value(&boost_range, idx, volt_uv);
	case NPM2100_SOURCE_LDOSW:
		return linear_range_get_value(&ldosw_range, idx, volt_uv);
	default:
		return -EINVAL;
	}
}

static int regulator_npm2100_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_npm2100_config *config = dev->config;
	uint16_t idx;
	int ret;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		ret = linear_range_get_win_index(&boost_range, min_uv, max_uv, &idx);
		if (ret == -EINVAL) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, BOOST_VOUT, idx);
		if (ret < 0) {
			return ret;
		}

		/* Enable SW control of boost voltage */
		return i2c_reg_write_byte_dt(&config->i2c, BOOST_VOUTSEL, 1U);

	case NPM2100_SOURCE_LDOSW:
		ret = linear_range_get_win_index(&ldosw_range, min_uv, max_uv, &idx);
		if (ret == -EINVAL) {
			return ret;
		}

		return i2c_reg_write_byte_dt(&config->i2c, LDOSW_VOUT, idx);

	default:
		return -ENODEV;
	}
}

static int regulator_npm2100_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_npm2100_config *config = dev->config;
	uint8_t idx;
	int ret;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		ret = i2c_reg_read_byte_dt(&config->i2c, BOOST_VOUTSEL, &idx);
		if (ret < 0) {
			return ret;
		}

		if (idx == 1U) {
			/* Voltage is selected by register value */
			ret = i2c_reg_read_byte_dt(&config->i2c, BOOST_VOUT, &idx);
			if (ret < 0) {
				return ret;
			}

			return linear_range_get_value(&boost_range, idx, volt_uv);
		}

		/* Voltage is selected by VSET pin */
		ret = i2c_reg_read_byte_dt(&config->i2c, BOOST_STATUS1, &idx);
		if (ret < 0) {
			return ret;
		}

		if ((idx & BOOST_STATUS1_VSET_MASK) == 0U) {
			/* VSET low, voltage is selected by VSET0 register */
			ret = i2c_reg_read_byte_dt(&config->i2c, BOOST_VSET0, &idx);
			if (ret < 0) {
				return ret;
			}

			return linear_range_get_value(&vset0_range, idx, volt_uv);
		}

		/* VSET high, voltage is selected by VSET1 register */
		ret = i2c_reg_read_byte_dt(&config->i2c, BOOST_VSET1, &idx);
		if (ret < 0) {
			return ret;
		}

		return linear_range_group_get_value(vset1_ranges, ARRAY_SIZE(vset1_ranges), idx,
						    volt_uv);

	case NPM2100_SOURCE_LDOSW:
		ret = i2c_reg_read_byte_dt(&config->i2c, LDOSW_VOUT, &idx);
		if (ret < 0) {
			return ret;
		}

		return linear_range_get_value(&ldosw_range, idx, volt_uv);

	default:
		return -ENODEV;
	}
}

static unsigned int regulator_npm2100_count_currents(const struct device *dev)
{
	const struct regulator_npm2100_config *config = dev->config;
	const struct regulator_npm2100_data *data = dev->data;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		return linear_range_values_count(&boost_ocp_range);
	case NPM2100_SOURCE_LDOSW:
		if (data->ldsw_mode) {
			return linear_range_group_values_count(ldsw_ocp_ranges,
							       ARRAY_SIZE(ldsw_ocp_ranges));
		}
		return linear_range_group_values_count(ldo_ocp_ranges, ARRAY_SIZE(ldo_ocp_ranges));
	default:
		return 0;
	}
}

static int regulator_npm2100_list_currents(const struct device *dev, unsigned int idx,
					   int32_t *current_ua)
{
	const struct regulator_npm2100_config *config = dev->config;
	const struct regulator_npm2100_data *data = dev->data;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		return linear_range_get_value(&boost_ocp_range, idx, current_ua);
	case NPM2100_SOURCE_LDOSW:
		if (data->ldsw_mode) {
			return linear_range_group_get_value(
				ldsw_ocp_ranges, ARRAY_SIZE(ldsw_ocp_ranges), idx, current_ua);
		}
		return linear_range_group_get_value(ldo_ocp_ranges, ARRAY_SIZE(ldo_ocp_ranges), idx,
						    current_ua);
	default:
		return -EINVAL;
	}
}

static int regulator_npm2100_set_current(const struct device *dev, int32_t min_ua, int32_t max_ua)
{
	const struct regulator_npm2100_config *config = dev->config;
	const struct regulator_npm2100_data *data = dev->data;
	uint16_t idx = 0;
	uint8_t shift = 0;
	int ret;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		ret = linear_range_get_win_index(&boost_ocp_range, min_ua, max_ua, &idx);
		if (ret == -EINVAL) {
			return ret;
		}

		if (idx == 1) {
			return i2c_reg_write_byte_dt(&config->i2c, BOOST_CTRLSET, BIT(3));
		}
		return i2c_reg_write_byte_dt(&config->i2c, BOOST_CTRLCLR, BIT(3));

	case NPM2100_SOURCE_LDOSW:
		if (data->ldsw_mode) {
			ret = linear_range_group_get_win_index(
				ldsw_ocp_ranges, ARRAY_SIZE(ldsw_ocp_ranges), min_ua, max_ua, &idx);
			idx = ldsw_ocp_lookup[idx];
			shift = 4U;
		} else {
			ret = linear_range_group_get_win_index(
				ldo_ocp_ranges, ARRAY_SIZE(ldo_ocp_ranges), min_ua, max_ua, &idx);
			idx = ldo_ocp_lookup[idx];
		}

		if (ret == -EINVAL) {
			return ret;
		}

		return i2c_reg_update_byte_dt(&config->i2c, LDOSW_PRGOCP, BIT_MASK(3) << shift,
					      idx << shift);

	default:
		return -ENODEV;
	}
}

static int set_boost_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_npm2100_config *config = dev->config;
	uint8_t reg;

	/* Normal mode */
	switch (mode & NPM2100_REG_OPER_MASK) {
	case NPM2100_REG_OPER_AUTO:
		reg = BOOST_OPER_MODE_AUTO;
		break;
	case NPM2100_REG_OPER_HP:
		reg = BOOST_OPER_MODE_HP;
		break;
	case NPM2100_REG_OPER_LP:
		reg = BOOST_OPER_MODE_LP;
		break;
	case NPM2100_REG_OPER_PASS:
		reg = BOOST_OPER_MODE_PASS;
		break;
	case NPM2100_REG_OPER_NOHP:
		reg = BOOST_OPER_MODE_NOHP;
		break;
	default:
		return -ENOTSUP;
	}

	/* Configure DPS mode */
	if ((mode & NPM2100_REG_DPS_MASK) != 0) {
		uint8_t dps_val = (mode & NPM2100_REG_DPS_MASK) == NPM2100_REG_DPS_ALLOW
					  ? BOOST_OPER_DPS_ALLOW
					  : BOOST_OPER_DPS_ALLOWLP;

		reg |= FIELD_PREP(BOOST_OPER_DPS_MASK, dps_val);
	}

	/* Update mode and dps fields, but not dpstimer */
	int ret = i2c_reg_update_byte_dt(&config->i2c, BOOST_OPER,
					 BOOST_OPER_MODE_MASK | BOOST_OPER_DPS_MASK, reg);
	if (ret < 0) {
		return ret;
	}

	/* Forced mode */
	switch (mode & NPM2100_REG_FORCE_MASK) {
	case 0U:
		return 0;
	case NPM2100_REG_FORCE_HP:
		reg = BOOST_PIN_FORCE_HP;
		break;
	case NPM2100_REG_FORCE_LP:
		reg = BOOST_PIN_FORCE_LP;
		break;
	case NPM2100_REG_FORCE_PASS:
		reg = BOOST_PIN_FORCE_PASS;
		break;
	case NPM2100_REG_FORCE_NOHP:
		reg = BOOST_PIN_FORCE_NOHP;
		break;
	default:
		return -ENOTSUP;
	}

	/* Forced mode is only valid when gpio is configured */
	if (config->mode_gpios.port == NULL) {
		return -EINVAL;
	}

	return i2c_reg_write_byte_dt(&config->i2c, BOOST_PIN, reg);
}

static int get_boost_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_npm2100_config *config = dev->config;
	uint8_t reg;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BOOST_STATUS0, &reg);
	if (ret < 0) {
		return ret;
	}

	switch (reg & BOOST_STATUS0_MODE_MASK) {
	case BOOST_STATUS0_MODE_HP:
		*mode = NPM2100_REG_OPER_HP;
		break;
	case BOOST_STATUS0_MODE_LP:
		*mode = NPM2100_REG_OPER_LP;
		break;
	case BOOST_STATUS0_MODE_ULP:
		*mode = NPM2100_REG_OPER_ULP;
		break;
	case BOOST_STATUS0_MODE_PT:
		*mode = NPM2100_REG_OPER_PASS;
		break;
	case BOOST_STATUS0_MODE_DPS:
		/* STATUS0 indicates whether DPS is enabled, regardless of ALLOW/ALLOWLP setting.
		 * BOOST_OPER_DPS_ALLOW chosen instead of creating new enum value.
		 */
		*mode = BOOST_OPER_DPS_ALLOW;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int get_ldosw_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_npm2100_config *config = dev->config;
	uint8_t reg;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, LDOSW_STATUS, &reg);
	if (ret < 0) {
		return ret;
	}

	*mode = 0U;
	if (reg & LDOSW_STATUS_SW) {
		*mode |= NPM2100_REG_LDSW_EN;
	}

	if (reg & LDOSW_STATUS_HP) {
		*mode |= NPM2100_REG_OPER_HP;
	} else if (reg & LDOSW_STATUS_ULP) {
		*mode |= NPM2100_REG_OPER_ULP;
	}

	return 0;
}

static int set_ldosw_gpio_mode(const struct device *dev, uint8_t inact, uint8_t act, uint8_t ldsw)
{
	const struct regulator_npm2100_config *config = dev->config;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->i2c, LDOSW_GPIO, LDOSW_GPIO_PINACT_MASK, inact | act);
	if (ret < 0) {
		return ret;
	}

	/* Set operating mode to pin control */
	return i2c_reg_write_byte_dt(&config->i2c, LDOSW_SEL, LDOSW_SEL_OPER_PIN | ldsw);
}

static int set_ldosw_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_npm2100_config *config = dev->config;
	struct regulator_npm2100_data *data = dev->data;
	uint8_t ldsw = mode & NPM2100_REG_LDSW_EN;
	uint8_t oper = mode & NPM2100_REG_OPER_MASK;
	uint8_t force = mode & NPM2100_REG_FORCE_MASK;

	/* Save load switch state, needed for OCP configuration */
	data->ldsw_mode = ldsw != 0;

	if (force == 0U) {
		/* SW control of mode */
		switch (oper) {
		case NPM2100_REG_OPER_AUTO:
			return i2c_reg_write_byte_dt(&config->i2c, LDOSW_SEL,
						     LDOSW_SEL_OPER_AUTO | ldsw);
		case NPM2100_REG_OPER_ULP:
			return i2c_reg_write_byte_dt(&config->i2c, LDOSW_SEL,
						     LDOSW_SEL_OPER_ULP | ldsw);
		case NPM2100_REG_OPER_HP:
			return i2c_reg_write_byte_dt(&config->i2c, LDOSW_SEL,
						     LDOSW_SEL_OPER_HP | ldsw);
		default:
			return -ENOTSUP;
		}
	}

	/* Forced mode is only valid when gpio is configured */
	if (config->mode_gpios.port == NULL) {
		return -EINVAL;
	}

	switch (oper | force) {
	case NPM2100_REG_OPER_OFF | NPM2100_REG_FORCE_ULP:
		return set_ldosw_gpio_mode(dev, LDOSW_GPIO_PININACT_OFF, LDOSW_GPIO_PINACT_ULP,
					   ldsw);
	case NPM2100_REG_OPER_OFF | NPM2100_REG_FORCE_HP:
		return set_ldosw_gpio_mode(dev, LDOSW_GPIO_PININACT_OFF, LDOSW_GPIO_PINACT_HP,
					   ldsw);
	case NPM2100_REG_OPER_ULP | NPM2100_REG_FORCE_HP:
		return set_ldosw_gpio_mode(dev, LDOSW_GPIO_PININACT_ULP, LDOSW_GPIO_PINACT_HP,
					   ldsw);
	default:
		return -ENOTSUP;
	}
}

static int regulator_npm2100_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_npm2100_config *config = dev->config;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		return set_boost_mode(dev, mode);
	case NPM2100_SOURCE_LDOSW:
		return set_ldosw_mode(dev, mode);
	default:
		return -ENOTSUP;
	}
}

static int regulator_npm2100_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_npm2100_config *config = dev->config;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		return get_boost_mode(dev, mode);
	case NPM2100_SOURCE_LDOSW:
		return get_ldosw_mode(dev, mode);
	default:
		return -ENOTSUP;
	}
}

static int regulator_npm2100_enable(const struct device *dev)
{
	const struct regulator_npm2100_config *config = dev->config;

	if (config->source != NPM2100_SOURCE_LDOSW) {
		return 0;
	}

	return i2c_reg_write_byte_dt(&config->i2c, LDOSW_ENABLE, 1U);
}

static int regulator_npm2100_disable(const struct device *dev)
{
	const struct regulator_npm2100_config *config = dev->config;

	if (config->source != NPM2100_SOURCE_LDOSW) {
		return 0;
	}

	return i2c_reg_write_byte_dt(&config->i2c, LDOSW_ENABLE, 0U);
}

static int init_pin_ctrl(const struct device *dev, const struct gpio_dt_spec *spec)
{
	const struct regulator_npm2100_config *config = dev->config;

	if (spec->port == NULL) {
		return 0;
	}

	int ret = gpio_pin_configure_dt(spec, GPIO_INPUT);

	if (ret != 0) {
		return ret;
	}

	uint8_t pin = spec->pin << 1U;
	uint8_t offset = ((spec->dt_flags & GPIO_ACTIVE_LOW) != 0U) ? 0U : 1U;

	switch (config->source) {
	case NPM2100_SOURCE_BOOST:
		return i2c_reg_write_byte_dt(&config->i2c, BOOST_GPIO, pin + offset + 1U);
	case NPM2100_SOURCE_LDOSW:
		return i2c_reg_update_byte_dt(&config->i2c, LDOSW_GPIO, LDOSW_GPIO_PIN_MASK,
					      pin + offset);
	default:
		return -ENODEV;
	}
}

static int regulator_npm2100_dvs_state_set(const struct device *dev, regulator_dvs_state_t state)
{
	const struct regulator_npm2100_pconfig *pconfig = dev->config;
	const struct gpio_dt_spec *spec;
	int ret;

	for (size_t idx = 0U; idx < 2U; idx++) {
		spec = &pconfig->dvs_state_pins[idx];

		if (spec->port != NULL) {
			ret = gpio_pin_set_dt(spec, ((state >> idx) & 1U) != 0U);

			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int regulator_npm2100_ship_mode(const struct device *dev)
{
	const struct regulator_npm2100_pconfig *pconfig = dev->config;

	return i2c_reg_write_byte_dt(&pconfig->i2c, SHIP_TASK_SHIP, 1U);
}


static DEVICE_API(regulator_parent, parent_api) = {
	.dvs_state_set = regulator_npm2100_dvs_state_set,
	.ship_mode = regulator_npm2100_ship_mode,
};

static int regulator_npm2100_common_init(const struct device *dev)
{
	const struct regulator_npm2100_pconfig *pconfig = dev->config;
	const struct gpio_dt_spec *spec;
	int ret;

	for (size_t idx = 0U; idx < 2U; idx++) {
		spec = &pconfig->dvs_state_pins[idx];

		if (spec->port != NULL) {
			if (!gpio_is_ready_dt(spec)) {
				return -ENODEV;
			}

			ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int regulator_npm2100_init(const struct device *dev)
{
	const struct regulator_npm2100_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	/* Configure GPIO pin control */
	ret = init_pin_ctrl(dev, &config->mode_gpios);
	if (ret != 0) {
		return ret;
	}

	/* BOOST is always enabled */
	if (config->source == NPM2100_SOURCE_BOOST) {
		ret = i2c_reg_write_byte_dt(
			&config->i2c, BOOST_OPER,
			FIELD_PREP(BOOST_OPER_DPSTIMER_MASK, config->dps_timer));
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, BOOST_LIMIT, config->dps_pulse_limit);
		if (ret < 0) {
			return ret;
		}

		return regulator_common_init(dev, true);
	}

	/* Configure LDOSW behavior during watchdog reset */
	if (config->ldosw_wd_reset) {
		ret = i2c_reg_write_byte_dt(&config->i2c, RESET_ALTCONFIG,
					    RESET_ALTCONFIG_LDOSW_OFF);
		if (ret < 0) {
			return ret;
		}
	}

	/* Get enable state for LDOSW */
	uint8_t enabled;

	ret = i2c_reg_read_byte_dt(&config->i2c, LDOSW_ENABLE, &enabled);
	if (ret < 0) {
		return ret;
	}

	return regulator_common_init(dev, enabled != 0U);
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_npm2100_enable,
	.disable = regulator_npm2100_disable,
	.count_voltages = regulator_npm2100_count_voltages,
	.list_voltage = regulator_npm2100_list_voltage,
	.set_voltage = regulator_npm2100_set_voltage,
	.get_voltage = regulator_npm2100_get_voltage,
	.count_current_limits = regulator_npm2100_count_currents,
	.list_current_limit = regulator_npm2100_list_currents,
	.set_current_limit = regulator_npm2100_set_current,
	.set_mode = regulator_npm2100_set_mode,
	.get_mode = regulator_npm2100_get_mode};

#define REGULATOR_NPM2100_DEFINE(node_id, id, _source)                                             \
	static struct regulator_npm2100_data data_##id;                                            \
                                                                                                   \
	static const struct regulator_npm2100_config config_##id = {                               \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.i2c = I2C_DT_SPEC_GET(DT_GPARENT(node_id)),                                       \
		.source = _source,                                                                 \
		.mode_gpios = GPIO_DT_SPEC_GET_OR(node_id, mode_gpios, {0}),                       \
		.ldosw_wd_reset = DT_PROP(node_id, ldosw_wd_reset),                                \
		.dps_timer = DT_ENUM_IDX_OR(node_id, dps_timer_us, 0),                             \
		.dps_pulse_limit = DT_PROP_OR(node_id, dps_pulse_limit, 0)};                       \
	BUILD_ASSERT(DT_PROP_OR(node_id, dps_pulse_limit, 0) >= 3 ||                               \
			     DT_PROP_OR(node_id, dps_pulse_limit, 0) == 0,                         \
		     "Invalid dps_pulse_limit value");                                             \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, regulator_npm2100_init, NULL, &data_##id, &config_##id,          \
			 POST_KERNEL, CONFIG_REGULATOR_NPM2100_INIT_PRIORITY, &api);

#define REGULATOR_NPM2100_DEFINE_COND(inst, child, source)                                         \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_NPM2100_DEFINE(DT_INST_CHILD(inst, child), child##inst, source)),   \
		    ())

#define REGULATOR_NPM2100_DEFINE_ALL(inst)                                                         \
	static const struct regulator_npm2100_pconfig config_##inst = {                            \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.dvs_state_pins = {GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, dvs_gpios, 0, {0}),       \
				   GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, dvs_gpios, 1, {0})}};     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_npm2100_common_init, NULL, NULL, &config_##inst,     \
			      POST_KERNEL, CONFIG_REGULATOR_NPM2100_COMMON_INIT_PRIORITY,          \
			      &parent_api);                                                        \
                                                                                                   \
	REGULATOR_NPM2100_DEFINE_COND(inst, boost, NPM2100_SOURCE_BOOST)                           \
	REGULATOR_NPM2100_DEFINE_COND(inst, ldosw, NPM2100_SOURCE_LDOSW)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_NPM2100_DEFINE_ALL)
