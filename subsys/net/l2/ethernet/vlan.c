/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ethernet_vlan, CONFIG_NET_L2_ETHERNET_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/virtual.h>
#include <zephyr/random/random.h>

#include "net_private.h"

#if defined(CONFIG_NET_VLAN_TXRX_DEBUG)
#define DEBUG_TX 1
#define DEBUG_RX 1
#else
#define DEBUG_TX 0
#define DEBUG_RX 0
#endif

#define MAX_VLAN_NAME_LEN MIN(sizeof("VLAN-<#####>"), \
			      CONFIG_NET_INTERFACE_NAME_LEN)
#define MAX_VIRT_NAME_LEN MIN(sizeof("<not attached>"), \
			      CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN)

static void vlan_iface_init(struct net_if *iface);
static int vlan_interface_attach(struct net_if *vlan_iface,
				 struct net_if *iface);
static enum net_verdict vlan_interface_recv(struct net_if *iface,
					    struct net_pkt *pkt);
static int vlan_interface_send(struct net_if *iface, struct net_pkt *pkt);
static int vlan_interface_stop(const struct device *dev);
static enum virtual_interface_caps vlan_get_capabilities(struct net_if *iface);
static int vlan_interface_start(const struct device *dev);
static int virt_dev_init(const struct device *dev);

static K_MUTEX_DEFINE(lock);

struct vlan_context {
	struct net_if *iface;
	struct net_if *attached_to;
	uint16_t tag;
	bool status : 1;    /* Is the interface enabled or not */
	bool is_used : 1;   /* Is there active config on this context */
	bool init_done : 1; /* Is interface init called for this context */
};

static const struct virtual_interface_api vlan_iface_api = {
	.iface_api.init = vlan_iface_init,

	.get_capabilities = vlan_get_capabilities,
	.start = vlan_interface_start,
	.stop = vlan_interface_stop,
	.send = vlan_interface_send,
	.recv = vlan_interface_recv,
	.attach = vlan_interface_attach,
};

#define ETH_DEFINE_VLAN(x, _)						\
	static struct vlan_context vlan_context_data_##x = {		\
		.tag = NET_VLAN_TAG_UNSPEC,				\
	};								\
	NET_VIRTUAL_INTERFACE_INIT_INSTANCE(vlan_##x,			\
					    "VLAN_" #x,			\
					    x,				\
					    virt_dev_init,		\
					    NULL,			\
					    &vlan_context_data_##x,	\
					    NULL, /* config */		\
					    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
					    &vlan_iface_api,		\
					    NET_ETH_MTU)

LISTIFY(CONFIG_NET_VLAN_COUNT, ETH_DEFINE_VLAN, (;), _);

#define INIT_VLAN_CONTEXT_PTR(x, _)					\
	[x] = &vlan_context_data_##x					\

static struct vlan_context *vlan_ctx[] = {
	LISTIFY(CONFIG_NET_VLAN_COUNT, INIT_VLAN_CONTEXT_PTR, (,), _)
};

#define INIT_VLAN_CONTEXT_IFACE(x, _)					\
	vlan_context_data_##x.iface = NET_IF_GET(vlan_##x, x)

static void init_context_iface(void)
{
	static bool init_done;

	if (init_done) {
		return;
	}

	init_done = true;

	LISTIFY(CONFIG_NET_VLAN_COUNT, INIT_VLAN_CONTEXT_IFACE, (;), _);
}

static int virt_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	init_context_iface();

	return 0;
}

static struct vlan_context *get_vlan_ctx(struct net_if *main_iface,
					 uint16_t vlan_tag,
					 bool any_tag)
{
	struct virtual_interface_context *vctx, *tmp;
	sys_slist_t *interfaces;
	struct vlan_context *ctx;

	interfaces = &main_iface->config.virtual_interfaces;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(interfaces, vctx, tmp, node) {
		enum virtual_interface_caps caps;

		if (vctx->virtual_iface == NULL) {
			continue;
		}

		caps = net_virtual_get_iface_capabilities(vctx->virtual_iface);
		if (!(caps & VIRTUAL_INTERFACE_VLAN)) {
			continue;
		}

		ctx = net_if_get_device(vctx->virtual_iface)->data;
		NET_ASSERT(vctx != NULL);

		if (any_tag) {
			if (ctx->tag != NET_VLAN_TAG_UNSPEC) {
				return ctx;
			}
		} else {
			if ((vlan_tag == NET_VLAN_TAG_UNSPEC ||
			     vlan_tag == ctx->tag)) {
				return ctx;
			}
		}
	}

	return NULL;
}

