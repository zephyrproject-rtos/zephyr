/*
 * Copyright (c) 2021 NXP
 * Copyright (c) 2023 Martin Kiepfer <mrmarteng@teleschirm.org>
 * Copyright (c) 2024 Lothar Felten <lothar.felten@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/regulator/axp192.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_instance.h>
#include <zephyr/drivers/mfd/axp192.h>

LOG_MODULE_REGISTER(regulator_axp192, CONFIG_REGULATOR_LOG_LEVEL);

/* AXP192 register defines */
#define AXP192_REG_EXTEN_DCDC2_CONTROL   0x10U
#define AXP192_REG_DCDC123_LDO23_CONTROL 0x12U
#define AXP192_REG_DCDC2_VOLTAGE         0x23U
#define AXP192_REG_DCDC2_SLOPE           0x25U
#define AXP192_REG_DCDC1_VOLTAGE         0x26U
#define AXP192_REG_DCDC3_VOLTAGE         0x27U
#define AXP192_REG_LDO23_VOLTAGE         0x28U
#define AXP192_REG_DCDC123_WORKMODE      0x80U
#define AXP192_REG_GPIO0_CONTROL         0x90U
#define AXP192_REG_LDOIO0_VOLTAGE        0x91U

/* AXP2101 register defines */
#define AXP2101_REG_CHGLED            0x69U
#define AXP2101_REG_DCDC12345_CONTROL 0x80U
#define AXP2101_REG_DCDCS_PWM_CONTROL 0x81U
#define AXP2101_REG_DCDC1_VOLTAGE     0x82U
#define AXP2101_REG_DCDC2_VOLTAGE     0x83U
#define AXP2101_REG_DCDC3_VOLTAGE     0x84U
#define AXP2101_REG_DCDC4_VOLTAGE     0x85U
#define AXP2101_REG_DCDC5_VOLTAGE     0x86U
#define AXP2101_REG_LDOGRP1_CONTROL   0x90U
#define AXP2101_REG_LDOGRP2_CONTROL   0x91U
#define AXP2101_REG_ALDO1_VOLTAGE     0x92U
#define AXP2101_REG_ALDO2_VOLTAGE     0x93U
#define AXP2101_REG_ALDO3_VOLTAGE     0x94U
#define AXP2101_REG_ALDO4_VOLTAGE     0x95U
#define AXP2101_REG_BLDO1_VOLTAGE     0x96U
#define AXP2101_REG_BLDO2_VOLTAGE     0x97U
#define AXP2101_REG_CPUSLDO_VOLTAGE   0x98U
#define AXP2101_REG_DLDO1_VOLTAGE     0x99U
#define AXP2101_REG_DLDO2_VOLTAGE     0x9AU

#define AXP2101_CHGLED_CTRL_MASK   0x3
#define AXP2101_CHGLED_CTRL_OFFSET 1

#define AXP2101_VBUS_CFG_REG                0
#define AXP2101_VBUS_CFG_VAL_VBUSEN_DISABLE 0

#define AXP_NODE_HAS_CHILD(node, child) DT_NODE_HAS_STATUS_OKAY(DT_CHILD(node, child)) |
#define AXP192_ANY_HAS_CHILD(child)                                                                \
	(DT_FOREACH_STATUS_OKAY_VARGS(x_powers_axp192_regulator, AXP_NODE_HAS_CHILD, child) 0)
#define AXP2101_ANY_HAS_CHILD(child)                                                               \
	(DT_FOREACH_STATUS_OKAY_VARGS(x_powers_axp2101_regulator, AXP_NODE_HAS_CHILD, child) 0)

struct regulator_axp192_desc {
	const uint8_t enable_reg;
	const uint8_t enable_mask;
	const uint8_t enable_val;
	const uint8_t vsel_reg;
	const uint8_t vsel_mask;
	const uint8_t vsel_bitpos;
	const int32_t max_ua;
	const uint8_t workmode_reg;
	const uint8_t workmode_mask;
	const uint8_t workmode_pwm_val;
	const uint8_t num_ranges;
	const struct linear_range *ranges;
};

struct regulator_axp192_data {
	struct regulator_common_data data;
};

struct regulator_axp192_config {
	struct regulator_common_config common;
	const struct regulator_axp192_desc *desc;
	const struct device *mfd;
	const struct i2c_dt_spec i2c;
};

static const struct linear_range axp192_dcdc1_ranges[] = {
	LINEAR_RANGE_INIT(700000U, 25000U, 0x00U, 0x7FU),
};

