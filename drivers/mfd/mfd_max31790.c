/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max31790

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/max31790.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(max_max31790, CONFIG_MFD_LOG_LEVEL);

struct max31790_config {
	struct i2c_dt_spec i2c;
};

static void max31790_set_globalconfiguration_i2cwatchdog(uint8_t *destination, uint8_t value)
{
	uint8_t length = MAX37190_GLOBALCONFIGURATION_I2CWATCHDOG_LENGTH;
	uint8_t pos = MAX37190_GLOBALCONFIGURATION_I2CWATCHDOG_POS;

	*destination &= ~GENMASK(pos + length - 1, pos);
	*destination |= FIELD_PREP(GENMASK(pos + length - 1, pos), value);
}

static int max31790_init(const struct device *dev)
{
	const struct max31790_config *config = dev->config;
	int result;
	uint8_t reg_value;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	reg_value = 0;
	reg_value &= ~MAX37190_GLOBALCONFIGURATION_STANDBY_BIT;
	reg_value |= MAX37190_GLOBALCONFIGURATION_RESET_BIT;
	reg_value |= MAX37190_GLOBALCONFIGURATION_BUSTIMEOUT_BIT;
	reg_value &= ~MAX37190_GLOBALCONFIGURATION_OSCILLATORSELECTION_BIT;
	max31790_set_globalconfiguration_i2cwatchdog(&reg_value, 0);
	reg_value &= ~MAX37190_GLOBALCONFIGURATION_I2CWATCHDOGSTATUS_BIT;

	result = i2c_reg_write_byte_dt(&config->i2c, MAX37190_REGISTER_GLOBALCONFIGURATION,
				       reg_value);
	if (result != 0) {
		return result;
	}

	k_sleep(K_USEC(MAX31790_RESET_TIMEOUT_IN_US));

	result = i2c_reg_read_byte_dt(&config->i2c, MAX37190_REGISTER_GLOBALCONFIGURATION,
				      &reg_value);
	if (result != 0) {
		return result;
	}

	if ((reg_value & MAX37190_GLOBALCONFIGURATION_STANDBY_BIT) != 0) {
		LOG_ERR("PWM controller is still in standby");
		return -ENODEV;
	}

	return 0;
}

#define MAX31790_INIT(inst)                                                                        \
	static const struct max31790_config max31790_##inst##_config = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max31790_init, NULL, NULL, &max31790_##inst##_config,          \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MAX31790_INIT);