static struct vlan_context *get_vlan(struct net_if *iface,
				     uint16_t vlan_tag)
{
	struct vlan_context *ctx = NULL;

	k_mutex_lock(&lock, K_FOREVER);

	/* If the interface is NULL, then get the VLAN that has the tag */
	if (iface == NULL) {
		ARRAY_FOR_EACH(vlan_ctx, i) {
			if (vlan_ctx[i] == NULL || !vlan_ctx[i]->is_used) {
				continue;
			}

			if (vlan_tag == vlan_ctx[i]->tag) {
				ctx = vlan_ctx[i];
				break;
			}
		}

		goto out;
	}

	/* If the interface is the main Ethernet one, then we only need
	 * to go through its attached virtual interfaces.
	 */
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {

		ctx = get_vlan_ctx(iface, vlan_tag, false);
		goto out;

	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		goto out;
	}

	/* If the interface is virtual, then it should be be the VLAN one.
	 * Just get the Ethernet interface it points to to get the context.
	 */
	ctx = get_vlan_ctx(net_virtual_get_iface(iface), vlan_tag, false);

out:
	k_mutex_unlock(&lock);

	return ctx;
}

static void set_priority(struct net_pkt *pkt)
{
	uint8_t vlan_priority;

	vlan_priority = net_priority2vlan(net_pkt_priority(pkt));
	net_pkt_set_vlan_priority(pkt, vlan_priority);
}

struct net_if *net_eth_get_vlan_iface(struct net_if *iface, uint16_t tag)
{
	struct vlan_context *ctx;

	ctx = get_vlan(iface, tag);
	if (ctx == NULL) {
		return NULL;
	}

	return ctx->iface;
}

struct net_if *net_eth_get_vlan_main(struct net_if *iface)
{
	struct vlan_context *ctx;

	ctx = get_vlan(iface, NET_VLAN_TAG_UNSPEC);
	if (ctx == NULL) {
		return NULL;
	}

	return ctx->attached_to;
}

static bool enable_vlan_iface(struct vlan_context *ctx,
			      struct net_if *iface)
{
	int iface_idx = net_if_get_by_iface(iface);
	char name[MAX(MAX_VLAN_NAME_LEN, MAX_VIRT_NAME_LEN)];
	int ret;

	if (iface_idx < 0) {
		return false;
	}

	ret = net_virtual_interface_attach(ctx->iface, iface);
	if (ret < 0) {
		NET_DBG("Cannot attach iface %d to %d",
			net_if_get_by_iface(ctx->iface),
			net_if_get_by_iface(ctx->attached_to));
		return false;
	}

	ctx->is_used = true;

	snprintk(name, sizeof(name), "VLAN-%d", ctx->tag);
	net_if_set_name(ctx->iface, name);

	snprintk(name, sizeof(name), "VLAN to %d",
		 net_if_get_by_iface(ctx->attached_to));
	net_virtual_set_name(ctx->iface, name);

	return true;
}

static bool disable_vlan_iface(struct vlan_context *ctx,
			       struct net_if *iface)
{
	int iface_idx = net_if_get_by_iface(iface);
	char name[MAX(MAX_VLAN_NAME_LEN, MAX_VIRT_NAME_LEN)];

	if (iface_idx < 0) {
		return false;
	}

	(void)net_virtual_interface_attach(iface, NULL);
	ctx->is_used = false;

	snprintk(name, sizeof(name), "VLAN-<free>");
	net_if_set_name(iface, name);

	snprintk(name, sizeof(name), "<not attached>");
	net_virtual_set_name(iface, name);

	return true;
}

static bool is_vlan_enabled_for_iface(struct net_if *iface)
{
	int iface_idx = net_if_get_by_iface(iface);
	struct vlan_context *ctx;
	bool ret = false;

	if (iface_idx < 0) {
		return false;
	}

	k_mutex_lock(&lock, K_FOREVER);

	ctx = get_vlan_ctx(iface, NET_VLAN_TAG_UNSPEC, true);
	ret = (ctx != NULL);

	k_mutex_unlock(&lock);

	return ret;
}

