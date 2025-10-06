/*
 * Copyright (c) 2025 Renesas Electronics Corporation
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
#include <zephyr/drivers/ethernet/eth_renesas_ra_rmac.h>
#include <zephyr/net/mdio.h>
#include <zephyr/drivers/gpio.h>
#include "r_rmac_phy.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(renesas_ra_mdio, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_ra_mdio_rmac

struct mdio_renesas_ra_config {
	const struct device *eswclk_dev;
	const struct pinctrl_dev_config *pincfg;
};

struct mdio_renesas_ra_data {
	struct k_mutex rw_lock;
	rmac_phy_instance_ctrl_t fsp_ctrl;
	ether_phy_cfg_t fsp_cfg;
};

static int mdio_renesas_ra_get_port(const struct device *dev, uint8_t prtad, uint8_t *port)
{
	struct mdio_renesas_ra_data *data = dev->data;
	rmac_phy_extended_cfg_t *p_extend =
		(rmac_phy_extended_cfg_t *)data->fsp_ctrl.p_ether_phy_cfg->p_extend;

	for (int i = 0; i < BSP_FEATURE_ETHER_NUM_CHANNELS; i++) {
		if (p_extend->p_phy_lsi_cfg_list[i]->address == prtad) {
			*port = i;
			return 0;
		}
	}

	return -ENXIO;
}

static int mdio_renesas_ra_read(const struct device *dev, uint8_t prtad, uint8_t regad,
				uint16_t *regval)
{
	struct mdio_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	uint32_t val = 0;
	uint8_t port;
	int ret;

	ret = r_rmac_phy_get_operation_mode(&data->fsp_ctrl);
	if (ret != RENESAS_RA_ETHA_OPERATION_MODE) {
		return -EACCES;
	}

	ret = mdio_renesas_ra_get_port(dev, prtad, &port);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->rw_lock, K_FOREVER);

	R_RMAC_PHY_ChipSelect(&data->fsp_ctrl, port);
	fsp_err = R_RMAC_PHY_Read(&data->fsp_ctrl, regad, &val);
	ret = (fsp_err == FSP_SUCCESS) ? 0 : -EIO;

	*regval = val & UINT16_MAX;

	k_mutex_unlock(&data->rw_lock);

	return ret;
}

static int mdio_renesas_ra_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				 uint16_t regval)
{
	struct mdio_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	uint8_t port;
	int ret;

	ret = r_rmac_phy_get_operation_mode(&data->fsp_ctrl);
	if (ret != RENESAS_RA_ETHA_OPERATION_MODE) {
		return -EACCES;
	}

	ret = mdio_renesas_ra_get_port(dev, prtad, &port);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->rw_lock, K_FOREVER);

	R_RMAC_PHY_ChipSelect(&data->fsp_ctrl, port);
	fsp_err = R_RMAC_PHY_Write(&data->fsp_ctrl, regad, (uint32_t)regval);
	ret = (fsp_err == FSP_SUCCESS) ? 0 : -EIO;

	k_mutex_unlock(&data->rw_lock);

	return ret;
}

static int mdio_renesas_ra_initialize(const struct device *dev)
{
	struct mdio_renesas_ra_data *data = dev->data;
	const struct mdio_renesas_ra_config *config = dev->config;
	rmac_phy_extended_cfg_t *p_extend = (rmac_phy_extended_cfg_t *)data->fsp_cfg.p_extend;
	uint32_t eswclk;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(config->eswclk_dev, NULL, &eswclk);
	if (ret < 0) {
		return ret;
	}

	p_extend->mdio_hold_time =
		DIV_ROUND_UP(eswclk * CONFIG_MDIO_RENESAS_RA_RMAC_MDIO_HOLD_NS, 1e9);
	p_extend->mdio_capture_time =
		DIV_ROUND_UP(eswclk * CONFIG_MDIO_RENESAS_RA_RMAC_MDIO_CAPTURE_NS, 1e9);

	fsp_err = R_RMAC_PHY_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to init mdio driver - R_RMAC_PHY_Open fail, err=%d", fsp_err);
		return -EIO;
	}

	k_mutex_init(&data->rw_lock);

	return 0;
}

static DEVICE_API(mdio, mdio_renesas_ra_api) = {
	.read = mdio_renesas_ra_read,
	.write = mdio_renesas_ra_write,
};

#define PHY_CONFIG_DEFINE(id)                                                                      \
	static ether_phy_lsi_cfg_t phy_cfg_list##id = {.address = DT_REG_ADDR(id),                 \
						       .type = ETHER_PHY_LSI_TYPE_CUSTOM};
#define PHY_CONFIG_PTR_DECLARE(id) &phy_cfg_list##id,

#define PHY_CONNECTION_TYPE(n)                                                                     \
	((DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rgmii)                                    \
		  ? ETHER_PHY_MII_TYPE_RGMII                                                       \
		  : (DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, gmii)                          \
			     ? ETHER_PHY_MII_TYPE_GMII                                             \
			     : (DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rmii)               \
					? ETHER_PHY_MII_TYPE_RMII                                  \
					: ETHER_PHY_MII_TYPE_MII))))

#define RENSAS_RA_MDIO_RMAC_DEFINE(n)                                                              \
	BUILD_ASSERT(DT_INST_CHILD_NUM(n) <= BSP_FEATURE_ETHER_NUM_CHANNELS);                      \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PHY_CONFIG_DEFINE);                                   \
                                                                                                   \
	static rmac_phy_extended_cfg_t renesas_ra_mdio##n##_p_extend = {                           \
		.frame_format = RMAC_PHY_FRAME_FORMAT_MDIO,                                        \
		.mdc_clock_rate = DT_INST_PROP(n, clock_frequency),                                \
		.p_phy_lsi_cfg_list = {                                                            \
			DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PHY_CONFIG_PTR_DECLARE)}};            \
                                                                                                   \
	static struct mdio_renesas_ra_data renesas_ra_mdio##n##_data = {                           \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.channel = DT_INST_PROP(n, channel),                               \
				.flow_control = ETHER_PHY_FLOW_CONTROL_DISABLE,                    \
				.mii_type = PHY_CONNECTION_TYPE(n),                                \
				.p_extend = &renesas_ra_mdio##n##_p_extend,                        \
			},                                                                         \
	};                                                                                         \
	static const struct mdio_renesas_ra_config renesas_ra_mdio##n##_cfg = {                    \
		.eswclk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(DT_INST_PARENT(n), eswclk)),    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &mdio_renesas_ra_initialize, NULL, &renesas_ra_mdio##n##_data,    \
			      &renesas_ra_mdio##n##_cfg, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &mdio_renesas_ra_api);

DT_INST_FOREACH_STATUS_OKAY(RENSAS_RA_MDIO_RMAC_DEFINE)
