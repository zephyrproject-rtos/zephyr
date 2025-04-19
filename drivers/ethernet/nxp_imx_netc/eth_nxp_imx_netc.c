/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_imx_eth);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <ethernet/eth_stats.h>
#include <zephyr/net/dsa_core.h>
#include "../eth.h"
#include "eth_nxp_imx_netc_priv.h"

#if !(defined(FSL_FEATURE_NETC_HAS_SWITCH_TAG) && FSL_FEATURE_NETC_HAS_SWITCH_TAG) && \
	defined(CONFIG_NET_DSA)
#define NETC_HAS_NO_SWITCH_TAG_SUPPORT 1
#endif

const struct device *netc_dev_list[NETC_DRV_MAX_INST_SUPPORT];

static int netc_eth_rx(const struct device *dev)
{
	struct netc_eth_data *data = dev->data;
	struct net_if *iface_dst = data->iface;
#if defined(NETC_HAS_NO_SWITCH_TAG_SUPPORT)
	struct ethernet_context *ctx = net_if_l2_data(iface_dst);
#endif
	netc_frame_attr_t attr = {0};
	struct net_pkt *pkt;
	int key;
	int ret = 0;
	status_t result;
	uint32_t length;

	key = irq_lock();

	/* Check rx frame */
	result = EP_GetRxFrameSize(&data->handle, 0, &length);
	if (result == kStatus_NETC_RxFrameEmpty) {
		ret = -ENOBUFS;
		goto out;
	}

	if (result != kStatus_Success) {
		LOG_ERR("Error on received frame");
		ret = -EIO;
		goto out;
	}

	/* Receive frame */
	result = EP_ReceiveFrameCopy(&data->handle, 0, data->rx_frame, length, &attr);
	if (result != kStatus_Success) {
		LOG_ERR("Error on received frame");
		ret = -EIO;
		goto out;
	}

#if defined(NETC_HAS_NO_SWITCH_TAG_SUPPORT)
	if (ctx->dsa_port == DSA_CONDUIT_PORT) {
		iface_dst = ctx->dsa_switch_ctx->iface_user[attr.srcPort];
	}
#endif
	/* Copy to pkt */
	pkt = net_pkt_rx_alloc_with_buffer(iface_dst, length, AF_UNSPEC, 0, NETC_TIMEOUT);
	if (pkt == NULL) {
		eth_stats_update_errors_rx(iface_dst);
		ret = -ENOBUFS;
		goto out;
	}

	ret = net_pkt_write(pkt, data->rx_frame, length);
	if (ret != 0) {
		eth_stats_update_errors_rx(iface_dst);
		net_pkt_unref(pkt);
		goto out;
	}

	/* Send to upper layer */
	ret = net_recv_data(iface_dst, pkt);
	if (ret < 0) {
		eth_stats_update_errors_rx(iface_dst);
		net_pkt_unref(pkt);
		LOG_ERR("Failed to enqueue frame into rx queue: %d", ret);
	}
out:
	irq_unlock(key);
	return ret;
}

static void netc_eth_rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	struct netc_eth_data *data = dev->data;
	int ret;
	int work;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (1) {
		ret = k_sem_take(&data->rx_sem, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Take rx_sem error: %d", ret);
			continue;
		}

		work = 0;
		while (netc_eth_rx(dev) != -ENOBUFS) {
			if (++work == CONFIG_ETH_NXP_IMX_RX_BUDGET) {
				/* more work to do, reschedule */
				work = 0;
				k_yield();
			}
		}
	}
}

static void msgintr_isr(void)
{
	uint32_t irqs = NETC_MSGINTR->MSI[NETC_MSGINTR_CHANNEL].MSIR;

	for (int i = 0; i < NETC_DRV_MAX_INST_SUPPORT; i++) {
		const struct device *dev = netc_dev_list[i];
		const struct netc_eth_config *config;
		struct netc_eth_data *data;

		if (!dev) {
			return;
		}

		config = dev->config;
		data = dev->data;
		/* Transmit interrupt */
		if (irqs & (1 << config->tx_intr_msg_data)) {
			EP_CleanTxIntrFlags(&data->handle, 1, 0);
			data->tx_done = true;
		}
		/* Receive interrupt */
		if (irqs & (1 << config->rx_intr_msg_data)) {
			EP_CleanRxIntrFlags(&data->handle, 1);
			k_sem_give(&data->rx_sem);
		}
	}

	SDK_ISR_EXIT_BARRIER;
}

