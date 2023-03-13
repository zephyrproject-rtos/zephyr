/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm1300_regulator

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/npm1300.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

/* nPM1300 voltage sources */
enum npm1300_sources {
	NPM1300_SOURCE_BUCK1,
	NPM1300_SOURCE_BUCK2,
	NPM1300_SOURCE_LDO1,
	NPM1300_SOURCE_LDO2,
};

/* nPM1300 gpio control channels */
enum npm1300_gpio_type {
	NPM1300_GPIO_TYPE_ENABLE,
	NPM1300_GPIO_TYPE_RETENTION,
	NPM1300_GPIO_TYPE_PWM
};

/* nPM1300 regulator base addresses */
#define BUCK_BASE 0x04U
#define LDSW_BASE 0x08U

/* nPM1300 regulator register offsets */
#define BUCK_OFFSET_EN_SET    0x00U
#define BUCK_OFFSET_EN_CLR    0x01U
#define BUCK_OFFSET_PWM_SET   0x04U
#define BUCK_OFFSET_PWM_CLR   0x05U
#define BUCK_OFFSET_VOUT_NORM 0x08U
#define BUCK_OFFSET_VOUT_RET  0x09U
#define BUCK_OFFSET_EN_CTRL   0x0CU
#define BUCK_OFFSET_VRET_CTRL 0x0DU
#define BUCK_OFFSET_PWM_CTRL  0x0EU
#define BUCK_OFFSET_SW_CTRL   0x0FU
#define BUCK_OFFSET_VOUT_STAT 0x10U
#define BUCK_OFFSET_CTRL0     0x15U

/* nPM1300 ldsw register offsets */
#define LDSW_OFFSET_EN_SET  0x00U
#define LDSW_OFFSET_EN_CLR  0x01U
#define LDSW_OFFSET_STATUS  0x04U
#define LDSW_OFFSET_GPISEL  0x05U
#define LDSW_OFFSET_CONFIG  0x07U
#define LDSW_OFFSET_LDOSEL  0x08U
#define LDSW_OFFSET_VOUTSEL 0x0CU

struct regulator_npm1300_pconfig {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec dvs_state_pins[5];
	uint8_t pad_val;
};

struct regulator_npm1300_config {
	struct regulator_common_config common;
	const struct device *p;
	uint8_t source;
	int32_t retention_uv;
	struct gpio_dt_spec enable_gpios;
	struct gpio_dt_spec retention_gpios;
	struct gpio_dt_spec pwm_gpios;
};

struct regulator_npm1300_data {
	struct regulator_common_data data;
};

/* Linear range for output voltage, common for all bucks and LDOs on this device */
static const struct linear_range buckldo_range = LINEAR_RANGE_INIT(1000000, 100000, 0U, 23U);

/* Read multiple registers from specified address */
static int reg_read_burst(const struct device *dev, uint8_t base, uint8_t offset, uint8_t *data,
			  size_t len)
{
	const struct regulator_npm1300_config *config = dev->config;
	const struct regulator_npm1300_pconfig *pconfig = config->p->config;
	uint8_t buff[] = {base, offset};

	return i2c_write_read_dt(&pconfig->i2c, buff, sizeof(buff), data, len);
}

static int reg_read(const struct device *dev, uint8_t base, uint8_t offset, uint8_t *data)
{
	return reg_read_burst(dev, base, offset, data, 1U);
}

/* Write single register to specified address */
static int reg_write(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data)
{
	const struct regulator_npm1300_config *config = dev->config;
	const struct regulator_npm1300_pconfig *pconfig = config->p->config;
	uint8_t buff[] = {base, offset, data};

	return i2c_write_dt(&pconfig->i2c, buff, sizeof(buff));
}

/* Modify bits in single register */
static int reg_update(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data,
		      uint8_t mask)
{
	uint8_t reg;
	int ret;

	ret = reg_read(dev, base, offset, &reg);

	if (ret < 0) {
		return ret;
	}

	reg = (reg & ~mask) | (data & mask);

	return reg_write(dev, base, offset, reg);
}