bool net_eth_is_vlan_enabled(struct ethernet_context *ctx,
			     struct net_if *iface)
{
	ARG_UNUSED(ctx);

	return is_vlan_enabled_for_iface(iface);
}

uint16_t net_eth_get_vlan_tag(struct net_if *iface)
{
	uint16_t tag = NET_VLAN_TAG_UNSPEC;
	struct vlan_context *ctx;

	k_mutex_lock(&lock, K_FOREVER);

	ctx = get_vlan_ctx(iface, tag, true);
	if (ctx != NULL) {
		/* The Ethernet interface does not have a tag so if user
		 * tried to use the main interface, then do not return
		 * the tag.
		 */
		if (ctx->attached_to != iface) {
			tag = ctx->tag;
		}
	}

	k_mutex_unlock(&lock);

	return tag;
}

bool net_eth_get_vlan_status(struct net_if *iface)
{
	bool status = false;
	struct vlan_context *ctx;

	k_mutex_lock(&lock, K_FOREVER);

	ctx = get_vlan_ctx(iface, NET_VLAN_TAG_UNSPEC, true);
	if (ctx != NULL) {
		status = ctx->status;
	}

	k_mutex_unlock(&lock);

	return status;
}

static void setup_link_address(struct vlan_context *ctx)
{
	struct net_linkaddr *ll_addr;

	ll_addr = net_if_get_link_addr(ctx->attached_to);

	(void)net_if_set_link_addr(ctx->iface,
				   ll_addr->addr,
				   ll_addr->len,
				   ll_addr->type);
}

int net_eth_vlan_enable(struct net_if *iface, uint16_t tag)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	const struct ethernet_api *eth = net_if_get_device(iface)->api;
	struct vlan_context *vlan;
	int ret;

	if (!eth) {
		return -ENOENT;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return -EINVAL;
	}

	if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_HW_VLAN)) {
		NET_DBG("Interface %d does not support VLAN",
			net_if_get_by_iface(iface));
		return -ENOTSUP;
	}

	if (!ctx->is_init) {
		return -EPERM;
	}

	if (tag >= NET_VLAN_TAG_UNSPEC) {
		return -EBADF;
	}

	vlan = get_vlan(iface, tag);
	if (vlan != NULL) {
		return -EALREADY;
	}

	/* This will make sure that the tag is not yet in use by some
	 * other interface.
	 */
	vlan = get_vlan(NULL, tag);
	if (vlan != NULL) {
		return -EALREADY;
	}

	ret = -ENOSPC;

	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(vlan_ctx, i) {
		if (vlan_ctx[i] == NULL || vlan_ctx[i]->is_used) {
			continue;
		}

		vlan = vlan_ctx[i];
		vlan->tag = tag;

		if (!enable_vlan_iface(vlan, iface)) {
			continue;
		}

		NET_DBG("[%d] Adding vlan tag %d to iface %d (%p) attached to %d (%p)",
			i, vlan->tag, net_if_get_by_iface(vlan->iface), vlan->iface,
			net_if_get_by_iface(iface), iface);

		/* Use MAC address of the attached Ethernet interface so that
		 * packet reception works without any tweaks.
		 */
		setup_link_address(vlan);

		if (eth->vlan_setup) {
			eth->vlan_setup(net_if_get_device(iface),
					iface, vlan->tag, true);
		}

		ethernet_mgmt_raise_vlan_enabled_event(vlan->iface, vlan->tag);

		ret = 0;
		break;
	}

	k_mutex_unlock(&lock);

	return ret;
}

int net_eth_vlan_disable(struct net_if *iface, uint16_t tag)
{
	const struct ethernet_api *eth;
	struct vlan_context *vlan;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET) &&
	    net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return -EINVAL;
	}

	if (tag == NET_VLAN_TAG_UNSPEC) {
		return -EBADF;
	}

	vlan = get_vlan(iface, tag);
	if (!vlan) {
		return -ESRCH;
	}

	eth = net_if_get_device(vlan->attached_to)->api;

	k_mutex_lock(&lock, K_FOREVER);

	NET_DBG("Removing vlan tag %d from VLAN iface %d (%p) attached to %d (%p)",
		vlan->tag, net_if_get_by_iface(vlan->iface), vlan->iface,
		net_if_get_by_iface(vlan->attached_to), vlan->attached_to);

	vlan->tag = NET_VLAN_TAG_UNSPEC;

	if (eth->vlan_setup) {
		eth->vlan_setup(net_if_get_device(vlan->attached_to),
				vlan->attached_to, tag, false);
	}

	ethernet_mgmt_raise_vlan_disabled_event(vlan->iface, tag);

	(void)disable_vlan_iface(vlan, vlan->iface);

	k_mutex_unlock(&lock);

	return 0;
}

