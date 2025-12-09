/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tps55287

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(tps55287, CONFIG_REGULATOR_LOG_LEVEL);

#define TPS55287_REG_REF     0x00U
#define TPS55287_REG_VOUT_FS 0x04U
#define TPS55287_REG_MODE    0x06U

#define TPS55287_REG_VOUT_FS_INTFB_MASK BIT_MASK(2)
#define TPS55287_REG_MODE_OE            BIT(7)

/* The order of the voltage ranges is important, as it maps to the VOUT_FS register */
static const struct linear_range core_ranges[] = {
	LINEAR_RANGE_INIT(800000u, 2500u, 0xf0, BIT_MASK(11)),
	LINEAR_RANGE_INIT(800000u, 5000u, 0x50, BIT_MASK(11)),
	LINEAR_RANGE_INIT(800000u, 7500u, 0x1a, BIT_MASK(11)),
	LINEAR_RANGE_INIT(800000u, 10000u, 0x00, BIT_MASK(11)),
};

struct regulator_tps55287_config {
	struct regulator_common_config common;
	struct i2c_dt_spec i2c;
};

struct regulator_tps55287_data {
	struct regulator_common_data data;
};

static unsigned int regulator_tps55287_count_voltages(const struct device *dev)
{
	return linear_range_group_values_count(core_ranges, ARRAY_SIZE(core_ranges));
}

static int regulator_tps55287_list_voltage(const struct device *dev, unsigned int idx,
					   int32_t *volt_uv)
{
	for (uint8_t i = 0U; i < ARRAY_SIZE(core_ranges); i++) {
		if (linear_range_get_value(&core_ranges[i], idx, volt_uv) == 0) {
			return 0;
		}
		idx -= linear_range_values_count(&core_ranges[i]);
	}

	return -EINVAL;
}

static int regulator_tps55287_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_tps55287_config *config = dev->config;
	uint8_t buf[3] = {0};
	uint16_t idx;
	uint8_t vout_fs_reg;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, TPS55287_REG_VOUT_FS, &vout_fs_reg);
	if (ret < 0) {
		return ret;
	}

	vout_fs_reg &= TPS55287_REG_VOUT_FS_INTFB_MASK;

	ret = linear_range_get_win_index(&core_ranges[vout_fs_reg], min_uv, max_uv, &idx);

	/* If we couldn't find a matching voltage in the current range, check the other ranges */
	if (ret < 0) {
		vout_fs_reg = ARRAY_SIZE(core_ranges);
		/* We start with the highest voltage range and work our way down */
		do {
			vout_fs_reg--;

			ret = linear_range_get_win_index(&core_ranges[vout_fs_reg], min_uv, max_uv,
							 &idx);
		} while ((ret < 0) && (vout_fs_reg > 0U));

		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, TPS55287_REG_VOUT_FS, vout_fs_reg);
		if (ret < 0) {
			return ret;
		}
	}

	LOG_DBG("%s: Setting voltage to range %u, index %u", dev->name, vout_fs_reg, idx);

	buf[0] = TPS55287_REG_REF;

	sys_put_le16(idx, &buf[1]);

	return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static int regulator_tps55287_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_tps55287_config *config = dev->config;
	uint8_t vout_fs_reg = 0;
	uint8_t buf[2] = {0};
	uint16_t idx;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, TPS55287_REG_VOUT_FS, &vout_fs_reg);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_burst_read_dt(&config->i2c, TPS55287_REG_REF, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	vout_fs_reg &= TPS55287_REG_VOUT_FS_INTFB_MASK;
	idx = sys_get_le16(buf);

	ret = linear_range_get_value(&core_ranges[vout_fs_reg], idx, volt_uv);

	LOG_DBG("%s: Got voltage: %d uV (range %u, index %u)", dev->name, *volt_uv, vout_fs_reg,
		idx);

	return ret;
}

static int regulator_tps55287_enable(const struct device *dev)
{
	const struct regulator_tps55287_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, TPS55287_REG_MODE, TPS55287_REG_MODE_OE,
				      TPS55287_REG_MODE_OE);
}

static int regulator_tps55287_disable(const struct device *dev)
{
	const struct regulator_tps55287_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, TPS55287_REG_MODE, TPS55287_REG_MODE_OE, 0);
}

static int regulator_tps55287_init(const struct device *dev)
{
	int ret;

	regulator_common_data_init(dev);

	ret = regulator_common_init(dev, false);
	if (ret < 0) {
		LOG_ERR("%s: Failed to initialize regulator: %d", dev->name, ret);
	}
	return ret;
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_tps55287_enable,
	.disable = regulator_tps55287_disable,
	.count_voltages = regulator_tps55287_count_voltages,
	.list_voltage = regulator_tps55287_list_voltage,
	.set_voltage = regulator_tps55287_set_voltage,
	.get_voltage = regulator_tps55287_get_voltage,
};

#define REGULATOR_TPS55287_DEFINE_ALL(inst)                                                        \
	static struct regulator_tps55287_data data_##inst;                                         \
                                                                                                   \
	static const struct regulator_tps55287_config config_##inst = {                            \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),                              \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_tps55287_init, NULL, &data_##inst, &config_##inst,   \
			      POST_KERNEL, CONFIG_REGULATOR_TPS55287_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_TPS55287_DEFINE_ALL)
