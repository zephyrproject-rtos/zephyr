/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stpmic1

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(regulator_stpmic1, CONFIG_REGULATOR_LOG_LEVEL);

#define STPMIC1_BUCK1_MAIN_CR    0x20U
#define STPMIC1_BUCK2_MAIN_CR    0x21U
#define STPMIC1_BUCK3_MAIN_CR    0x22U
#define STPMIC1_BUCK4_MAIN_CR    0x23U
#define STPMIC1_VREF_DDR_MAIN_CR 0x24U
#define STPMIC1_LDO1_MAIN_CR     0x25U
#define STPMIC1_LDO2_MAIN_CR     0x26U
#define STPMIC1_LDO3_MAIN_CR     0x27U
#define STPMIC1_LDO4_MAIN_CR     0x28U
#define STPMIC1_LDO5_MAIN_CR     0x29U
#define STPMIC1_LDO6_MAIN_CR     0x2AU
#define STPMIC1_BST_SW_CR        0x40U

/* BUCKx, LDOx and VREF_DDR MAIN_CR fields */
#define STPMIC1_OUT_ON    BIT(0)
#define STPMIC1_BUCK_VSEL GENMASK(7, 2)
#define STPMIC1_LDO_VSEL  GENMASK(6, 2)

/* BST_SW_CR enable bits */
#define STPMIC1_BST_ON     BIT(0)
#define STPMIC1_VBUSOTG_ON BIT(1)
#define STPMIC1_SWOUT_ON   BIT(2)

/* BST_SW_CR active discharge bits */
#define STPMIC1_VBUSOTG_DISCH BIT(4)
#define STPMIC1_SWOUT_DISCH   BIT(5)

/* Output ramp-up time after enable */
#define STPMIC1_REGU_ENABLE_DELAY_US 1000U
#define STPMIC1_SW_ENABLE_DELAY_US   1000U

/* Worst case output slew rate */
#define STPMIC1_RAMP_UV_PER_US 2200

struct regulator_stpmic1_desc {
	uint8_t ctrl_reg;
	uint8_t en_mask;
	/* Zero if the output voltage is not selectable */
	uint8_t vsel_mask;
	/* Zero if active discharge is not supported */
	uint8_t discharge_mask;
	uint16_t enable_delay_us;
	const struct linear_range *uv_range;
	uint8_t uv_nranges;
};

struct regulator_stpmic1_config {
	struct regulator_common_config common;
	struct i2c_dt_spec bus;
	struct regulator_stpmic1_desc desc;
};

struct regulator_stpmic1_data {
	struct regulator_common_data common;
};

/*
 * Descriptors are expanded in the config of the nodes that use them, so unused outputs take no
 * flash. Voltage ranges are indexed by the VSEL register value and cover it from 0 to its last
 * valid value.
 */
#define STPMIC1_RANGES(...)                                                                        \
	.uv_range = (const struct linear_range[]){__VA_ARGS__},                                    \
	.uv_nranges = ARRAY_SIZE(((const struct linear_range[]){__VA_ARGS__}))

#define STPMIC1_REGU_DESC(_reg, _vsel, ...)                                                        \
	{                                                                                          \
		.ctrl_reg = (_reg),                                                                \
		.en_mask = STPMIC1_OUT_ON,                                                         \
		.vsel_mask = (_vsel),                                                              \
		.enable_delay_us = STPMIC1_REGU_ENABLE_DELAY_US,                                   \
		STPMIC1_RANGES(__VA_ARGS__)                                                        \
	}

#define STPMIC1_SWITCH_DESC(_reg, _en, _disch, _delay)                                             \
	{                                                                                          \
		.ctrl_reg = (_reg),                                                                \
		.en_mask = (_en),                                                                  \
		.discharge_mask = (_disch),                                                        \
		.enable_delay_us = (_delay),                                                       \
	}

#define STPMIC1_BUCK1_RANGES                                                                       \
	LINEAR_RANGE_INIT(725000, 0, 0, 5),                                                        \
	LINEAR_RANGE_INIT(750000, 25000, 6, 36),                                                   \
	LINEAR_RANGE_INIT(1500000, 0, 37, 63)

