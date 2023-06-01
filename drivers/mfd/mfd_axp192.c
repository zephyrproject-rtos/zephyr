/*
 * Copyright (c) 2023 Martin Kiepfer <mrmarteng@teleschirm.org>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT x_powers_axp192

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_axp192, CONFIG_MFD_LOG_LEVEL);

/* Chip ID value */
#define AXP192_CHIP_ID 0x03U

/* Registers definitions */
#define AXP192_REG_CHIP_ID 0x03U

struct mfd_axp192_config {
	struct i2c_dt_spec i2c;
};

static int mfd_axp192_init(const struct device *dev)
{
	const struct mfd_axp192_config *config = dev->config;
	uint8_t chip_id;
	int ret;

	LOG_DBG("Initializing instance");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Check if axp192 chip is available */
	ret = i2c_reg_read_byte_dt(&config->i2c, AXP192_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		return ret;
	}

	if (chip_id != AXP192_CHIP_ID) {
		LOG_ERR("Invalid Chip detected (%d)", chip_id);
		return -EINVAL;
	}

	return 0;
}

#define MFD_AXP192_DEFINE(inst)                                                                    \
	static const struct mfd_axp192_config config##inst = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_axp192_init, NULL, NULL, &config##inst, POST_KERNEL,       \
			      CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_AXP192_DEFINE)
