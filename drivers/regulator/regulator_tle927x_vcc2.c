/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Deere & Company
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tle927x_vcc2

#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mfd/tle927x.h>
#include <zephyr/dt-bindings/regulator/tle927x.h>

#define TLE927X_VCC2_VOLTAGE_UV 5000000

struct regulator_tle927x_vcc2_config {
	struct regulator_common_config common;
	const struct device *parent;
};

struct regulator_tle927x_vcc2_data {
	struct regulator_common_data data;
	regulator_mode_t mode;
};

static int regulator_tle927x_vcc2_write(const struct device *dev, uint8_t vcc2_on)
{
	const struct regulator_tle927x_vcc2_config *config = dev->config;

	return mfd_tle927x_update_reg(config->parent, TLE927X_REG_MS_CTRL,
				      TLE927X_MS_CTRL_VCC2_ON_MASK,
				      FIELD_PREP(TLE927X_MS_CTRL_VCC2_ON_MASK, vcc2_on));
}

static int regulator_tle927x_vcc2_enable(const struct device *dev)
{
	struct regulator_tle927x_vcc2_data *data = dev->data;

	return regulator_tle927x_vcc2_write(dev, data->mode);
}

static int regulator_tle927x_vcc2_disable(const struct device *dev)
{
	return regulator_tle927x_vcc2_write(dev, TLE927X_VCC2_ON_OFF);
}

static int regulator_tle927x_vcc2_set_mode(const struct device *dev, regulator_mode_t mode)
{
	struct regulator_tle927x_vcc2_data *data = dev->data;
	int ret;

	if ((mode != TLE927X_VCC2_MODE_NORMAL) && (mode != TLE927X_VCC2_MODE_NORMAL_STOP) &&
	    (mode != TLE927X_VCC2_MODE_ALL)) {
		return -ENOTSUP;
	}

	if (regulator_is_enabled(dev)) {
		ret = regulator_tle927x_vcc2_write(dev, mode);
		if (ret != 0) {
			return ret;
		}
	}

	data->mode = mode;

	return 0;
}

static int regulator_tle927x_vcc2_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	struct regulator_tle927x_vcc2_data *data = dev->data;

	if (mode == NULL) {
		return -EINVAL;
	}

	*mode = data->mode;

	return 0;
}

static int regulator_tle927x_vcc2_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	ARG_UNUSED(dev);

	if (volt_uv == NULL) {
		return -EINVAL;
	}

	*volt_uv = TLE927X_VCC2_VOLTAGE_UV;

	return 0;
}

static int regulator_tle927x_vcc2_get_error_flags(const struct device *dev,
						  regulator_error_flags_t *flags)
{
	const struct regulator_tle927x_vcc2_config *config = dev->config;
	uint8_t sup_stat;
	int ret;

	if (flags == NULL) {
		return -EINVAL;
	}

	ret = mfd_tle927x_read_reg(config->parent, TLE927X_REG_SUP_STAT, &sup_stat);
	if (ret != 0) {
		return ret;
	}

	*flags = 0U;

	if ((sup_stat & TLE927X_SUP_STAT_VCC2_UV) != 0U) {
		*flags |= REGULATOR_ERROR_UNDER_VOLTAGE;
	}

	if ((sup_stat & TLE927X_SUP_STAT_VCC2_OT) != 0U) {
		*flags |= REGULATOR_ERROR_OVER_TEMP;
	}

	return 0;
}

static DEVICE_API(regulator, regulator_tle927x_vcc2_api) = {
	.enable = regulator_tle927x_vcc2_enable,
	.disable = regulator_tle927x_vcc2_disable,
	.set_mode = regulator_tle927x_vcc2_set_mode,
	.get_mode = regulator_tle927x_vcc2_get_mode,
	.get_voltage = regulator_tle927x_vcc2_get_voltage,
	.get_error_flags = regulator_tle927x_vcc2_get_error_flags,
};

static int regulator_tle927x_vcc2_init(const struct device *dev)
{
	const struct regulator_tle927x_vcc2_config *config = dev->config;
	struct regulator_tle927x_vcc2_data *data = dev->data;
	uint8_t ms_ctrl, vcc2_on;
	int ret;

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	regulator_common_data_init(dev);

	ret = mfd_tle927x_read_reg(config->parent, TLE927X_REG_MS_CTRL, &ms_ctrl);
	if (ret != 0) {
		return ret;
	}

	vcc2_on = FIELD_GET(TLE927X_MS_CTRL_VCC2_ON_MASK, ms_ctrl);

	if (vcc2_on == TLE927X_VCC2_ON_OFF) {
		data->mode = TLE927X_VCC2_MODE_NORMAL;
	} else {
		data->mode = vcc2_on;
	}

	ret = regulator_common_init(dev, vcc2_on != TLE927X_VCC2_ON_OFF);
	if (ret != 0) {
		return ret;
	}

	if ((vcc2_on != TLE927X_VCC2_ON_OFF) && (vcc2_on != data->mode) &&
	    regulator_is_enabled(dev)) {
		ret = regulator_tle927x_vcc2_write(dev, data->mode);
	}

	return ret;
}

#define REGULATOR_TLE927X_VCC2_DEFINE(inst)                                                        \
	static struct regulator_tle927x_vcc2_data regulator_tle927x_vcc2_data_##inst;              \
                                                                                                   \
	static const struct regulator_tle927x_vcc2_config regulator_tle927x_vcc2_config_##inst = { \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),                              \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(                                                                     \
		inst, regulator_tle927x_vcc2_init, NULL, &regulator_tle927x_vcc2_data_##inst,      \
		&regulator_tle927x_vcc2_config_##inst, POST_KERNEL,                                \
		CONFIG_REGULATOR_TLE927X_INIT_PRIORITY, &regulator_tle927x_vcc2_api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_TLE927X_VCC2_DEFINE)
