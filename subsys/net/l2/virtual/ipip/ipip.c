/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_virtual_ipip, CONFIG_NET_L2_IPIP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/virtual.h>

#include "ipv4.h"
#include "ipv6.h"
#include "net_private.h"

#if defined(CONFIG_NET_L2_IPIP_TXRX_DEBUG)
#define DEBUG_TX 1
#define DEBUG_RX 1
#else
#define DEBUG_TX 0
#define DEBUG_RX 0
#endif

#define IPIPV4_MTU NET_IPV4_MTU
#define IPIPV6_MTU NET_IPV6_MTU

#define PKT_ALLOC_TIME K_MSEC(50)

struct ipip_context {
	struct net_if *iface;
	struct net_if *attached_to;
	union {
		sa_family_t family;
		struct net_addr peer;
	};

	union {
		const struct in_addr *my4addr;
		const struct in6_addr *my6addr;
	};

	bool status;
	bool init_done;
};

static int ipip_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void iface_init(struct net_if *iface)
{
	struct ipip_context *ctx = net_if_get_device(iface)->data;

	if (ctx->init_done) {
		return;
	}

	ctx->iface = iface;
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_POINTOPOINT);

	(void)net_virtual_set_flags(iface, NET_L2_POINT_TO_POINT);

	ctx->init_done = true;
}

static enum virtual_interface_caps get_capabilities(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return VIRTUAL_INTERFACE_IPIP;
}

static int interface_start(const struct device *dev)
{
	struct ipip_context *ctx = dev->data;
	int ret = 0;

	if (ctx->status) {
		return -EALREADY;
	}

	ctx->status = true;

	NET_DBG("Starting iface %d", net_if_get_by_iface(ctx->iface));

	return ret;
}

static int interface_stop(const struct device *dev)
{
	struct ipip_context *ctx = dev->data;

	if (!ctx->status) {
		return -EALREADY;
	}

	ctx->status = false;

	NET_DBG("Stopping iface %d", net_if_get_by_iface(ctx->iface));

	return 0;
}

static uint8_t ipv4_get_tos(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *ipv4_hdr;

	ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	if (!ipv4_hdr) {
		return 0;
	}

	return ipv4_hdr->tos;
}

