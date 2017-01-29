/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_BLUETOOTH)
#define SYS_LOG_DOMAIN "net/bt"
#define NET_LOG_ENABLED 1
#endif

#include <kernel.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <errno.h>

#include <board.h>
#include <device.h>
#include <init.h>

#include <net/nbuf.h>
#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/bt.h>
#include <6lo.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/l2cap.h>

#include "ipv6.h"

#define L2CAP_IPSP_PSM 0x0023
#define L2CAP_IPSP_MTU 1280

#define CHAN_CTXT(_ch) CONTAINER_OF(_ch, struct bt_context, ipsp_chan.chan)

#if defined(CONFIG_NET_L2_BLUETOOTH_MGMT)
static struct bt_conn *default_conn;
#endif

struct bt_context {
	struct net_if *iface;
	struct bt_l2cap_le_chan ipsp_chan;
	bt_addr_t src;
	bt_addr_t dst;
};

static enum net_verdict net_bt_recv(struct net_if *iface, struct net_buf *buf)
{
	uint32_t src;
	uint32_t dst;

	NET_DBG("iface %p buf %p len %u", iface, buf, net_buf_frags_len(buf));

	/* Uncompress will drop the current fragment. Buf ll src/dst address
	 * will then be wrong and must be updated according to the new fragment.
	 */
	src = net_nbuf_ll_src(buf)->addr ?
		net_nbuf_ll_src(buf)->addr - net_nbuf_ll(buf) : 0;
	dst = net_nbuf_ll_dst(buf)->addr ?
		net_nbuf_ll_dst(buf)->addr - net_nbuf_ll(buf) : 0;

	if (!net_6lo_uncompress(buf)) {
		NET_DBG("Packet decompression failed");
		return NET_DROP;
	}

	net_nbuf_ll_src(buf)->addr = src ? net_nbuf_ll(buf) + src : NULL;
	net_nbuf_ll_dst(buf)->addr = dst ? net_nbuf_ll(buf) + dst : NULL;

	return NET_CONTINUE;
}

static enum net_verdict net_bt_send(struct net_if *iface, struct net_buf *buf)
{
	struct bt_context *ctxt = net_if_get_device(iface)->driver_data;

	NET_DBG("iface %p buf %p len %u", iface, buf, net_buf_frags_len(buf));

	/* Only accept IPv6 packets */
	if (net_nbuf_family(buf) != AF_INET6) {
		return NET_DROP;
	}

	/* TODO: Move ll address check to the stack */

	/* If the ll address is not set at all, then we must set
	 * it here.
	 */
	if (!net_nbuf_ll_src(buf)->addr) {
		net_nbuf_ll_src(buf)->addr = net_nbuf_ll_if(buf)->addr;
		net_nbuf_ll_src(buf)->len = net_nbuf_ll_if(buf)->len;
	}

	/* If the ll dst address is not set check if it is present in the nbr
	 * cache.
	 */
	if (!net_nbuf_ll_dst(buf)->addr &&
	    !net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
		buf = net_ipv6_prepare_for_send(buf);
		if (!buf) {
			return NET_CONTINUE;
		}
	}

	if (!net_6lo_compress(buf, true, NULL)) {
		NET_DBG("Packet compression failed");
		return NET_DROP;
	}

	net_if_queue_tx(ctxt->iface, buf);

	return NET_OK;
}

static inline uint16_t net_bt_reserve(struct net_if *iface, void *unused)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(unused);

	return 0;
}

static int net_bt_enable(struct net_if *iface, bool state)
{
	struct bt_context *ctxt = net_if_get_device(iface)->driver_data;

	NET_DBG("iface %p %s", iface, state ? "up" : "down");

	if (state && ctxt->ipsp_chan.chan.state != BT_L2CAP_CONNECTED) {
		return -ENETDOWN;
	}

	return 0;
}

NET_L2_INIT(BLUETOOTH_L2, net_bt_recv, net_bt_send, net_bt_reserve,
	    net_bt_enable);

static void ipsp_connected(struct bt_l2cap_chan *chan)
{
	struct bt_context *ctxt = CHAN_CTXT(chan);
	struct bt_conn_info info;
#if defined(CONFIG_NET_DEBUG_L2_BLUETOOTH)
	char src[BT_ADDR_LE_STR_LEN];
	char dst[BT_ADDR_LE_STR_LEN];
#endif

	bt_conn_get_info(chan->conn, &info);

#if defined(CONFIG_NET_DEBUG_L2_BLUETOOTH)
	bt_addr_le_to_str(info.le.src, src, sizeof(src));
	bt_addr_le_to_str(info.le.dst, dst, sizeof(dst));

	NET_DBG("Channel %p Source %s connected to Destination %s", chan,
		src, dst);
#endif

	/* Swap bytes since net APIs expect big endian address */
	sys_memcpy_swap(ctxt->src.val, info.le.src->a.val, sizeof(ctxt->src));
	sys_memcpy_swap(ctxt->dst.val, info.le.dst->a.val, sizeof(ctxt->dst));

	net_if_set_link_addr(ctxt->iface, ctxt->src.val, sizeof(ctxt->src.val));

	/* Set iface up */
	net_if_up(ctxt->iface);
}

