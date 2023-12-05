/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_bt, CONFIG_NET_L2_BT_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <string.h>
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/capture.h>
#include <zephyr/net/bt.h>
#include <6lo.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/l2cap.h>

#include "net_private.h"
#include "ipv6.h"

#define BUF_TIMEOUT K_MSEC(50)

#define L2CAP_IPSP_PSM 0x0023
#define L2CAP_IPSP_MTU 1280

#define CHAN_CONN(_conn) CONTAINER_OF(_conn, struct bt_if_conn, ipsp_chan.chan)

#if defined(CONFIG_NET_L2_BT_MGMT)
static struct bt_conn *default_conn;
#endif

#if defined(CONFIG_NET_L2_BT_SHELL)
extern int net_bt_shell_init(void);
#else
#define net_bt_shell_init(...)
#endif

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
#define IPSP_FRAG_LEN CONFIG_NET_BUF_DATA_SIZE
#else
#define IPSP_FRAG_LEN L2CAP_IPSP_MTU
#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */

struct bt_if_conn {
	struct net_if *iface;
	struct bt_l2cap_le_chan ipsp_chan;
	bt_addr_t src;
	bt_addr_t dst;
};

struct bt_context {
	struct bt_if_conn conns[CONFIG_BT_MAX_CONN];
};

static enum net_verdict net_bt_recv(struct net_if *iface, struct net_pkt *pkt)
{
	NET_DBG("iface %p pkt %p len %zu", iface, pkt, net_pkt_get_len(pkt));

	if (!net_6lo_uncompress(pkt)) {
		NET_DBG("Packet decompression failed");
		return NET_DROP;
	}

	return NET_CONTINUE;
}

static struct bt_if_conn *net_bt_get_conn(struct net_if *iface)
{
	struct bt_context *ctxt = net_if_get_device(iface)->data;
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (ctxt->conns[i].iface == iface) {
			return &ctxt->conns[i];
		}
	}

	return NULL;
}

static int net_bt_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct bt_if_conn *conn = net_bt_get_conn(iface);
	struct net_buf *buffer;
	int length;
	int ret;

	NET_DBG("iface %p pkt %p len %zu", iface, pkt, net_pkt_get_len(pkt));

	/* Only accept IPv6 packets */
	if (net_pkt_family(pkt) != AF_INET6) {
		return -EINVAL;
	}

	ret = net_6lo_compress(pkt, true);
	if (ret < 0) {
		NET_DBG("Packet compression failed");
		return ret;
	}

	length = net_pkt_get_len(pkt);

	net_capture_pkt(iface, pkt);

	/* Detach data fragments for packet */
	buffer = pkt->buffer;
	pkt->buffer = NULL;

	ret = bt_l2cap_chan_send(&conn->ipsp_chan.chan, buffer);
	if (ret < 0) {
		NET_ERR("Unable to send packet: %d", ret);
		bt_l2cap_chan_disconnect(&conn->ipsp_chan.chan);
		net_buf_unref(buffer);
		return ret;
	}

	net_pkt_unref(pkt);

	return length;
}

static int net_bt_enable(struct net_if *iface, bool state)
{
	NET_DBG("iface %p %s", iface, state ? "up" : "down");

	return 0;
}

static enum net_l2_flags net_bt_flags(struct net_if *iface)
{
	/* TODO: add NET_L2_MULTICAST_SKIP_JOIN_SOLICIT_NODE once the stack
	 * supports Address Registration Option for neighbor discovery.
	 */
	return NET_L2_MULTICAST;
}

NET_L2_INIT(BLUETOOTH_L2, net_bt_recv, net_bt_send,
	    net_bt_enable, net_bt_flags);

