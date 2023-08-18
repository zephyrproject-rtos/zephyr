/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_emdio, CONFIG_MDIO_LOG_LEVEL);

#include <Netc_EthSwt_Ip.h>

#define MDIO_NODE	DT_NODELABEL(emdio)
#define NETC_SWT_IDX	0

struct nxp_s32_mdio_config {
	const struct pinctrl_dev_config *pincfg;
};

struct nxp_s32_mdio_data {
	struct k_mutex rw_mutex;
};

static int nxp_s32_mdio_read(const struct device *dev, uint8_t prtad,
			     uint8_t regad, uint16_t *regval)
{
	struct nxp_s32_mdio_data *data = dev->data;
	Std_ReturnType status;

	k_mutex_lock(&data->rw_mutex, K_FOREVER);
	status = Netc_EthSwt_Ip_ReadTrcvRegister(NETC_SWT_IDX, prtad, regad, regval);
	k_mutex_unlock(&data->rw_mutex);

	return status == E_OK ? 0 : -EIO;
}

static int nxp_s32_mdio_write(const struct device *dev, uint8_t prtad,
			      uint8_t regad, uint16_t regval)
{
	struct nxp_s32_mdio_data *data = dev->data;
	Std_ReturnType status;

	k_mutex_lock(&data->rw_mutex, K_FOREVER);
	status = Netc_EthSwt_Ip_WriteTrcvRegister(NETC_SWT_IDX, prtad, regad, regval);
	k_mutex_unlock(&data->rw_mutex);

	return status == E_OK ? 0 : -EIO;
}

static int nxp_s32_mdio_initialize(const struct device *dev)
{
	struct nxp_s32_mdio_data *data = dev->data;
	const struct nxp_s32_mdio_config *cfg = dev->config;
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	k_mutex_init(&data->rw_mutex);

	return 0;
}

static void nxp_s32_mdio_noop(const struct device *dev)
{
	/* intentionally left empty */
}

static const struct mdio_driver_api nxp_s32_mdio_api = {
	.read = nxp_s32_mdio_read,
	.write = nxp_s32_mdio_write,
	/* NETC does not support enabling/disabling EMDIO controller independently */
	.bus_enable = nxp_s32_mdio_noop,
	.bus_disable = nxp_s32_mdio_noop,
};

PINCTRL_DT_DEFINE(MDIO_NODE);

static struct nxp_s32_mdio_data nxp_s32_mdio0_data;

static const struct nxp_s32_mdio_config nxp_s32_mdio0_cfg = {
	.pincfg = PINCTRL_DT_DEV_CONFIG_GET(MDIO_NODE),
};

DEVICE_DT_DEFINE(MDIO_NODE,
		 &nxp_s32_mdio_initialize,
		 NULL,
		 &nxp_s32_mdio0_data,
		 &nxp_s32_mdio0_cfg,
		 POST_KERNEL,
		 CONFIG_MDIO_INIT_PRIORITY,
		 &nxp_s32_mdio_api);
