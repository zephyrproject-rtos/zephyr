/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_virtual, CONFIG_NET_L2_VIRTUAL_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/random/random.h>

#include "net_private.h"
#include "net_stats.h"

#define NET_BUF_TIMEOUT K_MSEC(100)

static enum net_verdict virtual_recv(struct net_if *iface,
				     struct net_pkt *pkt)
{
	struct virtual_interface_context *ctx, *tmp;
	const struct virtual_interface_api *api;
	enum net_verdict verdict;
	sys_slist_t *interfaces;

	interfaces = &iface->config.virtual_interfaces;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(interfaces, ctx, tmp, node) {
		if (ctx->virtual_iface == NULL) {
			continue;
		}

		api = net_if_get_device(ctx->virtual_iface)->api;
		if (!api || api->recv == NULL) {
			continue;
		}

		if (!net_if_is_up(ctx->virtual_iface)) {
			NET_DBG("Interface %d is down.",
				net_if_get_by_iface(ctx->virtual_iface));
			continue;
		}

		verdict = api->recv(ctx->virtual_iface, pkt);
		if (verdict == NET_CONTINUE) {
			continue;
		}

		if (IS_ENABLED(CONFIG_NET_STATISTICS)) {
			size_t pkt_len;

			pkt_len = net_pkt_get_len(pkt);

			NET_DBG("Received pkt %p len %zu", pkt, pkt_len);

			net_stats_update_bytes_recv(ctx->virtual_iface,
						    pkt_len);
		}

		if (verdict == NET_DROP) {
			net_stats_update_processing_error(ctx->virtual_iface);
		}

		return verdict;
	}

	NET_DBG("No handler, dropping pkt %p len %zu", pkt, net_pkt_get_len(pkt));

	return NET_DROP;
}

static int virtual_send(struct net_if *iface, struct net_pkt *pkt)
{
	const struct virtual_interface_api *api = net_if_get_device(iface)->api;
	size_t pkt_len;
	int ret;

	if (!api) {
		return -ENOENT;
	}

	if (!net_if_is_up(iface)) {
		NET_DBG("Interface %d is down.",
			net_if_get_by_iface(iface));
		return -ENETDOWN;
	}

	if (IS_ENABLED(CONFIG_NET_STATISTICS)) {
		pkt_len = net_pkt_get_len(pkt);
	}

	/* As we are just passing data through, the net_pkt is not freed here.
	 */
	ret = api->send(iface, pkt);

	if (IS_ENABLED(CONFIG_NET_STATISTICS) && ret == 0) {
		NET_DBG("Sent pkt %p len %zu", pkt, pkt_len);
		net_stats_update_bytes_sent(iface, pkt_len);
	}

	return ret;
}

static int virtual_enable(struct net_if *iface, bool state)
{
	const struct virtual_interface_api *virt;
	struct virtual_interface_context *ctx;
	int ret = 0;

	virt = net_if_get_device(iface)->api;
	if (!virt) {
		return -ENOENT;
	}

	ctx = net_if_l2_data(iface);

	if (state) {
		/* Take the interfaces below this interface up as
		 * it does not make sense otherwise.
		 */

		while (ctx->iface) {
			if (net_if_is_up(ctx->iface)) {
				/* Network interfaces below this must be up too
				 * so we can bail out at this point.
				 */
				break;
			}

			if (net_if_l2(ctx->iface) !=
						&NET_L2_GET_NAME(VIRTUAL)) {
				net_if_up(ctx->iface);
				break;
			}

			NET_DBG("Taking iface %d up", net_if_get_by_iface(ctx->iface));

			net_if_up(ctx->iface);
			ctx = net_if_l2_data(ctx->iface);
		}

		if (virt->start) {
			ret = virt->start(net_if_get_device(iface));
		}

		return ret;
	}

	if (virt->stop) {
		ret = virt->stop(net_if_get_device(iface));
	}

	return ret;
}

enum net_l2_flags virtual_flags(struct net_if *iface)
{
	struct virtual_interface_context *ctx = net_if_l2_data(iface);

	return ctx->virtual_l2_flags;
}

#if defined(CONFIG_NET_L2_ETHERNET_RESERVE_HEADER) && defined(CONFIG_NET_VLAN)
extern int vlan_alloc_buffer(struct net_if *iface, struct net_pkt *pkt,
			     size_t size, uint16_t proto, k_timeout_t timeout);

static int virtual_l2_alloc(struct net_if *iface, struct net_pkt *pkt,
			    size_t size, enum net_ip_protocol proto,
			    k_timeout_t timeout)
{
	return vlan_alloc_buffer(iface, pkt, size, proto, timeout);
}
#else
#define virtual_l2_alloc NULL
#endif

NET_L2_INIT(VIRTUAL_L2, virtual_recv, virtual_send, virtual_enable,
	    virtual_flags, virtual_l2_alloc);

static void random_linkaddr(uint8_t *linkaddr, size_t len)
{
	sys_rand_get(linkaddr, len);

	linkaddr[0] |= 0x02; /* force LAA bit */
}

int net_virtual_interface_attach(struct net_if *virtual_iface,
				 struct net_if *iface)
{
	const struct virtual_interface_api *api;
	struct virtual_interface_context *ctx;
	bool up = false;

