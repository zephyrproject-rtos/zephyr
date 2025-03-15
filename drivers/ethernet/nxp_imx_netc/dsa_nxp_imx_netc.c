/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dsa_netc, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/ethernet.h>

#include "nxp_imx_netc.h"
#include "fsl_netc_switch.h"

#define DT_DRV_COMPAT nxp_netc_switch
#define PRV_DATA(ctx) ((struct dsa_netc_data *const)(ctx)->prv_data)

struct dsa_netc_data {
	swt_config_t swt_config;
	swt_handle_t swt_handle;
	netc_cmd_bd_t *cmd_bd;
};

static int dsa_netc_sw_write_reg(const struct device *dev, uint16_t reg_addr, uint8_t value)
{
	/* This API is for PHY switch. Not available here. */
	return -ENOTSUP;
}

static int dsa_netc_sw_read_reg(const struct device *dev, uint16_t reg_addr, uint8_t *value)
{
	/* This API is for PHY switch. Not available here. */
	return -ENOTSUP;
}

static int dsa_netc_set_mac_table_entry(const struct device *dev, const uint8_t *mac,
					uint8_t fw_port, uint16_t tbl_entry_idx, uint16_t flags)
{
	/* TODO: support it */
	return -ENOTSUP;
}

static int dsa_netc_get_mac_table_entry(const struct device *dev, uint8_t *buf,
					uint16_t tbl_entry_idx)
{
	/* TODO: support it */
	return -ENOTSUP;
}

static int dsa_netc_port_init(const struct device *dev)
{
	const struct dsa_port_config *cfg = dev->config;
	struct dsa_context *dsa_ctx = dev->data;
	struct dsa_netc_data *prv = PRV_DATA(dsa_ctx);
	swt_config_t *swt_config = &prv->swt_config;

	/* Get default config when beginning to init the first port */
	if (dsa_ctx->init_ports == 1) {
		SWT_GetDefaultConfig(swt_config);
		swt_config->bridgeCfg.dVFCfg.portMembership = 0x0;
	}

	/* miiSpeed and miiDuplex will get set correctly when link is up */
	swt_config->ports[cfg->port_idx].ethMac.miiMode =
		(strcmp(cfg->phy_mode, "mii") == 0
			 ? kNETC_MiiMode
			 : (strcmp(cfg->phy_mode, "rmii") == 0
				    ? kNETC_RmiiMode
				    : (strcmp(cfg->phy_mode, "rgmii") == 0
					       ? kNETC_RgmiiMode
					       : (strcmp(cfg->phy_mode, "gmii") == 0
							  ? kNETC_GmiiMode
							  : kNETC_RmiiMode))));
	swt_config->bridgeCfg.dVFCfg.portMembership |= (1 << cfg->port_idx);
	swt_config->ports[cfg->port_idx].bridgeCfg.enMacStationMove = true;

	return 0;
}

static int dsa_netc_switch_setup(const struct dsa_context *dsa_ctx)
{
	struct dsa_netc_data *prv = PRV_DATA(dsa_ctx);
	swt_config_t *swt_config = &prv->swt_config;
	status_t result;

	swt_config->cmdRingUse = 1U;
	swt_config->cmdBdrCfg[0].bdBase = prv->cmd_bd;
	swt_config->cmdBdrCfg[0].bdLength = 8U;

	result = SWT_Init(&prv->swt_handle, &prv->swt_config);
	if (result != kStatus_Success) {
		return -EIO;
	}

	return 0;
}

static void dsa_netc_port_phylink_change(const struct device *phydev, struct phy_link_state *state,
					 void *user_data)
{
	const struct device *dev = (struct device *)user_data;
	struct net_if *iface = net_if_lookup_by_dev(dev);
	const struct dsa_port_config *cfg = dev->config;
	struct dsa_context *dsa_ctx = dev->data;
	struct dsa_netc_data *prv = PRV_DATA(dsa_ctx);
	status_t result;

	if (state->is_up) {
		LOG_INF("DSA slave port %d Link up", cfg->port_idx);
		result = SWT_SetEthPortMII(&prv->swt_handle, cfg->port_idx,
					   PHY_TO_NETC_SPEED(state->speed),
					   PHY_TO_NETC_DUPLEX_MODE(state->speed));
		if (result != kStatus_Success) {
			LOG_ERR("DSA slave port %d failed to set MAC up", cfg->port_idx);
		}
		net_eth_carrier_on(iface);
	} else {
		LOG_INF("DSA slave port %d Link down", cfg->port_idx);
		net_eth_carrier_off(iface);
	}
}

static struct dsa_api dsa_netc_api = {
	.switch_read = dsa_netc_sw_read_reg,
	.switch_write = dsa_netc_sw_write_reg,
	.switch_set_mac_table_entry = dsa_netc_set_mac_table_entry,
	.switch_get_mac_table_entry = dsa_netc_get_mac_table_entry,
	.port_init = dsa_netc_port_init,
	.switch_setup = dsa_netc_switch_setup,
	.port_phylink_change = dsa_netc_port_phylink_change,

};

#define DSA_NETC_DEVICE(n)                                                                         \
	AT_NONCACHEABLE_SECTION_ALIGN(static netc_cmd_bd_t dsa_netc_##n##_cmd_bd[8],               \
				      NETC_BD_ALIGN);                                              \
	static struct dsa_netc_data dsa_netc_data_prv_##n = {                                      \
		.cmd_bd = dsa_netc_##n##_cmd_bd,                                                   \
	};                                                                                         \
	DSA_INIT_INSTANCE(n, &dsa_netc_api, &dsa_netc_data_prv_##n);

DT_INST_FOREACH_STATUS_OKAY(DSA_NETC_DEVICE);