unsigned int regulator_npm1300_count_voltages(const struct device *dev)
{
	const struct regulator_npm1300_config *config = dev->config;

	switch (config->source) {
	case NPM1300_SOURCE_BUCK1:
	case NPM1300_SOURCE_BUCK2:
	case NPM1300_SOURCE_LDO1:
	case NPM1300_SOURCE_LDO2:
		return linear_range_values_count(&buckldo_range);
	default:
		return 0;
	}
}

int regulator_npm1300_list_voltage(const struct device *dev, unsigned int idx, int32_t *volt_uv)
{
	const struct regulator_npm1300_config *config = dev->config;

	switch (config->source) {
	case NPM1300_SOURCE_BUCK1:
	case NPM1300_SOURCE_BUCK2:
	case NPM1300_SOURCE_LDO1:
	case NPM1300_SOURCE_LDO2:
		return linear_range_get_value(&buckldo_range, idx, volt_uv);
	default:
		return -EINVAL;
	}
}

static int retention_set_voltage(const struct device *dev, int32_t retention_uv)
{
	const struct regulator_npm1300_config *config = dev->config;
	uint16_t idx;
	uint8_t chan;
	int ret;

	switch (config->source) {
	case NPM1300_SOURCE_BUCK1:
		chan = 0U;
		break;
	case NPM1300_SOURCE_BUCK2:
		chan = 1U;
		break;
	default:
		return -ENOTSUP;
	}

	ret = linear_range_get_win_index(&buckldo_range, retention_uv, retention_uv, &idx);

	if (ret == -EINVAL) {
		return ret;
	}

	return reg_write(dev, BUCK_BASE, BUCK_OFFSET_VOUT_RET + (chan * 2U), idx);
}

static int buck_set_voltage(const struct device *dev, uint8_t chan, int32_t min_uv, int32_t max_uv)
{
	uint8_t mask;
	uint16_t idx;
	int ret;

	ret = linear_range_get_win_index(&buckldo_range, min_uv, max_uv, &idx);

	if (ret == -EINVAL) {
		return ret;
	}

	ret = reg_write(dev, BUCK_BASE, BUCK_OFFSET_VOUT_NORM + (chan * 2U), idx);

	if (ret < 0) {
		return ret;
	}

	/* Enable SW control of buck output */
	mask = BIT(chan);
	return reg_update(dev, BUCK_BASE, BUCK_OFFSET_SW_CTRL, mask, mask);
}

static int ldo_set_voltage(const struct device *dev, uint8_t chan, int32_t min_uv, int32_t max_uv)
{
	uint16_t idx;
	int ret;

	ret = linear_range_get_win_index(&buckldo_range, min_uv, max_uv, &idx);

	if (ret == -EINVAL) {
		return ret;
	}

	return reg_write(dev, LDSW_BASE, LDSW_OFFSET_VOUTSEL + chan, idx);
}

int regulator_npm1300_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_npm1300_config *config = dev->config;

	switch (config->source) {
	case NPM1300_SOURCE_BUCK1:
		return buck_set_voltage(dev, 0, min_uv, max_uv);
	case NPM1300_SOURCE_BUCK2:
		return buck_set_voltage(dev, 1, min_uv, max_uv);
	case NPM1300_SOURCE_LDO1:
		return ldo_set_voltage(dev, 0, min_uv, max_uv);
	case NPM1300_SOURCE_LDO2:
		return ldo_set_voltage(dev, 1, min_uv, max_uv);
	default:
		return -ENODEV;
	}
}

static int buck_get_voltage(const struct device *dev, uint8_t chan, int32_t *volt_uv)
{
	uint8_t sel;
	uint8_t idx;
	int ret;

	ret = reg_read(dev, BUCK_BASE, BUCK_OFFSET_SW_CTRL, &sel);

	if (ret < 0) {
		return ret;
	}

	if ((sel >> chan) & 1U) {
		/* SW control */
		ret = reg_read(dev, BUCK_BASE, BUCK_OFFSET_VOUT_NORM + (chan * 2U), &idx);
	} else {
		/* VSET pin control */
		ret = reg_read(dev, BUCK_BASE, BUCK_OFFSET_VOUT_STAT + chan, &idx);
	}

	if (ret < 0) {
		return ret;
	}

	return linear_range_get_value(&buckldo_range, idx, volt_uv);
}