__maybe_unused static const struct regulator_axp192_desc axp192_dcdc1_desc = {
	.enable_reg = AXP192_REG_DCDC123_LDO23_CONTROL,
	.enable_mask = 0x01U,
	.enable_val = 0x01U,
	.vsel_reg = AXP192_REG_DCDC1_VOLTAGE,
	.vsel_mask = 0x7FU,
	.vsel_bitpos = 0U,
	.max_ua = 1200000U,
	.workmode_reg = AXP192_REG_DCDC123_WORKMODE,
	.workmode_mask = 0x08U,
	.workmode_pwm_val = 0x08U,
	.ranges = axp192_dcdc1_ranges,
	.num_ranges = ARRAY_SIZE(axp192_dcdc1_ranges),
};

static const struct linear_range axp192_dcdc2_ranges[] = {
	LINEAR_RANGE_INIT(700000U, 25000U, 0x00U, 0x3FU),
};

__maybe_unused static const struct regulator_axp192_desc axp192_dcdc2_desc = {
	.enable_reg = AXP192_REG_EXTEN_DCDC2_CONTROL,
	.enable_mask = 0x01U,
	.enable_val = 0x01U,
	.vsel_reg = AXP192_REG_DCDC2_VOLTAGE,
	.vsel_mask = 0x3FU,
	.vsel_bitpos = 0U,
	.max_ua = 1600000U,
	.workmode_reg = AXP192_REG_DCDC123_WORKMODE,
	.workmode_mask = 0x04U,
	.workmode_pwm_val = 0x04U,
	.ranges = axp192_dcdc2_ranges,
	.num_ranges = ARRAY_SIZE(axp192_dcdc2_ranges),
};

static const struct linear_range axp192_dcdc3_ranges[] = {
	LINEAR_RANGE_INIT(700000U, 25000U, 0x00U, 0x7FU),
};

__maybe_unused static const struct regulator_axp192_desc axp192_dcdc3_desc = {
	.enable_reg = AXP192_REG_DCDC123_LDO23_CONTROL,
	.enable_mask = 0x02U,
	.enable_val = 0x02U,
	.vsel_reg = AXP192_REG_DCDC3_VOLTAGE,
	.vsel_mask = 0x7FU,
	.vsel_bitpos = 0U,
	.max_ua = 700000U,
	.workmode_reg = AXP192_REG_DCDC123_WORKMODE,
	.workmode_mask = 0x02U,
	.workmode_pwm_val = 0x02U,
	.ranges = axp192_dcdc3_ranges,
	.num_ranges = ARRAY_SIZE(axp192_dcdc3_ranges),
};

static const struct linear_range axp192_ldoio0_ranges[] = {
	LINEAR_RANGE_INIT(1800000u, 100000u, 0x00u, 0x0Fu),
};

__maybe_unused static const struct regulator_axp192_desc axp192_ldoio0_desc = {
	.enable_reg = AXP192_REG_GPIO0_CONTROL,
	.enable_mask = 0x07u,
	.enable_val = 0x03u,
	.vsel_reg = AXP192_REG_LDOIO0_VOLTAGE,
	.vsel_mask = 0xF0u,
	.vsel_bitpos = 4u,
	.max_ua = 50000u,
	.workmode_reg = 0u,
	.workmode_mask = 0u,
	.ranges = axp192_ldoio0_ranges,
	.num_ranges = ARRAY_SIZE(axp192_ldoio0_ranges),
};

static const struct linear_range axp192_ldo2_ranges[] = {
	LINEAR_RANGE_INIT(1800000U, 100000U, 0x00U, 0x0FU),
};

__maybe_unused static const struct regulator_axp192_desc axp192_ldo2_desc = {
	.enable_reg = AXP192_REG_DCDC123_LDO23_CONTROL,
	.enable_mask = 0x04U,
	.enable_val = 0x04U,
	.vsel_reg = AXP192_REG_LDO23_VOLTAGE,
	.vsel_mask = 0xF0U,
	.vsel_bitpos = 4U,
	.max_ua = 200000U,
	.workmode_reg = 0U,
	.workmode_mask = 0U,
	.ranges = axp192_ldo2_ranges,
	.num_ranges = ARRAY_SIZE(axp192_ldo2_ranges),
};

static const struct linear_range axp192_ldo3_ranges[] = {
	LINEAR_RANGE_INIT(1800000U, 100000U, 0x00U, 0x0FU),
};