static int interface_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct ipip_context *ctx = net_if_get_device(iface)->data;
	struct net_pkt *tmp = NULL;
	uint8_t nexthdr;
	uint8_t tos = 0;
	int ret;

	if (ctx->attached_to == NULL) {
		return -ENOENT;
	}

	if (net_pkt_family(pkt) == AF_INET) {
		nexthdr = IPPROTO_IPIP;
		tos = ipv4_get_tos(pkt);
	} else if (net_pkt_family(pkt) == AF_INET6) {
		nexthdr = IPPROTO_IPV6;
	} else {
		return -EINVAL;
	}

	/* Add new IP header */
	if (IS_ENABLED(CONFIG_NET_IPV6) && ctx->family == AF_INET6) {
		tmp = net_pkt_alloc_with_buffer(iface,
						sizeof(struct net_ipv6_hdr),
						AF_INET6, IPPROTO_IPV6,
						PKT_ALLOC_TIME);
		if (tmp == NULL) {
			return -ENOMEM;
		}

		if (ctx->my6addr == NULL) {
			ctx->my6addr = net_if_ipv6_select_src_addr(
						ctx->attached_to,
						&ctx->peer.in6_addr);
		}

		ret = net_ipv6_create(tmp, ctx->my6addr, &ctx->peer.in6_addr);
		if (ret < 0) {
			goto out;
		}

		net_buf_frag_add(tmp->buffer, pkt->buffer);
		pkt->buffer = tmp->buffer;
		tmp->buffer = NULL;

		net_pkt_unref(tmp);
		tmp = NULL;

		net_pkt_cursor_init(pkt);

		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
		net_pkt_set_ipv6_ext_opt_len(pkt, 0);
		net_pkt_set_iface(pkt, ctx->attached_to);

		ret = net_ipv6_finalize(pkt, nexthdr);
		if (ret < 0) {
			goto out;
		}

		net_pkt_set_family(pkt, AF_INET6);

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && ctx->family == AF_INET) {
		tmp = net_pkt_alloc_with_buffer(iface,
						sizeof(struct net_ipv4_hdr),
						AF_INET, IPPROTO_IP,
						PKT_ALLOC_TIME);
		if (tmp == NULL) {
			return -ENOMEM;
		}

		if (ctx->my4addr == NULL) {
			ctx->my4addr = net_if_ipv4_select_src_addr(
						ctx->attached_to,
						&ctx->peer.in_addr);
		}

		if (net_if_ipv4_get_ttl(ctx->attached_to) == 0) {
			NET_WARN("Interface %d TTL set to 0",
				 net_if_get_by_iface(ctx->attached_to));
			return -EINVAL;
		}

		net_pkt_set_ipv4_ttl(tmp,
				     net_if_ipv4_get_ttl(ctx->attached_to));

		/* RFC2003 chapter 3.1 */
		ret = net_ipv4_create_full(tmp, ctx->my4addr,
					   &ctx->peer.in_addr,
					   tos, 0U, NET_IPV4_DF,
					   0U, net_pkt_ipv4_ttl(tmp));
		if (ret < 0) {
			goto out;
		}

		net_buf_frag_add(tmp->buffer, pkt->buffer);
		pkt->buffer = tmp->buffer;
		tmp->buffer = NULL;

		net_pkt_unref(tmp);
		tmp = NULL;

		net_pkt_cursor_init(pkt);

		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
		net_pkt_set_ipv4_opts_len(pkt, 0);
		net_pkt_set_iface(pkt, ctx->attached_to);

		ret = net_ipv4_finalize(pkt, nexthdr);
		if (ret < 0) {
			goto out;
		}

		net_pkt_set_family(pkt, AF_INET);
	}

	if (DEBUG_TX) {
		char str[sizeof("TX iface xx")];

		snprintk(str, sizeof(str), "TX iface %d",
			 net_if_get_by_iface(net_pkt_iface(pkt)));

		net_pkt_hexdump(pkt, str);
	}

	return net_send_data(pkt);

out:
	if (tmp) {
		net_pkt_unref(tmp);
	}

	return ret;
}

static enum net_verdict interface_recv(struct net_if *iface,
				       struct net_pkt *pkt)
{
	if (DEBUG_RX) {
		char str[sizeof("RX iface xx")];

		snprintk(str, sizeof(str), "RX iface %d",
			 net_if_get_by_iface(iface));

		net_pkt_hexdump(pkt, str);
	}

	return NET_CONTINUE;
}

static bool verify_remote_addr(struct ipip_context *ctx,
			       struct net_addr *remote_addr)
{
	if (ctx->family != remote_addr->family) {
		return false;
	}

	if (ctx->family == AF_INET) {
		if (memcmp(&ctx->peer.in_addr, &remote_addr->in_addr,
			   sizeof(struct in_addr)) == 0) {
			return true;
		}
	} else {
		if (memcmp(&ctx->peer.in6_addr, &remote_addr->in6_addr,
			   sizeof(struct in6_addr)) == 0) {
			return true;
		}
	}

	return false;
}

static enum net_verdict interface_input(struct net_if *input_iface,
					struct net_if *virtual_iface,
					struct net_addr *remote_addr,
					struct net_pkt *pkt)
{
	struct ipip_context *ctx = net_if_get_device(virtual_iface)->data;
	struct net_if *iface;

	/* Make sure we are receiving data from remote end of the
	 * tunnel. See RFC4213 chapter 4 for details.
	 */
	if (!verify_remote_addr(ctx, remote_addr)) {
		NET_DBG("DROP: remote address unknown");
		return NET_DROP;
	}

