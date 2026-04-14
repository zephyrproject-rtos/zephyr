/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/net/wifi_mgmt.h>
#include "siwx91x_nwp_bus.h"
#include "siwx91x_wifi.h"

LOG_MODULE_DECLARE(siwx91x_wifi, CONFIG_WIFI_LOG_LEVEL);

int siwx91x_wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct siwx91x_wifi_config *config = dev->config;

	__ASSERT(net_buf_headroom(pkt->buffer) >= sizeof(struct siwx91x_frame_desc),
		 "No supported");
	net_buf_push(pkt->buffer, sizeof(struct siwx91x_frame_desc));
	siwx91x_nwp_send_frame(config->nwp_dev, pkt->buffer,
			       SLI_SEND_RAW_DATA, SLI_WLAN_DATA_Q,
			       SIWX91X_FRAME_FLAG_ASYNC);
	net_pkt_unref(pkt);
	return 0;
}

void siwx91x_wifi_on_rx(const struct siwx91x_nwp_wifi_cb *ctxt, struct net_buf *buf)
{
	struct net_if *iface = net_if_get_first_wifi();
	const struct net_linkaddr *ll = net_if_get_link_addr(iface);
	struct net_pkt *pkt;
	int ret;

	/* All socket are created with SLI_SI91X_SOCKET_FEAT_SYNCHRONOUS, so call to this callabck
	 * is not expected.
	 */
	__ASSERT(!IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD), "Unexpected NWP event");
	__ASSERT(buf->frags == NULL, "Corrupted data from NWP driver");
	net_buf_pull(buf, sizeof(struct siwx91x_frame_desc));

	/* Multicast Tx frames are echoed back by the AP. This breaks IPv6 DAD algorithm. */
	/* FIXME: use sl_wifi_configure_multicast_filter() to filter this */
	if (!memcmp(((const struct net_eth_hdr *)buf->data)->src.addr,
		    ll->addr, WIFI_MAC_ADDR_LEN)) {
		return;
	}
	if (CONFIG_NET_BUF_DATA_SIZE >= SIWX91X_MAX_PAYLOAD_SIZE) {
		/* In this case, NWP driver allcate data on network RX pool. Not need for
		 * duplication.
		 */
		pkt = net_pkt_rx_alloc_on_iface(iface, K_NO_WAIT);
		if (!pkt) {
			goto pkt_alloc_fail;
		}
		net_pkt_frag_add(pkt, net_buf_ref(buf));
	} else {
		pkt = net_pkt_rx_alloc_with_buffer(iface, buf->len, AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			goto pkt_alloc_fail;
		}
		ret = net_pkt_write(pkt, buf->data, buf->len);
		if (ret) {
			goto rx_fail;
		}
	}

	ret = net_recv_data(iface, pkt);
	if (ret) {
		goto rx_fail;
	}
	return;

rx_fail:
	net_pkt_unref(pkt);
pkt_alloc_fail:
	LOG_WRN("Dropped frame");
}