__maybe_unused static const struct regulator_axp192_desc axp192_ldo3_desc = {
	.enable_reg = AXP192_REG_DCDC123_LDO23_CONTROL,
	.enable_mask = 0x08U,
	.enable_val = 0x08U,
	.vsel_reg = AXP192_REG_LDO23_VOLTAGE,
	.vsel_mask = 0x0FU,
	.vsel_bitpos = 0U,
	.max_ua = 200000U,
	.workmode_reg = 0U,
	.workmode_mask = 0U,
	.ranges = axp192_ldo3_ranges,
	.num_ranges = ARRAY_SIZE(axp192_ldo3_ranges),
};

static const struct linear_range axp2101_dcdc1_ranges[] = {
	LINEAR_RANGE_INIT(1500000U, 100000U, 0U, 19U),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_dcdc1_desc = {
	.enable_reg = AXP2101_REG_DCDC12345_CONTROL,
	.enable_mask = 0x01U,
	.enable_val = 0x01U,
	.vsel_reg = AXP2101_REG_DCDC1_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 2000000U,
	.ranges = axp2101_dcdc1_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_dcdc1_ranges),
	.workmode_reg = AXP2101_REG_DCDCS_PWM_CONTROL,
	.workmode_mask = BIT(2),
	.workmode_pwm_val = BIT(2),
};

static const struct linear_range axp2101_dcdc2_ranges[] = {
	LINEAR_RANGE_INIT(500000U, 10000U, 0U, 70U),
	LINEAR_RANGE_INIT(1220000U, 20000U, 71U, 87U),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_dcdc2_desc = {
	.enable_reg = AXP2101_REG_DCDC12345_CONTROL,
	.enable_mask = 0x02U,
	.enable_val = 0x02U,
	.vsel_reg = AXP2101_REG_DCDC2_VOLTAGE,
	.vsel_mask = 0x7FU,
	.vsel_bitpos = 0U,
	.max_ua = 2000000U,
	.ranges = axp2101_dcdc2_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_dcdc2_ranges),
	.workmode_reg = AXP2101_REG_DCDCS_PWM_CONTROL,
	.workmode_mask = BIT(3),
	.workmode_pwm_val = BIT(3),
};

static const struct linear_range axp2101_dcdc3_ranges[] = {
	LINEAR_RANGE_INIT(500000U, 10000U, 0U, 70U),
	LINEAR_RANGE_INIT(1220000U, 20000U, 71U, 87U),
	LINEAR_RANGE_INIT(1600000U, 100000U, 88U, 106U),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_dcdc3_desc = {
	.enable_reg = AXP2101_REG_DCDC12345_CONTROL,
	.enable_mask = 0x04U,
	.enable_val = 0x04U,
	.vsel_reg = AXP2101_REG_DCDC3_VOLTAGE,
	.vsel_mask = 0x7FU,
	.vsel_bitpos = 0U,
	.max_ua = 2000000U,
	.ranges = axp2101_dcdc3_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_dcdc3_ranges),
	.workmode_reg = AXP2101_REG_DCDCS_PWM_CONTROL,
	.workmode_mask = BIT(4),
	.workmode_pwm_val = BIT(4),
};

static const struct linear_range axp2101_dcdc4_ranges[] = {
	LINEAR_RANGE_INIT(500000U, 10000U, 0U, 70U),
	LINEAR_RANGE_INIT(1220000U, 20000U, 71U, 102U),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_dcdc4_desc = {
	.enable_reg = AXP2101_REG_DCDC12345_CONTROL,
	.enable_mask = 0x08U,
	.enable_val = 0x08U,
	.vsel_reg = AXP2101_REG_DCDC4_VOLTAGE,
	.vsel_mask = 0x7FU,
	.vsel_bitpos = 0U,
	.max_ua = 1500000U,
	.ranges = axp2101_dcdc4_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_dcdc4_ranges),
	.workmode_reg = AXP2101_REG_DCDCS_PWM_CONTROL,
	.workmode_mask = BIT(5),
	.workmode_pwm_val = BIT(5),
};

static const struct linear_range axp2101_dcdc5_ranges[] = {
	LINEAR_RANGE_INIT(1400000U, 100000U, 0U, 23U),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_dcdc5_desc = {
	.enable_reg = AXP2101_REG_DCDC12345_CONTROL,
	.enable_mask = 0x10U,
	.enable_val = 0x10U,
	.vsel_reg = AXP2101_REG_DCDC5_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 1000000U,
	.ranges = axp2101_dcdc5_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_dcdc5_ranges),
};

