/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_netc_psi

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_imx_eth_psi);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <ethernet/eth_stats.h>

#include "eth.h"
#include "eth_nxp_imx_netc_priv.h"

static void netc_eth_phylink_callback(const struct device *pdev, struct phy_link_state *state,
				      void *user_data)
{
	const struct device *dev = (struct device *)user_data;
	struct netc_eth_data *data = dev->data;
	status_t result;

	ARG_UNUSED(pdev);

	if (state->is_up) {
		LOG_DBG("Link up");
		result = EP_Up(&data->handle, PHY_TO_NETC_SPEED(state->speed),
			       PHY_TO_NETC_DUPLEX_MODE(state->speed));
		if (result != kStatus_Success) {
			LOG_ERR("Failed to set MAC up");
		}
		net_eth_carrier_on(data->iface);
	} else {
		LOG_DBG("Link down");
		result = EP_Down(&data->handle);
		if (result != kStatus_Success) {
			LOG_ERR("Failed to set MAC down");
		}
		net_eth_carrier_off(data->iface);
	}
}

static void netc_eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct netc_eth_data *data = dev->data;
	const struct netc_eth_config *cfg = dev->config;
	status_t result;

	/*
	 * For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (data->iface == NULL) {
		data->iface = iface;
	}

	/* Set MAC address */
	result = EP_SetPrimaryMacAddr(&data->handle, (uint8_t *)data->mac_addr);
	if (result != kStatus_Success) {
		LOG_ERR("Failed to set MAC address");
	}

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	LOG_INF("SI%d MAC: %02x:%02x:%02x:%02x:%02x:%02x", cfg->si_idx, data->mac_addr[0],
		data->mac_addr[1], data->mac_addr[2], data->mac_addr[3], data->mac_addr[4],
		data->mac_addr[5]);

	ethernet_init(iface);

	if (cfg->pseudo_mac) {
		return;
	}

	/*
	 * PSI controls the PHY. If PHY is configured either as fixed
	 * link or autoneg, the callback is executed at least once
	 * immediately after setting it.
	 */
	if (!device_is_ready(cfg->phy_dev)) {
		LOG_ERR("PHY device (%p) is not ready, cannot init iface", cfg->phy_dev);
		return;
	}
	phy_link_callback_set(cfg->phy_dev, &netc_eth_phylink_callback, (void *)dev);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);
}

static int netc_eth_init(const struct device *dev)
{
	const struct netc_eth_config *cfg = dev->config;
	int err;

	if (!cfg->pseudo_mac) {
		err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (err) {
			return err;
		}
	}

	return netc_eth_init_common(dev);
}

static const struct device *netc_eth_get_phy(const struct device *dev)
{
	const struct netc_eth_config *cfg = dev->config;

	return cfg->phy_dev;
}

static const struct ethernet_api netc_eth_api = {.iface_api.init = netc_eth_iface_init,
						 .get_capabilities = netc_eth_get_capabilities,
						 .get_phy = netc_eth_get_phy,
						 .set_config = netc_eth_set_config,
						 .send = netc_eth_tx};

