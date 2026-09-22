/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_elc

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/misc/interconn/renesas_elc/renesas_elc.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <soc.h>
#include <r_elc.h>

struct renesas_ra_elc_config {
	const elc_cfg_t fsp_cfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
};

struct renesas_ra_elc_data {
	elc_instance_ctrl_t fsp_ctrl;
};

static int renesas_ra_elc_software_event_generate(const struct device *dev, uint32_t event)
{
	struct renesas_ra_elc_data *data = (struct renesas_ra_elc_data *const)(dev)->data;
	fsp_err_t err;

	err = R_ELC_SoftwareEventGenerate(&data->fsp_ctrl, event);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int renesas_ra_elc_link_set(const struct device *dev, uint32_t peripheral, uint32_t event)
{
	struct renesas_ra_elc_data *data = (struct renesas_ra_elc_data *const)(dev)->data;
	fsp_err_t err;

	if (BIT(peripheral) & BSP_ELC_PERIPHERAL_MASK) {
		err = R_ELC_LinkSet(&data->fsp_ctrl, peripheral, event);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int renesas_ra_elc_link_break(const struct device *dev, uint32_t peripheral)
{
	struct renesas_ra_elc_data *data = (struct renesas_ra_elc_data *const)(dev)->data;
	fsp_err_t err;

	if (BIT(peripheral) & BSP_ELC_PERIPHERAL_MASK) {
		err = R_ELC_LinkBreak(&data->fsp_ctrl, peripheral);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int renesas_ra_elc_enable(const struct device *dev)
{
	struct renesas_ra_elc_data *data = (struct renesas_ra_elc_data *const)(dev)->data;
	fsp_err_t err;

	err = R_ELC_Enable(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int renesas_ra_elc_disable(const struct device *dev)
{
	struct renesas_ra_elc_data *data = (struct renesas_ra_elc_data *const)(dev)->data;
	fsp_err_t err;

	err = R_ELC_Disable(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int renesas_ra_elc_init(const struct device *dev)
{
	struct renesas_ra_elc_data *data = (struct renesas_ra_elc_data *const)(dev)->data;
	struct renesas_ra_elc_config *cfg = (struct renesas_ra_elc_config *const)(dev)->config;
	fsp_err_t err;
	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	err = R_ELC_Open(&data->fsp_ctrl, &cfg->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static DEVICE_API(renesas_elc, renesas_ra_elc_driver_api) = {
	.software_event_generate = renesas_ra_elc_software_event_generate,
	.link_set = renesas_ra_elc_link_set,
	.link_break = renesas_ra_elc_link_break,
	.enable = renesas_ra_elc_enable,
	.disable = renesas_ra_elc_disable,
};

#define RA_ELC_INIT(inst)                                                                          \
	static const struct renesas_ra_elc_config renesas_ra_elc_config_##inst = {                 \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.link = {0},                                                       \
				.p_extend = NULL,                                                  \
			},                                                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, mstp),       \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, stop_bit),         \
			},                                                                         \
	};                                                                                         \
	static struct renesas_ra_elc_data renesas_ra_elc_data_##inst;                              \
	DEVICE_DT_INST_DEFINE(inst, renesas_ra_elc_init, NULL, &renesas_ra_elc_data_##inst,        \
			      &renesas_ra_elc_config_##inst, PRE_KERNEL_1,                         \
			      CONFIG_RENESAS_ELC_INIT_PRIORITY, &renesas_ra_elc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RA_ELC_INIT)
