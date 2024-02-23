/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_netc_vsi

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_eth_vsi);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <ethernet/eth_stats.h>

#include <soc.h>
#include <Netc_Eth_Ip.h>
#include <Netc_Eth_Ip_Irq.h>
#include <Netc_EthSwt_Ip.h>

#include "eth.h"
#include "eth_nxp_s32_netc_priv.h"

#define TX_RING_IDX	0
#define RX_RING_IDX	0

static void nxp_s32_eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct nxp_s32_eth_data *ctx = dev->data;
	const struct nxp_s32_eth_config *cfg = dev->config;
	const struct nxp_s32_eth_msix *msix;
#if defined(CONFIG_NET_IPV6)
	static struct net_if_mcast_monitor mon;

	net_if_mcast_mon_register(&mon, iface, nxp_s32_eth_mcast_cb);
#endif /* CONFIG_NET_IPV6 */

	/*
	 * For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (ctx->iface == NULL) {
		ctx->iface = iface;
	}

	Netc_Eth_Ip_SetMacAddr(cfg->si_idx, (const uint8_t *)ctx->mac_addr);
	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);

	LOG_INF("SI%d MAC: %02x:%02x:%02x:%02x:%02x:%02x", cfg->si_idx,
		ctx->mac_addr[0], ctx->mac_addr[1], ctx->mac_addr[2],
		ctx->mac_addr[3], ctx->mac_addr[4], ctx->mac_addr[5]);

	ethernet_init(iface);

	/* Assumes PSI is already started and link is up, iface will auto-start after init */
	net_eth_carrier_on(iface);

	for (int i = 0; i < NETC_MSIX_EVENTS_COUNT; i++) {
		msix = &cfg->msix[i];
		if (mbox_is_ready_dt(&msix->mbox_spec)) {
			if (mbox_set_enabled_dt(&msix->mbox_spec, true)) {
				LOG_ERR("Failed to enable MRU channel %u",
					msix->mbox_spec.channel_id);
			}
		}
	}
}

static const struct ethernet_api nxp_s32_eth_api = {
	.iface_api.init = nxp_s32_eth_iface_init,
	.get_capabilities = nxp_s32_eth_get_capabilities,
	.set_config = nxp_s32_eth_set_config,
	.send = nxp_s32_eth_tx
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(nxp_s32_netc_vsi) == 1, "Only one VSI enabled supported");

