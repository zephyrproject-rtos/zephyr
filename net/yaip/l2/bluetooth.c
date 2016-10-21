/*
 * Copyright (c) 2016 Intel Corporation.
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

#if defined(CONFIG_NET_DEBUG_L2_BLUETOOTH)
#define SYS_LOG_DOMAIN "net/bt"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_DEBUG 1
#endif

#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <errno.h>

#include <misc/sys_log.h>

#include <board.h>
#include <device.h>
#include <init.h>

#include <net/nbuf.h>
#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <6lo.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <bluetooth/l2cap.h>

#define L2CAP_IPSP_PSM 0x0023
#define L2CAP_IPSP_MTU 1280

#define CHAN_CTXT(_ch) CONTAINER_OF(_ch, struct bt_context, ipsp_chan.chan)

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

	NET_DBG("iface %p buf %p", iface, buf);

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

	NET_DBG("iface %p buf %p", iface, buf);

	/* Only accept IPv6 packets */
	if (net_nbuf_family(buf) != AF_INET6) {
		return NET_DROP;
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

NET_L2_INIT(BLUETOOTH_L2, net_bt_recv, net_bt_send, net_bt_reserve);

static void ipsp_connected(struct bt_l2cap_chan *chan)
{
	struct bt_context *ctxt = CHAN_CTXT(chan);
	struct bt_conn_info info;
#if NET_DEBUG
	char src[BT_ADDR_LE_STR_LEN];
	char dst[BT_ADDR_LE_STR_LEN];
#endif

	bt_conn_get_info(chan->conn, &info);

#if NET_DEBUG
	bt_addr_le_to_str(info.le.src, src, sizeof(src));
	bt_addr_le_to_str(info.le.dst, dst, sizeof(dst));

	NET_DBG("Channel %p Source %s connected to Destination %s", chan,
		src, dst);
#endif

	/* Swap bytes since net APIs expect big endian address */
	sys_memcpy_swap(ctxt->src.val, info.le.src->a.val, sizeof(ctxt->src));
	sys_memcpy_swap(ctxt->dst.val, info.le.dst->a.val, sizeof(ctxt->dst));

	net_if_set_link_addr(ctxt->iface, ctxt->src.val, sizeof(ctxt->src.val));
}

static void ipsp_disconnected(struct bt_l2cap_chan *chan)
{
	NET_DBG("Channel %p disconnected", chan);
}

static void ipsp_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_context *ctxt = CHAN_CTXT(chan);
	struct net_buf *nbuf;

	NET_DBG("Incoming data channel %p len %u", chan, buf->len);

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

	NET_DBG("iface %p buf %p", iface, buf);

	/* bt_l2cap_chan_send will attempt to unref but net_nbuf_unref does
	 * some extra book keeping so add a reference to prevent it to be
	 * freed.
	 */
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
	.accept		= ipsp_accept,
};

static int net_bt_init(struct device *dev)
{
	NET_DBG("dev %p driver_data %p", dev, dev->driver_data);

	bt_l2cap_server_register(&server);

	return 0;
}

NET_DEVICE_INIT(net_bt, "net_bt", net_bt_init, &bt_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&bt_if_api, BLUETOOTH_L2,
		NET_L2_GET_CTX_TYPE(BLUETOOTH_L2), L2CAP_IPSP_MTU);
