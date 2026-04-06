/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adp5360_regulator

#include <errno.h>

#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/adp5360.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

#include "mfd_adp5360.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(regulator_adp5360, CONFIG_REGULATOR_LOG_LEVEL);

/* ADP5360 regulator related registers */
#define ADP5360_BUCK_CFG        0x29U
#define ADP5360_BUCK_OUTPUT     0x2AU
#define ADP5360_BUCKBST_CFG     0x2BU
#define ADP5360_BUCKBST_OUTPUT  0x2CU
#define ADP5360_SUPERVISORY_CFG 0x2DU

/* Supervisory enable masks for RESET pin */
#define ADP5360_SUPERVISORY_BUCK_RST_MASK    BIT(7)
#define ADP5360_SUPERVISORY_BUCKBST_RST_MASK BIT(6)

/* Buck/boost configure register. */
#define ADP5360_BUCK_CFG_SS_MSK        GENMASK(7, 6)
#define ADP5360_BUCK_CFG_SS_POS        6U
#define ADP5360_BUCK_CFG_BST_ILIM_MSK  GENMASK(5, 3)
#define ADP5360_BUCK_CFG_BST_ILIM_POS  3U
#define ADP5360_BUCK_CFG_BUCK_ILIM_MSK GENMASK(5, 4)
#define ADP5360_BUCK_CFG_BUCK_ILIM_POS 4U
#define ADP5360_BUCK_CFG_BUCK_MODE_MSK BIT(3)
#define ADP5360_BUCK_CFG_BUCK_MODE_POS 3U
#define ADP5360_BUCK_CFG_STP_MSK       BIT(2)
#define ADP5360_BUCK_CFG_DISCHG_MSK    BIT(1)
#define ADP5360_BUCK_CFG_EN_MSK        BIT(0)

/* Buck/boost output voltage setting register. */
#define ADP5360_BUCK_OUTPUT_VOUT_MSK GENMASK(5, 0)
#define ADP5360_BUCK_OUTPUT_VOUT_POS 0U
#define ADP5360_BUCK_OUTPUT_DLY_MSK  GENMASK(7, 6)
#define ADP5360_BUCK_OUTPUT_DLY_POS  6U

struct regulator_adp5360_desc {
	uint8_t cfg_reg;
	uint8_t out_reg;
	const struct linear_range *v_ranges;
	uint8_t n_vranges;
	const struct linear_range *i_ranges;
	uint8_t n_iranges;
	bool is_buckboost;
};

static const struct linear_range buck_v_ranges[] = {
	LINEAR_RANGE_INIT(600000, 50000, 0x0u, 0x3Fu),
};

static const struct linear_range buck_i_ranges[] = {
	LINEAR_RANGE_INIT(100000, 100000, 0x0u, 0x3u),
};

static const struct regulator_adp5360_desc buck_desc = {
	.cfg_reg = ADP5360_BUCK_CFG,
	.out_reg = ADP5360_BUCK_OUTPUT,
	.v_ranges = buck_v_ranges,
	.n_vranges = ARRAY_SIZE(buck_v_ranges),
	.i_ranges = buck_i_ranges,
	.n_iranges = ARRAY_SIZE(buck_i_ranges),
	.is_buckboost = false,
};

static const struct linear_range buckboost_v_ranges[] = {
	LINEAR_RANGE_INIT(1800000, 100000u, 0x0u, 0x0Bu),
	LINEAR_RANGE_INIT(2950000, 50000u, 0xCu, 0x3Fu),
};

static const struct linear_range buckboost_i_ranges[] = {
	LINEAR_RANGE_INIT(100000, 100000, 0x0u, 0x7u),
};

static const struct regulator_adp5360_desc buckboost_desc = {
	.cfg_reg = ADP5360_BUCKBST_CFG,
	.out_reg = ADP5360_BUCKBST_OUTPUT,
	.v_ranges = buckboost_v_ranges,
	.n_vranges = ARRAY_SIZE(buckboost_v_ranges),
	.i_ranges = buckboost_i_ranges,
	.n_iranges = ARRAY_SIZE(buckboost_i_ranges),
	.is_buckboost = true,
};