static const struct linear_range axp2101_abldox_ranges[] = {
	LINEAR_RANGE_INIT(500000U, 100000U, 0U, 30U),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_aldo1_desc = {
	.enable_reg = AXP2101_REG_LDOGRP1_CONTROL,
	.enable_mask = 0x01U,
	.enable_val = 0x01U,
	.vsel_reg = AXP2101_REG_ALDO1_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 300000U,
	.ranges = axp2101_abldox_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_abldox_ranges),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_aldo2_desc = {
	.enable_reg = AXP2101_REG_LDOGRP1_CONTROL,
	.enable_mask = 0x02U,
	.enable_val = 0x02U,
	.vsel_reg = AXP2101_REG_ALDO2_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 300000U,
	.ranges = axp2101_abldox_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_abldox_ranges),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_aldo3_desc = {
	.enable_reg = AXP2101_REG_LDOGRP1_CONTROL,
	.enable_mask = 0x04U,
	.enable_val = 0x04U,
	.vsel_reg = AXP2101_REG_ALDO3_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 300000U,
	.ranges = axp2101_abldox_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_abldox_ranges),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_aldo4_desc = {
	.enable_reg = AXP2101_REG_LDOGRP1_CONTROL,
	.enable_mask = 0x08U,
	.enable_val = 0x08U,
	.vsel_reg = AXP2101_REG_ALDO4_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 300000U,
	.ranges = axp2101_abldox_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_abldox_ranges),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_bldo1_desc = {
	.enable_reg = AXP2101_REG_LDOGRP1_CONTROL,
	.enable_mask = 0x10U,
	.enable_val = 0x10U,
	.vsel_reg = AXP2101_REG_BLDO1_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 300000U,
	.ranges = axp2101_abldox_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_abldox_ranges),
	.workmode_reg = 0U,
	.workmode_mask = 0U,
};

__maybe_unused static const struct regulator_axp192_desc axp2101_bldo2_desc = {
	.enable_reg = AXP2101_REG_LDOGRP1_CONTROL,
	.enable_mask = 0x20U,
	.enable_val = 0x20U,
	.vsel_reg = AXP2101_REG_BLDO2_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 300000U,
	.ranges = axp2101_abldox_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_abldox_ranges),
	.workmode_reg = 0U,
	.workmode_mask = 0U,
};

static const struct linear_range axp2101_cpusldo_ranges[] = {
	LINEAR_RANGE_INIT(500000U, 50000U, 0U, 19U),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_cpusldo_desc = {
	.enable_reg = AXP2101_REG_LDOGRP1_CONTROL,
	.enable_mask = 0x40U,
	.enable_val = 0x40U,
	.vsel_reg = AXP2101_REG_CPUSLDO_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 30000U,
	.ranges = axp2101_cpusldo_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_cpusldo_ranges),
	.workmode_reg = 0U,
	.workmode_mask = 0U,
};

static const struct linear_range axp2101_dldo1_ranges[] = {
	LINEAR_RANGE_INIT(500000U, 100000U, 0U, 28U),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_dldo1_desc = {
	.enable_reg = AXP2101_REG_LDOGRP1_CONTROL,
	.enable_mask = 0x80U,
	.enable_val = 0x80U,
	.vsel_reg = AXP2101_REG_DLDO1_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 300000U,
	.ranges = axp2101_dldo1_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_dldo1_ranges),
	.workmode_reg = 0U,
	.workmode_mask = 0U,
};

static const struct linear_range axp2101_dldo2_ranges[] = {
	LINEAR_RANGE_INIT(500000U, 50000U, 0U, 19U),
};

__maybe_unused static const struct regulator_axp192_desc axp2101_dldo2_desc = {
	.enable_reg = AXP2101_REG_LDOGRP2_CONTROL,
	.enable_mask = 0x01U,
	.enable_val = 0x01U,
	.vsel_reg = AXP2101_REG_DLDO2_VOLTAGE,
	.vsel_mask = 0x1FU,
	.vsel_bitpos = 0U,
	.max_ua = 300000U,
	.ranges = axp2101_dldo2_ranges,
	.num_ranges = ARRAY_SIZE(axp2101_dldo2_ranges),
	.workmode_reg = 0U,
	.workmode_mask = 0U,
};