	/* net_pkt cursor must point to correct place so that we can fetch
	 * the network header.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		NET_PKT_DATA_ACCESS_DEFINE(access, struct net_ipv6_hdr);
		struct net_ipv6_hdr *hdr;

		hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &access);
		if (!hdr) {
			return NET_DROP;
		}

		/* RFC4213 chapter 3.6 */
		iface = net_if_ipv6_select_src_iface((struct in6_addr *)hdr->dst);
		if (iface == NULL) {
			NET_DBG("DROP: not for me (dst %s)",
				net_sprint_ipv6_addr(&hdr->dst));
			return NET_DROP;
		}

		if (!net_if_is_up(iface)) {
			NET_DBG("DROP: interface %d down",
				net_if_get_by_iface(iface));
			return NET_DROP;
		}

		if (input_iface != ctx->attached_to ||
		    virtual_iface != iface) {
			NET_DBG("DROP: wrong interface");
			return NET_DROP;
		}

		/* Hop limit fields is decremented, RFC2473 chapter 3.1 and
		 * RFC4213 chapter 3.3
		 */
		hdr->hop_limit--;
		(void)net_pkt_set_data(pkt, &access);

		net_pkt_set_iface(pkt, iface);

		return net_if_recv_data(iface, pkt);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		NET_PKT_DATA_ACCESS_DEFINE(access, struct net_ipv4_hdr);
		struct net_ipv4_hdr *hdr;

		hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &access);
		if (!hdr) {
			return NET_DROP;
		}

		iface = net_if_ipv4_select_src_iface((struct in_addr *)hdr->dst);
		if (iface == NULL) {
			NET_DBG("DROP: not for me (dst %s)",
				net_sprint_ipv4_addr(&hdr->dst));
			return NET_DROP;
		}

		if (!net_if_is_up(iface)) {
			NET_DBG("DROP: interface %d down",
				net_if_get_by_iface(iface));
			return NET_DROP;
		}

		/* TTL fields is decremented, RFC2003 chapter 3.1 */
		hdr->ttl--;
		(void)net_pkt_set_data(pkt, &access);

		net_pkt_set_iface(pkt, iface);

		return net_if_recv_data(iface, pkt);
	}

	return NET_CONTINUE;
}

static int interface_attach(struct net_if *iface, struct net_if *lower_iface)
{
	struct ipip_context *ctx;

	if (net_if_get_by_iface(iface) < 0) {
		return -ENOENT;
	}

	ctx = net_if_get_device(iface)->data;
	ctx->attached_to = lower_iface;

	if (IS_ENABLED(CONFIG_NET_IPV6) && ctx->family == AF_INET6) {
		struct net_if_addr *ifaddr;
		struct in6_addr iid;

		/* RFC4213 chapter 3.7 */
		net_ipv6_addr_create_iid(&iid, net_if_get_link_addr(iface));

		ifaddr = net_if_ipv6_addr_add(iface, &iid, NET_ADDR_AUTOCONF,
					      0);
		if (!ifaddr) {
			NET_ERR("Cannot add %s address to interface %p",
				net_sprint_ipv6_addr(&iid),
				iface);
		}
	}

	return 0;
}