struct regulator_adp5360_config {
	struct regulator_common_config common;
	const struct device *mfd_dev;
	const struct regulator_adp5360_desc *desc;
	struct gpio_dt_spec en_gpio;
	int8_t dly_idx;
	int8_t ss_idx;
	int8_t ilim_idx;
	bool stp_en;
};

struct regulator_adp5360_data {
	struct regulator_common_data data;
};

static unsigned int regulator_adp5360_count_voltages(const struct device *dev)
{
	const struct regulator_adp5360_config *config = dev->config;

	return linear_range_group_values_count(config->desc->v_ranges, config->desc->n_vranges);
}

static int regulator_adp5360_list_voltage(const struct device *dev, unsigned int idx,
					  int32_t *volt_uv)
{
	const struct regulator_adp5360_config *config = dev->config;

	return linear_range_group_get_value(config->desc->v_ranges, config->desc->n_vranges, idx,
					    volt_uv);
}

static int regulator_adp5360_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_adp5360_config *config = dev->config;
	uint16_t idx;
	int ret;

	ret = linear_range_group_get_win_index(config->desc->v_ranges, config->desc->n_vranges,
					       min_uv, max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return mfd_adp5360_reg_update(config->mfd_dev, config->desc->out_reg,
				      ADP5360_BUCK_OUTPUT_VOUT_MSK, idx);
}

static int regulator_adp5360_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_adp5360_config *config = dev->config;
	int ret;
	uint8_t raw_reg;

	ret = mfd_adp5360_reg_read(config->mfd_dev, config->desc->out_reg, &raw_reg);
	if (ret < 0) {
		return ret;
	}

	raw_reg = FIELD_GET(ADP5360_BUCK_OUTPUT_VOUT_MSK, raw_reg);

	return linear_range_group_get_value(config->desc->v_ranges, config->desc->n_vranges,
					    raw_reg, volt_uv);
}

static unsigned int regulator_adp5360_count_current_limits(const struct device *dev)
{
	const struct regulator_adp5360_config *config = dev->config;

	return linear_range_group_values_count(config->desc->i_ranges, config->desc->n_iranges);
}

static int regulator_adp5360_list_current_limit(const struct device *dev, unsigned int idx,
						int32_t *current_ua)
{
	const struct regulator_adp5360_config *config = dev->config;

	return linear_range_group_get_value(config->desc->i_ranges, config->desc->n_iranges, idx,
					    current_ua);
}

static int regulator_adp5360_set_current_limit(const struct device *dev, int32_t min_ua,
					       int32_t max_ua)
{
	const struct regulator_adp5360_config *config = dev->config;
	uint16_t idx;
	uint8_t mask = ADP5360_BUCK_CFG_BUCK_ILIM_MSK;
	int ret;

	ret = linear_range_group_get_win_index(config->desc->i_ranges, config->desc->n_iranges,
					       min_ua, max_ua, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	if (config->desc->is_buckboost) {
		mask = ADP5360_BUCK_CFG_BST_ILIM_MSK;
	}

	return mfd_adp5360_reg_update(config->mfd_dev, config->desc->cfg_reg, mask, idx);
}

static int regulator_adp5360_get_current_limit(const struct device *dev, int32_t *current_ua)
{
	const struct regulator_adp5360_config *config = dev->config;
	int ret;
	uint8_t raw_reg;
	uint8_t mask = ADP5360_BUCK_CFG_BUCK_ILIM_MSK;

	ret = mfd_adp5360_reg_read(config->mfd_dev, config->desc->cfg_reg, &raw_reg);
	if (ret < 0) {
		return ret;
	}

	if (config->desc->is_buckboost) {
		mask = ADP5360_BUCK_CFG_BST_ILIM_MSK;
	}

	raw_reg = FIELD_GET(mask, raw_reg);

	return linear_range_group_get_value(config->desc->i_ranges, config->desc->n_iranges,
					    raw_reg, current_ua);
}

static int regulator_adp5360_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_adp5360_config *config = dev->config;

	if (config->desc->is_buckboost) {
		return -ENOTSUP;
	}

	if (mode > ADP5360_MODE_PWM) {
		return -EINVAL;
	}

	return mfd_adp5360_reg_update(config->mfd_dev, config->desc->cfg_reg,
				      ADP5360_BUCK_CFG_BUCK_MODE_MSK, mode);
}