static int axp192_enable(const struct device *dev)
{
	const struct regulator_axp192_config *config = dev->config;
	int ret;

	LOG_DBG("Enabling regulator");
	LOG_DBG("[0x%02x]=0x%02x mask=0x%02x", config->desc->enable_reg,
		     config->desc->enable_val, config->desc->enable_mask);

#if AXP192_ANY_HAS_CHILD(ldoio0)
	/* special case for LDOIO0, which is multiplexed with GPIO0 */
	if (config->desc->enable_reg == AXP192_REG_GPIO0_CONTROL) {
		ret = mfd_axp192_gpio_func_ctrl(config->mfd, dev, 0, AXP192_GPIO_FUNC_LDO);
	} else {
#endif
		ret = i2c_reg_update_byte_dt(&config->i2c, config->desc->enable_reg,
					     config->desc->enable_mask, config->desc->enable_val);
#if AXP192_ANY_HAS_CHILD(ldoio0)
	}
#endif

	if (ret != 0) {
		LOG_ERR("Failed to enable regulator");
	}

	return ret;
}

static int axp192_disable(const struct device *dev)
{
	const struct regulator_axp192_config *config = dev->config;
	int ret;

	LOG_DBG("Disabling regulator");
	LOG_DBG("[0x%02x]=0 mask=0x%x", config->desc->enable_reg,
		     config->desc->enable_mask);

#if AXP192_ANY_HAS_CHILD(ldoio0)
	/* special case for LDOIO0, which is multiplexed with GPIO0 */
	if (config->desc->enable_reg == AXP192_REG_GPIO0_CONTROL) {
		ret = mfd_axp192_gpio_func_ctrl(config->mfd, dev, 0, AXP192_GPIO_FUNC_OUTPUT_LOW);
	} else {
#endif
		ret = i2c_reg_update_byte_dt(&config->i2c, config->desc->enable_reg,
					     config->desc->enable_mask, 0u);
#if AXP192_ANY_HAS_CHILD(ldoio0)
	}
#endif

	if (ret != 0) {
		LOG_ERR("Failed to disable regulator");
	}

	return ret;
}

static unsigned int axp192_count_voltages(const struct device *dev)
{
	const struct regulator_axp192_config *config = dev->config;

	return linear_range_group_values_count(config->desc->ranges, config->desc->num_ranges);
}

static int axp192_list_voltage(const struct device *dev, unsigned int idx, int32_t *volt_uv)
{
	const struct regulator_axp192_config *config = dev->config;

	return linear_range_group_get_value(config->desc->ranges, config->desc->num_ranges, idx,
					    volt_uv);
}

static int axp192_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_axp192_config *config = dev->config;
	uint16_t idx;
	int ret;

	LOG_DBG("voltage = [min=%d, max=%d]", min_uv, max_uv);

	/* set voltage */
	ret = linear_range_group_get_win_index(config->desc->ranges, config->desc->num_ranges,
					       min_uv, max_uv, &idx);
	if (ret != 0) {
		LOG_ERR("No voltage range window could be detected");
		return ret;
	}

	idx <<= config->desc->vsel_bitpos;

	LOG_DBG("[0x%x]=0x%x mask=0x%x", config->desc->vsel_reg, idx,
		     config->desc->vsel_mask);
	ret = i2c_reg_update_byte_dt(&config->i2c, config->desc->vsel_reg, config->desc->vsel_mask,
				     (uint8_t)idx);
	if (ret != 0) {
		LOG_ERR("Failed to set regulator voltage");
	}

	return ret;
}

static int axp192_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_axp192_config *config = dev->config;
	int ret;
	uint8_t raw_reg;

	/* read voltage */
	ret = i2c_reg_read_byte_dt(&config->i2c, config->desc->vsel_reg, &raw_reg);
	if (ret != 0) {
		return ret;
	}

	raw_reg = (raw_reg & config->desc->vsel_mask) >> config->desc->vsel_bitpos;

	ret = linear_range_group_get_value(config->desc->ranges, config->desc->num_ranges, raw_reg,
					   volt_uv);

	return ret;
}

static int axp192_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_axp192_config *config = dev->config;
	int ret;

	/* setting workmode is only possible for DCDC1-3 */
	if ((mode == AXP192_DCDC_MODE_PWM) && (config->desc->workmode_reg != 0)) {

		/* configure PWM mode */
		LOG_DBG("PWM mode enabled");
		ret = i2c_reg_update_byte_dt(&config->i2c, config->desc->workmode_reg,
					     config->desc->workmode_mask,
					     config->desc->workmode_pwm_val);
		if (ret != 0) {
			return ret;
		}
	} else if (mode == AXP192_DCDC_MODE_AUTO) {

		/* configure AUTO mode (default) */
		if (config->desc->workmode_reg != 0) {
			ret = i2c_reg_update_byte_dt(&config->i2c, config->desc->workmode_reg,
						     config->desc->workmode_mask, 0u);
			if (ret != 0) {
				return ret;
			}
		} else {

			/* AUTO is default mode for LDOs that cannot be configured */
			return 0;
		}
	} else {
		LOG_ERR("Setting DCDC workmode failed");
		return -ENOTSUP;
	}

	return 0;
}

