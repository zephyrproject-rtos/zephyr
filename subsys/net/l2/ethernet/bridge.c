/*
 * Copyright (c) 2021 BayLibre SAS
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_eth_bridge, CONFIG_NET_ETHERNET_BRIDGE_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/sys/slist.h>
#include <zephyr/random/random.h>

#include "net_private.h"
#include "bridge.h"

#if defined(CONFIG_NET_ETHERNET_BRIDGE_TXRX_DEBUG)
#define DEBUG_TX 1
#define DEBUG_RX 1
#else
#define DEBUG_TX 0
#define DEBUG_RX 0
#endif

#define MAX_BRIDGE_NAME_LEN MIN(sizeof("bridge##"), CONFIG_NET_INTERFACE_NAME_LEN)
#define MAX_VIRT_NAME_LEN MIN(sizeof("<no config>"), CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN)

static void lock_bridge(struct eth_bridge_iface_context *ctx)
{
	k_mutex_lock(&ctx->lock, K_FOREVER);
}

static void unlock_bridge(struct eth_bridge_iface_context *ctx)
{
	k_mutex_unlock(&ctx->lock);
}

struct ud {
	eth_bridge_cb_t cb;
	void *user_data;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *br_user_data = user_data;
	struct eth_bridge_iface_context *ctx;
	enum virtual_interface_caps caps;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	caps = net_virtual_get_iface_capabilities(iface);
	if (!(caps & VIRTUAL_INTERFACE_BRIDGE)) {
		return;
	}

	ctx = net_if_get_device(iface)->data;

	br_user_data->cb(ctx, br_user_data->user_data);
}

void net_eth_bridge_foreach(eth_bridge_cb_t cb, void *user_data)
{
	struct ud br_user_data = {
		.cb = cb,
		.user_data = user_data,
	};

	net_if_foreach(iface_cb, &br_user_data);
}

int eth_bridge_get_index(struct net_if *br)
{
	return net_if_get_by_iface(br);
}

struct net_if *eth_bridge_get_by_index(int index)
{
	return net_if_get_by_index(index);
}

int eth_bridge_iface_add(struct net_if *br, struct net_if *iface)
{
	struct eth_bridge_iface_context *ctx = net_if_get_device(br)->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	bool found = false;
	int count = 0;
	int ret;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET) ||
	    !(net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE)) {
		return -EINVAL;
	}

	if (net_if_l2(br) != &NET_L2_GET_NAME(VIRTUAL) ||
	    !(net_virtual_get_iface_capabilities(br) & VIRTUAL_INTERFACE_BRIDGE)) {
		return -EINVAL;
	}

	lock_bridge(ctx);

	if (eth_ctx->bridge == br) {
		/* This Ethernet interface was already added to the bridge */
		found = true;
	}

	ARRAY_FOR_EACH(ctx->eth_iface, i) {
		if (!found && ctx->eth_iface[i] == NULL) {
			ctx->eth_iface[i] = iface;
			eth_ctx->bridge = br;
			found = true;
		}

		/* Calculate how many interfaces are added to this bridge */
		if (ctx->eth_iface[i] != NULL) {
			struct ethernet_context *tmp = net_if_l2_data(ctx->eth_iface[i]);

			if (tmp->bridge == br) {
				count++;
			}
		}
	}

	unlock_bridge(ctx);

	if (!found) {
		return -ENOMEM;
	}

	ret = net_eth_promisc_mode(iface, true);
	if (ret != 0 && ret != -EALREADY) {
		NET_DBG("iface %d promiscuous mode failed: %d",
			net_if_get_by_iface(iface), ret);
		eth_bridge_iface_remove(br, iface);
		return ret;
	}

	NET_DBG("iface %d added to bridge %d", net_if_get_by_iface(iface),
		net_if_get_by_iface(br));

	if (count > 1) {
		ctx->is_setup = true;

		NET_INFO("Bridge %d is %ssetup", net_if_get_by_iface(eth_ctx->bridge), "");

		net_virtual_set_name(ctx->iface, "<config ok>");
	}

	ctx->count = count;

	return 0;
}