static int ldo_get_voltage(const struct device *dev, uint8_t chan, int32_t *volt_uv)
{
	uint8_t idx;
	int ret;

	ret = reg_read(dev, LDSW_BASE, LDSW_OFFSET_VOUTSEL + chan, &idx);

	if (ret < 0) {
		return ret;
	}

	return linear_range_get_value(&buckldo_range, idx, volt_uv);
}

int regulator_npm1300_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_npm1300_config *config = dev->config;

	switch (config->source) {
	case NPM1300_SOURCE_BUCK1:
		return buck_get_voltage(dev, 0, volt_uv);
	case NPM1300_SOURCE_BUCK2:
		return buck_get_voltage(dev, 1, volt_uv);
	case NPM1300_SOURCE_LDO1:
		return ldo_get_voltage(dev, 0, volt_uv);
	case NPM1300_SOURCE_LDO2:
		return ldo_get_voltage(dev, 1, volt_uv);
	default:
		return -ENODEV;
	}
}

static int set_buck_mode(const struct device *dev, uint8_t chan, regulator_mode_t mode)
{
	switch (mode) {
	case NPM1300_BUCK_MODE_PWM:
		return reg_write(dev, BUCK_BASE, BUCK_OFFSET_PWM_SET + (chan * 2U), 1U);
	case NPM1300_BUCK_MODE_AUTO:
		return reg_write(dev, BUCK_BASE, BUCK_OFFSET_PWM_CLR + (chan * 2U), 1U);
	default:
		return -ENOTSUP;
	}
}

static int set_ldsw_mode(const struct device *dev, uint8_t chan, regulator_mode_t mode)
{
	switch (mode) {
	case NPM1300_LDSW_MODE_LDO:
		return reg_write(dev, LDSW_BASE, LDSW_OFFSET_LDOSEL + chan, 1U);
	case NPM1300_LDSW_MODE_LDSW:
		return reg_write(dev, LDSW_BASE, LDSW_OFFSET_LDOSEL + chan, 0U);
	default:
		return -ENOTSUP;
	}
}

int regulator_npm1300_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_npm1300_config *config = dev->config;

	switch (config->source) {
	case NPM1300_SOURCE_BUCK1:
		return set_buck_mode(dev, 0, mode);
	case NPM1300_SOURCE_BUCK2:
		return set_buck_mode(dev, 1, mode);
	case NPM1300_SOURCE_LDO1:
		return set_ldsw_mode(dev, 0, mode);
	case NPM1300_SOURCE_LDO2:
		return set_ldsw_mode(dev, 1, mode);
	default:
		return -ENOTSUP;
	}
}

int regulator_npm1300_enable(const struct device *dev)
{
	const struct regulator_npm1300_config *config = dev->config;

	switch (config->source) {
	case NPM1300_SOURCE_BUCK1:
		return reg_write(dev, BUCK_BASE, BUCK_OFFSET_EN_SET, 1U);
	case NPM1300_SOURCE_BUCK2:
		return reg_write(dev, BUCK_BASE, BUCK_OFFSET_EN_SET + 2U, 1U);
	case NPM1300_SOURCE_LDO1:
		return reg_write(dev, LDSW_BASE, LDSW_OFFSET_EN_SET, 1U);
	case NPM1300_SOURCE_LDO2:
		return reg_write(dev, LDSW_BASE, LDSW_OFFSET_EN_SET + 2U, 1U);
	default:
		return 0;
	}
}

int regulator_npm1300_disable(const struct device *dev)
{
	const struct regulator_npm1300_config *config = dev->config;

	switch (config->source) {
	case NPM1300_SOURCE_BUCK1:
		return reg_write(dev, BUCK_BASE, BUCK_OFFSET_EN_CLR, 1U);
	case NPM1300_SOURCE_BUCK2:
		return reg_write(dev, BUCK_BASE, BUCK_OFFSET_EN_CLR + 2U, 1U);
	case NPM1300_SOURCE_LDO1:
		return reg_write(dev, LDSW_BASE, LDSW_OFFSET_EN_CLR, 1U);
	case NPM1300_SOURCE_LDO2:
		return reg_write(dev, LDSW_BASE, LDSW_OFFSET_EN_CLR + 2U, 1U);
	default:
		return 0;
	}
}

