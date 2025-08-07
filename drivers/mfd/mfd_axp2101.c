/*
 * Copyright (c) 2025 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_axp2101, CONFIG_MFD_LOG_LEVEL);

struct mfd_axp2101_config {
	struct i2c_dt_spec i2c;
};

/* Registers */
#define AXP2101_REG_CHIP_ID		0x03U
	#define AXP2101_CHIP_ID				0x4AU

static int mfd_axp2101_init(const struct device *dev)
{
	const struct mfd_axp2101_config *config = dev->config;
	uint8_t chip_id;
	int ret;

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

#define MFD_AXP2101_DEFINE(node)                                                                   \
	static const struct mfd_axp2101_config config##node = {                                    \
		.i2c = I2C_DT_SPEC_GET(node),                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node, mfd_axp2101_init, NULL, NULL,                                       \
			 &config##node, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_FOREACH_STATUS_OKAY(x_powers_axp2101, MFD_AXP2101_DEFINE);