static void ipsp_connected(struct bt_l2cap_chan *chan)
{
	struct bt_if_conn *conn = CHAN_CONN(chan);
	struct bt_conn_info info;
	struct net_linkaddr ll;
	struct in6_addr in6;

	if (bt_conn_get_info(chan->conn, &info) < 0) {
		NET_ERR("Unable to get connection info");
		bt_l2cap_chan_disconnect(chan);
		return;
	}

	if (CONFIG_NET_L2_BT_LOG_LEVEL >= LOG_LEVEL_DBG) {
		char src[BT_ADDR_LE_STR_LEN];
		char dst[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(info.le.src, src, sizeof(src));
		bt_addr_le_to_str(info.le.dst, dst, sizeof(dst));

		NET_DBG("Channel %p Source %s connected to Destination %s",
			chan, src, dst);
	}

	/* Swap bytes since net APIs expect big endian address */
	sys_memcpy_swap(conn->src.val, info.le.src->a.val, sizeof(conn->src));
	sys_memcpy_swap(conn->dst.val, info.le.dst->a.val, sizeof(conn->dst));

	net_if_set_link_addr(conn->iface, conn->src.val, sizeof(conn->src.val),
			     NET_LINK_BLUETOOTH);

	ll.addr = conn->dst.val;
	ll.len = sizeof(conn->dst.val);
	ll.type = NET_LINK_BLUETOOTH;

	/* Add remote link-local address to the nbr cache to avoid sending ns:
	 * https://tools.ietf.org/html/rfc7668#section-3.2.3
	 * A Bluetooth LE 6LN MUST NOT register its link-local address.
	 */
	net_ipv6_addr_create_iid(&in6, &ll);
	net_ipv6_nbr_add(conn->iface, &in6, &ll, false,
			 NET_IPV6_NBR_STATE_STATIC);

	/* Leave dormant state (iface goes up if set to admin up) */
	net_if_dormant_off(conn->iface);
}

static void ipsp_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_if_conn *conn = CHAN_CONN(chan);

	NET_DBG("Channel %p disconnected", chan);

	/* Enter dormant state (iface goes down) */
	net_if_dormant_on(conn->iface);

#if defined(CONFIG_NET_L2_BT_MGMT)
	if (chan->conn != default_conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
#endif
}

static int ipsp_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_if_conn *conn = CHAN_CONN(chan);
	struct net_pkt *pkt;

	NET_DBG("Incoming data channel %p len %zu", chan,
		net_buf_frags_len(buf));

	/* Get packet for bearer / protocol related data */
	pkt = net_pkt_rx_alloc_on_iface(conn->iface, BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	/* Set destination address */
	net_pkt_lladdr_dst(pkt)->addr = conn->src.val;
	net_pkt_lladdr_dst(pkt)->len = sizeof(conn->src);
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_BLUETOOTH;

	/* Set source address */
	net_pkt_lladdr_src(pkt)->addr = conn->dst.val;
	net_pkt_lladdr_src(pkt)->len = sizeof(conn->dst);
	net_pkt_lladdr_src(pkt)->type = NET_LINK_BLUETOOTH;

	/* Add data buffer as fragment of RX buffer, take a reference while
	 * doing so since L2CAP will unref the buffer after return.
	 */
	net_pkt_append_buffer(pkt, net_buf_ref(buf));

	if (net_recv_data(conn->iface, pkt) < 0) {
		NET_DBG("Packet dropped by NET stack");
		net_pkt_unref(pkt);
	}

	return 0;
}

static struct net_buf *ipsp_alloc_buf(struct bt_l2cap_chan *chan)
{
	NET_DBG("Channel %p requires buffer", chan);

	return net_pkt_get_reserve_rx_data(IPSP_FRAG_LEN, BUF_TIMEOUT);
}

static const struct bt_l2cap_chan_ops ipsp_ops = {
	.alloc_buf	= ipsp_alloc_buf,
	.recv		= ipsp_recv,
	.connected	= ipsp_connected,
	.disconnected	= ipsp_disconnected,
};

static struct bt_context bt_context_data = {
	.conns[0 ... (CONFIG_BT_MAX_CONN - 1)] = {
		.iface			= NULL,
		.ipsp_chan.chan.ops	= &ipsp_ops,
		.ipsp_chan.rx.mtu	= L2CAP_IPSP_MTU,
	}
};

static void bt_iface_init(struct net_if *iface)
{
	struct bt_context *ctxt = net_if_get_device(iface)->data;
	struct bt_if_conn *conn = NULL;
	int i;

	NET_DBG("iface %p", iface);

	/* Find unused slot to store the iface */
	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (!ctxt->conns[i].iface) {
			conn = &ctxt->conns[i];
			NET_DBG("[%d] alloc ctxt %p iface %p", i, ctxt, iface);
			break;
		}
	}

	if (!conn) {
		NET_ERR("Unable to allocate iface");
		return;
	}

	conn->iface = iface;

	net_if_dormant_on(iface);

#if defined(CONFIG_NET_L2_BT_ZEP1656)
	/* Workaround Linux bug, see:
	 * https://github.com/zephyrproject-rtos/zephyr/issues/3111
	 */
	net_if_flag_set(iface, NET_IF_POINTOPOINT);
#endif
}