static int axp192_get_current_limit(const struct device *dev, int32_t *curr_ua)
{
	const struct regulator_axp192_config *config = dev->config;

	*curr_ua = config->desc->max_ua;

	return 0;
}

static DEVICE_API(regulator, api) = {
	.enable = axp192_enable,
	.disable = axp192_disable,
	.count_voltages = axp192_count_voltages,
	.list_voltage = axp192_list_voltage,
	.set_voltage = axp192_set_voltage,
	.get_voltage = axp192_get_voltage,
	.set_mode = axp192_set_mode,
	.get_current_limit = axp192_get_current_limit,
};

static int regulator_axp192_init(const struct device *dev)
{
	const struct regulator_axp192_config *config = dev->config;
	uint8_t enabled_val;
	bool is_enabled;
	int ret = 0;

	regulator_common_data_init(dev);

	if (!device_is_ready(config->mfd)) {
		LOG_ERR("Parent instance not ready!");
		return -ENODEV;
	}

	/* read regulator state */
	ret = i2c_reg_read_byte_dt(&config->i2c, config->desc->enable_reg, &enabled_val);
	if (ret != 0) {
		LOG_ERR("Reading enable status failed!");
		return ret;
	}
	is_enabled = ((enabled_val & config->desc->enable_mask) == config->desc->enable_val);
	LOG_DBG("is_enabled: %d", is_enabled);

	return regulator_common_init(dev, is_enabled);
}

#define REGULATOR_AXP192_DEFINE(node_id, id, name)                                                 \
	static struct regulator_axp192_data data_##id;                                             \
	static const struct regulator_axp192_config config_##id = {                                \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.desc = &id##_desc,                                                                \
		.mfd = DEVICE_DT_GET(DT_GPARENT(node_id)),                                         \
		.i2c = I2C_DT_SPEC_GET(DT_GPARENT(node_id))};                                      \
	DEVICE_DT_DEFINE(node_id, regulator_axp192_init, NULL, &data_##id, &config_##id,           \
			 POST_KERNEL, CONFIG_REGULATOR_AXP192_AXP2101_INIT_PRIORITY, &api);

#define REGULATOR_AXP192_DEFINE_COND(node, child)                                                  \
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(node, child)),                                         \
		    (REGULATOR_AXP192_DEFINE(DT_CHILD(node, child), axp192_##child, child)), ())

#define REGULATOR_AXP2101_DEFINE_COND(node, child)                                                 \
	COND_CODE_1(DT_NODE_EXISTS(DT_CHILD(node, child)),                                         \
		    (REGULATOR_AXP192_DEFINE(DT_CHILD(node, child), axp2101_##child, child)), ())

#define REGULATOR_AXP192_DEFINE_ALL(node)                                                          \
	REGULATOR_AXP192_DEFINE_COND(node, dcdc1)                                                  \
	REGULATOR_AXP192_DEFINE_COND(node, dcdc2)                                                  \
	REGULATOR_AXP192_DEFINE_COND(node, dcdc3)                                                  \
	REGULATOR_AXP192_DEFINE_COND(node, ldoio0)                                                 \
	REGULATOR_AXP192_DEFINE_COND(node, ldo2)                                                   \
	REGULATOR_AXP192_DEFINE_COND(node, ldo3)

#define REGULATOR_AXP2101_DEFINE_ALL(node)                                                         \
	REGULATOR_AXP2101_DEFINE_COND(node, dcdc1)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, dcdc2)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, dcdc3)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, dcdc4)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, dcdc5)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, aldo1)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, aldo2)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, aldo3)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, aldo4)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, bldo1)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, bldo2)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, cpusldo)                                               \
	REGULATOR_AXP2101_DEFINE_COND(node, dldo1)                                                 \
	REGULATOR_AXP2101_DEFINE_COND(node, dldo2)

DT_FOREACH_STATUS_OKAY(x_powers_axp192_regulator, REGULATOR_AXP192_DEFINE_ALL)
DT_FOREACH_STATUS_OKAY(x_powers_axp2101_regulator, REGULATOR_AXP2101_DEFINE_ALL)