static void ipsp_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_context *ctxt = CHAN_CTXT(chan);

	NET_DBG("Channel %p disconnected", chan);

	/* Set iface down */
	net_if_down(ctxt->iface);

#if defined(CONFIG_NET_L2_BLUETOOTH_MGMT)
	if (chan->conn != default_conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
#endif
}

static void ipsp_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_context *ctxt = CHAN_CTXT(chan);
	struct net_buf *nbuf;

	NET_DBG("Incoming data channel %p len %u", chan,
		net_buf_frags_len(buf));

	/* Get buffer for bearer / protocol related data */
	nbuf = net_nbuf_get_reserve_rx(0);

	/* Set destination address */
	net_nbuf_ll_dst(nbuf)->addr = ctxt->src.val;
	net_nbuf_ll_dst(nbuf)->len = sizeof(ctxt->src);

	/* Set source address */
	net_nbuf_ll_src(nbuf)->addr = ctxt->dst.val;
	net_nbuf_ll_src(nbuf)->len = sizeof(ctxt->dst);

	/* Add data buffer as fragment of RX buffer, take a reference while
	 * doing so since L2CAP will unref the buffer after return.
	 */
	net_buf_frag_add(nbuf, net_buf_ref(buf));

	if (net_recv_data(ctxt->iface, nbuf) < 0) {
		NET_DBG("Packet dropped by NET stack");
		net_nbuf_unref(nbuf);
	}
}

static struct net_buf *ipsp_alloc_buf(struct bt_l2cap_chan *chan)
{
	NET_DBG("Channel %p requires buffer", chan);

	return net_nbuf_get_reserve_data(0);
}

static struct bt_l2cap_chan_ops ipsp_ops = {
	.alloc_buf	= ipsp_alloc_buf,
	.recv		= ipsp_recv,
	.connected	= ipsp_connected,
	.disconnected	= ipsp_disconnected,
};

static struct bt_context bt_context_data = {
	.iface			= NULL,
	.ipsp_chan.chan.ops	= &ipsp_ops,
	.ipsp_chan.rx.mtu	= L2CAP_IPSP_MTU,
};

static int bt_iface_send(struct net_if *iface, struct net_buf *buf)
{
	struct bt_context *ctxt = net_if_get_device(iface)->driver_data;
	int ret;

	NET_DBG("iface %p buf %p len %u", iface, buf, net_buf_frags_len(buf));

	ret = bt_l2cap_chan_send(&ctxt->ipsp_chan.chan, buf);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static void bt_iface_init(struct net_if *iface)
{
	struct bt_context *ctxt = net_if_get_device(iface)->driver_data;

	NET_DBG("iface %p", iface);

	ctxt->iface = iface;
}

static struct net_if_api bt_if_api = {
	.init = bt_iface_init,
	.send = bt_iface_send,
};

static int ipsp_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	NET_DBG("Incoming conn %p", conn);

	if (bt_context_data.ipsp_chan.chan.conn) {
		NET_ERR("No channels available");
		return -ENOMEM;
	}

	*chan = &bt_context_data.ipsp_chan.chan;

	return 0;
}

static struct bt_l2cap_server server = {
	.psm		= L2CAP_IPSP_PSM,
	.sec_level	= CONFIG_NET_L2_BLUETOOTH_SEC_LEVEL,
	.accept		= ipsp_accept,
};

#if defined(CONFIG_NET_L2_BLUETOOTH_MGMT)

static int bt_connect(uint32_t mgmt_request, struct net_if *iface, void *data,
		      size_t len)
{
	struct bt_context *ctxt = net_if_get_device(iface)->driver_data;
	bt_addr_le_t *addr = data;

	if (len != sizeof(*addr)) {
		NET_ERR("Invalid address");
		return -EINVAL;
	}

	if (ctxt->ipsp_chan.chan.conn) {
		NET_ERR("No channels available");
		return -ENOMEM;
	}

	if (default_conn) {
		return bt_l2cap_chan_connect(default_conn,
					     &ctxt->ipsp_chan.chan,
					     L2CAP_IPSP_PSM);
	}

	default_conn = bt_conn_create_le(addr, BT_LE_CONN_PARAM_DEFAULT);

	return 0;
}