#define STPMIC1_BUCK2_RANGES                                                                       \
	LINEAR_RANGE_INIT(1000000, 0, 0, 17),                                                      \
	LINEAR_RANGE_INIT(1050000, 0, 18, 19),                                                     \
	LINEAR_RANGE_INIT(1100000, 0, 20, 21),                                                     \
	LINEAR_RANGE_INIT(1150000, 0, 22, 23),                                                     \
	LINEAR_RANGE_INIT(1200000, 0, 24, 25),                                                     \
	LINEAR_RANGE_INIT(1250000, 0, 26, 27),                                                     \
	LINEAR_RANGE_INIT(1300000, 0, 28, 29),                                                     \
	LINEAR_RANGE_INIT(1350000, 0, 30, 31),                                                     \
	LINEAR_RANGE_INIT(1400000, 0, 32, 33),                                                     \
	LINEAR_RANGE_INIT(1450000, 0, 34, 35),                                                     \
	LINEAR_RANGE_INIT(1500000, 0, 36, 36)

#define STPMIC1_BUCK3_RANGES                                                                       \
	LINEAR_RANGE_INIT(1000000, 0, 0, 19),                                                      \
	LINEAR_RANGE_INIT(1100000, 0, 20, 23),                                                     \
	LINEAR_RANGE_INIT(1200000, 0, 24, 27),                                                     \
	LINEAR_RANGE_INIT(1300000, 0, 28, 31),                                                     \
	LINEAR_RANGE_INIT(1400000, 0, 32, 35),                                                     \
	LINEAR_RANGE_INIT(1500000, 100000, 36, 55)

#define STPMIC1_BUCK4_RANGES                                                                       \
	LINEAR_RANGE_INIT(600000, 25000, 0, 27),                                                   \
	LINEAR_RANGE_INIT(1300000, 0, 28, 29),                                                     \
	LINEAR_RANGE_INIT(1350000, 0, 30, 31),                                                     \
	LINEAR_RANGE_INIT(1400000, 0, 32, 33),                                                     \
	LINEAR_RANGE_INIT(1450000, 0, 34, 35),                                                     \
	LINEAR_RANGE_INIT(1500000, 100000, 36, 60)

#define STPMIC1_LDO12_RANGES                                                                       \
	LINEAR_RANGE_INIT(1700000, 0, 0, 8),                                                       \
	LINEAR_RANGE_INIT(1800000, 100000, 9, 24)

/* Indices 31 and 32 (VOUT2/2 and VREF_DDR bypass modes) are not supported */
#define STPMIC1_LDO3_RANGES                                                                        \
	LINEAR_RANGE_INIT(1700000, 0, 0, 8),                                                       \
	LINEAR_RANGE_INIT(1800000, 100000, 9, 24),                                                 \
	LINEAR_RANGE_INIT(3300000, 0, 25, 30)

#define STPMIC1_LDO4_RANGES LINEAR_RANGE_INIT(3300000, 0, 0, 0)

#define STPMIC1_LDO5_RANGES                                                                        \
	LINEAR_RANGE_INIT(1700000, 0, 0, 8),                                                       \
	LINEAR_RANGE_INIT(1800000, 100000, 9, 30)

#define STPMIC1_LDO6_RANGES LINEAR_RANGE_INIT(900000, 100000, 0, 24)

#define STPMIC1_DESC_STPMIC1_BUCK1                                                                 \
	STPMIC1_REGU_DESC(STPMIC1_BUCK1_MAIN_CR, STPMIC1_BUCK_VSEL, STPMIC1_BUCK1_RANGES)

#define STPMIC1_DESC_STPMIC1_BUCK2                                                                 \
	STPMIC1_REGU_DESC(STPMIC1_BUCK2_MAIN_CR, STPMIC1_BUCK_VSEL, STPMIC1_BUCK2_RANGES)

