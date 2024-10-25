/*
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifi_w91.h"
#include <zephyr/net/net_pkt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(w91_wifi_l2, CONFIG_WIFI_LOG_LEVEL);

static enum net_verdict wifi_w91_recv(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	net_pkt_lladdr_src(pkt)->addr = NULL;
	net_pkt_lladdr_src(pkt)->len = 0U;
	net_pkt_lladdr_src(pkt)->type = NET_LINK_ETHERNET;
	net_pkt_lladdr_dst(pkt)->addr = NULL;
	net_pkt_lladdr_dst(pkt)->len = 0U;
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_ETHERNET;

	return NET_CONTINUE;
}

static int wifi_w91_send(struct net_if *iface, struct net_pkt *pkt)
{
	int ret = 0;

	do {
		if (!pkt) {
			break;
		}

		const struct device *dev = net_if_get_device(iface);
		struct wifi_w91_data *data = dev->data;
		const struct wifi_w91_config *config = dev->config;
		size_t tx_len = net_pkt_get_len(pkt);

		if (!tx_len) {
			break;
		}
		if (tx_len > sizeof(data->l2.ipc_tx.data)) {
			LOG_ERR("tx data exceeds MTU size");
			ret = -ENOBUFS;
			break;
		}
		ret = net_pkt_read(pkt, data->l2.ipc_tx.data, tx_len);
		if (ret < 0) {
			LOG_ERR("tx data read error");
			break;
		}
		net_capture_pkt(iface, pkt);

		data->l2.ipc_tx.id = IPC_DISPATCHER_MK_ID(
			IPC_DISPATCHER_WIFI_L2_DATA, config->instance_id);
		ret = ipc_dispatcher_send(&data->l2.ipc_tx,
			offsetof(struct ipc_msg, data) + tx_len);
		if (ret < 0) {
			LOG_ERR("tx ipc send failed");
			break;
		}
	} while (0);

	if (ret >= 0 && pkt) {
		net_pkt_unref(pkt);
	}

	return ret;
}

static void wifi_w91_on_rx_data(const void *data, size_t len, void *ctx)
{
	bool rx_done = false;
	struct net_pkt *rx = NULL;

	do {
		if (len < offsetof(struct ipc_msg, data)) {
			LOG_ERR("rx malformed ip packet");
			break;
		}

		rx = net_pkt_rx_alloc_with_buffer((struct net_if *)ctx,
			len - offsetof(struct ipc_msg, data), AF_UNSPEC, 0, K_NO_WAIT);

		if (!rx) {
			LOG_ERR("rx ip packet not allocated");
			break;
		}
		if (net_pkt_write(rx, ((struct ipc_msg *)data)->data,
			len - offsetof(struct ipc_msg, data)) < 0) {
			LOG_ERR("rx write to a packet failed");
			break;
		}
		if (net_recv_data(net_pkt_iface(rx), rx) < 0) {
			LOG_ERR("rx data receive failed");
			break;
		}
		rx_done = true;
	} while (0);

	if (!rx_done && rx) {
		net_pkt_unref(rx);
	}
}

static int wifi_w91_enable(struct net_if *iface, bool state)
{
	LOG_INF("%s [%u]", __func__, state);

	__ASSERT(sizeof(struct ipc_msg) <= CONFIG_PBUF_RX_READ_BUF_SIZE,
		"IPC_SERVICE_ICMSG_CB_BUF_SIZE can't contain Ethernet MTU. %u (at least %u)",
		CONFIG_PBUF_RX_READ_BUF_SIZE, sizeof(struct ipc_msg));

	const struct device *dev = net_if_get_device(iface);
	const struct wifi_w91_config *config = dev->config;

	if (state) {
		ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(
			IPC_DISPATCHER_WIFI_L2_DATA, config->instance_id),
			wifi_w91_on_rx_data, iface);
	} else {
		ipc_dispatcher_rm(IPC_DISPATCHER_MK_ID(
			IPC_DISPATCHER_WIFI_L2_DATA, config->instance_id));
	}

	return 0;
}

static enum net_l2_flags wifi_w91_flags(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return NET_L2_MULTICAST;
}

NET_L2_INIT(W91_WIFI_L2, wifi_w91_recv, wifi_w91_send, wifi_w91_enable, wifi_w91_flags);
