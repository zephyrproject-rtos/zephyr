/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_netc_emdio

#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#include "fsl_netc_mdio.h"
LOG_MODULE_REGISTER(nxp_imx_netc_emdio, CONFIG_MDIO_LOG_LEVEL);

struct nxp_imx_netc_mdio_config {
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct nxp_imx_netc_mdio_data {
	struct k_mutex rw_mutex;
	netc_mdio_handle_t handle;
};

static int nxp_imx_netc_mdio_read(const struct device *dev, uint8_t prtad, uint8_t regad,
				  uint16_t *regval)
{
	struct nxp_imx_netc_mdio_data *data = dev->data;
	status_t result;

	k_mutex_lock(&data->rw_mutex, K_FOREVER);
	result = NETC_MDIORead(&data->handle, prtad, regad, regval);
	k_mutex_unlock(&data->rw_mutex);

	return result == kStatus_Success ? 0 : -EIO;
}

static int nxp_imx_netc_mdio_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				   uint16_t regval)
{
	struct nxp_imx_netc_mdio_data *data = dev->data;
	status_t result;

	k_mutex_lock(&data->rw_mutex, K_FOREVER);
	result = NETC_MDIOWrite(&data->handle, prtad, regad, regval);
	k_mutex_unlock(&data->rw_mutex);

	return result == kStatus_Success ? 0 : -EIO;
}

static int nxp_imx_netc_mdio_initialize(const struct device *dev)
{
	struct nxp_imx_netc_mdio_data *data = dev->data;
	const struct nxp_imx_netc_mdio_config *cfg = dev->config;
	netc_mdio_config_t mdio_config = {0};
	status_t result;
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	k_mutex_init(&data->rw_mutex);

	err = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &mdio_config.srcClockHz);
	if (err) {
		return err;
	}

	result = NETC_MDIOInit(&data->handle, &mdio_config);
	if (result != kStatus_Success) {
		return -EIO;
	}

	return 0;
}

static const struct mdio_driver_api nxp_imx_netc_mdio_api = {
	.read = nxp_imx_netc_mdio_read,
	.write = nxp_imx_netc_mdio_write,
};

#define NXP_IMX_NETC_MDIO_INSTANCE_DEFINE(n)                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct nxp_imx_netc_mdio_data nxp_imx_netc_mdio##n##_data;                          \
	static const struct nxp_imx_netc_mdio_config nxp_imx_netc_mdio##n##_cfg = {                \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &nxp_imx_netc_mdio_initialize, NULL,                              \
			      &nxp_imx_netc_mdio##n##_data, &nxp_imx_netc_mdio##n##_cfg,           \
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY, &nxp_imx_netc_mdio_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_IMX_NETC_MDIO_INSTANCE_DEFINE)