#define STPMIC1_DESC_STPMIC1_BUCK3                                                                 \
	STPMIC1_REGU_DESC(STPMIC1_BUCK3_MAIN_CR, STPMIC1_BUCK_VSEL, STPMIC1_BUCK3_RANGES)

#define STPMIC1_DESC_STPMIC1_BUCK4                                                                 \
	STPMIC1_REGU_DESC(STPMIC1_BUCK4_MAIN_CR, STPMIC1_BUCK_VSEL, STPMIC1_BUCK4_RANGES)

#define STPMIC1_DESC_STPMIC1_LDO1                                                                  \
	STPMIC1_REGU_DESC(STPMIC1_LDO1_MAIN_CR, STPMIC1_LDO_VSEL, STPMIC1_LDO12_RANGES)

#define STPMIC1_DESC_STPMIC1_LDO2                                                                  \
	STPMIC1_REGU_DESC(STPMIC1_LDO2_MAIN_CR, STPMIC1_LDO_VSEL, STPMIC1_LDO12_RANGES)

#define STPMIC1_DESC_STPMIC1_LDO3                                                                  \
	STPMIC1_REGU_DESC(STPMIC1_LDO3_MAIN_CR, STPMIC1_LDO_VSEL, STPMIC1_LDO3_RANGES)

#define STPMIC1_DESC_STPMIC1_LDO4                                                                  \
	STPMIC1_REGU_DESC(STPMIC1_LDO4_MAIN_CR, 0U, STPMIC1_LDO4_RANGES)

#define STPMIC1_DESC_STPMIC1_LDO5                                                                  \
	STPMIC1_REGU_DESC(STPMIC1_LDO5_MAIN_CR, STPMIC1_LDO_VSEL, STPMIC1_LDO5_RANGES)

#define STPMIC1_DESC_STPMIC1_LDO6                                                                  \
	STPMIC1_REGU_DESC(STPMIC1_LDO6_MAIN_CR, STPMIC1_LDO_VSEL, STPMIC1_LDO6_RANGES)

#define STPMIC1_DESC_STPMIC1_VREF_DDR                                                              \
	STPMIC1_SWITCH_DESC(STPMIC1_VREF_DDR_MAIN_CR, STPMIC1_OUT_ON, 0U,                          \
			    STPMIC1_REGU_ENABLE_DELAY_US)

#define STPMIC1_DESC_STPMIC1_BOOST                                                                 \
	STPMIC1_SWITCH_DESC(STPMIC1_BST_SW_CR, STPMIC1_BST_ON, 0U, STPMIC1_REGU_ENABLE_DELAY_US)

#define STPMIC1_DESC_STPMIC1_PWR_SW1                                                               \
	STPMIC1_SWITCH_DESC(STPMIC1_BST_SW_CR, STPMIC1_VBUSOTG_ON, STPMIC1_VBUSOTG_DISCH,          \
			    STPMIC1_SW_ENABLE_DELAY_US)

#define STPMIC1_DESC_STPMIC1_PWR_SW2                                                               \
	STPMIC1_SWITCH_DESC(STPMIC1_BST_SW_CR, STPMIC1_SWOUT_ON, STPMIC1_SWOUT_DISCH,              \
			    STPMIC1_SW_ENABLE_DELAY_US)

static int regulator_stpmic1_set_enable(const struct device *dev, bool enable)
{
	const struct regulator_stpmic1_config *config = dev->config;
	const struct regulator_stpmic1_desc *desc = &config->desc;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->bus, desc->ctrl_reg, desc->en_mask,
				     enable ? desc->en_mask : 0U);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("%s: %s", dev->name, enable ? "enabled" : "disabled");

	if (enable) {
		k_sleep(K_USEC(desc->enable_delay_us));
	}

	return 0;
}

static int regulator_stpmic1_enable(const struct device *dev)
{
	return regulator_stpmic1_set_enable(dev, true);
}

static int regulator_stpmic1_disable(const struct device *dev)
{
	return regulator_stpmic1_set_enable(dev, false);
}

