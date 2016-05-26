/* net_driver_bt.c - IP Bluetooth LE driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include <net/buf.h>
#include <net/ip_buf.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_socket.h>
#include "contiki/netstack.h"
#include <net_driver_bt.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/gatt.h>

#define L2CAP_IPSP_PSM 0x0023
#define L2CAP_IPSP_MTU IP_BUF_MAX_DATA

static inline void memswap(void *dst, const void *src, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		((uint8_t *)dst)[i] = ((uint8_t *)src)[(len - 1) - i];
	}
}

static void ipsp_connected(struct bt_l2cap_chan *chan)
{
	struct bt_conn_info info;
	char src[BT_ADDR_LE_STR_LEN];
	char dst[BT_ADDR_LE_STR_LEN];
	linkaddr_t addr;

	bt_conn_get_info(chan->conn, &info);

	bt_addr_le_to_str(info.le.src, src, sizeof(src));
	bt_addr_le_to_str(info.le.dst, dst, sizeof(dst));

	NET_DBG("Channel %p Source %s connected to Destination %s\n", chan,
		src, dst);

	/* Swap bytes since net_set_mac expect big endian address */
	memswap(addr.u8, info.le.src->a.val, sizeof(addr.u8));

	net_set_mac(addr.u8, sizeof(addr));
}

static void ipsp_disconnected(struct bt_l2cap_chan *chan)
{
	NET_DBG("Channel %p disconnected\n", chan);
}

static void ipsp_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_conn_info info;
	linkaddr_t src;
	linkaddr_t dst;

	NET_DBG("Incoming data channel %p len %u\n", chan, ip_buf_len(buf));

	bt_conn_get_info(chan->conn, &info);

	/* Swap bytes since linkaddr_copy expect big endian address */
	memswap(src.u8, info.le.src->a.val, sizeof(src));
	memswap(dst.u8, info.le.dst->a.val, sizeof(dst));

	/* Add MAC addresses to the buffer */
	linkaddr_copy(&ip_buf_ll_dest(buf), &src);
	linkaddr_copy(&ip_buf_ll_src(buf), &dst);

	/* Initialize uip_len */
	uip_len(buf) = ip_buf_len(buf);
	uip_first_frag_len(buf) = 0;

	/* Uncompress data */
	if (!NETSTACK_COMPRESS.uncompress(buf)) {
		NET_ERR("uncompression failed\n");
		return;
	}

	/* net_recv takes ownership of the buffer */
	net_buf_ref(buf);

	/* Add buffer to rx_queue */
	if (net_recv(buf) < 0) {
		NET_ERR("input to IP stack failed\n");
		net_buf_unref(buf);
		return;
	}
}

static struct net_buf *ipsp_alloc_buf(struct bt_l2cap_chan *chan)
{
	NET_DBG("Channel %p requires buffer\n", chan);

	return ip_buf_get_reserve_rx(0);
}

static struct bt_l2cap_chan_ops ipsp_ops = {
	.alloc_buf	= ipsp_alloc_buf,
	.recv		= ipsp_recv,
	.connected	= ipsp_connected,
	.disconnected	= ipsp_disconnected,
};

static struct bt_l2cap_le_chan ipsp_chan = {
	.chan.ops	= &ipsp_ops,
	.rx.mtu		= L2CAP_IPSP_MTU,
};

static int ipsp_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	NET_DBG("Incoming conn %p\n", conn);

	if (ipsp_chan.chan.conn) {
		NET_ERR("No channels available");
		return -ENOMEM;
	}

	*chan = &ipsp_chan.chan;

	return 0;
}

static struct bt_l2cap_server server = {
	.psm		= L2CAP_IPSP_PSM,
	.accept		= ipsp_accept,
};

static struct bt_gatt_attr attrs[] = {
	/* IPSS Service Declaration */
	BT_GATT_PRIMARY_SERVICE(BT_UUID_IPSS),
};

static int net_driver_bt_open(void)
{
	bt_gatt_register(attrs, ARRAY_SIZE(attrs));

	bt_l2cap_server_register(&server);

	return 0;
}

static int net_driver_bt_send(struct net_buf *buf)
{
#ifdef CONFIG_NETWORKING_WITH_LOGGING
	int orig_len = ip_buf_len(buf);
#endif

	if (!NETSTACK_COMPRESS.compress(buf)) {
		NET_DBG("compression failed\n");
		ip_buf_unref(buf);
		return -EINVAL;
	}

	NET_DBG("sending %d bytes (original len %d)\n", ip_buf_len(buf),
		orig_len);

	return bt_l2cap_chan_send(&ipsp_chan.chan, buf);
}

static struct net_driver net_driver_bt = {
	.head_reserve = 0,
	.open = net_driver_bt_open,
	.send = net_driver_bt_send,
};

int net_driver_bt_init(void)
{
	NETSTACK_COMPRESS.init();

	net_register_driver(&net_driver_bt);

	return 0;
}