static struct net_if_api bt_if_api = {
	.init = bt_iface_init,
};

static int ipsp_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
		       struct bt_l2cap_chan **chan)
{
	struct bt_if_conn *if_conn = NULL;
	int i;

	NET_DBG("Incoming conn %p", (void *)conn);

	/* Find unused slot to store the iface */
	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (bt_context_data.conns[i].iface &&
		    !bt_context_data.conns[i].ipsp_chan.chan.conn) {
			if_conn = &bt_context_data.conns[i];
			break;
		}
	}

	if (!if_conn) {
		NET_ERR("No channels available");
		return -ENOMEM;
	}

	*chan = &if_conn->ipsp_chan.chan;

	return 0;
}

static struct bt_l2cap_server server = {
	.psm		= L2CAP_IPSP_PSM,
	.sec_level	= CONFIG_NET_L2_BT_SEC_LEVEL,
	.accept		= ipsp_accept,
};

#if defined(CONFIG_NET_L2_BT_MGMT)

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)
#define UNKNOWN_APPEARANCE	0x0000

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_IPSS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static int bt_advertise(uint32_t mgmt_request, struct net_if *iface, void *data,
		      size_t len)
{
	if (!strcmp(data, "on")) {
		return bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
				       sd, ARRAY_SIZE(sd));
	} else if (!strcmp(data, "off")) {
		return bt_le_adv_stop();
	} else {
		return -EINVAL;
	}

	return 0;
}

static int bt_connect(uint32_t mgmt_request, struct net_if *iface, void *data,
		      size_t len)
{
	struct bt_if_conn *conn = net_bt_get_conn(iface);
	bt_addr_le_t *addr = data;

	if (len != sizeof(*addr)) {
		NET_ERR("Invalid address");
		return -EINVAL;
	}

	if (conn->ipsp_chan.chan.conn) {
		NET_ERR("No channels available");
		return -ENOMEM;
	}

	if (default_conn) {
		return bt_l2cap_chan_connect(default_conn,
					     &conn->ipsp_chan.chan,
					     L2CAP_IPSP_PSM);
	}

	return bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				 BT_LE_CONN_PARAM_DEFAULT, &default_conn);
}

static bool eir_found(uint8_t type, const uint8_t *data, uint8_t data_len,
		      void *user_data)
{
	int i;

	if (type != BT_DATA_UUID16_SOME && type != BT_DATA_UUID16_ALL) {
		return false;
	}

	if (data_len % sizeof(uint16_t) != 0U) {
		NET_ERR("AD malformed\n");
		return false;
	}

	for (i = 0; i < data_len; i += sizeof(uint16_t)) {
		const struct bt_uuid *uuid;
		uint16_t u16;

		memcpy(&u16, &data[i], sizeof(u16));
		uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
		if (bt_uuid_cmp(uuid, BT_UUID_IPSS)) {
			continue;
		}

		if (CONFIG_NET_L2_BT_LOG_LEVEL >= LOG_LEVEL_DBG) {
			bt_addr_le_t *addr = user_data;
			char dev[BT_ADDR_LE_STR_LEN];

			bt_addr_le_to_str(addr, dev, sizeof(dev));
			NET_DBG("[DEVICE]: %s", dev);
		}

		/* TODO: Notify device address found */
		net_mgmt_event_notify(NET_EVENT_BT_SCAN_RESULT,
				      bt_context_data.conns[0].iface);

		return true;
	}

	return false;
}