static int regulator_npm1300_set_buck_pin_ctrl(const struct device *dev, uint8_t chan, uint8_t pin,
					       uint8_t inv, enum npm1300_gpio_type type)
{
	uint8_t ctrl;
	uint8_t mask;

	switch (chan) {
	case 0:
		/* Invert control in bit 6, pin control in bits 2-0 */
		ctrl = (inv << 6U) | (pin + 1U);
		mask = BIT(6U) | BIT_MASK(3U);
		break;
	case 1:
		/* Invert control in bit 7, pin control in bits 5-3 */
		ctrl = (inv << 7U) | ((pin + 1U) << 3U);
		mask = BIT(7U) | (BIT_MASK(3U) << 3U);
		break;
	default:
		return -EINVAL;
	}

	switch (type) {
	case NPM1300_GPIO_TYPE_ENABLE:
		return reg_update(dev, BUCK_BASE, BUCK_OFFSET_EN_CTRL, ctrl, mask);
	case NPM1300_GPIO_TYPE_PWM:
		return reg_update(dev, BUCK_BASE, BUCK_OFFSET_PWM_CTRL, ctrl, mask);
	case NPM1300_GPIO_TYPE_RETENTION:
		return reg_update(dev, BUCK_BASE, BUCK_OFFSET_VRET_CTRL, ctrl, mask);
	default:
		return -ENOTSUP;
	}
}

static int regulator_npm1300_set_ldsw_pin_ctrl(const struct device *dev, uint8_t chan, uint8_t pin,
					       uint8_t inv, enum npm1300_gpio_type type)
{
	uint8_t ctrl;

	if (type != NPM1300_GPIO_TYPE_ENABLE) {
		return -ENOTSUP;
	}

	ctrl = (pin + 1U) | (inv << 3U);

	return reg_write(dev, LDSW_BASE, LDSW_OFFSET_GPISEL + chan, type);
}

int regulator_npm1300_set_pin_ctrl(const struct device *dev, const struct gpio_dt_spec *spec,
				   enum npm1300_gpio_type type)
{
	const struct regulator_npm1300_config *config = dev->config;
	uint8_t inv;

	if (spec->port == NULL) {
		return 0;
	}

	inv = (spec->dt_flags & GPIO_ACTIVE_LOW) != 0U;

	switch (config->source) {
	case NPM1300_SOURCE_BUCK1:
		return regulator_npm1300_set_buck_pin_ctrl(dev, 0, spec->pin, inv, type);
	case NPM1300_SOURCE_BUCK2:
		return regulator_npm1300_set_buck_pin_ctrl(dev, 1, spec->pin, inv, type);
	case NPM1300_SOURCE_LDO1:
		return regulator_npm1300_set_ldsw_pin_ctrl(dev, 0, spec->pin, inv, type);
	case NPM1300_SOURCE_LDO2:
		return regulator_npm1300_set_ldsw_pin_ctrl(dev, 1, spec->pin, inv, type);
	default:
		return -ENODEV;
	}
}

