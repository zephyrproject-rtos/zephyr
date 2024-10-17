/*
 * Copyright (c) 2024 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT x_powers_axp2101

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/axp2101.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_axp2101, CONFIG_MFD_LOG_LEVEL);

struct mfd_axp2101_config {
	struct i2c_dt_spec i2c;
};

static int mfd_axp2101_init(const struct device *dev)
{
	const struct mfd_axp2101_config *config = dev->config;
	uint8_t regval = AXP2101_REG_CHIP_ID;
	int ret;

	/* Check if axp2101 chip is available */
	ret = i2c_write_read_dt(&config->i2c, &regval, 1, &regval, 1);
	if (ret < 0) {
		return ret;
	}

	if (regval != AXP2101_CHIP_ID) {
		LOG_ERR("Invalid Chip detected (%d)", regval);
		return -EINVAL;
	}

	/*
	 * The LED is initialized in a disabled state.
	 * If an LED node exists, it is re-enabled during the initialization process.
	 */
	ret = i2c_reg_write_byte_dt(&config->i2c, AXP2101_REG_CHGLED, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define MFD_AXP2101_DEFINE(inst)                                                                   \
	static const struct mfd_axp2101_config config##inst = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_axp2101_init, NULL, NULL, &config##inst, POST_KERNEL,      \
			      CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_AXP2101_DEFINE);