static bool eir_found(uint8_t type, const uint8_t *data, uint8_t data_len,
		      void *user_data)
{
	int i;
#if defined(CONFIG_NET_DEBUG_L2_BLUETOOTH)
	bt_addr_le_t *addr = user_data;
	char dev[BT_ADDR_LE_STR_LEN];
#endif
	if (type != BT_DATA_UUID16_SOME && type != BT_DATA_UUID16_ALL) {
		return false;
	}

	if (data_len % sizeof(uint16_t) != 0) {
		NET_ERR("AD malformed\n");
		return false;
	}

	for (i = 0; i < data_len; i += sizeof(uint16_t)) {
		uint16_t u16;

		memcpy(&u16, &data[i], sizeof(u16));
		if (sys_le16_to_cpu(u16) != BT_UUID_IPSS_VAL) {
			continue;
		}

#if defined(CONFIG_NET_DEBUG_L2_BLUETOOTH)
		bt_addr_le_to_str(addr, dev, sizeof(dev));
		NET_DBG("[DEVICE]: %s", dev);
#endif

		/* TODO: Notify device address found */
		net_mgmt_event_notify(NET_EVENT_BT_SCAN_RESULT,
				      bt_context_data.iface);

		return true;
	}

	return false;
}

static bool ad_parse(struct net_buf_simple *ad,
		     bool (*func)(uint8_t type, const uint8_t *data,
				  uint8_t data_len, void *user_data),
		     void *user_data)
{
	while (ad->len > 1) {
		uint8_t len = net_buf_simple_pull_u8(ad);
		uint8_t type;

		/* Check for early termination */
		if (len == 0) {
			return false;
		}

		if (len > ad->len || ad->len < 1) {
			NET_ERR("AD malformed\n");
			return false;
		}

		type = net_buf_simple_pull_u8(ad);

		if (func(type, ad->data, len - 1, user_data)) {
			return true;
		}

		net_buf_simple_pull(ad, len - 1);
	}

	return false;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	/* We're only interested in connectable events */
	if (type == BT_LE_ADV_IND || type == BT_LE_ADV_DIRECT_IND) {
		ad_parse(ad, eir_found, (void *)addr);
	}
}

static void bt_active_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		NET_ERR("Bluetooth set active scan failed (err %d)\n", err);
	}
}

static void bt_passive_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		NET_ERR("Bluetooth set passive scan failed (err %d)\n", err);
	}
}

static void bt_scan_off(void)
{
	int err;

	err = bt_le_scan_stop();
	if (err) {
		NET_ERR("Stopping scanning failed (err %d)\n", err);
	}
}

static int bt_scan(uint32_t mgmt_request, struct net_if *iface, void *data,
		   size_t len)
{
	if (!strcmp(data, "on") || !strcmp(data, "active")) {
		bt_active_scan();
	} else if (!strcmp(data, "passive")) {
		bt_passive_scan();
	} else if (!strcmp("off", data)) {
		bt_scan_off();
	} else {
		return -EINVAL;
	}

	return 0;
}

static int bt_disconnect(uint32_t mgmt_request, struct net_if *iface,
			 void *data, size_t len)
{
	struct bt_context *ctxt = net_if_get_device(iface)->driver_data;

	if (!ctxt->ipsp_chan.chan.conn) {
		NET_ERR("Not connected");
		return -ENOTCONN;
	}

	/* Release connect reference in case of central/router role */
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	return bt_l2cap_chan_disconnect(&ctxt->ipsp_chan.chan);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
#if defined(CONFIG_NET_DEBUG_L2_BLUETOOTH)
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		NET_ERR("Failed to connect to %s (%u)\n", addr, err);
#endif
		return;
	}

	if (conn != default_conn) {
		return;
	}

	bt_l2cap_chan_connect(conn, &bt_context_data.ipsp_chan.chan,
			      L2CAP_IPSP_PSM);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
#if defined(CONFIG_NET_DEBUG_L2_BLUETOOTH)
	char addr[BT_ADDR_LE_STR_LEN];
#endif

	if (conn != default_conn) {
		return;
	}

#if defined(CONFIG_NET_DEBUG_L2_BLUETOOTH)
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	NET_DBG("Disconnected: %s (reason %u)\n", addr, reason);
#endif

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};
#endif /* CONFIG_NET_L2_BLUETOOTH_MGMT */

static int net_bt_init(struct device *dev)
{
	NET_DBG("dev %p driver_data %p", dev, dev->driver_data);

#if defined(CONFIG_NET_L2_BLUETOOTH_MGMT)
	bt_conn_cb_register(&conn_callbacks);
#endif
	bt_l2cap_server_register(&server);

	return 0;
}

#if defined(CONFIG_NET_L2_BLUETOOTH_MGMT)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_BT_CONNECT, bt_connect);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_BT_SCAN, bt_scan);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_BT_DISCONNECT, bt_disconnect);
#endif

NET_DEVICE_INIT(net_bt, "net_bt", net_bt_init, &bt_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&bt_if_api, BLUETOOTH_L2,
		NET_L2_GET_CTX_TYPE(BLUETOOTH_L2), L2CAP_IPSP_MTU);