static int interface_set_config(struct net_if *iface,
				enum virtual_interface_config_type type,
				const struct virtual_interface_config *config)
{
	struct ipip_context *ctx = net_if_get_device(iface)->data;

	switch (type) {
	case VIRTUAL_INTERFACE_CONFIG_TYPE_PEER_ADDRESS:
		if (IS_ENABLED(CONFIG_NET_IPV4) && config->family == AF_INET) {
			char peer[INET_ADDRSTRLEN];
			char *addr_str;

			net_ipaddr_copy(&ctx->peer.in_addr, &config->peer4addr);

			addr_str = net_addr_ntop(AF_INET, &ctx->peer.in_addr,
						 peer, sizeof(peer));

			ctx->family = AF_INET;
			net_virtual_set_name(iface, "IPv4 tunnel");

			if (ctx->attached_to == NULL) {
				(void)net_virtual_interface_attach(iface,
					net_if_ipv4_select_src_iface(
						&ctx->peer.in_addr));
			}

			if (ctx->attached_to) {
				net_if_ipv4_set_ttl(iface,
					net_if_ipv4_get_ttl(ctx->attached_to));
			}

			NET_DBG("Interface %d peer address %s attached to %d",
				net_if_get_by_iface(iface),
				addr_str,
				net_if_get_by_iface(ctx->attached_to));

			ctx->my4addr = NULL;

		} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
			   config->family == AF_INET6) {
			char peer[INET6_ADDRSTRLEN];
			char *addr_str;

			net_ipaddr_copy(&ctx->peer.in6_addr,
					&config->peer6addr);

			addr_str = net_addr_ntop(AF_INET6, &ctx->peer.in6_addr,
						 peer, sizeof(peer));

			ctx->family = AF_INET6;
			net_virtual_set_name(iface, "IPv6 tunnel");

			net_ipv6_set_hop_limit(iface, 64);

			if (ctx->attached_to == NULL) {
				(void)net_virtual_interface_attach(iface,
					net_if_ipv6_select_src_iface(
						&ctx->peer.in6_addr));
			}

			NET_DBG("Interface %d peer address %s attached to %d",
				net_if_get_by_iface(iface),
				addr_str,
				net_if_get_by_iface(ctx->attached_to));

			ctx->my6addr = NULL;
		} else {
			return -EINVAL;
		}

		return 0;

	case VIRTUAL_INTERFACE_CONFIG_TYPE_MTU:
		NET_DBG("Interface %d MTU set to %d",
			net_if_get_by_iface(iface), config->mtu);
		net_if_set_mtu(iface, config->mtu);
		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

static int interface_get_config(struct net_if *iface,
				enum virtual_interface_config_type type,
				struct virtual_interface_config *config)
{
	struct ipip_context *ctx = net_if_get_device(iface)->data;

	switch (type) {
	case VIRTUAL_INTERFACE_CONFIG_TYPE_PEER_ADDRESS:
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    ctx->family == AF_INET6) {
			net_ipaddr_copy(&config->peer6addr,
					&ctx->peer.in6_addr);

		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   ctx->family == AF_INET) {
			net_ipaddr_copy(&config->peer4addr,
					&ctx->peer.in_addr);

		} else {
			return -EINVAL;
		}

		config->family = ctx->family;
		return 0;

	case VIRTUAL_INTERFACE_CONFIG_TYPE_MTU:
		config->mtu = net_if_get_mtu(iface);
		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

static const struct virtual_interface_api ipip_iface_api = {
	.iface_api.init = iface_init,

	.get_capabilities = get_capabilities,
	.start = interface_start,
	.stop = interface_stop,
	.send = interface_send,
	.recv = interface_recv,
	.input = interface_input,
	.attach = interface_attach,
	.set_config = interface_set_config,
	.get_config = interface_get_config,
};

#define NET_IPIP_DATA(x, _)						\
	static struct ipip_context ipip_context_data_##x = {		\
	}

#define NET_IPIP_INTERFACE_INIT(x, _)					\
	NET_VIRTUAL_INTERFACE_INIT(ipip##x, "IP_TUNNEL" #x, ipip_init,	\
				   NULL, &ipip_context_data_##x, NULL,	\
				   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
				   &ipip_iface_api, IPIPV4_MTU)

LISTIFY(CONFIG_NET_L2_IPIP_TUNNEL_COUNT, NET_IPIP_DATA, (;), _);
LISTIFY(CONFIG_NET_L2_IPIP_TUNNEL_COUNT, NET_IPIP_INTERFACE_INIT, (;), _);
