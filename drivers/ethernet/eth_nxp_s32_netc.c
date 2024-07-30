/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_eth);

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

/* Global MAC filter hash table required for the baremetal driver */
Netc_Eth_Ip_MACFilterHashTableEntryType * MACFilterHashTableAddrs[FEATURE_NETC_ETH_NUMBER_OF_CTRLS];

static void nxp_s32_eth_rx_thread(void *arg1, void *unused1, void *unused2);

static void nxp_s32_eth_msix_wrapper(const struct device *dev, uint32_t channel,
				     void *user_data, struct mbox_msg *msg)
{
	const struct nxp_s32_eth_msix *msix = (const struct nxp_s32_eth_msix *)user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(msg);

	/* Handler doesn't require any data to be passed, used only for signalling */
	msix->handler(channel, NULL, 0);
}

static inline struct net_if *get_iface(struct nxp_s32_eth_data *ctx)
{
	return ctx->iface;
}

int nxp_s32_eth_initialize_common(const struct device *dev)
{
	const struct nxp_s32_eth_config *cfg = dev->config;
	struct nxp_s32_eth_data *ctx = dev->data;
	Netc_Eth_Ip_StatusType status;
	const struct nxp_s32_eth_msix *msix;
	int err;

	/* Populate the MAC filter hash table addresses for this SI */
	__ASSERT_NO_MSG(cfg->si_idx < FEATURE_NETC_ETH_NUMBER_OF_CTRLS);
	MACFilterHashTableAddrs[cfg->si_idx] = cfg->mac_filter_hash_table;

	status = Netc_Eth_Ip_Init(cfg->si_idx, &cfg->netc_cfg);
	if (status != NETC_ETH_IP_STATUS_SUCCESS) {
		LOG_ERR("Failed to initialize SI%d (%d)", cfg->si_idx, status);
		return -EIO;
	}

	for (int i = 0; i < NETC_MSIX_EVENTS_COUNT; i++) {
		msix = &cfg->msix[i];
		if (mbox_is_ready_dt(&msix->mbox_spec)) {
			err = mbox_register_callback_dt(&msix->mbox_spec,
							nxp_s32_eth_msix_wrapper,
							(void *)msix);
			if (err != 0) {
				LOG_ERR("Failed to register MRU callback on channel %u",
					msix->mbox_spec.channel_id);
				return err;
			}
		}
	}

	k_mutex_init(&ctx->tx_mutex);
	k_sem_init(&ctx->rx_sem, 0, 1);

	k_thread_create(&ctx->rx_thread, ctx->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(ctx->rx_thread_stack),
			nxp_s32_eth_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_NXP_S32_RX_THREAD_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&ctx->rx_thread, "nxp_s32_eth_rx");

	status = Netc_Eth_Ip_EnableController(cfg->si_idx);
	if (status != NETC_ETH_IP_STATUS_SUCCESS) {
		LOG_ERR("Failed to enable ENETC SI%d (%d)", cfg->si_idx, status);
		return -EIO;
	}

	if (cfg->generate_mac) {
		cfg->generate_mac(&ctx->mac_addr[0]);
	}

	return 0;
}

void nxp_s32_eth_mcast_filter(const struct device *dev, const struct ethernet_filter *filter)
{
	const struct nxp_s32_eth_config *cfg = dev->config;
	Netc_Eth_Ip_StatusType status;

	if (filter->set) {
		status = Netc_Eth_Ip_AddMulticastDstAddrToHashFilter(cfg->si_idx,
								     filter->mac_address.addr);
	} else {
		status = Netc_Eth_Ip_RemoveMulticastDstAddrFromHashFilter(cfg->si_idx,
									  filter->mac_address.addr);
	}
	if (status != NETC_ETH_IP_STATUS_SUCCESS) {
		LOG_ERR("Failed to update multicast hash table: %d", status);
	}
}

int nxp_s32_eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct nxp_s32_eth_data *ctx = dev->data;
	const struct nxp_s32_eth_config *cfg = dev->config;
	size_t pkt_len = net_pkt_get_len(pkt);
	int res = 0;
	Netc_Eth_Ip_StatusType status;
	Netc_Eth_Ip_BufferType buf;

	__ASSERT(pkt, "Packet pointer is NULL");

	k_mutex_lock(&ctx->tx_mutex, K_FOREVER);

	buf.length = (uint16_t)pkt_len;
	buf.data = NULL;
	status = Netc_Eth_Ip_GetTxBuff(cfg->si_idx, cfg->tx_ring_idx, &buf, NULL);
	if (status == NETC_ETH_IP_STATUS_TX_BUFF_BUSY) {
		/* Reclaim the buffers already transmitted and try again */
		Netc_Eth_Ip_ReleaseTxBuffers(cfg->si_idx, cfg->tx_ring_idx);
		status = Netc_Eth_Ip_GetTxBuff(cfg->si_idx, cfg->tx_ring_idx, &buf, NULL);
	}
	if (status != NETC_ETH_IP_STATUS_SUCCESS) {
		LOG_ERR("Failed to get tx buffer: %d", status);
		res = -ENOBUFS;
		goto error;
	}
	buf.length = (uint16_t)pkt_len;

	res = net_pkt_read(pkt, buf.data, pkt_len);
	if (res) {
		LOG_ERR("Failed to copy packet to tx buffer: %d", res);
		res = -ENOBUFS;
		goto error;
	}

	status = Netc_Eth_Ip_SendFrame(cfg->si_idx, cfg->tx_ring_idx, &buf, NULL);
	if (status != NETC_ETH_IP_STATUS_SUCCESS) {
		LOG_ERR("Failed to tx frame: %d", status);
		res = -EIO;
		goto error;
	}

