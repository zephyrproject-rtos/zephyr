/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME dsa_netc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/linker/sections.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/phy.h>
#include <zephyr/devicetree.h>

#include "fsl_netc_switch.h"

#define DT_DRV_COMPAT nxp_imx_netc_dsa
#include "dsa_nxp_imx_netc.h"

#define PRV_DATA(ctx) ((struct dsa_netc_data *const)(ctx)->prv_data)

struct dsa_netc_data {
	int port_num;
	int port_init_count;
	int iface_init_count;
	swt_config_t swt_config;
	swt_handle_t swt_handle;
	const struct device *dev_master;
	netc_cmd_bd_t *cmd_bd;
};

struct dsa_netc_slave_config {
	/** MAC address for each LAN{123.,} ports */
	uint8_t mac_addr[6];
	const struct device *phy_dev;
	netc_hw_mii_mode_t phy_mode;
	volatile bool pseudo_mac;
	const struct pinctrl_dev_config *pincfg;
	int port_idx;
};

int dsa_netc_port_init(const struct device *dev)
{
	const struct dsa_netc_slave_config *cfg = dev->config;
	struct dsa_context *ctx = dev->data;
	struct dsa_netc_data *prv = PRV_DATA(ctx);
	swt_config_t *swt_config = &prv->swt_config;

	status_t result;
	int err;

	/* Get default config for whole switch before first port init */
	if (prv->port_init_count == 0) {
		SWT_GetDefaultConfig(swt_config);
		swt_config->bridgeCfg.dVFCfg.portMembership = 0x0;
	}

	prv->port_init_count++;

	if (!cfg->pseudo_mac) {
		err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (err) {
			return err;
		}
	}

	/* miiSpeed and miiDuplex will get set correctly when link is up */
	swt_config->ports[cfg->port_idx].ethMac.miiMode = cfg->phy_mode;

	swt_config->bridgeCfg.dVFCfg.portMembership |= (1 << cfg->port_idx);

	swt_config->ports[cfg->port_idx].bridgeCfg.enMacStationMove = true;

	/* Switch init after all ports init */
	if (prv->port_init_count == prv->port_num) {
		swt_config->cmdRingUse = 1U;
		swt_config->cmdBdrCfg[0].bdBase = prv->cmd_bd;
		swt_config->cmdBdrCfg[0].bdLength = 8U;

		result = SWT_Init(&prv->swt_handle, &prv->swt_config);
		if (result != kStatus_Success) {
			return -EIO;
		}
	}

	return 0;
}

static int dsa_netc_sw_write_reg(const struct device *dev, uint16_t reg_addr, uint8_t value)
{
	/* This API is for PHY switch. Not available here. */
	return 0;
}

static int dsa_netc_sw_read_reg(const struct device *dev, uint16_t reg_addr, uint8_t *value)
{
	/* This API is for PHY switch. Not available here. */
	return 0;
}

/**
 * @brief Set entry to DSA MAC address table
 *
 * @param dev DSA device
 * @param mac The MAC address to be set in the table
 * @param fw_port Port number to forward packets
 * @param tbl_entry_idx The index of entry in the table
 * @param flags Flags to be set in the entry
 *
 * @return 0 if ok, < 0 if error
 */
static int dsa_netc_set_mac_table_entry(const struct device *dev, const uint8_t *mac,
					uint8_t fw_port, uint16_t tbl_entry_idx, uint16_t flags)
{
	return 0;
}

/**
 * @brief Get DSA MAC address table entry
 *
 * @param dev DSA device
 * @param buf The buffer for data read from the table
 * @param tbl_entry_idx The index of entry in the table
 *
 * @return 0 if ok, < 0 if error
 */
static int dsa_netc_get_mac_table_entry(const struct device *dev, uint8_t *buf,
					uint16_t tbl_entry_idx)
{
	return 0;
}

static void netc_eth_phylink_callback(const struct device *dev, struct phy_link_state *state,
				      void *user_data)
{
	const struct device *ndev = (struct device *)user_data;
	struct dsa_context *context = ndev->data;
	struct dsa_netc_data *prv = PRV_DATA(context);

	struct net_if *iface = net_if_lookup_by_dev(ndev);
	struct ethernet_context *ctx = net_if_l2_data(iface);
	status_t result;

	if (state->is_up) {
		LOG_INF("DSA slave port %d Link up", ctx->dsa_port_idx);
		result = SWT_SetEthPortMII(&prv->swt_handle, ctx->dsa_port_idx,
					   PHY_TO_NETC_SPEED(state->speed),
					   PHY_TO_NETC_DUPLEX_MODE(state->speed));
		if (result != kStatus_Success) {
			LOG_ERR("DSA slave port %d failed to set MAC up", ctx->dsa_port_idx);
		}
		net_eth_carrier_on(iface);
	} else {
		LOG_INF("DSA slave port %d Link down", ctx->dsa_port_idx);
		net_eth_carrier_off(iface);
	}
}