static bool ad_parse(struct net_buf_simple *ad_buf,
		     bool (*func)(uint8_t type, const uint8_t *data,
				  uint8_t data_len, void *user_data),
		     void *user_data)
{
	while (ad_buf->len > 1) {
		uint8_t len = net_buf_simple_pull_u8(ad_buf);
		uint8_t type;

		/* Check for early termination */
		if (len == 0U) {
			return false;
		}

		if (len > ad_buf->len) {
			NET_ERR("AD malformed\n");
			return false;
		}

		type = net_buf_simple_pull_u8(ad_buf);

		if (func(type, ad_buf->data, len - 1, user_data)) {
			return true;
		}

		net_buf_simple_pull(ad_buf, len - 1);
	}

	return false;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad_buf)
{
	/* We're only interested in connectable events */
	if (type == BT_GAP_ADV_TYPE_ADV_IND ||
	    type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		ad_parse(ad_buf, eir_found, (void *)addr);
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
	struct bt_if_conn *conn = net_bt_get_conn(iface);

	if (!conn->ipsp_chan.chan.conn) {
		NET_ERR("Not connected");
		return -ENOTCONN;
	}

	/* Release connect reference in case of central/router role */
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	return bt_l2cap_chan_disconnect(&conn->ipsp_chan.chan);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	int i;

	if (err) {
		if (CONFIG_NET_L2_BT_LOG_LEVEL >= LOG_LEVEL_DBG) {
			char addr[BT_ADDR_LE_STR_LEN];

			bt_addr_le_to_str(bt_conn_get_dst(conn), addr,
					  sizeof(addr));

			NET_ERR("Failed to connect to %s (%u)\n",
				addr, err);
		}

		return;
	}

	if (conn != default_conn) {
		return;
	}

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_if_conn *if_conn = &bt_context_data.conns[i];

		if (if_conn->ipsp_chan.chan.conn == conn) {
			bt_l2cap_chan_connect(conn, &if_conn->ipsp_chan.chan,
					      L2CAP_IPSP_PSM);
			break;
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != default_conn) {
		return;
	}

	if (CONFIG_NET_L2_BT_LOG_LEVEL >= LOG_LEVEL_DBG) {
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		NET_DBG("Disconnected: %s (reason 0x%02x)\n",
			addr, reason);
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};
#endif /* CONFIG_NET_L2_BT_MGMT */

static int net_bt_init(const struct device *dev)
{
	int err;

	NET_DBG("dev %p driver_data %p", dev, dev->data);

	err = bt_l2cap_server_register(&server);
	if (err) {
		return err;
	}

	net_bt_shell_init();

	return 0;
}

#if defined(CONFIG_NET_L2_BT_MGMT)
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_BT_ADVERTISE, bt_advertise);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_BT_CONNECT, bt_connect);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_BT_SCAN, bt_scan);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_BT_DISCONNECT, bt_disconnect);
#endif

DEVICE_DEFINE(net_bt, "net_bt", net_bt_init, NULL, &bt_context_data, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &bt_if_api);
NET_L2_DATA_INIT(net_bt, 0, NET_L2_GET_CTX_TYPE(BLUETOOTH_L2));
NET_IF_INIT(net_bt, 0, BLUETOOTH_L2, L2CAP_IPSP_MTU, CONFIG_BT_MAX_CONN);