#define NETC_PSI_INSTANCE_DEFINE(n)                                                                \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	NETC_GENERATE_MAC_ADDRESS(n)                                                               \
	AT_NONCACHEABLE_SECTION_ALIGN(                                                             \
		static uint8_t eth##n##_tx_buff[CONFIG_ETH_NXP_IMX_TX_RING_BUF_SIZE],              \
		NETC_BUFF_ALIGN);                                                                  \
	AT_NONCACHEABLE_SECTION_ALIGN(                                                             \
		static netc_tx_bd_t eth##n##_txbd_array[CONFIG_ETH_NXP_IMX_TX_RING_NUM]            \
						       [CONFIG_ETH_NXP_IMX_TX_RING_LEN],           \
		NETC_BD_ALIGN);                                                                    \
	static netc_tx_frame_info_t eth##n##_txdirty_array[CONFIG_ETH_NXP_IMX_TX_RING_NUM]         \
							  [CONFIG_ETH_NXP_IMX_TX_RING_LEN];        \
	AT_NONCACHEABLE_SECTION_ALIGN(                                                             \
		static rx_buffer_t eth##n##_rx_buff[CONFIG_ETH_NXP_IMX_RX_RING_NUM]                \
						   [CONFIG_ETH_NXP_IMX_RX_RING_LEN],               \
		NETC_BUFF_ALIGN);                                                                  \
	static uint64_t eth##n##_rx_buff_addr_array[CONFIG_ETH_NXP_IMX_RX_RING_NUM]                \
						   [CONFIG_ETH_NXP_IMX_RX_RING_LEN];               \
	AT_NONCACHEABLE_SECTION(static uint8_t eth##n##_rx_frame[NETC_RX_RING_BUF_SIZE_ALIGN]);    \
	AT_NONCACHEABLE_SECTION_ALIGN(                                                             \
		static netc_rx_bd_t eth##n##_rxbd_array[CONFIG_ETH_NXP_IMX_RX_RING_NUM]            \
						       [CONFIG_ETH_NXP_IMX_RX_RING_LEN],           \
		NETC_BD_ALIGN);                                                                    \
	static void netc_eth##n##_bdr_init(netc_bdr_config_t *bdr_config,                          \
					   netc_rx_bdr_config_t *rx_bdr_config,                    \
					   netc_tx_bdr_config_t *tx_bdr_config)                    \
	{                                                                                          \
		for (uint8_t ring = 0U; ring < CONFIG_ETH_NXP_IMX_RX_RING_NUM; ring++) {           \
			for (uint8_t bd = 0U; bd < CONFIG_ETH_NXP_IMX_RX_RING_LEN; bd++) {         \
				eth##n##_rx_buff_addr_array[ring][bd] =                            \
					(uint64_t)(uintptr_t)&eth##n##_rx_buff[ring][bd];          \
			}                                                                          \
		}                                                                                  \
		memset(bdr_config, 0, sizeof(netc_bdr_config_t));                                  \
		memset(rx_bdr_config, 0, sizeof(netc_rx_bdr_config_t));                            \
		memset(tx_bdr_config, 0, sizeof(netc_tx_bdr_config_t));                            \
		bdr_config->rxBdrConfig = rx_bdr_config;                                           \
		bdr_config->txBdrConfig = tx_bdr_config;                                           \
		bdr_config->rxBdrConfig[0].bdArray = &eth##n##_rxbd_array[0][0];                   \
		bdr_config->rxBdrConfig[0].len = CONFIG_ETH_NXP_IMX_RX_RING_LEN;                   \
		bdr_config->rxBdrConfig[0].buffAddrArray = &eth##n##_rx_buff_addr_array[0][0];     \
		bdr_config->rxBdrConfig[0].buffSize = NETC_RX_RING_BUF_SIZE_ALIGN;                 \
		bdr_config->rxBdrConfig[0].msixEntryIdx = NETC_RX_MSIX_ENTRY_IDX;                  \
		bdr_config->rxBdrConfig[0].extendDescEn = false;                                   \
		bdr_config->rxBdrConfig[0].enThresIntr = true;                                     \
		bdr_config->rxBdrConfig[0].enCoalIntr = true;                                      \
		bdr_config->rxBdrConfig[0].intrThreshold = 1;                                      \
		bdr_config->txBdrConfig[0].bdArray = &eth##n##_txbd_array[0][0];                   \
		bdr_config->txBdrConfig[0].len = CONFIG_ETH_NXP_IMX_TX_RING_LEN;                   \
		bdr_config->txBdrConfig[0].dirtyArray = &eth##n##_txdirty_array[0][0];             \
		bdr_config->txBdrConfig[0].msixEntryIdx = NETC_TX_MSIX_ENTRY_IDX;                  \
		bdr_config->txBdrConfig[0].enIntr = true;                                          \
	}                                                                                          \
	static struct netc_eth_data netc_eth##n##_data = {                                         \
		.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),                            \
		.tx_buff = eth##n##_tx_buff,                                                       \
		.rx_frame = eth##n##_rx_frame,                                                     \
	};                                                                                         \
	static const struct netc_eth_config netc_eth##n##_config = {                               \
		.generate_mac = netc_eth##n##_generate_mac,                                        \
		.bdr_init = netc_eth##n##_bdr_init,                                                \
		.phy_dev = (COND_CODE_1(DT_INST_NODE_HAS_PROP(n, phy_handle),                      \
					(DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle))), NULL)),   \
		.phy_mode = NETC_PHY_MODE(DT_DRV_INST(n)),                                         \
		.pseudo_mac = NETC_IS_PSEUDO_MAC(DT_DRV_INST(n)),                                  \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.si_idx = (DT_INST_PROP(n, mac_index) << 8) | DT_INST_PROP(n, si_index),           \
		.tx_intr_msg_data = NETC_TX_INTR_MSG_DATA_START + n,                               \
		.rx_intr_msg_data = NETC_RX_INTR_MSG_DATA_START + n,                               \
	};                                                                                         \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, netc_eth_init, NULL, &netc_eth##n##_data,                 \
				      &netc_eth##n##_config, CONFIG_ETH_INIT_PRIORITY,             \
				      &netc_eth_api, NET_ETH_MTU);
DT_INST_FOREACH_STATUS_OKAY(NETC_PSI_INSTANCE_DEFINE)