	if (net_if_get_by_iface(virtual_iface) < 0 ||
	    (iface != NULL && net_if_get_by_iface(iface) < 0)) {
		return -EINVAL;
	}

	if (virtual_iface == iface) {
		return -EINVAL;
	}

	api = net_if_get_device(virtual_iface)->api;
	if (api->attach == NULL) {
		return -ENOENT;
	}

	ctx = net_if_l2_data(virtual_iface);

	if (ctx->iface) {
		if (iface != NULL) {
			/* We are already attached */
			return -EALREADY;
		}

		/* Detaching, take the interface down */
		net_if_down(virtual_iface);

		(void)sys_slist_find_and_remove(
				&ctx->iface->config.virtual_interfaces,
				&ctx->node);

		NET_DBG("Detaching %d from %d",
			net_if_get_by_iface(virtual_iface),
			net_if_get_by_iface(ctx->iface));

		ctx->iface = NULL;
	} else {
		if (iface == NULL) {
			/* We are already detached */
			return -EALREADY;
		}

		/* Attaching, take the interface up if auto start is enabled.
		 */
		ctx->iface = iface;
		sys_slist_append(&ctx->iface->config.virtual_interfaces,
				 &ctx->node);

		NET_DBG("Attaching %d to %d",
			net_if_get_by_iface(virtual_iface),
			net_if_get_by_iface(ctx->iface));

		up = true;
	}

	/* Figure out the link address for this interface. The actual link
	 * address is randomized. This must be done before attach is called so
	 * that the attach callback can create link local address for the
	 * network interface (if IPv6). The actual link address is typically
	 * not need in tunnels.
	 */
	if (iface) {
		random_linkaddr(ctx->lladdr.addr, sizeof(ctx->lladdr.addr));

		ctx->lladdr.len = sizeof(ctx->lladdr.addr);
		ctx->lladdr.type = NET_LINK_UNKNOWN;

		net_if_set_link_addr(virtual_iface, ctx->lladdr.addr,
				     ctx->lladdr.len, ctx->lladdr.type);
	}

	api->attach(virtual_iface, iface);

	if (up && !net_if_flag_is_set(virtual_iface,
				      NET_IF_NO_AUTO_START)) {
		net_if_up(virtual_iface);
	}

	return 0;
}

void net_virtual_disable(struct net_if *iface)
{
	struct virtual_interface_context *ctx, *tmp;
	sys_slist_t *interfaces;

	if (net_if_get_by_iface(iface) < 0) {
		return;
	}

	interfaces = &iface->config.virtual_interfaces;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(interfaces, ctx, tmp, node) {
		NET_DBG("Iface %d down, setting virtual iface %d carrier off",
			net_if_get_by_iface(iface),
			net_if_get_by_iface(ctx->virtual_iface));
		net_if_carrier_off(ctx->virtual_iface);
	}
}

void net_virtual_enable(struct net_if *iface)
{
	struct virtual_interface_context *ctx, *tmp;
	sys_slist_t *interfaces;

	if (net_if_get_by_iface(iface) < 0) {
		return;
	}

	interfaces = &iface->config.virtual_interfaces;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(interfaces, ctx, tmp, node) {
		NET_DBG("Iface %d up, setting virtual iface %d carrier on",
			net_if_get_by_iface(iface),
			net_if_get_by_iface(ctx->virtual_iface));
		net_if_carrier_on(ctx->virtual_iface);
	}
}

struct net_if *net_virtual_get_iface(struct net_if *iface)
{
	struct virtual_interface_context *ctx;

	if (net_if_get_by_iface(iface) < 0) {
		return NULL;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return NULL;
	}

	ctx = net_if_l2_data(iface);

	return ctx->iface;
}

char *net_virtual_get_name(struct net_if *iface, char *buf, size_t len)
{
	struct virtual_interface_context *ctx;

	if (net_if_get_by_iface(iface) < 0) {
		return NULL;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return NULL;
	}

	ctx = net_if_l2_data(iface);

	strncpy(buf, ctx->name, MIN(len, sizeof(ctx->name)));
	buf[len - 1] = '\0';

	return buf;
}

void net_virtual_set_name(struct net_if *iface, const char *name)
{
	struct virtual_interface_context *ctx;

	if (net_if_get_by_iface(iface) < 0) {
		return;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	ctx = net_if_l2_data(iface);

	strncpy(ctx->name, name, CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN);
	ctx->name[CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN - 1] = '\0';
}

enum net_l2_flags net_virtual_set_flags(struct net_if *iface,
					enum net_l2_flags flags)
{
	struct virtual_interface_context *ctx;
	enum net_l2_flags old_flags;

	if (net_if_get_by_iface(iface) < 0) {
		return 0;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return 0;
	}

	ctx = net_if_l2_data(iface);
	old_flags = ctx->virtual_l2_flags;
	ctx->virtual_l2_flags = flags;

	return old_flags;
}

void net_virtual_init(struct net_if *iface)
{
	struct virtual_interface_context *ctx;

	sys_slist_init(&iface->config.virtual_interfaces);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	ctx = net_if_l2_data(iface);
	if (ctx->is_init) {
		return;
	}

	NET_DBG("Initializing virtual L2 %p for iface %d (%p)", ctx,
		net_if_get_by_iface(iface), iface);

	ctx->virtual_iface = iface;
	ctx->virtual_l2_flags = 0;
	ctx->is_init = true;
}