int eth_bridge_iface_remove(struct net_if *br, struct net_if *iface)
{
	struct eth_bridge_iface_context *ctx = net_if_get_device(br)->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	bool found = false;
	int count = 0;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return -EINVAL;
	}

	if (net_if_l2(br) != &NET_L2_GET_NAME(VIRTUAL) ||
	    !(net_virtual_get_iface_capabilities(br) & VIRTUAL_INTERFACE_BRIDGE)) {
		return -EINVAL;
	}

	lock_bridge(ctx);

	ARRAY_FOR_EACH(ctx->eth_iface, i) {
		if (!found && ctx->eth_iface[i] == iface) {
			ctx->eth_iface[i] = NULL;
			eth_ctx->bridge = NULL;
			found = true;
		}

		/* Calculate how many interfaces are added to this bridge */
		if (ctx->eth_iface[i] != NULL) {
			struct ethernet_context *tmp = net_if_l2_data(ctx->eth_iface[i]);

			if (tmp->bridge == br) {
				count++;
			}
		}
	}

	unlock_bridge(ctx);

	NET_DBG("iface %d removed from bridge %d", net_if_get_by_iface(iface),
		net_if_get_by_iface(br));

	if (count < 2) {
		ctx->is_setup = false;

		NET_INFO("Bridge %d is %ssetup", net_if_get_by_iface(br), "not ");

		net_virtual_set_name(ctx->iface, "<no config>");
	}

	ctx->count = count;

	return 0;
}

static inline bool is_link_local_addr(struct net_eth_addr *addr)
{
	if (addr->addr[0] == 0x01 &&
	    addr->addr[1] == 0x80 &&
	    addr->addr[2] == 0xc2 &&
	    addr->addr[3] == 0x00 &&
	    addr->addr[4] == 0x00 &&
	    (addr->addr[5] & 0x0f) == 0x00) {
		return true;
	}

	return false;
}

static void random_linkaddr(uint8_t *linkaddr, size_t len)
{
	sys_rand_get(linkaddr, len);
}

static void bridge_iface_init(struct net_if *iface)
{
	struct eth_bridge_iface_context *ctx = net_if_get_device(iface)->data;
	struct virtual_interface_context *vctx = net_if_l2_data(iface);
	char name[MAX_BRIDGE_NAME_LEN];

	if (ctx->is_init) {
		return;
	}

	k_mutex_init(&ctx->lock);

	ctx->iface = iface;

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_clear(iface, NET_IF_IPV4);
	net_if_flag_clear(iface, NET_IF_IPV6);
	net_if_flag_clear(iface, NET_IF_FORWARD_MULTICASTS);

	net_virtual_set_flags(iface, NET_L2_PROMISC_MODE);

	snprintk(name, sizeof(name), "bridge%d", ctx->id);
	net_if_set_name(iface, name);

	net_virtual_set_name(iface, "<no config>");

	/* We need to set the link address here as normally it would be set in
	 * virtual interface API attach function but we do not use that in
	 * bridging.
	 */
	random_linkaddr(vctx->lladdr.addr, sizeof(vctx->lladdr.addr));

	vctx->lladdr.len = sizeof(vctx->lladdr.addr);
	vctx->lladdr.type = NET_LINK_UNKNOWN;

	net_if_set_link_addr(iface, vctx->lladdr.addr,
			     vctx->lladdr.len, vctx->lladdr.type);

	ctx->is_init = true;
	ctx->is_setup = false;
}

static enum virtual_interface_caps bridge_get_capabilities(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return VIRTUAL_INTERFACE_BRIDGE;
}

static int bridge_iface_start(const struct device *dev)
{
	struct eth_bridge_iface_context *ctx = dev->data;

	if (!ctx->is_setup) {
		NET_DBG("Bridge interface %d not configured yet.",
			net_if_get_by_iface(ctx->iface));
		return -ENOENT;
	}

	if (ctx->status) {
		return -EALREADY;
	}

	ctx->status = true;

	NET_DBG("Starting iface %d", net_if_get_by_iface(ctx->iface));

	NET_INFO("Bridge %d is %sactive", net_if_get_by_iface(ctx->iface), "");

	net_virtual_set_name(ctx->iface, "<enabled>");

	return 0;
}

static int bridge_iface_stop(const struct device *dev)
{
	struct eth_bridge_iface_context *ctx = dev->data;

	if (!ctx->status) {
		return -EALREADY;
	}

	ctx->status = false;

	NET_DBG("Stopping iface %d", net_if_get_by_iface(ctx->iface));

	NET_INFO("Bridge %d is %sactive", net_if_get_by_iface(ctx->iface), "not ");

	if (ctx->is_setup) {
		net_virtual_set_name(ctx->iface, "<disabled>");
	} else {
		net_virtual_set_name(ctx->iface, "<no config>");
	}

	return 0;
}

