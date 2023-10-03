/*
 * Copyright 2023 Jonas Otto
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fcs_fusb302b_vbus

#include <zephyr/drivers/usb_c/usbc_vbus.h>
#include <sys/errno.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/drivers/usb_c/fusb302b.h>

struct fusb302_vbus_data {
};

struct fusb302_vbus_cfg {
	const struct device *fusb_dev;
};

static int fusb_vbus_init(const struct device *dev)
{
	return 0;
}

/**
 * @param[out] vbus_meas voltage in mV
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int fusb_measure(const struct device *dev, int *vbus_meas)
{
	const struct fusb302_vbus_cfg *const cfg = dev->config;

	return fusb302_measure_vbus(cfg->fusb_dev, vbus_meas);
}

static bool fusb_check_level(const struct device *dev, enum tc_vbus_level level)
{
	const struct fusb302_vbus_cfg *const cfg = dev->config;

	return fusb302_check_vbus_level(cfg->fusb_dev, level);
}

static int fusb_discharge(const struct device *dev, bool enable)
{
	return -ENOENT;
}

static int fusb_enable(const struct device *dev, bool enable)
{
	return 0;
}

static struct usbc_vbus_driver_api fusb302b_vbus_api = {.check_level = fusb_check_level,
							.measure = fusb_measure,
							.discharge = fusb_discharge,
							.enable = fusb_enable};

#define FUSB302B_VBUS_DEFINE(inst)                                                                 \
	static struct fusb302_vbus_data fusb302b_vbus_data_##inst;                                 \
                                                                                                   \
	static const struct fusb302_vbus_cfg fusb302b_vbus_config_##inst = {                       \
		.fusb_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, fusb302b))};                       \
	DEVICE_DT_INST_DEFINE(inst, fusb_vbus_init, /* pm_device = */ NULL,                        \
			      &fusb302b_vbus_data_##inst, &fusb302b_vbus_config_##inst,            \
			      /* level = */ POST_KERNEL, CONFIG_USBC_VBUS_INIT_PRIORITY,           \
			      &fusb302b_vbus_api);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible FUSB302B-VBUS instance found");

DT_INST_FOREACH_STATUS_OKAY(FUSB302B_VBUS_DEFINE)
