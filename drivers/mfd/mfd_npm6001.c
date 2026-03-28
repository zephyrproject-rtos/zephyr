/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm6001

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

/* nPM6001 registers */
#define NPM6001_SWREADY          0x01U
#define NPM6001_BUCK3SELDAC      0x44U
#define NPM6001_BUCKMODEPADCONF  0x4EU
#define NPM6001_PADDRIVESTRENGTH 0x53U

/* nPM6001 BUCKMODEPADCONF fields */
#define NPM6001_BUCKMODEPADCONF_BUCKMODE0PADTYPE_CMOS  BIT(0)
#define NPM6001_BUCKMODEPADCONF_BUCKMODE1PADTYPE_CMOS  BIT(1)
#define NPM6001_BUCKMODEPADCONF_BUCKMODE2PADTYPE_CMOS  BIT(2)
#define NPM6001_BUCKMODEPADCONF_BUCKMODE0PULLD_ENABLED BIT(4)
#define NPM6001_BUCKMODEPADCONF_BUCKMODE1PULLD_ENABLED BIT(5)
#define NPM6001_BUCKMODEPADCONF_BUCKMODE2PULLD_ENABLED BIT(6)

/* nPM6001 PADDRIVESTRENGTH fields */
#define NPM6001_PADDRIVESTRENGTH_READY_HIGH BIT(2)
#define NPM6001_PADDRIVESTRENGTH_NINT_HIGH  BIT(3)
#define NPM6001_PADDRIVESTRENGTH_SDA_HIGH   BIT(5)

struct mfd_npm6001_config {
	struct i2c_dt_spec i2c;
	uint8_t buck_pad_val;
	uint8_t pad_val;
};

static int mfd_npm6001_init(const struct device *dev)
{
	const struct mfd_npm6001_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	/* always select BUCK3 DAC (does not increase power consumption) */
	ret = i2c_reg_write_byte_dt(&config->i2c, NPM6001_BUCK3SELDAC, 1U);
	if (ret < 0) {
		return ret;
	}

	/* configure pad properties */
	ret = i2c_reg_write_byte_dt(&config->i2c, NPM6001_BUCKMODEPADCONF, config->buck_pad_val);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, NPM6001_PADDRIVESTRENGTH, config->pad_val);
	if (ret < 0) {
		return ret;
	}

	/* Enable switching to hysteresis mode */
	ret = i2c_reg_write_byte_dt(&config->i2c, NPM6001_SWREADY, 1U);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define MFD_NPM6001_DEFINE(inst)                                                                   \
	static const struct mfd_npm6001_config config##inst = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.buck_pad_val = ((DT_INST_ENUM_IDX(inst, nordic_buck_mode0_input_type) *           \
				  NPM6001_BUCKMODEPADCONF_BUCKMODE0PADTYPE_CMOS) |                 \
				 (DT_INST_ENUM_IDX(inst, nordic_buck_mode1_input_type) *           \
				  NPM6001_BUCKMODEPADCONF_BUCKMODE1PADTYPE_CMOS) |                 \
				 (DT_INST_ENUM_IDX(inst, nordic_buck_mode2_input_type) *           \
				  NPM6001_BUCKMODEPADCONF_BUCKMODE2PADTYPE_CMOS) |                 \
				 (DT_INST_PROP(inst, nordic_buck_mode0_pull_down) *                \
				  NPM6001_BUCKMODEPADCONF_BUCKMODE0PULLD_ENABLED) |                \
				 (DT_INST_PROP(inst, nordic_buck_mode1_pull_down) *                \
				  NPM6001_BUCKMODEPADCONF_BUCKMODE1PULLD_ENABLED) |                \
				 (DT_INST_PROP(inst, nordic_buck_mode2_pull_down) *                \
				  NPM6001_BUCKMODEPADCONF_BUCKMODE2PULLD_ENABLED)),                \
		.pad_val = ((DT_INST_PROP(inst, nordic_ready_high_drive) *                         \
			     NPM6001_PADDRIVESTRENGTH_READY_HIGH) |                                \
			    (DT_INST_PROP(inst, nordic_nint_high_drive) *                          \
			     NPM6001_PADDRIVESTRENGTH_NINT_HIGH) |                                 \
			    (DT_INST_PROP(inst, nordic_sda_high_drive) *                           \
			     NPM6001_PADDRIVESTRENGTH_SDA_HIGH)),                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_npm6001_init, NULL, NULL, &config##inst, POST_KERNEL,      \
			      CONFIG_MFD_NPM6001_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_NPM6001_DEFINE)
