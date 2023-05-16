/*
 * Copyright (c) 2023 M5Stack
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xpowers_axp192_regulator

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

/* X-Powers AXP192 chip ID */
#define AXP192_CHIP_ID 0x03

/* X-Powers AXP192 register */
#define AXP192_REG_POWER_STATUS      0x00
#define AXP192_REG_CHIP_ID           0x03
#define AXP192_REG_BUCK123_LDO23_CTL 0x12
#define AXP192_REG_BUCK1_OUT_VOL     0x26
#define AXP192_REG_BUCK2_OUT_VOL     0x23
#define AXP192_REG_BUCK3_OUT_VOL     0x27
#define AXP192_REG_LDO23_OUT_VOL     0x28
#define AXP192_REG_VBUS_IPS_CTL      0x30
#define AXP192_REG_CHG_CTL           0x33
#define AXP192_REG_PER_KEY_CTL       0x36
#define AXP192_REG_ACIN_IN_VOL       0x56
#define AXP192_REG_ACIN_IN_CUR       0x58
#define AXP192_REG_VBUS_IN_VOL       0x5A
#define AXP192_REG_VBUS_IN_CUR       0x5C
#define AXP192_REG_INTER_TEMP        0x5E
#define AXP192_REG_BAT_VOL           0x78
#define AXP192_REG_BAT_CHG_CUR       0x7A
#define AXP192_REG_BAT_DCHG_CUR      0x7C
#define AXP192_REG_GPIO0_MODE_CTL    0x90
#define AXP192_REG_LDOIO0_OUT_VOL    0x91

/* X-Powers AXP192 buck and ldo output control bit */
#define AXP192_BUCK1_CTL_BIT BIT(0)
#define AXP192_BUCK2_CTL_BIT BIT(4)
#define AXP192_BUCK3_CTL_BIT BIT(1)
#define AXP192_LDO2_CTL_BIT  BIT(2)
#define AXP192_LDO3_CTL_BIT  BIT(3)

#define AXP192_GPIO0_LDO_MODE 0x02
#define AXP192_GPIO0_LOW_MODE 0x05

/* X-Powers AXP192 voltage sources */
enum axp192_voltage_sources {
	AXP192_SOURCE_BUCK1,
	AXP192_SOURCE_BUCK2,
	AXP192_SOURCE_BUCK3,
	AXP192_SOURCE_LDO1,
	AXP192_SOURCE_LDO2,
	AXP192_SOURCE_LDO3,
	AXP192_SOURCE_LDOIO0
};

struct regulator_axp192_pconfig {
	struct i2c_dt_spec i2c;
};

struct regulator_axp192_config {
	struct regulator_common_config common;
	const struct device *p;
	uint8_t source;
};

struct regulator_axp192_data {
	struct regulator_common_data data;
};

/* Linear range for output voltage */
static const struct linear_range buck13_range = LINEAR_RANGE_INIT(700000, 25000, 0U, 112U);
static const struct linear_range buck2_range = LINEAR_RANGE_INIT(700000, 25000, 0U, 63U);
static const struct linear_range ldo23_io0_range = LINEAR_RANGE_INIT(1800000, 100000, 0U, 15U);

static int buck_output_enable(const struct device *dev, uint8_t reg_addr, uint8_t ctl_bit,
			      bool enable)
{
	const struct regulator_axp192_config *config = dev->config;
	const struct regulator_axp192_pconfig *pconfig = config->p->config;

	if (enable) {
		return i2c_reg_update_byte_dt(&pconfig->i2c, reg_addr, ctl_bit, ctl_bit);
	} else {
		return i2c_reg_update_byte_dt(&pconfig->i2c, reg_addr, ctl_bit, 0);
	}
}

static int buck_set_voltage(const struct device *dev, const struct linear_range *range,
			    uint8_t reg_addr, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_axp192_config *config = dev->config;
	const struct regulator_axp192_pconfig *pconfig = config->p->config;
	uint16_t idx;
	int ret;

	ret = linear_range_get_win_index(range, min_uv, max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&pconfig->i2c, reg_addr, idx);
}