static enum net_verdict bridge_iface_process(struct net_if *iface,
					     struct net_pkt *pkt,
					     bool is_send)
{
	struct eth_bridge_iface_context *ctx = net_if_get_device(iface)->data;
	struct net_if *orig_iface;
	struct net_pkt *send_pkt;
	size_t count;

	/* Drop all link-local packets for now. */
	if (is_link_local_addr((struct net_eth_addr *)net_pkt_lladdr_dst(pkt))) {
		NET_DBG("DROP: lladdr");
		return NET_DROP;
	}

	lock_bridge(ctx);

	/* Keep the original packet interface so that we can send to each
	 * bridged interface.
	 */
	orig_iface = net_pkt_orig_iface(pkt);

	count = ctx->count;

	/* Pass the data to all the Ethernet interface except the originator
	 * Ethernet interface.
	 */
	ARRAY_FOR_EACH(ctx->eth_iface, i) {
		if (ctx->eth_iface[i] != NULL && ctx->eth_iface[i] != orig_iface) {
			/* Skip it if not up */
			if (!net_if_flag_is_set(ctx->eth_iface[i], NET_IF_UP)) {
				continue;
			}

			/* Clone the packet if we have more than two interfaces in the bridge
			 * because the first send might mess the data part of the message.
			 */
			if (count > 2) {
				send_pkt = net_pkt_clone(pkt, K_NO_WAIT);
				net_pkt_ref(send_pkt);
			} else {
				send_pkt = net_pkt_ref(pkt);
			}

			net_pkt_set_family(send_pkt, AF_UNSPEC);
			net_pkt_set_iface(send_pkt, ctx->eth_iface[i]);
			net_if_queue_tx(ctx->eth_iface[i], send_pkt);

			NET_DBG("%s iface %d pkt %p (ref %d)",
				is_send ? "Send" : "Recv",
				net_if_get_by_iface(ctx->eth_iface[i]),
				send_pkt, (int)atomic_get(&send_pkt->atomic_ref));

			net_pkt_unref(send_pkt);
		}
	}

	unlock_bridge(ctx);

	/* The packet was cloned by the caller so remove it here. */
	net_pkt_unref(pkt);

	return NET_OK;

}

int bridge_iface_send(struct net_if *iface, struct net_pkt *pkt)
{
	if (DEBUG_TX) {
		char str[sizeof("TX iface xx")];

		snprintk(str, sizeof(str), "TX iface %d",
			 net_if_get_by_iface(net_pkt_iface(pkt)));

		net_pkt_hexdump(pkt, str);
	}

	(void)bridge_iface_process(iface, pkt, true);

	return 0;
}

static enum net_verdict bridge_iface_recv(struct net_if *iface,
					  struct net_pkt *pkt)
{
	if (DEBUG_RX) {
		char str[sizeof("RX iface xx")];

		snprintk(str, sizeof(str), "RX iface %d",
			 net_if_get_by_iface(net_pkt_iface(pkt)));

		net_pkt_hexdump(pkt, str);
	}

	return bridge_iface_process(iface, pkt, false);
}

/* We cannot attach the bridge interface to Ethernet interface because
 * the attachment can be done to only one Ethernet interface and we
 * need to "attach" at least two Ethernet interfaces to the bridge interface.
 * So we return -ENOTSUP here so that the attachment fails if it is tried.
 */
static int bridge_iface_attach(struct net_if *br,
			       struct net_if *iface)
{
	ARG_UNUSED(br);
	ARG_UNUSED(iface);

	return -ENOTSUP;
}

static const struct virtual_interface_api bridge_iface_api = {
	.iface_api.init = bridge_iface_init,

	.get_capabilities = bridge_get_capabilities,
	.start = bridge_iface_start,
	.stop = bridge_iface_stop,
	.send = bridge_iface_send,
	.recv = bridge_iface_recv,
	.attach = bridge_iface_attach,
};

#define ETH_DEFINE_BRIDGE(x, _)						\
	static struct eth_bridge_iface_context bridge_context_data_##x = { \
		.id = x,						\
	};								\
	NET_VIRTUAL_INTERFACE_INIT_INSTANCE(bridge_##x,			\
					    "BRIDGE_" #x,		\
					    x,				\
					    NULL,			\
					    NULL,			\
					    &bridge_context_data_##x,	\
					    NULL, /* config */		\
					    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
					    &bridge_iface_api,		\
					    NET_ETH_MTU)

LISTIFY(CONFIG_NET_ETHERNET_BRIDGE_COUNT, ETH_DEFINE_BRIDGE, (;), _);