static unsigned int regulator_stpmic1_count_voltages(const struct device *dev)
{
	const struct regulator_stpmic1_config *config = dev->config;

	return linear_range_group_values_count(config->desc.uv_range, config->desc.uv_nranges);
}

static int regulator_stpmic1_list_voltage(const struct device *dev, unsigned int idx,
					  int32_t *volt_uv)
{
	const struct regulator_stpmic1_config *config = dev->config;

	return linear_range_group_get_value(config->desc.uv_range, config->desc.uv_nranges, idx,
					    volt_uv);
}

static int regulator_stpmic1_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_stpmic1_config *config = dev->config;
	const struct regulator_stpmic1_desc *desc = &config->desc;
	int32_t old_uv = 0;
	int32_t new_uv = 0;
	int32_t delay_us;
	uint16_t old_idx;
	uint16_t idx;
	uint8_t val;
	int ret;

	if (desc->uv_range == NULL) {
		return -ENOSYS;
	}

	ret = linear_range_group_get_win_index(desc->uv_range, desc->uv_nranges, min_uv, max_uv,
					       &idx);
	if (ret < 0) {
		return ret;
	}

	if (desc->vsel_mask == 0U) {
		return 0;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, desc->ctrl_reg, &val);
	if (ret < 0) {
		return ret;
	}

	old_idx = FIELD_GET(desc->vsel_mask, val);

	(void)linear_range_group_get_value(desc->uv_range, desc->uv_nranges, idx, &new_uv);
	ret = linear_range_group_get_value(desc->uv_range, desc->uv_nranges, old_idx, &old_uv);
	delay_us = (ret == 0) ? (abs(new_uv - old_uv) / STPMIC1_RAMP_UV_PER_US) : 0;

	val = (val & ~desc->vsel_mask) | FIELD_PREP(desc->vsel_mask, idx);

	ret = i2c_reg_write_byte_dt(&config->bus, desc->ctrl_reg, val);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("%s: voltage set to %d mV (idx %u -> %u)", dev->name, new_uv / 1000, old_idx, idx);

	if (delay_us > 0) {
		k_usleep(delay_us);
	}

	return 0;
}

static int regulator_stpmic1_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_stpmic1_config *config = dev->config;
	const struct regulator_stpmic1_desc *desc = &config->desc;
	uint16_t idx = 0U;
	uint8_t val;
	int ret;

	if (desc->uv_range == NULL) {
		return -ENOSYS;
	}

	if (desc->vsel_mask != 0U) {
		ret = i2c_reg_read_byte_dt(&config->bus, desc->ctrl_reg, &val);
		if (ret < 0) {
			return ret;
		}

		idx = FIELD_GET(desc->vsel_mask, val);
	}

	return linear_range_group_get_value(desc->uv_range, desc->uv_nranges, idx, volt_uv);
}

static int regulator_stpmic1_set_active_discharge(const struct device *dev, bool active_discharge)
{
	const struct regulator_stpmic1_config *config = dev->config;
	const struct regulator_stpmic1_desc *desc = &config->desc;

	if (desc->discharge_mask == 0U) {
		return -ENOTSUP;
	}

	return i2c_reg_update_byte_dt(&config->bus, desc->ctrl_reg, desc->discharge_mask,
				      active_discharge ? desc->discharge_mask : 0U);
}

static int regulator_stpmic1_get_active_discharge(const struct device *dev, bool *active_discharge)
{
	const struct regulator_stpmic1_config *config = dev->config;
	const struct regulator_stpmic1_desc *desc = &config->desc;
	uint8_t val;
	int ret;

	if (desc->discharge_mask == 0U) {
		return -ENOTSUP;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, desc->ctrl_reg, &val);
	if (ret < 0) {
		return ret;
	}

	*active_discharge = (val & desc->discharge_mask) != 0U;

	return 0;
}

