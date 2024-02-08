/*
 * Copyright (c) 2024, Lothar Felten <lothar.felten@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT x_powers_axp2101

#include <errno.h>
#include <stdbool.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_axp2101, CONFIG_MFD_LOG_LEVEL);

/* Chip ID value */
#define AXP2101_CHIP_ID 0x03U

/* Registers definitions */
#define AXP2101_REG_CHIP_ID 0x03U

struct mfd_axp2101_config {
	struct i2c_dt_spec i2c;
};

static int mfd_axp2101_init(const struct device *dev)
{
	const struct mfd_axp2101_config *config = dev->config;
	uint8_t chip_id;
	uint8_t vbus_val;
	int ret;

	LOG_DBG("Initializing instance");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Check if axp2101 chip is available */
	ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		return ret;
	}
	if (chip_id != AXP2101_CHIP_ID) {
		LOG_ERR("Invalid Chip detected (%d)", chip_id);
		return -EINVAL;
	}

	return 0;
}

#define MFD_AXP2101_DEFINE(inst)                                                             \
	static const struct mfd_axp2101_config config##inst = {                              \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                           \
	};                                                                                   \
                                                                                             \
	DEVICE_DT_INST_DEFINE(inst, mfd_axp2101_init, NULL, NULL, &config##inst,             \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_AXP2101_DEFINE);
