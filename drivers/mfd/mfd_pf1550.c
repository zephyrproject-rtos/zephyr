/*
 * Copyright (c) 2024 Arduino SA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pf1550

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#define PF1550_REG_CHIP_ID 0x00
#define PF1550_CHIP_ID_VAL ((15 << 3) | 4)

struct mfd_pf1550_config {
	struct i2c_dt_spec bus;
};

static int mfd_pf1550_init(const struct device *dev)
{
	const struct mfd_pf1550_config *config = dev->config;
	uint8_t val;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, PF1550_REG_CHIP_ID, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != PF1550_CHIP_ID_VAL) {
		return -ENODEV;
	}

	return 0;
}

#define MFD_PF1550_DEFINE(inst)                                                                    \
	static const struct mfd_pf1550_config mfd_pf1550_config##inst = {                          \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(inst, mfd_pf1550_init, NULL, NULL, &mfd_pf1550_config##inst,         \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_PF1550_DEFINE)