int netc_eth_init_common(const struct device *dev)
{
	const struct netc_eth_config *config = dev->config;
	struct netc_eth_data *data = dev->data;
	netc_msix_entry_t msix_entry[NETC_MSIX_ENTRY_NUM];
	netc_rx_bdr_config_t rx_bdr_config = {0};
	netc_tx_bdr_config_t tx_bdr_config = {0};
	netc_bdr_config_t bdr_config = {0};
	ep_config_t ep_config;
	uint32_t msg_addr;
	status_t result;

	config->bdr_init(&bdr_config, &rx_bdr_config, &tx_bdr_config);

	/* MSIX entry configuration */
	msg_addr = MSGINTR_GetIntrSelectAddr(NETC_MSGINTR, NETC_MSGINTR_CHANNEL);
	msix_entry[NETC_TX_MSIX_ENTRY_IDX].control = kNETC_MsixIntrMaskBit;
	msix_entry[NETC_TX_MSIX_ENTRY_IDX].msgAddr = msg_addr;
	msix_entry[NETC_TX_MSIX_ENTRY_IDX].msgData = config->tx_intr_msg_data;

	msix_entry[NETC_RX_MSIX_ENTRY_IDX].control = kNETC_MsixIntrMaskBit;
	msix_entry[NETC_RX_MSIX_ENTRY_IDX].msgAddr = msg_addr;
	msix_entry[NETC_RX_MSIX_ENTRY_IDX].msgData = config->rx_intr_msg_data;

	if (!irq_is_enabled(NETC_MSGINTR_IRQ)) {
		IRQ_CONNECT(NETC_MSGINTR_IRQ, 0, msgintr_isr, 0, 0);
		irq_enable(NETC_MSGINTR_IRQ);
	}

	/* Endpoint configuration. */
	EP_GetDefaultConfig(&ep_config);
	ep_config.si = config->si_idx;
	ep_config.siConfig.txRingUse = 1;
	ep_config.siConfig.rxRingUse = 1;
	ep_config.userData = data;
	ep_config.reclaimCallback = NULL;
	ep_config.msixEntry = &msix_entry[0];
	ep_config.entryNum = NETC_MSIX_ENTRY_NUM;
	ep_config.port.ethMac.miiMode = config->phy_mode;
	ep_config.port.ethMac.miiSpeed = kNETC_MiiSpeed100M;
	ep_config.port.ethMac.miiDuplex = kNETC_MiiFullDuplex;
	ep_config.rxCacheMaintain = true;
	ep_config.txCacheMaintain = true;

	config->generate_mac(&data->mac_addr[0]);

	result = EP_Init(&data->handle, &data->mac_addr[0], &ep_config, &bdr_config);
	if (result != kStatus_Success) {
		return -ENOBUFS;
	}

	/*
	 * For management ENETC, the SI 0 hardware Tx ring index 0 should be used for
	 * direct switch enqueue feature.
	 * hal enetc driver reserved ring 0 for hal switch driver, so re-enable it here.
	 */
	if (config->pseudo_mac) {
		if (NETC_SIConfigTxBDR(data->handle.hw.si, 0, &tx_bdr_config) != kStatus_Success) {
			return -ENOBUFS;
		}
	}

	for (int i = 0; i < NETC_DRV_MAX_INST_SUPPORT; i++) {
		if (!netc_dev_list[i]) {
			netc_dev_list[i] = dev;
			break;
		}
	}

	/* Unmask MSIX message interrupt. */
	EP_MsixSetEntryMask(&data->handle, NETC_TX_MSIX_ENTRY_IDX, false);
	EP_MsixSetEntryMask(&data->handle, NETC_RX_MSIX_ENTRY_IDX, false);

	k_mutex_init(&data->tx_mutex);

	k_sem_init(&data->rx_sem, 0, 1);
	k_thread_create(&data->rx_thread, data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(data->rx_thread_stack), netc_eth_rx_thread,
			(void *)dev, NULL, NULL, K_PRIO_COOP(CONFIG_ETH_NXP_IMX_RX_THREAD_PRIO), 0,
			K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "netc_eth_rx");

	return 0;
}

int netc_eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	const struct netc_eth_config *cfg = dev->config;
	struct netc_eth_data *data = dev->data;
	netc_buffer_struct_t buff = {.buffer = data->tx_buff, .length = sizeof(data->tx_buff)};
	netc_frame_struct_t frame = {.buffArray = &buff, .length = 1};
	netc_tx_frame_info_t *frame_info;
	struct net_if *iface_dst;
	size_t pkt_len = net_pkt_get_len(pkt);