static int regulator_adp5360_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_adp5360_config *config = dev->config;
	uint8_t val;
	int ret;

	if (config->desc->is_buckboost) {
		return -ENOTSUP;
	}

	ret = mfd_adp5360_reg_read(config->mfd_dev, config->desc->cfg_reg, &val);
	if (ret < 0) {
		return ret;
	}

	*mode = FIELD_GET(ADP5360_BUCK_CFG_BUCK_MODE_MSK, val);

	return 0;
}

static int regulator_adp5360_enable(const struct device *dev)
{
	const struct regulator_adp5360_config *config = dev->config;
	int ret;

	/*
	 * For buck-boost regulator, enable control can be via GPIO (EN2 pin) or I2C register bit
	 * (EN_BUCKBST). These two control methods are logically OR'ed together.
	 */
	if (config->desc->is_buckboost && config->en_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->en_gpio, 1);
		if (ret < 0) {
			LOG_ERR("Failed to set buck-boost enable GPIO pin");
			return ret;
		}
	}

	return mfd_adp5360_reg_update(config->mfd_dev, config->desc->cfg_reg,
				      ADP5360_BUCK_CFG_EN_MSK, 1u);
}

static int regulator_adp5360_disable(const struct device *dev)
{
	const struct regulator_adp5360_config *config = dev->config;
	int ret;

	/*
	 * For buck-boost regulator, enable control can be via GPIO (EN2 pin) or I2C register bit
	 * (EN_BUCKBST). These two control methods are logically OR'ed together.
	 */
	if (config->desc->is_buckboost && config->en_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->en_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Failed to set buck-boost enable GPIO pin");
			return ret;
		}
	}

	return mfd_adp5360_reg_update(config->mfd_dev, config->desc->cfg_reg,
				      ADP5360_BUCK_CFG_EN_MSK, 0u);
}

static int regulator_adp5360_set_active_discharge(const struct device *dev, bool active_discharge)
{
	const struct regulator_adp5360_config *config = dev->config;

	return mfd_adp5360_reg_update(config->mfd_dev, config->desc->cfg_reg,
				      ADP5360_BUCK_CFG_DISCHG_MSK, active_discharge);
}

static int regulator_adp5360_get_active_discharge(const struct device *dev, bool *active_discharge)
{
	const struct regulator_adp5360_config *config = dev->config;
	int ret;
	uint8_t raw_reg;

	ret = mfd_adp5360_reg_read(config->mfd_dev, config->desc->cfg_reg, &raw_reg);
	if (ret < 0) {
		return ret;
	}

	if (FIELD_GET(ADP5360_BUCK_CFG_DISCHG_MSK, raw_reg)) {
		*active_discharge = true;
	} else {
		*active_discharge = false;
	}

	return 0;
}

