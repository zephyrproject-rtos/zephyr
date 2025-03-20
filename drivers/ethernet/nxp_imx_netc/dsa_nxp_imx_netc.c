/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dsa_netc, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/pinctrl.h>

#include "../eth.h"
#include "nxp_imx_netc.h"
#include "fsl_netc_switch.h"

#define DT_DRV_COMPAT nxp_netc_switch
#define PRV_DATA(ctx) ((struct dsa_netc_data *const)(ctx)->prv_data)

struct dsa_netc_port_config {
	const struct pinctrl_dev_config *pincfg;
	netc_hw_mii_mode_t phy_mode;
};

struct dsa_netc_data {
	swt_config_t swt_config;
	swt_handle_t swt_handle;
	netc_cmd_bd_t *cmd_bd;
};

static int dsa_netc_port_init(const struct device *dev)
{
	const struct dsa_port_config *cfg = dev->config;
	struct dsa_netc_port_config *prv_cfg = cfg->prv_config;
	struct dsa_switch_context *dsa_switch_ctx = dev->data;
	struct dsa_netc_data *prv = PRV_DATA(dsa_switch_ctx);
	swt_config_t *swt_config = &prv->swt_config;
	int ret;

	if (prv_cfg->pincfg != NULL) {
		ret = pinctrl_apply_state(prv_cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
	}

	/* Get default config when beginning to init the first port */
	if (dsa_switch_ctx->init_ports == 1) {
		SWT_GetDefaultConfig(swt_config);
		swt_config->bridgeCfg.dVFCfg.portMembership = 0x0;
	}

	/* miiSpeed and miiDuplex will get set correctly when link is up */
	swt_config->ports[cfg->port_idx].ethMac.miiMode = prv_cfg->phy_mode;

	swt_config->bridgeCfg.dVFCfg.portMembership |= (1 << cfg->port_idx);
	swt_config->ports[cfg->port_idx].bridgeCfg.enMacStationMove = true;

	return 0;
}

static void dsa_netc_port_generate_random_mac(uint8_t *mac_addr)
{
	gen_random_mac(mac_addr, FREESCALE_OUI_B0, FREESCALE_OUI_B1, FREESCALE_OUI_B2);
}

static int dsa_netc_switch_setup(const struct dsa_switch_context *dsa_switch_ctx)
{
	struct dsa_netc_data *prv = PRV_DATA(dsa_switch_ctx);
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
	struct dsa_switch_context *dsa_switch_ctx = dev->data;
	struct dsa_netc_data *prv = PRV_DATA(dsa_switch_ctx);
	status_t result;

	if (state->is_up) {
		LOG_INF("DSA user port %d Link up", cfg->port_idx);
		result = SWT_SetEthPortMII(&prv->swt_handle, cfg->port_idx,
					   PHY_TO_NETC_SPEED(state->speed),
					   PHY_TO_NETC_DUPLEX_MODE(state->speed));
		if (result != kStatus_Success) {
			LOG_ERR("DSA user port %d failed to set MAC up", cfg->port_idx);
		}
		net_eth_carrier_on(iface);
	} else {
		LOG_INF("DSA user port %d Link down", cfg->port_idx);
		net_eth_carrier_off(iface);
	}
}

static struct dsa_api dsa_netc_api = {
	.port_init = dsa_netc_port_init,
	.port_generate_random_mac = dsa_netc_port_generate_random_mac,
	.switch_setup = dsa_netc_switch_setup,
	.port_phylink_change = dsa_netc_port_phylink_change,
};

#define DSA_NETC_PORT_INST_INIT(port, n)                                                    \
	COND_CODE_1(DT_NUM_PINCTRL_STATES(port),                                            \
			(PINCTRL_DT_DEFINE(port);), (EMPTY))                                \
	struct dsa_netc_port_config dsa_netc_##n##_##port##_config = {                      \
		.pincfg = COND_CODE_1(DT_NUM_PINCTRL_STATES(port),                          \
				(PINCTRL_DT_DEV_CONFIG_GET(port)), NULL),                   \
		.phy_mode = NETC_PHY_MODE(port),                                            \
	};                                                                                  \
	struct dsa_port_config dsa_##n##_##port##_config = {                                \
		.use_random_mac_addr = DT_NODE_HAS_PROP(port, zephyr_random_mac_address),   \
		.mac_addr = DT_PROP_OR(port, local_mac_address, {0}),                       \
		.port_idx = DT_REG_ADDR(port),                                              \
		.phy_dev = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port, phy_handle)),             \
		.phy_mode = DT_PROP_OR(port, phy_connection_type, ""),                      \
		.ethernet_connection = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port, ethernet)),   \
		.prv_config = &dsa_netc_##n##_##port##_config,                              \
	};                                                                                  \
	DSA_PORT_INST_INIT(port, n, &dsa_##n##_##port##_config)

#define DSA_NETC_DEVICE(n)                                                                  \
	AT_NONCACHEABLE_SECTION_ALIGN(static netc_cmd_bd_t dsa_netc_##n##_cmd_bd[8],        \
				      NETC_BD_ALIGN);                                       \
	static struct dsa_netc_data dsa_netc_data_##n = {                                   \
		.cmd_bd = dsa_netc_##n##_cmd_bd,                                            \
	};                                                                                  \
	DSA_SWITCH_INST_INIT(n, &dsa_netc_api, &dsa_netc_data_##n, DSA_NETC_PORT_INST_INIT);

DT_INST_FOREACH_STATUS_OKAY(DSA_NETC_DEVICE);
