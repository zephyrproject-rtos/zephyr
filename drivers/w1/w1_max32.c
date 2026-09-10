/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/w1.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>

#include <wrap_max32_owm.h>

#define DT_DRV_COMPAT adi_max32_w1

LOG_MODULE_REGISTER(w1_max32, CONFIG_W1_LOG_LEVEL);

struct max32_w1_config {
	struct w1_master_config w1_config;
	mxc_owm_regs_t *regs;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	struct max32_perclk perclk;
	uint8_t internal_pullup;
	uint8_t external_pullup;
	uint8_t long_line_mode;
};

struct max32_w1_data {
	struct w1_master_data w1_data;
	uint8_t reg_device_config;
};

static int api_reset_bus(const struct device *dev)
{
	int ret;
	const struct max32_w1_config *const cfg = dev->config;
	mxc_owm_regs_t *regs = cfg->regs;

	/* 0 if no 1-wire devices responded during the presence pulse, 1 otherwise */
	ret = MXC_OWM_Reset();
	if (ret > 0) {
		/* Check OW pin input state due to presence detect pin seems not work well */
		if (regs->ctrl_stat & MXC_F_OWM_CTRL_STAT_OW_INPUT) {
			ret = 1; /* At least 1 device on the line */
		} else {
			ret = 0; /* no device on the line */
		}
	}

	return ret;
}

static int api_read_bit(const struct device *dev)
{
	int ret;

	ret = MXC_OWM_ReadBit();
	if (ret < 0) {
		if (MXC_OWM_GetPresenceDetect() == 0) {
			/* if no slave connected to the bus, read bits shall be logical ones */
			ret = 1;
		} else {
			return -EIO;
		}
	}

	return ret;
}

static int api_write_bit(const struct device *dev, bool bit)
{
	int ret;

	ret = MXC_OWM_WriteBit(bit);
	if (ret < 0) {
		if (MXC_OWM_GetPresenceDetect() == 0) {
			/* if no slave connected to the bus, write shall success */
			ret = 0;
		} else {
			return -EIO;
		}
	}

	return ret;
}

static int api_read_byte(const struct device *dev)
{
	int ret;

	ret = MXC_OWM_ReadByte();
	if (ret < 0) {
		if (MXC_OWM_GetPresenceDetect() == 0) {
			/* if no slave connected to the bus, read bits shall be logical ones */
			ret = 0xff;
		} else {
			return -EIO;
		}
	}

	return ret;
}

static int api_write_byte(const struct device *dev, uint8_t byte)
{
	int ret;

	ret = MXC_OWM_WriteByte(byte);
	if (ret < 0) {
		if (MXC_OWM_GetPresenceDetect() == 0) {
			/* if no slave connected to the bus, write shall success */
			ret = 0;
		} else {
			return -EIO;
		}
	}

	return ret;
}

static int api_configure(const struct device *dev, enum w1_settings_type type, uint32_t value)
{
	int ret = 0;

	switch (type) {
	case W1_SETTING_SPEED:
		MXC_OWM_SetOverdrive(value);
		break;
	case W1_SETTING_STRONG_PULLUP:
		const struct max32_w1_config *const dev_config = dev->config;
		mxc_owm_regs_t *regs = dev_config->regs;

		if (value == 1) {
			regs->cfg |= MXC_F_OWM_CFG_EXT_PULLUP_MODE;
		} else {
			regs->cfg &= ~MXC_F_OWM_CFG_EXT_PULLUP_MODE;
		}
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int w1_max32_init(const struct device *dev)
{
	int ret;
	const struct max32_w1_config *const cfg = dev->config;
	mxc_owm_cfg_t mxc_owm_cfg;

	if (!device_is_ready(cfg->clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	MXC_OWM_Shutdown();

	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
	if (ret != 0) {
		LOG_ERR("cannot enable OWM clock");
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	mxc_owm_cfg.int_pu_en = cfg->internal_pullup;
	mxc_owm_cfg.ext_pu_mode = cfg->external_pullup;
	mxc_owm_cfg.long_line_mode = cfg->long_line_mode;

	ret = Wrap_MXC_OWM_Init(&mxc_owm_cfg);

	return ret;
}

static DEVICE_API(w1, w1_max32_driver_api) = {
	.reset_bus = api_reset_bus,
	.read_bit = api_read_bit,
	.write_bit = api_write_bit,
	.read_byte = api_read_byte,
	.write_byte = api_write_byte,
	.configure = api_configure,
};

#define MAX32_W1_INIT(_num)                                                                        \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	static const struct max32_w1_config max32_w1_config_##_num = {                             \
		.w1_config.slave_count = W1_INST_SLAVE_COUNT(_num),                                \
		.regs = (mxc_owm_regs_t *)DT_INST_REG_ADDR(_num),                                  \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
		.internal_pullup = DT_INST_PROP(_num, internal_pullup),                            \
		.external_pullup = DT_INST_PROP_OR(_num, external_pullup, 0),                      \
		.long_line_mode = DT_INST_PROP(_num, long_line_mode),                              \
	};                                                                                         \
	static struct max32_w1_data max32_owm_data##_num;                                          \
	DEVICE_DT_INST_DEFINE(_num, w1_max32_init, NULL, &max32_owm_data##_num,                    \
			      &max32_w1_config_##_num, POST_KERNEL, CONFIG_W1_INIT_PRIORITY,       \
			      &w1_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX32_W1_INIT)