static enum virtual_interface_caps vlan_get_capabilities(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return VIRTUAL_INTERFACE_VLAN;
}

static int vlan_interface_start(const struct device *dev)
{
	struct vlan_context *ctx = dev->data;

	if (!ctx->is_used) {
		NET_DBG("VLAN interface %d not configured yet.",
			net_if_get_by_iface(ctx->iface));
		return -ENOENT;
	}

	if (ctx->status) {
		return -EALREADY;
	}

	ctx->status = true;

	NET_DBG("Starting iface %d", net_if_get_by_iface(ctx->iface));

	/* You can implement here any special action that is needed
	 * when the network interface is coming up.
	 */

	return 0;
}

static int vlan_interface_stop(const struct device *dev)
{
	struct vlan_context *ctx = dev->data;

	if (!ctx->is_used) {
		NET_DBG("VLAN interface %d not configured yet.",
			net_if_get_by_iface(ctx->iface));
		return -ENOENT;
	}

	if (!ctx->status) {
		return -EALREADY;
	}

	ctx->status = false;

	NET_DBG("Stopping iface %d", net_if_get_by_iface(ctx->iface));

	/* You can implement here any special action that is needed
	 * when the network interface is going down.
	 */

	return 0;
}

static int vlan_interface_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct vlan_context *ctx = net_if_get_device(iface)->data;

	if (ctx->attached_to == NULL) {
		return -ENOENT;
	}

	net_pkt_set_vlan_tag(pkt, ctx->tag);
	net_pkt_set_iface(pkt, ctx->attached_to);
	set_priority(pkt);

	if (DEBUG_TX) {
		char str[sizeof("TX iface xx (tag xxxx)")];

		snprintk(str, sizeof(str), "TX iface %d (tag %d)",
			 net_if_get_by_iface(net_pkt_iface(pkt)),
			 ctx->tag);

		net_pkt_hexdump(pkt, str);
	}

	return net_send_data(pkt);
}

static enum net_verdict vlan_interface_recv(struct net_if *iface,
					    struct net_pkt *pkt)
{
	struct vlan_context *ctx = net_if_get_device(iface)->data;

	if (net_pkt_vlan_tag(pkt) != ctx->tag) {
		return NET_CONTINUE;
	}

	if (DEBUG_RX) {
		char str[sizeof("RX iface xx (tag xxxx)")];

		snprintk(str, sizeof(str), "RX iface %d (tag %d)",
			 net_pkt_vlan_tag(pkt),
			 net_if_get_by_iface(iface));

		net_pkt_hexdump(pkt, str);
	}

	return NET_OK;
}

static int vlan_interface_attach(struct net_if *vlan_iface,
				 struct net_if *iface)
{
	struct vlan_context *ctx = net_if_get_device(vlan_iface)->data;

	if (iface == NULL) {
		NET_DBG("VLAN interface %d (%p) detached from %d (%p)",
			net_if_get_by_iface(vlan_iface), vlan_iface,
			net_if_get_by_iface(ctx->attached_to), ctx->attached_to);
	} else {
		NET_DBG("VLAN interface %d (%p) attached to %d (%p)",
			net_if_get_by_iface(vlan_iface), vlan_iface,
			net_if_get_by_iface(iface), iface);
	}

	ctx->attached_to = iface;

	return 0;
}

static void vlan_iface_init(struct net_if *iface)
{
	struct vlan_context *ctx = net_if_get_device(iface)->data;
	char name[MAX(MAX_VLAN_NAME_LEN, MAX_VIRT_NAME_LEN)];

	if (ctx->init_done) {
		return;
	}

	ctx->iface = iface;
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	snprintk(name, sizeof(name), "VLAN-<free>");
	net_if_set_name(iface, name);

	snprintk(name, sizeof(name), "not attached");
	net_virtual_set_name(iface, name);

	(void)net_virtual_set_flags(ctx->iface, NET_L2_MULTICAST);

	ctx->init_done = true;
}
