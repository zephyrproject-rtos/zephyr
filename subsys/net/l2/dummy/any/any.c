/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_any, CONFIG_NET_PSEUDO_IFACE_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/virtual.h>

#include "net_private.h"
#include "net_stats.h"

struct any_context {
	struct net_if *iface;
};

static struct any_context any_data;

static void any_iface_init(struct net_if *iface)
{
	struct any_context *ctx = net_if_get_device(iface)->data;
	int ret;

	ctx->iface = iface;

	ret = net_if_set_name(iface, "any");
	if (ret < 0) {
		NET_DBG("Cannot set any interface name (%d)", ret);
	}

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_clear(iface, NET_IF_IPV6);
	net_if_flag_clear(iface, NET_IF_IPV4);
}

static enum net_verdict any_recv(struct net_if *iface, struct net_pkt *pkt)
{
	enum net_verdict verdict = NET_DROP;
	struct virtual_interface_context *ctx;
	sys_snode_t *first;

	if (!pkt->buffer) {
		goto drop;
	}

	first = sys_slist_peek_head(&iface->config.virtual_interfaces);
	if (first == NULL) {
		goto drop;
	}

	ctx = CONTAINER_OF(first, struct virtual_interface_context, node);
	if (ctx == NULL) {
		goto drop;
	}

	if (net_if_l2(ctx->virtual_iface)->recv == NULL) {
		goto drop;
	}

	NET_DBG("Passing pkt %p (len %zd) to virtual L2",
		pkt, net_pkt_get_len(pkt));

	verdict = net_if_l2(ctx->virtual_iface)->recv(iface, pkt);

	NET_DBG("Verdict for pkt %p is %s (%d)", pkt, net_verdict2str(verdict),
		verdict);

drop:
	return verdict;
}

static struct dummy_api any_api = {
	.iface_api.init = any_iface_init,
	.recv = any_recv,
};

NET_DEVICE_INIT(any,
		"NET_ANY",
		NULL,
		NULL,
		&any_data,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&any_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		1024);