int regulator_npm1300_dvs_state_set(const struct device *dev, regulator_dvs_state_t state)
{
	const struct regulator_npm1300_pconfig *pconfig = dev->config;
	const struct gpio_dt_spec *spec;
	int ret;

	for (size_t idx = 0U; idx < 5U; idx++) {
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

static const struct regulator_parent_driver_api parent_api = {
	.dvs_state_set = regulator_npm1300_dvs_state_set,
};

int regulator_npm1300_common_init(const struct device *dev)
{
	const struct regulator_npm1300_pconfig *pconfig = dev->config;
	const struct gpio_dt_spec *spec;
	int ret;

	for (size_t idx = 0U; idx < 5U; idx++) {
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

int regulator_npm1300_init(const struct device *dev)
{
	const struct regulator_npm1300_config *config = dev->config;
	int ret = 0;

	if (!device_is_ready(config->p)) {
		return -ENODEV;
	}

	ret = regulator_common_init(dev, false);
	if (ret < 0) {
		return ret;
	}

	/* Configure retention voltage */
	if (config->retention_uv != 0) {
		ret = retention_set_voltage(dev, config->retention_uv);
		if (ret != 0) {
			return ret;
		}
	}

	/* Configure GPIO pin control */
	ret = regulator_npm1300_set_pin_ctrl(dev, &config->enable_gpios, NPM1300_GPIO_TYPE_ENABLE);
	if (ret != 0) {
		return ret;
	}

	ret = regulator_npm1300_set_pin_ctrl(dev, &config->retention_gpios,
					     NPM1300_GPIO_TYPE_RETENTION);
	if (ret != 0) {
		return ret;
	}

	ret = regulator_npm1300_set_pin_ctrl(dev, &config->pwm_gpios, NPM1300_GPIO_TYPE_PWM);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

static const struct regulator_driver_api api = {.enable = regulator_npm1300_enable,
						.disable = regulator_npm1300_disable,
						.count_voltages = regulator_npm1300_count_voltages,
						.list_voltage = regulator_npm1300_list_voltage,
						.set_voltage = regulator_npm1300_set_voltage,
						.get_voltage = regulator_npm1300_get_voltage,
						.set_mode = regulator_npm1300_set_mode};

#define REGULATOR_NPM1300_DEFINE(node_id, id, _source, parent)                                     \
	static struct regulator_npm1300_data data_##id;                                            \
                                                                                                   \
	static const struct regulator_npm1300_config config_##id = {                               \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.p = parent,                                                                       \
		.source = _source,                                                                 \
		.retention_uv = DT_PROP_OR(node_id, retention_microvolt, 0),                       \
		.enable_gpios = GPIO_DT_SPEC_GET_OR(node_id, enable_gpios, {0}),                   \
		.retention_gpios = GPIO_DT_SPEC_GET_OR(node_id, retention_gpios, {0}),             \
		.pwm_gpios = GPIO_DT_SPEC_GET_OR(node_id, pwm_gpios, {0})};                        \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, regulator_npm1300_init, NULL, &data_##id, &config_##id,          \
			 POST_KERNEL, CONFIG_REGULATOR_NPM1300_INIT_PRIORITY, &api);

#define REGULATOR_NPM1300_DEFINE_COND(inst, child, source, parent)                                 \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_NPM1300_DEFINE(DT_INST_CHILD(inst, child), child##inst, source,     \
					      parent)),                                            \
		    ())

#define REGULATOR_NPM1300_DEFINE_ALL(inst)                                                         \
	static const struct regulator_npm1300_pconfig config_##inst = {                            \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.dvs_state_pins = {GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, dvs_gpios, 0, {0}),       \
				   GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, dvs_gpios, 1, {0}),       \
				   GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, dvs_gpios, 2, {0}),       \
				   GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, dvs_gpios, 3, {0}),       \
				   GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, dvs_gpios, 4, {0})}};     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_npm1300_common_init, NULL, NULL, &config_##inst,     \
			      POST_KERNEL, CONFIG_REGULATOR_NPM1300_COMMON_INIT_PRIORITY,          \
			      &parent_api);                                                        \
                                                                                                   \
	REGULATOR_NPM1300_DEFINE_COND(inst, buck1, NPM1300_SOURCE_BUCK1, DEVICE_DT_INST_GET(inst)) \
	REGULATOR_NPM1300_DEFINE_COND(inst, buck2, NPM1300_SOURCE_BUCK2, DEVICE_DT_INST_GET(inst)) \
	REGULATOR_NPM1300_DEFINE_COND(inst, ldo1, NPM1300_SOURCE_LDO1, DEVICE_DT_INST_GET(inst))   \
	REGULATOR_NPM1300_DEFINE_COND(inst, ldo2, NPM1300_SOURCE_LDO1, DEVICE_DT_INST_GET(inst))

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_NPM1300_DEFINE_ALL)