#define NETC_VSI_INSTANCE_DEFINE(n)								\
	NETC_GENERATE_MAC_ADDRESS(n)								\
												\
	void nxp_s32_eth_vsi##n##_rx_event(uint8_t chan, const uint32_t *buf, uint8_t buf_size)	\
	{											\
		Netc_Eth_Ip_MSIX_Rx(NETC_SI_NXP_S32_HW_INSTANCE(n));				\
	}											\
												\
	static void nxp_s32_eth##n##_rx_callback(const uint8_t unused, const uint8_t ring)	\
	{											\
		const struct device *dev = DEVICE_DT_INST_GET(n);				\
		const struct nxp_s32_eth_config *cfg = dev->config;				\
		struct nxp_s32_eth_data *ctx = dev->data;					\
												\
		if (ring == cfg->rx_ring_idx) {							\
			k_sem_give(&ctx->rx_sem);						\
		}										\
	}											\
												\
	static Netc_Eth_Ip_StateType nxp_s32_eth##n##_state;					\
	Netc_Eth_Ip_VsiToPsiMsgType nxp_s32_eth##n##_vsi2psi_msg				\
		__aligned(FEATURE_NETC_ETH_VSI_MSG_ALIGNMENT);					\
	static Netc_Eth_Ip_MACFilterHashTableEntryType						\
	nxp_s32_eth##n##_mac_filter_hash_table[CONFIG_ETH_NXP_S32_MAC_FILTER_TABLE_SIZE];	\
												\
	NETC_RX_RING(n, TX_RING_IDX, CONFIG_ETH_NXP_S32_RX_RING_LEN,				\
			 CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE);					\
	NETC_TX_RING(n, RX_RING_IDX, CONFIG_ETH_NXP_S32_TX_RING_LEN,				\
			 CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE);					\
												\
	static const Netc_Eth_Ip_RxRingConfigType nxp_s32_eth##n##_rxring_cfg[1] = {		\
		{										\
			.RingDesc = nxp_s32_eth##n##_rxring0_desc,				\
			.Buffer = nxp_s32_eth##n##_rxring0_buf,					\
			.ringSize = CONFIG_ETH_NXP_S32_RX_RING_LEN,				\
			.maxRingSize = CONFIG_ETH_NXP_S32_RX_RING_LEN,				\
			.bufferLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,			\
			.maxBuffLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,			\
			.TimerThreshold = CONFIG_ETH_NXP_S32_RX_IRQ_TIMER_THRESHOLD,		\
			.PacketsThreshold = CONFIG_ETH_NXP_S32_RX_IRQ_PACKET_THRESHOLD,		\
			.Callback = nxp_s32_eth##n##_rx_callback,				\
		}										\
	};											\
	static const Netc_Eth_Ip_TxRingConfigType nxp_s32_eth##n##_txring_cfg[1] = {		\
		{										\
			.RingDesc = nxp_s32_eth##n##_txring0_desc,				\
			.Buffer = nxp_s32_eth##n##_txring0_buf,					\
			.ringSize = CONFIG_ETH_NXP_S32_TX_RING_LEN,				\
			.maxRingSize = CONFIG_ETH_NXP_S32_TX_RING_LEN,				\
			.bufferLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,			\
			.maxBuffLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,			\
		}										\
	};											\
												\
	static const Netc_Eth_Ip_StationInterfaceConfigType nxp_s32_eth##n##_si_cfg = {		\
		.NumberOfRxBDR = 1,								\
		.NumberOfTxBDR = 1,								\
		.txMruMailboxAddr = NULL,							\
		.rxMruMailboxAddr = (uint32_t *)MRU_MBOX_ADDR(DT_DRV_INST(n), rx),		\
		.RxInterrupts = (uint32_t)true,							\
		.TxInterrupts = (uint32_t)false,						\
		.MACFilterTableMaxNumOfEntries = CONFIG_ETH_NXP_S32_MAC_FILTER_TABLE_SIZE,	\
		.VSItoPSIMsgCommand = &nxp_s32_eth##n##_vsi2psi_msg,				\
	};											\
												\
	static struct nxp_s32_eth_data nxp_s32_eth##n##_data = {				\
		.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),				\
	};											\
												\
	static const struct nxp_s32_eth_config nxp_s32_eth##n##_cfg = {				\
		.netc_cfg = {									\
			.SiType = NETC_ETH_IP_VIRTUAL_SI,					\
			.siConfig = &nxp_s32_eth##n##_si_cfg,					\
			.stateStructure = &nxp_s32_eth##n##_state,				\
			.paCtrlRxRingConfig = &nxp_s32_eth##n##_rxring_cfg,			\
			.paCtrlTxRingConfig = &nxp_s32_eth##n##_txring_cfg,			\
		},										\
		.si_idx = NETC_SI_NXP_S32_HW_INSTANCE(n),					\
		.tx_ring_idx = TX_RING_IDX,							\
		.rx_ring_idx = RX_RING_IDX,							\
		.msix = {									\
			NETC_MSIX(DT_DRV_INST(n), rx, nxp_s32_eth_vsi##n##_rx_event),		\
		},										\
		.mac_filter_hash_table = &nxp_s32_eth##n##_mac_filter_hash_table[0],		\
		.generate_mac = nxp_s32_eth##n##_generate_mac,					\
	};											\
												\
	ETH_NET_DEVICE_DT_INST_DEFINE(n,							\
				nxp_s32_eth_initialize_common,					\
				NULL,								\
				&nxp_s32_eth##n##_data,						\
				&nxp_s32_eth##n##_cfg,						\
				CONFIG_ETH_NXP_S32_VSI_INIT_PRIORITY,				\
				&nxp_s32_eth_api,						\
				NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(NETC_VSI_INSTANCE_DEFINE)