static int regulator_stpmic1_init(const struct device *dev)
{
	const struct regulator_stpmic1_config *config = dev->config;
	uint8_t val;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	regulator_common_data_init(dev);

	ret = i2c_reg_read_byte_dt(&config->bus, config->desc.ctrl_reg, &val);
	if (ret < 0) {
		LOG_ERR("%s: PMIC access failed (%d)", dev->name, ret);
		return ret;
	}

	ret = regulator_common_init(dev, (val & config->desc.en_mask) != 0U);
	if (ret < 0) {
		LOG_ERR("%s: init failed (%d)", dev->name, ret);
	}

	return ret;
}

static DEVICE_API(regulator, regulator_stpmic1_api) = {
	.enable = regulator_stpmic1_enable,
	.disable = regulator_stpmic1_disable,
	.count_voltages = regulator_stpmic1_count_voltages,
	.list_voltage = regulator_stpmic1_list_voltage,
	.set_voltage = regulator_stpmic1_set_voltage,
	.get_voltage = regulator_stpmic1_get_voltage,
	.set_active_discharge = regulator_stpmic1_set_active_discharge,
	.get_active_discharge = regulator_stpmic1_get_active_discharge,
};

#define REGULATOR_STPMIC1_DEFINE(node_id, id, rail)                                                \
	static const struct regulator_stpmic1_config regulator_stpmic1_config_##id = {             \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.bus = I2C_DT_SPEC_GET(DT_PARENT(node_id)),                                        \
		.desc = STPMIC1_DESC_##rail,                                                       \
	};                                                                                         \
                                                                                                   \
	static struct regulator_stpmic1_data regulator_stpmic1_data_##id;                          \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, regulator_stpmic1_init, NULL, &regulator_stpmic1_data_##id,      \
			 &regulator_stpmic1_config_##id, POST_KERNEL,                              \
			 CONFIG_REGULATOR_STPMIC1_INIT_PRIORITY, &regulator_stpmic1_api);

#define REGULATOR_STPMIC1_DEFINE_COND(inst, child, rail)                                           \
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_INST_CHILD(inst, child)),                           \
		    (REGULATOR_STPMIC1_DEFINE(DT_INST_CHILD(inst, child), child##inst, rail)),     \
		    ())

BUILD_ASSERT(CONFIG_REGULATOR_STPMIC1_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY,
	     "REGULATOR_STPMIC1_INIT_PRIORITY must be greater than I2C_INIT_PRIORITY");

#define REGULATOR_STPMIC1_DEFINE_ALL(inst)                                                         \
	REGULATOR_STPMIC1_DEFINE_COND(inst, buck1, STPMIC1_BUCK1)                                  \
	REGULATOR_STPMIC1_DEFINE_COND(inst, buck2, STPMIC1_BUCK2)                                  \
	REGULATOR_STPMIC1_DEFINE_COND(inst, buck3, STPMIC1_BUCK3)                                  \
	REGULATOR_STPMIC1_DEFINE_COND(inst, buck4, STPMIC1_BUCK4)                                  \
	REGULATOR_STPMIC1_DEFINE_COND(inst, ldo1, STPMIC1_LDO1)                                    \
	REGULATOR_STPMIC1_DEFINE_COND(inst, ldo2, STPMIC1_LDO2)                                    \
	REGULATOR_STPMIC1_DEFINE_COND(inst, ldo3, STPMIC1_LDO3)                                    \
	REGULATOR_STPMIC1_DEFINE_COND(inst, ldo4, STPMIC1_LDO4)                                    \
	REGULATOR_STPMIC1_DEFINE_COND(inst, ldo5, STPMIC1_LDO5)                                    \
	REGULATOR_STPMIC1_DEFINE_COND(inst, ldo6, STPMIC1_LDO6)                                    \
	REGULATOR_STPMIC1_DEFINE_COND(inst, vref_ddr, STPMIC1_VREF_DDR)                            \
	REGULATOR_STPMIC1_DEFINE_COND(inst, boost, STPMIC1_BOOST)                                  \
	REGULATOR_STPMIC1_DEFINE_COND(inst, pwr_sw1, STPMIC1_PWR_SW1)                              \
	REGULATOR_STPMIC1_DEFINE_COND(inst, pwr_sw2, STPMIC1_PWR_SW2)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_STPMIC1_DEFINE_ALL)