static int buck_get_voltage(const struct device *dev, const struct linear_range *range,
			    uint8_t reg_addr, int32_t *volt_uv)
{
	const struct regulator_axp192_config *config = dev->config;
	const struct regulator_axp192_pconfig *pconfig = config->p->config;
	uint8_t idx;
	int ret;

	ret = i2c_reg_read_byte_dt(&pconfig->i2c, reg_addr, &idx);
	if (ret < 0) {
		return ret;
	}

	return linear_range_get_value(range, idx, volt_uv);
}

static int ldo_output_enable(const struct device *dev, uint8_t reg_addr, uint8_t ctl_bit,
			     bool enable)
{
	const struct regulator_axp192_config *config = dev->config;
	const struct regulator_axp192_pconfig *pconfig = config->p->config;

	if (enable) {
		return i2c_reg_update_byte_dt(&pconfig->i2c, reg_addr, ctl_bit, ctl_bit);
	} else {
		return i2c_reg_update_byte_dt(&pconfig->i2c, reg_addr, ctl_bit, 0);
	}
}

static int ldo_set_voltage(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t shift,
			   int32_t min_uv, int32_t max_uv)
{
	const struct regulator_axp192_config *config = dev->config;
	const struct regulator_axp192_pconfig *pconfig = config->p->config;
	uint16_t idx;
	int ret;

	ret = linear_range_get_win_index(&ldo23_io0_range, min_uv, max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return i2c_reg_update_byte_dt(&pconfig->i2c, reg_addr, mask, idx << shift);
}

static int ldo_get_voltage(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t shift,
			   int32_t *volt_uv)
{
	const struct regulator_axp192_config *config = dev->config;
	const struct regulator_axp192_pconfig *pconfig = config->p->config;
	uint8_t idx;
	int ret;

	ret = i2c_reg_read_byte_dt(&pconfig->i2c, reg_addr, &idx);
	if (ret < 0) {
		return ret;
	}

	return linear_range_get_value(&ldo23_io0_range, (idx & mask) >> shift, volt_uv);
}

static int ldoio0_output_enable(const struct device *dev, bool enable)
{
	const struct regulator_axp192_config *config = dev->config;
	const struct regulator_axp192_pconfig *pconfig = config->p->config;

	/* LDOIO0 does not have a register to control the output, here we control the GPIO mode to
	 * control the output */
	if (enable) {
		return i2c_reg_write_byte_dt(&pconfig->i2c, AXP192_REG_GPIO0_MODE_CTL,
					     AXP192_GPIO0_LDO_MODE);
	} else {
		return i2c_reg_write_byte_dt(&pconfig->i2c, AXP192_REG_GPIO0_MODE_CTL,
					     AXP192_GPIO0_LOW_MODE);
	}
}

static int gpio_set_mode(const struct device *dev, uint8_t reg_addr, uint8_t mode)
{
	const struct regulator_axp192_config *config = dev->config;
	const struct regulator_axp192_pconfig *pconfig = config->p->config;

	return i2c_reg_write_byte_dt(&pconfig->i2c, reg_addr, mode);
}

int regulator_axp192_enable(const struct device *dev)
{
	const struct regulator_axp192_config *config = dev->config;

	switch (config->source) {
	case AXP192_SOURCE_BUCK1:
		return buck_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_BUCK1_CTL_BIT,
					  true);
	case AXP192_SOURCE_BUCK2:
		return buck_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_BUCK2_CTL_BIT,
					  true);
	case AXP192_SOURCE_BUCK3:
		return buck_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_BUCK3_CTL_BIT,
					  true);
	case AXP192_SOURCE_LDO1:
		return 0;
	case AXP192_SOURCE_LDO2:
		return ldo_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_LDO2_CTL_BIT,
					 true);
	case AXP192_SOURCE_LDO3:
		return ldo_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_LDO3_CTL_BIT,
					 true);
	case AXP192_SOURCE_LDOIO0:
		return ldoio0_output_enable(dev, true);
	default:
		return -ENODEV;
	}
}

int regulator_axp192_disable(const struct device *dev)
{
	const struct regulator_axp192_config *config = dev->config;

	switch (config->source) {
	case AXP192_SOURCE_BUCK1:
		return buck_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_BUCK1_CTL_BIT,
					  false);
	case AXP192_SOURCE_BUCK2:
		return buck_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_BUCK2_CTL_BIT,
					  false);
	case AXP192_SOURCE_BUCK3:
		return buck_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_BUCK3_CTL_BIT,
					  false);
	case AXP192_SOURCE_LDO1:
		return 0;
	case AXP192_SOURCE_LDO2:
		return ldo_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_LDO2_CTL_BIT,
					 false);
	case AXP192_SOURCE_LDO3:
		return ldo_output_enable(dev, AXP192_REG_BUCK123_LDO23_CTL, AXP192_LDO2_CTL_BIT,
					 false);
	case AXP192_SOURCE_LDOIO0:
		return ldoio0_output_enable(dev, false);
	default:
		return -ENODEV;
	}
}