static int regulator_adp5360_init(const struct device *dev)
{
	const struct regulator_adp5360_config *config = dev->config;
	int ret;
	uint8_t nval = 0u, temp_sup = 0u, msk = 0u;

	regulator_common_data_init(dev);

	/* disable RESET from triggering during regulator initialization */
	msk = ADP5360_SUPERVISORY_BUCK_RST_MASK | ADP5360_SUPERVISORY_BUCKBST_RST_MASK;

	ret = mfd_adp5360_reg_read(config->mfd_dev, ADP5360_SUPERVISORY_CFG, &temp_sup);
	if (ret < 0) {
		return ret;
	}

	ret = mfd_adp5360_reg_update(config->mfd_dev, ADP5360_SUPERVISORY_CFG, msk, 0u);
	if (ret < 0) {
		return ret;
	}

	msk = ADP5360_BUCK_OUTPUT_DLY_MSK;

	ret = mfd_adp5360_reg_update(config->mfd_dev, config->desc->out_reg, msk, config->dly_idx);
	if (ret < 0) {
		return ret;
	}

	/* apply optional initial configuration */
	nval = FIELD_PREP(ADP5360_BUCK_CFG_SS_MSK, config->ss_idx) |
	       FIELD_PREP(ADP5360_BUCK_CFG_STP_MSK, config->stp_en);

	if (config->desc->is_buckboost) {
		nval |= FIELD_PREP(ADP5360_BUCK_CFG_BST_ILIM_MSK, config->ilim_idx);
	} else {
		nval |= FIELD_PREP(ADP5360_BUCK_CFG_BUCK_ILIM_MSK, config->ilim_idx);
	}

	ret = mfd_adp5360_reg_write(config->mfd_dev, config->desc->cfg_reg, nval);
	if (ret < 0) {
		return ret;
	}

	if (config->en_gpio.port) {
		ret = gpio_pin_configure_dt(&config->en_gpio,
					    config->desc->is_buckboost ?
					    GPIO_OUTPUT_INACTIVE : GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure enable GPIO pin");
			return ret;
		}
	}


	ret = regulator_common_init(dev, 0u);
	if (ret < 0) {
		return ret;
	}

	return mfd_adp5360_reg_read(config->mfd_dev, ADP5360_SUPERVISORY_CFG, &temp_sup);
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_adp5360_enable,
	.disable = regulator_adp5360_disable,
	.count_voltages = regulator_adp5360_count_voltages,
	.list_voltage = regulator_adp5360_list_voltage,
	.set_voltage = regulator_adp5360_set_voltage,
	.get_voltage = regulator_adp5360_get_voltage,
	.count_current_limits = regulator_adp5360_count_current_limits,
	.list_current_limit = regulator_adp5360_list_current_limit,
	.set_current_limit = regulator_adp5360_set_current_limit,
	.get_current_limit = regulator_adp5360_get_current_limit,
	.set_mode = regulator_adp5360_set_mode,
	.get_mode = regulator_adp5360_get_mode,
	.set_active_discharge = regulator_adp5360_set_active_discharge,
	.get_active_discharge = regulator_adp5360_get_active_discharge,
};

#define REGULATOR_ADP5360_DEFINE(node_id, id, name)                                                \
	static struct regulator_adp5360_data data_##id;                                            \
                                                                                                   \
	static const struct regulator_adp5360_config config_##id = {                               \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.mfd_dev = DEVICE_DT_GET(DT_GPARENT(node_id)),                                     \
		.desc = &name##_desc,                                                              \
		.dly_idx = DT_ENUM_IDX(node_id, adi_switch_delay_us),                              \
		.ss_idx = DT_ENUM_IDX(node_id, adi_soft_start_ms),                                 \
		.ilim_idx = DT_ENUM_IDX(node_id, adi_ilim_milliamp),                               \
		.stp_en = DT_PROP(node_id, adi_enable_stop_pulse),                                 \
		.en_gpio = GPIO_DT_SPEC_GET_OR(node_id, enable_gpio, {0}),                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, regulator_adp5360_init, NULL, &data_##id, &config_##id,          \
			 POST_KERNEL, CONFIG_REGULATOR_ADP5360_INIT_PRIORITY, &api);

#define REGULATOR_ADP5360_DEFINE_COND(inst, child)                                                 \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_ADP5360_DEFINE(DT_INST_CHILD(inst, child), child##inst, child)),    \
		    ())

#define REGULATOR_ADP5360_DEFINE_ALL(inst)                                                         \
	REGULATOR_ADP5360_DEFINE_COND(inst, buck)                                                  \
	REGULATOR_ADP5360_DEFINE_COND(inst, buckboost)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_ADP5360_DEFINE_ALL)
