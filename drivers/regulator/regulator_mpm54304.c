/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mps_mpm54304

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mps_mpm54304, CONFIG_REGULATOR_LOG_LEVEL);

#define MPM54304_REG_EN        0x0CU
#define MPM54304_REG_VENDOR_ID 0x13U

#define MPM54304_BUCK1_EN_MASK BIT(7)
#define MPM54304_BUCK2_EN_MASK BIT(6)
#define MPM54304_BUCK3_EN_MASK BIT(5)
#define MPM54304_BUCK4_EN_MASK BIT(4)

struct regulator_mpm54304_config {
	struct regulator_common_config common;
	struct i2c_dt_spec bus;
	uint8_t enable_mask;
};

struct regulator_mpm54304_data {
	struct regulator_common_data common;
};

static int regulator_mpm54304_enable(const struct device *dev)
{
	const struct regulator_mpm54304_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->bus, MPM54304_REG_EN, config->enable_mask,
				      config->enable_mask);
}

static int regulator_mpm54304_disable(const struct device *dev)
{
	const struct regulator_mpm54304_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->bus, MPM54304_REG_EN, config->enable_mask, 0x00);
}

static int regulator_mpm54304_init(const struct device *dev)
{
	const struct regulator_mpm54304_config *config = dev->config;
	uint8_t value;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	/* dummy read needed for chip to function properly */
	(void)i2c_reg_read_byte_dt(&config->bus, MPM54304_REG_VENDOR_ID, &value);
	LOG_DBG("vendor id: 0x%x", value >> 4);

	return 0;
}

static DEVICE_API(regulator, mpm54304_api) = {
	.enable = regulator_mpm54304_enable,
	.disable = regulator_mpm54304_disable,
};

#define REGULATOR_MPM54304_DEFINE(node_id, id, child_name)                                         \
	static const struct regulator_mpm54304_config regulator_mpm54304_config_##id = {           \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.bus = I2C_DT_SPEC_GET(DT_PARENT(node_id)),                                        \
		.enable_mask = MPM54304_##child_name##_EN_MASK,                                    \
	};                                                                                         \
                                                                                                   \
	static struct regulator_mpm54304_data regulator_mpm54304_data_##id;                        \
	DEVICE_DT_DEFINE(node_id, regulator_mpm54304_init, NULL, &regulator_mpm54304_data_##id,    \
			 &regulator_mpm54304_config_##id, POST_KERNEL,                             \
			 CONFIG_REGULATOR_MPM54304_INIT_PRIORITY, &mpm54304_api);

#define REGULATOR_MPM54304_DEFINE_COND(inst, child, child_name)                                    \
	IF_ENABLED(                                                                                \
		DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                        \
		(REGULATOR_MPM54304_DEFINE(DT_INST_CHILD(inst, child), child##inst, child_name)))

#define REGULATOR_MPM54304_DEFINE_ALL(inst)                                                        \
	REGULATOR_MPM54304_DEFINE_COND(inst, buck1, BUCK1)                                         \
	REGULATOR_MPM54304_DEFINE_COND(inst, buck2, BUCK2)                                         \
	REGULATOR_MPM54304_DEFINE_COND(inst, buck3, BUCK3)                                         \
	REGULATOR_MPM54304_DEFINE_COND(inst, buck4, BUCK4)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_MPM54304_DEFINE_ALL)