unsigned int regulator_axp192_count_voltages(const struct device *dev)
{
	const struct regulator_axp192_config *config = dev->config;

	switch (config->source) {
	case AXP192_SOURCE_BUCK1:
	case AXP192_SOURCE_BUCK3:
		return linear_range_values_count(&buck13_range);
	case AXP192_SOURCE_BUCK2:
		return linear_range_values_count(&buck2_range);
	case AXP192_SOURCE_LDO1:
		return -ENOTSUP;
	case AXP192_SOURCE_LDO2:
	case AXP192_SOURCE_LDO3:
	case AXP192_SOURCE_LDOIO0:
		return linear_range_values_count(&ldo23_io0_range);
	default:
		return -ENODEV;
	}
}

int regulator_axp192_list_voltage(const struct device *dev, unsigned int idx, int32_t *volt_uv)
{
	const struct regulator_axp192_config *config = dev->config;

	switch (config->source) {
	case AXP192_SOURCE_BUCK1:
	case AXP192_SOURCE_BUCK3:
		return linear_range_get_value(&buck13_range, idx, volt_uv);
	case AXP192_SOURCE_BUCK2:
		return linear_range_get_value(&buck2_range, idx, volt_uv);
	case AXP192_SOURCE_LDO1:
		return -ENOTSUP;
	case AXP192_SOURCE_LDO2:
	case AXP192_SOURCE_LDO3:
	case AXP192_SOURCE_LDOIO0:
		return linear_range_get_value(&ldo23_io0_range, idx, volt_uv);
	default:
		return -ENODEV;
	}
}

int regulator_axp192_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_axp192_config *config = dev->config;

	switch (config->source) {
	case AXP192_SOURCE_BUCK1:
		return buck_set_voltage(dev, &buck13_range, AXP192_REG_BUCK1_OUT_VOL, min_uv,
					max_uv);
	case AXP192_SOURCE_BUCK2:
		return buck_set_voltage(dev, &buck2_range, AXP192_REG_BUCK2_OUT_VOL, min_uv,
					max_uv);
	case AXP192_SOURCE_BUCK3:
		return buck_set_voltage(dev, &buck13_range, AXP192_REG_BUCK3_OUT_VOL, min_uv,
					max_uv);
	case AXP192_SOURCE_LDO1:
		return -ENOTSUP;
	case AXP192_SOURCE_LDO2:
		return ldo_set_voltage(dev, AXP192_REG_LDO23_OUT_VOL, 0xf0, 4, min_uv, max_uv);
	case AXP192_SOURCE_LDO3:
		return ldo_set_voltage(dev, AXP192_REG_LDO23_OUT_VOL, 0x0f, 0, min_uv, max_uv);
	case AXP192_SOURCE_LDOIO0:
		return ldo_set_voltage(dev, AXP192_REG_LDOIO0_OUT_VOL, 0xf0, 4, min_uv, max_uv);
	default:
		return -ENODEV;
	}
}

int regulator_axp192_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_axp192_config *config = dev->config;

	switch (config->source) {
	case AXP192_SOURCE_BUCK1:
		return buck_get_voltage(dev, &buck13_range, AXP192_REG_BUCK1_OUT_VOL, volt_uv);
	case AXP192_SOURCE_BUCK2:
		return buck_get_voltage(dev, &buck2_range, AXP192_REG_BUCK2_OUT_VOL, volt_uv);
	case AXP192_SOURCE_BUCK3:
		return buck_get_voltage(dev, &buck13_range, AXP192_REG_BUCK3_OUT_VOL, volt_uv);
	case AXP192_SOURCE_LDO1:
		return -ENOTSUP;
	case AXP192_SOURCE_LDO2:
		return ldo_get_voltage(dev, AXP192_REG_LDO23_OUT_VOL, 0xf0, 4, volt_uv);
	case AXP192_SOURCE_LDO3:
		return ldo_get_voltage(dev, AXP192_REG_LDO23_OUT_VOL, 0x0f, 0, volt_uv);
	case AXP192_SOURCE_LDOIO0:
		return ldo_get_voltage(dev, AXP192_REG_LDOIO0_OUT_VOL, 0xf0, 4, volt_uv);
	default:
		return -ENODEV;
	}
}

