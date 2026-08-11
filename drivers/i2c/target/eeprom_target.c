/*
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_i2c_target_eeprom

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/eeprom.h>
#include <zephyr/drivers/i2c/target/regset_target_lib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_eeprom, CONFIG_I2C_LOG_LEVEL);

struct i2c_eeprom_target_data {
	struct regset_target_lib_data regset_data;
};

struct i2c_eeprom_target_config {
	struct regset_target_lib_config regset_cfg;
};

REGSET_TARGET_LIB_STRUCT_CHECK(struct i2c_eeprom_target_config,
			       struct i2c_eeprom_target_data);

void eeprom_target_set_changed_callback(const struct device *dev,
					eeprom_target_changed_handler_t handler,
					void *user_data)
{
	regset_target_lib_set_changed_callback(dev, handler, user_data);
}

size_t eeprom_target_get_size(const struct device *dev)
{
	return regset_target_lib_get_size(dev);
}

int eeprom_target_read_data(const struct device *dev, off_t offset,
			    void *data, size_t len)
{
	return regset_target_lib_read_data(dev, offset, data, len);
}

int eeprom_target_write_data(const struct device *dev, off_t offset,
			     const void *data, size_t len)
{
	return regset_target_lib_write_data(dev, offset, data, len);
}

#ifdef CONFIG_I2C_EEPROM_TARGET_RUNTIME_ADDR
int eeprom_target_set_addr(const struct device *dev, uint8_t addr)
{
	return regset_target_lib_set_addr(dev, addr);
}
#endif /* CONFIG_I2C_EEPROM_TARGET_RUNTIME_ADDR */

static int i2c_eeprom_target_init(const struct device *dev)
{
	return regset_target_lib_init(dev);
}

#define I2C_EEPROM_INIT(inst)								\
	REGSET_TARGET_LIB_DT_INST_BUILD_ASSERT(inst)					\
											\
	static struct i2c_eeprom_target_data i2c_eeprom_target_##inst##_dev_data;	\
											\
	static const struct i2c_eeprom_target_config i2c_eeprom_target_##inst##_cfg = {	\
		.regset_cfg = REGSET_TARGET_LIB_DT_INST_CONFIG_INIT(inst),		\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, &i2c_eeprom_target_init, NULL,			\
			      &i2c_eeprom_target_##inst##_dev_data,			\
			      &i2c_eeprom_target_##inst##_cfg,				\
			      POST_KERNEL, CONFIG_I2C_TARGET_INIT_PRIORITY,		\
			      &regset_target_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_EEPROM_INIT)