#if defined(NETC_HAS_NO_SWITCH_TAG_SUPPORT)
	struct ethernet_context *eth_ctx = net_if_l2_data(data->iface);
#endif
	status_t result;
	int ret;

	__ASSERT(pkt, "Packet pointer is NULL");

	iface_dst = data->iface;

	if (cfg->pseudo_mac) {
#if defined(NETC_HAS_NO_SWITCH_TAG_SUPPORT)
		/* DSA conduit port not used */
		if (eth_ctx->dsa_port != DSA_CONDUIT_PORT) {
			return -ENOSYS;
		}
		/* DSA driver redirects the iface */
		iface_dst = pkt->iface;
#else
		return -ENOSYS;
#endif
	}

	k_mutex_lock(&data->tx_mutex, K_FOREVER);

	/* Copy packet to tx buffer */
	buff.length = (uint16_t)pkt_len;
	ret = net_pkt_read(pkt, buff.buffer, pkt_len);
	if (ret) {
		LOG_ERR("Failed to copy packet to tx buffer: %d", ret);
		ret = -ENOBUFS;
		goto error;
	}

	/* Send */
	data->tx_done = false;

#if defined(NETC_HAS_NO_SWITCH_TAG_SUPPORT)
	if (eth_ctx->dsa_port == DSA_CONDUIT_PORT) {
		const struct dsa_port_config *port_cfg = net_if_get_device(iface_dst)->config;
		const int dst_port = port_cfg->port_idx;
		netc_tx_bd_t txDesc[2] = {0};

		txDesc[0].standard.flags = NETC_SI_TXDESCRIP_RD_FLQ(2) |
					   NETC_SI_TXDESCRIP_RD_SMSO_MASK |
					   NETC_SI_TXDESCRIP_RD_PORT(dst_port);
		result = EP_SendFrameCommon(&data->handle, &data->handle.txBdRing[0], 0, &frame,
					    NULL, &txDesc[0], data->handle.cfg.txCacheMaintain);
	} else {
		result = EP_SendFrame(&data->handle, 0, &frame, NULL, NULL);
	}
#else
	result = EP_SendFrame(&data->handle, 0, &frame, NULL, NULL);
#endif
	if (result != kStatus_Success) {
		LOG_ERR("Failed to tx frame");
		ret = -EIO;
		goto error;
	}

	while (!data->tx_done) {
	}

	do {
		frame_info = EP_ReclaimTxDescCommon(&data->handle, &data->handle.txBdRing[0],
						    0, false);
		if (frame_info != NULL) {
			if (frame_info->status != kNETC_EPTxSuccess) {
				memset(frame_info, 0, sizeof(netc_tx_frame_info_t));
				LOG_ERR("Failed to tx frame");
				ret = -EIO;
				goto error;
			}
			memset(frame_info, 0, sizeof(netc_tx_frame_info_t));
		}
	} while (frame_info != NULL);

	ret = 0;
error:
	k_mutex_unlock(&data->tx_mutex);

	if (ret != 0) {
		eth_stats_update_errors_tx(iface_dst);
	}
	return ret;
}

enum ethernet_hw_caps netc_eth_get_capabilities(const struct device *dev)
{
	uint32_t caps;

	caps = (ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T | ETHERNET_LINK_1000BASE_T |
		ETHERNET_HW_RX_CHKSUM_OFFLOAD | ETHERNET_HW_FILTERING
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
	);

	return caps;
}

int netc_eth_set_config(const struct device *dev, enum ethernet_config_type type,
			const struct ethernet_config *config)
{
	struct netc_eth_data *data = dev->data;
	const struct netc_eth_config *cfg = dev->config;
	status_t result;
	int ret = 0;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		/* Set new Ethernet MAC address and register it with the upper layer */
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));
		result = EP_SetPrimaryMacAddr(&data->handle, (uint8_t *)data->mac_addr);
		if (result != kStatus_Success) {
			LOG_ERR("PHY device (%p) is not ready, cannot init iface", cfg->phy_dev);
			ret = -ENOTSUP;
			break;
		}
		net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
				     NET_LINK_ETHERNET);
		LOG_INF("SI%d MAC set to: %02x:%02x:%02x:%02x:%02x:%02x", getSiIdx(cfg->si_idx),
			data->mac_addr[0], data->mac_addr[1], data->mac_addr[2], data->mac_addr[3],
			data->mac_addr[4], data->mac_addr[5]);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
