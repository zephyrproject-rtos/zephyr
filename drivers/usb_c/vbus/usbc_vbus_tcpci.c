/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_usb_c_vbus_tcpci

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbc_vbus_tcpci, CONFIG_USBC_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/usb_c/tcpci_priv.h>
#include <zephyr/drivers/usb_c/usbc_vbus.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/usb_c/tcpci.h>

/** Configuration structure for device instances */
struct vbus_tcpci_cfg {
	/** I2C bus and address used for communication, set from parent node of device. */
	const struct i2c_dt_spec i2c;
};

static int tcpci_measure(const struct device *dev, int *vbus_meas)
{
	const struct vbus_tcpci_cfg *cfg = dev->config;
	uint16_t measure;
	int ret;

	__ASSERT(vbus_meas != NULL, "TCPCI VBUS meas must not be NULL");

	ret = tcpci_read_reg16(&cfg->i2c, TCPC_REG_VBUS_VOLTAGE, &measure);
	if (ret != 0) {
		return ret;
	}

	*vbus_meas = TCPC_REG_VBUS_VOLTAGE_VBUS(measure);

	return 0;
}

static bool tcpci_check_level(const struct device *dev, enum tc_vbus_level level)
{
	int measure;
	int ret;

	ret = tcpci_measure(dev, &measure);
	if (ret != 0) {
		return false;
	}

	switch (level) {
	case TC_VBUS_SAFE0V:
		return (measure < PD_V_SAFE_0V_MAX_MV);
	case TC_VBUS_PRESENT:
		return (measure >= PD_V_SAFE_5V_MIN_MV);
	case TC_VBUS_REMOVED:
		return (measure < TC_V_SINK_DISCONNECT_MAX_MV);
	}

	return false;
}

static int tcpci_discharge(const struct device *dev, bool enable)
{
	const struct vbus_tcpci_cfg *cfg = dev->config;

	return tcpci_update_reg8(&cfg->i2c, TCPC_REG_POWER_CTRL,
				 TCPC_REG_POWER_CTRL_FORCE_DISCHARGE,
				 (enable) ? TCPC_REG_POWER_CTRL_FORCE_DISCHARGE : 0);
}

static int tcpci_enable(const struct device *dev, bool enable)
{
	const struct vbus_tcpci_cfg *cfg = dev->config;

	return tcpci_update_reg8(&cfg->i2c, TCPC_REG_POWER_CTRL,
				 TCPC_REG_POWER_CTRL_VBUS_VOL_MONITOR_DIS,
				 (enable) ? 0 : TCPC_REG_POWER_CTRL_VBUS_VOL_MONITOR_DIS);
}

static DEVICE_API(usbc_vbus, vbus_tcpci_api) = {
	.measure = tcpci_measure,
	.check_level = tcpci_check_level,
	.discharge = tcpci_discharge,
	.enable = tcpci_enable,
};

static int tcpci_init(const struct device *dev)
{
	return 0;
}

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible USB-C VBUS Measurement instance found");

#define VBUS_TCPCI_INIT_CFG(node)                                                                  \
	{                                                                                          \
		.i2c = {I2C_DT_SPEC_GET_ON_I2C(DT_PARENT(node))},                                  \
	}

#define DRIVER_INIT(inst)                                                                          \
	static const struct vbus_tcpci_cfg drv_config_##inst =                                     \
		VBUS_TCPCI_INIT_CFG(DT_DRV_INST(inst));                                            \
	DEVICE_DT_INST_DEFINE(inst, &tcpci_init, NULL, NULL, &drv_config_##inst, POST_KERNEL,      \
			      CONFIG_USBC_VBUS_INIT_PRIORITY, &vbus_tcpci_api);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_INIT)
