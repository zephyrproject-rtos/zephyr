/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/mdio.h>
#include "r_ether_phy.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(renesas_ra_mdio, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_ra_mdio

struct renesas_ra_mdio_config {
	const struct pinctrl_dev_config *pincfg;
	uint8_t instance;
};

struct renesas_ra_mdio_data {
	struct k_mutex rw_mutex;
	struct st_ether_phy_cfg ether_phy_cfg;
	struct st_ether_phy_instance_ctrl ether_phy_ctrl;
};

static int renesas_ra_mdio_read(const struct device *dev, uint8_t prtad, uint8_t regad,
				uint16_t *data)
{
	struct renesas_ra_mdio_data *dev_data = dev->data;
	uint32_t read;
	fsp_err_t err;

	dev_data->ether_phy_ctrl.phy_lsi_address = prtad;

	k_mutex_lock(&dev_data->rw_mutex, K_FOREVER);

	err = R_ETHER_PHY_Read(&dev_data->ether_phy_ctrl, regad, &read);

	k_mutex_unlock(&dev_data->rw_mutex);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	*data = read & UINT16_MAX;

	return 0;
}

static int renesas_ra_mdio_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				 uint16_t data)
{
	struct renesas_ra_mdio_data *dev_data = dev->data;
	fsp_err_t err;

	dev_data->ether_phy_ctrl.phy_lsi_address = prtad;

	k_mutex_lock(&dev_data->rw_mutex, K_FOREVER);

	err = R_ETHER_PHY_Write(&dev_data->ether_phy_ctrl, regad, data);

	k_mutex_unlock(&dev_data->rw_mutex);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int renesas_ra_mdio_initialize(const struct device *dev)
{
	struct renesas_ra_mdio_data *data = dev->data;
	const struct renesas_ra_mdio_config *cfg = dev->config;
	int err;
	fsp_err_t fsp_err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	fsp_err = R_ETHER_PHY_Open(&data->ether_phy_ctrl, &data->ether_phy_cfg);

	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to init mdio driver - R_ETHER_PHY_Open fail");
	}

	k_mutex_init(&data->rw_mutex);

	return 0;
}

static DEVICE_API(mdio, renesas_ra_mdio_api) = {
	.read = renesas_ra_mdio_read,
	.write = renesas_ra_mdio_write,
};

#define RENSAS_RA_MDIO_INSTANCE_DEFINE(node)                                                       \
	PINCTRL_DT_INST_DEFINE(node);                                                              \
	static struct renesas_ra_mdio_data renesas_ra_mdio##node##_data = {                        \
		.ether_phy_cfg = {                                                                 \
			.channel = 0,                                                              \
			.phy_reset_wait_time = 0x00020000,                                         \
			.mii_bit_access_wait_time = 8,                                             \
			.phy_lsi_type = ETHER_PHY_LSI_TYPE_CUSTOM,                                 \
			.flow_control = ETHER_PHY_FLOW_CONTROL_DISABLE,                            \
		}};                                                                                \
	static const struct renesas_ra_mdio_config renesas_ra_mdio##node##_cfg = {                 \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(node)};                                   \
	DEVICE_DT_INST_DEFINE(node, &renesas_ra_mdio_initialize, NULL,                             \
			      &renesas_ra_mdio##node##_data, &renesas_ra_mdio##node##_cfg,         \
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY, &renesas_ra_mdio_api);

DT_INST_FOREACH_STATUS_OKAY(RENSAS_RA_MDIO_INSTANCE_DEFINE)