static void dsa_netc_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_netc_slave_config *cfg = (struct dsa_netc_slave_config *)dev->config;
	struct dsa_context *context = dev->data;
	struct dsa_netc_data *prv = PRV_DATA(context);

	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct ethernet_context *ctx_master;
	int i = cfg->port_idx;

	/* Find master port */
	if (context->iface_master == NULL) {
		context->iface_master = net_if_lookup_by_dev(prv->dev_master);
		if (context->iface_master == NULL) {
			LOG_ERR("DSA: Master iface NOT found!");
			return;
		}

		/*
		 * Provide pointer to DSA context to master's eth interface
		 * struct ethernet_context
		 */
		ctx_master = net_if_l2_data(context->iface_master);
		ctx_master->dsa_ctx = context;
	}

	if (context->iface_slave[i] == NULL) {
		context->iface_slave[i] = iface;
		net_if_set_link_addr(iface, cfg->mac_addr, sizeof(cfg->mac_addr),
				     NET_LINK_ETHERNET);
		ctx->dsa_port_idx = i;
		ctx->dsa_ctx = context;

		/*
		 * Initialize ethernet context 'work' for this iface to
		 * be able to monitor the carrier status.
		 */
		ethernet_init(iface);
	}

	prv->iface_init_count++;

	/* Do not start the interface until link is up */
	net_if_carrier_off(iface);

	if (cfg->pseudo_mac) {
		return;
	}

	if (!device_is_ready(cfg->phy_dev)) {
		LOG_ERR("PHY device (%p) is not ready, cannot init iface", cfg->phy_dev);
		return;
	}

	phy_link_callback_set(cfg->phy_dev, &netc_eth_phylink_callback, (void *)dev);
}

static enum ethernet_hw_caps dsa_netc_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_DSA_SLAVE_PORT | ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T |
	       ETHERNET_LINK_1000BASE_T;
}

static const struct device *dsa_netc_get_phy(const struct device *dev)
{
	const struct dsa_netc_slave_config *cfg = dev->config;

	return cfg->phy_dev;
}

static int dsa_netc_tx(const struct device *dev, struct net_pkt *pkt)
{
	/* TODO: frame tagging not supported on old NETC versions. Need implementation here. */
	return 0;
}

const struct ethernet_api dsa_netc_eth_api = {
	.iface_api.init = dsa_netc_iface_init,
	.get_capabilities = dsa_netc_get_capabilities,
	.get_phy = dsa_netc_get_phy,
	.send = dsa_netc_tx,
};

static struct dsa_api dsa_netc_api = {
	.switch_read = dsa_netc_sw_read_reg,
	.switch_write = dsa_netc_sw_write_reg,
	.switch_set_mac_table_entry = dsa_netc_set_mac_table_entry,
	.switch_get_mac_table_entry = dsa_netc_get_mac_table_entry,
};

#define DSA_NETC_SLAVE_DEVICE_INIT_INSTANCE(slave, n)                                              \
	PINCTRL_DT_DEFINE(slave);                                                                  \
	const struct dsa_netc_slave_config dsa_netc_##n##_slave_##slave##_config = {               \
		.mac_addr = DT_PROP_OR(slave, local_mac_address, {0}),                             \
		.phy_dev = (COND_CODE_1(DT_NODE_HAS_PROP(slave, phy_handle),                       \
					DEVICE_DT_GET(DT_PHANDLE_BY_IDX(slave, phy_handle, 0)),    \
					NULL)),                                                    \
		.phy_mode = NETC_PHY_MODE(slave),                                                  \
		.pseudo_mac = NETC_IS_PSEUDO_MAC(slave),                                           \
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(slave),                                        \
		.port_idx = DT_REG_ADDR_BY_IDX(slave, 0),                                          \
	};                                                                                         \
	NET_DEVICE_INIT_INSTANCE(                                                                  \
		CONCAT(dsa_slave_port_, slave),                                                    \
		"switch_port" STRINGIFY(n), n, dsa_netc_port_init, NULL, &dsa_netc_context_##n,    \
		&dsa_netc_##n##_slave_##slave##_config,                                            \
		CONFIG_ETH_INIT_PRIORITY, &dsa_netc_eth_api, ETHERNET_L2,                          \
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

#define DSA_NETC_DEVICE(n)                                                                         \
	AT_NONCACHEABLE_SECTION_ALIGN(static netc_cmd_bd_t dsa_netc_##n##_cmd_bd[8],               \
				      NETC_BD_ALIGN);                                              \
	static struct dsa_netc_data dsa_netc_data_prv_##n = {                                      \
		.port_num = DT_INST_CHILD_NUM_STATUS_OKAY(n),                                      \
		.port_init_count = 0,                                                              \
		.iface_init_count = 0,                                                             \
		.dev_master = DEVICE_DT_GET(DT_INST_PHANDLE(n, dsa_master_port)),                  \
		.cmd_bd = dsa_netc_##n##_cmd_bd,                                                   \
	};                                                                                         \
	static struct dsa_context dsa_netc_context_##n = {                                         \
		.num_slave_ports = DT_INST_CHILD_NUM(n),                                           \
		.dapi = &dsa_netc_api,                                                             \
		.prv_data = (void *)&dsa_netc_data_prv_##n,                                        \
	};                                                                                         \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, DSA_NETC_SLAVE_DEVICE_INIT_INSTANCE, n);

DT_INST_FOREACH_STATUS_OKAY(DSA_NETC_DEVICE);
