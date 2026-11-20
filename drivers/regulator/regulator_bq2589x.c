/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/bq2589x.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(regulator_bq2589x, CONFIG_REGULATOR_LOG_LEVEL);

/* Register 0x03 - Charge Control */
#define BQ2589X_REG_CHARGE_CTRL 0x03

#define BQ2589X_OTG_CONFIG BIT(5)

/* Register 0x0A - Boost Mode Control */
#define BQ2589X_REG_BOOST_CTRL 0x0A

#define BQ2589X_BOOSTV_MASK    GENMASK(7, 4)
#define BQ2589X_BOOST_LIM_MASK GENMASK(2, 0)

static const struct linear_range boostv_range = LINEAR_RANGE_INIT(4550000, 64000, 0U, 15U);

/* Boost current limits, indexed by BOOST_LIM. The BQ25896 reserves the last code. */
#if DT_HAS_COMPAT_STATUS_OKAY(ti_bq25896_regulator)
static const int32_t bq25896_boost_lim_ua[] = {
	500000, 750000, 1200000, 1400000, 1650000, 1875000, 2150000,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(silergy_sy6970_regulator)
static const int32_t sy6970_boost_lim_ua[] = {
	500000, 750000, 1200000, 1400000, 1650000, 1875000, 2150000, 2450000,
};
#endif

struct regulator_bq2589x_config {
	struct regulator_common_config common;
	struct i2c_dt_spec i2c;
	const struct device *mfd;
	const int32_t *boost_lim_ua;
	uint8_t num_boost_lim;
};

struct regulator_bq2589x_data {
	struct regulator_common_data common;
};

static int regulator_bq2589x_enable(const struct device *dev)
{
	const struct regulator_bq2589x_config *config = dev->config;

	return mfd_bq2589x_reg_update(config->mfd, BQ2589X_REG_CHARGE_CTRL, BQ2589X_OTG_CONFIG,
				      BQ2589X_OTG_CONFIG);
}

static int regulator_bq2589x_disable(const struct device *dev)
{
	const struct regulator_bq2589x_config *config = dev->config;

	return mfd_bq2589x_reg_update(config->mfd, BQ2589X_REG_CHARGE_CTRL, BQ2589X_OTG_CONFIG, 0U);
}

static unsigned int regulator_bq2589x_count_voltages(const struct device *dev)
{
	ARG_UNUSED(dev);

	return linear_range_values_count(&boostv_range);
}

static int regulator_bq2589x_list_voltage(const struct device *dev, unsigned int idx,
					  int32_t *volt_uv)
{
	ARG_UNUSED(dev);

	return linear_range_get_value(&boostv_range, idx, volt_uv);
}

static int regulator_bq2589x_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_bq2589x_config *config = dev->config;
	uint16_t idx;
	int ret;

	ret = linear_range_get_win_index(&boostv_range, min_uv, max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return mfd_bq2589x_reg_update(config->mfd, BQ2589X_REG_BOOST_CTRL, BQ2589X_BOOSTV_MASK,
				      FIELD_PREP(BQ2589X_BOOSTV_MASK, idx));
}

static int regulator_bq2589x_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_bq2589x_config *config = dev->config;
	uint8_t reg;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2589X_REG_BOOST_CTRL, &reg);
	if (ret < 0) {
		return ret;
	}

	return linear_range_get_value(&boostv_range, FIELD_GET(BQ2589X_BOOSTV_MASK, reg), volt_uv);
}

static unsigned int regulator_bq2589x_count_current_limits(const struct device *dev)
{
	const struct regulator_bq2589x_config *config = dev->config;

	return config->num_boost_lim;
}

static int regulator_bq2589x_list_current_limit(const struct device *dev, unsigned int idx,
						int32_t *curr_ua)
{
	const struct regulator_bq2589x_config *config = dev->config;

	if (idx >= config->num_boost_lim) {
		return -EINVAL;
	}

	*curr_ua = config->boost_lim_ua[idx];

	return 0;
}

static int regulator_bq2589x_set_current_limit(const struct device *dev, int32_t min_ua,
					       int32_t max_ua)
{
	const struct regulator_bq2589x_config *config = dev->config;

	for (uint8_t i = 0U; i < config->num_boost_lim; i++) {
		if (IN_RANGE(config->boost_lim_ua[i], min_ua, max_ua)) {
			return mfd_bq2589x_reg_update(config->mfd, BQ2589X_REG_BOOST_CTRL,
						      BQ2589X_BOOST_LIM_MASK,
						      FIELD_PREP(BQ2589X_BOOST_LIM_MASK, i));
		}
	}

	return -EINVAL;
}

static int regulator_bq2589x_get_current_limit(const struct device *dev, int32_t *curr_ua)
{
	const struct regulator_bq2589x_config *config = dev->config;
	uint8_t reg, idx;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2589X_REG_BOOST_CTRL, &reg);
	if (ret < 0) {
		return ret;
	}

	idx = FIELD_GET(BQ2589X_BOOST_LIM_MASK, reg);
	if (idx >= config->num_boost_lim) {
		return -EIO;
	}

	*curr_ua = config->boost_lim_ua[idx];

	return 0;
}

static int regulator_bq2589x_init(const struct device *dev)
{
	const struct regulator_bq2589x_config *config = dev->config;
	uint8_t reg;
	int ret;

	if (!device_is_ready(config->mfd)) {
		LOG_ERR("Parent device not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2589X_REG_CHARGE_CTRL, &reg);
	if (ret < 0) {
		return ret;
	}

	regulator_common_data_init(dev);

	return regulator_common_init(dev, (reg & BQ2589X_OTG_CONFIG) != 0U);
}

static DEVICE_API(regulator, regulator_bq2589x_api) = {
	.enable = regulator_bq2589x_enable,
	.disable = regulator_bq2589x_disable,
	.count_voltages = regulator_bq2589x_count_voltages,
	.list_voltage = regulator_bq2589x_list_voltage,
	.set_voltage = regulator_bq2589x_set_voltage,
	.get_voltage = regulator_bq2589x_get_voltage,
	.count_current_limits = regulator_bq2589x_count_current_limits,
	.list_current_limit = regulator_bq2589x_list_current_limit,
	.set_current_limit = regulator_bq2589x_set_current_limit,
	.get_current_limit = regulator_bq2589x_get_current_limit,
};

#define REGULATOR_BQ2589X_DEFINE(inst, name, boost_lim)                                            \
	static struct regulator_bq2589x_data name##_data_##inst;                                   \
                                                                                                   \
	static const struct regulator_bq2589x_config name##_config_##inst = {                      \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),                              \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.boost_lim_ua = boost_lim,                                                         \
		.num_boost_lim = ARRAY_SIZE(boost_lim),                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_bq2589x_init, NULL, &name##_data_##inst,             \
			      &name##_config_##inst, POST_KERNEL,                                  \
			      CONFIG_REGULATOR_BQ2589X_INIT_PRIORITY, &regulator_bq2589x_api);

#define DT_DRV_COMPAT ti_bq25896_regulator
DT_INST_FOREACH_STATUS_OKAY_VARGS(REGULATOR_BQ2589X_DEFINE, bq25896, bq25896_boost_lim_ua)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT silergy_sy6970_regulator
DT_INST_FOREACH_STATUS_OKAY_VARGS(REGULATOR_BQ2589X_DEFINE, sy6970, sy6970_boost_lim_ua)
#undef DT_DRV_COMPAT