int regulator_axp192_common_init(const struct device *dev)
{
	const struct regulator_axp192_pconfig *pconfig = dev->config;
	uint8_t chip_id;
	int ret = 0;

	ret = i2c_reg_read_byte_dt(&pconfig->i2c, AXP192_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		return ret;
	}

	if (chip_id != AXP192_CHIP_ID) {
		return -EINVAL;
	}

	return 0;
}

int regulator_axp192_init(const struct device *dev)
{
	const struct regulator_axp192_config *config = dev->config;
	bool is_enabled = false;
	int ret = 0;

	regulator_common_data_init(dev);

	if (!device_is_ready(config->p)) {
		return -ENODEV;
	}

	regulator_common_data_init(dev);

	if (!device_is_ready(config->p)) {
		return -ENODEV;
	}

	if (config->source == AXP192_SOURCE_LDOIO0) {
		ret = gpio_set_mode(dev, AXP192_REG_GPIO0_MODE_CTL, AXP192_GPIO0_LDO_MODE);
		if (ret < 0) {
			return ret;
		}
	}

	/* LDO1 is always on */
	is_enabled = (config->source == AXP192_SOURCE_LDO1);

	return regulator_common_init(dev, is_enabled);
}

static const struct regulator_driver_api api = {.enable = regulator_axp192_enable,
						.disable = regulator_axp192_disable,
						.count_voltages = regulator_axp192_count_voltages,
						.list_voltage = regulator_axp192_list_voltage,
						.set_voltage = regulator_axp192_set_voltage,
						.get_voltage = regulator_axp192_get_voltage};

#define REGULATOR_AXP192_DEFINE(node_id, id, _source, parent)                                      \
	static struct regulator_axp192_data data_##id;                                             \
                                                                                                   \
	static const struct regulator_axp192_config config_##id = {                                \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.p = parent,                                                                       \
		.source = _source};                                                                \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, regulator_axp192_init, NULL, &data_##id, &config_##id,           \
			 POST_KERNEL, CONFIG_REGULATOR_AXP192_INIT_PRIORITY, &api);

#define REGULATOR_AXP192_DEFINE_COND(inst, child, source, parent)                                  \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_AXP192_DEFINE(DT_INST_CHILD(inst, child), child##inst, source,      \
					     parent)),                                             \
		    ())

#define REGULATOR_AXP192_DEFINE_ALL(inst)                                                          \
	static const struct regulator_axp192_pconfig config_##inst = {                             \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst))};                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_axp192_common_init, NULL, NULL, &config_##inst,      \
			      POST_KERNEL, CONFIG_REGULATOR_AXP192_COMMON_INIT_PRIORITY, NULL);    \
                                                                                                   \
	REGULATOR_AXP192_DEFINE_COND(inst, buck1, AXP192_SOURCE_BUCK1, DEVICE_DT_INST_GET(inst))   \
	REGULATOR_AXP192_DEFINE_COND(inst, buck2, AXP192_SOURCE_BUCK2, DEVICE_DT_INST_GET(inst))   \
	REGULATOR_AXP192_DEFINE_COND(inst, buck3, AXP192_SOURCE_BUCK3, DEVICE_DT_INST_GET(inst))   \
	REGULATOR_AXP192_DEFINE_COND(inst, ldo1, AXP192_SOURCE_LDO1, DEVICE_DT_INST_GET(inst))     \
	REGULATOR_AXP192_DEFINE_COND(inst, ldo2, AXP192_SOURCE_LDO2, DEVICE_DT_INST_GET(inst))     \
	REGULATOR_AXP192_DEFINE_COND(inst, ldo3, AXP192_SOURCE_LDO3, DEVICE_DT_INST_GET(inst))     \
	REGULATOR_AXP192_DEFINE_COND(inst, ldoio0, AXP192_SOURCE_LDOIO0, DEVICE_DT_INST_GET(inst))

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_AXP192_DEFINE_ALL)