error:
	k_mutex_unlock(&ctx->tx_mutex);

	if (res != 0) {
		eth_stats_update_errors_tx(ctx->iface);
	}
	return res;
}

static struct net_pkt *nxp_s32_eth_get_pkt(const struct device *dev,
					   Netc_Eth_Ip_BufferType *buf)
{
	struct nxp_s32_eth_data *ctx = dev->data;
	struct net_pkt *pkt = NULL;
	int res = 0;

	/* Use root iface, it will be updated later in net_recv_data() */
	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, buf->length,
					   AF_UNSPEC, 0, NETC_TIMEOUT);
	if (!pkt) {
		goto exit;
	}

	res = net_pkt_write(pkt, buf->data, buf->length);
	if (res) {
		net_pkt_unref(pkt);
		pkt = NULL;
		goto exit;
	}

exit:
	if (!pkt) {
		eth_stats_update_errors_rx(get_iface(ctx));
	}

	return pkt;
}

static int nxp_s32_eth_rx(const struct device *dev)
{
	struct nxp_s32_eth_data *ctx = dev->data;
	const struct nxp_s32_eth_config *cfg = dev->config;
	Netc_Eth_Ip_BufferType buf;
	Netc_Eth_Ip_RxInfoType info;
	Netc_Eth_Ip_StatusType status;
	struct net_pkt *pkt;
	int key;
	int res = 0;

	key = irq_lock();
	status = Netc_Eth_Ip_ReadFrame(cfg->si_idx, cfg->rx_ring_idx, &buf, &info);
	if (status == NETC_ETH_IP_STATUS_RX_QUEUE_EMPTY) {
		res = -ENOBUFS;
	} else if (status != NETC_ETH_IP_STATUS_SUCCESS) {
		LOG_ERR("Error on received frame: %d (0x%X)", status, info.rxStatus);
		res = -EIO;
	} else {
		pkt = nxp_s32_eth_get_pkt(dev, &buf);
		Netc_Eth_Ip_ProvideRxBuff(cfg->si_idx, cfg->rx_ring_idx, &buf);

		if (pkt != NULL) {
			res = net_recv_data(get_iface(ctx), pkt);
			if (res < 0) {
				eth_stats_update_errors_rx(get_iface(ctx));
				net_pkt_unref(pkt);
				LOG_ERR("Failed to enqueue frame into rx queue: %d", res);
			}
		}
	}
	irq_unlock(key);

	return res;
}

static void nxp_s32_eth_rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	struct nxp_s32_eth_data *ctx = dev->data;
	int res;
	int work;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	__ASSERT_NO_MSG(arg1 != NULL);
	__ASSERT_NO_MSG(ctx != NULL);

	while (1) {
		res = k_sem_take(&ctx->rx_sem, K_FOREVER);
		__ASSERT_NO_MSG(res == 0);

		work = 0;
		while (nxp_s32_eth_rx(dev) != -ENOBUFS) {
			if (++work == CONFIG_ETH_NXP_S32_RX_BUDGET) {
				/* more work to do, reschedule */
				work = 0;
				k_yield();
			}
		}
	}
}

enum ethernet_hw_caps nxp_s32_eth_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (ETHERNET_LINK_10BASE_T
		| ETHERNET_LINK_100BASE_T
		| ETHERNET_LINK_1000BASE_T
		| ETHERNET_HW_RX_CHKSUM_OFFLOAD
		| ETHERNET_HW_FILTERING
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
	);
}

int nxp_s32_eth_set_config(const struct device *dev, enum ethernet_config_type type,
			   const struct ethernet_config *config)
{
	struct nxp_s32_eth_data *ctx = dev->data;
	const struct nxp_s32_eth_config *cfg = dev->config;
	int res = 0;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		/* Set new Ethernet MAC address and register it with the upper layer */
		memcpy(ctx->mac_addr, config->mac_address.addr, sizeof(ctx->mac_addr));
		Netc_Eth_Ip_SetMacAddr(cfg->si_idx, (const uint8_t *)ctx->mac_addr);
		net_if_set_link_addr(ctx->iface, ctx->mac_addr, sizeof(ctx->mac_addr),
				     NET_LINK_ETHERNET);
		LOG_INF("SI%d MAC set to: %02x:%02x:%02x:%02x:%02x:%02x", cfg->si_idx,
			ctx->mac_addr[0], ctx->mac_addr[1], ctx->mac_addr[2],
			ctx->mac_addr[3], ctx->mac_addr[4], ctx->mac_addr[5]);
		break;
	case ETHERNET_CONFIG_TYPE_FILTER:
		nxp_s32_eth_mcast_filter(dev, &config->filter);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	return res;
}

BUILD_ASSERT((CONFIG_ETH_NXP_S32_RX_RING_LEN % 8) == 0,
	     "Rx ring length must be multiple of 8");
BUILD_ASSERT((CONFIG_ETH_NXP_S32_TX_RING_LEN % 8) == 0,
	     "Tx ring length must be multiple of 8");
BUILD_ASSERT((CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE % 8) == 0,
	     "Rx ring data buffer size must be multiple of 8");
BUILD_ASSERT((CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE % 8) == 0,
	     "Tx ring data buffer size must be multiple of 8");
