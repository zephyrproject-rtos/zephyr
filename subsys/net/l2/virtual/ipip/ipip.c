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

static K_MUTEX_DEFINE(lock);

static void init_context_iface(void);

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

	bool is_used;
	bool status;
	bool init_done;
};

static int virt_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	init_context_iface();

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
					   0U);
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

static bool verify_remote_addr(struct ipip_context *ctx,
			       struct sockaddr *remote_addr)
{
	if (ctx->family != remote_addr->sa_family) {
		return false;
	}

	if (ctx->family == AF_INET) {
		if (memcmp(&ctx->peer.in_addr, &net_sin(remote_addr)->sin_addr,
			   sizeof(struct in_addr)) == 0) {
			return true;
		}
	} else {
		if (memcmp(&ctx->peer.in6_addr, &net_sin6(remote_addr)->sin6_addr,
			   sizeof(struct in6_addr)) == 0) {
			return true;
		}
	}

	return false;
}

static enum net_verdict interface_recv(struct net_if *iface,
				       struct net_pkt *pkt)
{
	struct ipip_context *ctx = net_if_get_device(iface)->data;
	struct net_pkt_cursor hdr_start;
	uint8_t iptype;

	net_pkt_cursor_backup(pkt, &hdr_start);

	if (net_pkt_read_u8(pkt, &iptype)) {
		return NET_DROP;
	}

	net_pkt_cursor_restore(pkt, &hdr_start);

	switch (iptype & 0xf0) {
	case 0x60:
		net_pkt_set_family(pkt, AF_INET6);
		break;
	case 0x40:
		net_pkt_set_family(pkt, AF_INET);
		break;
	default:
		return NET_DROP;
	}

	/* Make sure we are receiving data from remote end of the
	 * tunnel. See RFC4213 chapter 4 for details.
	 */
	if (!verify_remote_addr(ctx, net_pkt_remote_address(pkt))) {
		NET_DBG("DROP: remote address %s unknown",
			net_pkt_remote_address(pkt)->sa_family == AF_INET6 ?
			net_sprint_ipv6_addr(&net_sin6(net_pkt_remote_address(pkt))->sin6_addr) :
			net_sprint_ipv4_addr(&net_sin(net_pkt_remote_address(pkt))->sin_addr));
		return NET_DROP;
	}

	if (DEBUG_RX) {
		char str[sizeof("RX iface xx")];

		snprintk(str, sizeof(str), "RX iface %d",
			 net_if_get_by_iface(iface));

		net_pkt_hexdump(pkt, str);
	}

	/* net_pkt cursor must point to correct place so that we can fetch
	 * the network header.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		NET_PKT_DATA_ACCESS_DEFINE(access, struct net_ipv6_hdr);
		struct net_ipv6_hdr *hdr;
		struct net_if *iface_test;

		net_pkt_cursor_backup(pkt, &hdr_start);

		hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &access);
		if (!hdr) {
			return NET_DROP;
		}

		/* RFC4213 chapter 3.6 */
		iface_test = net_if_ipv6_select_src_iface((struct in6_addr *)hdr->dst);
		if (iface_test == NULL) {
			NET_DBG("DROP: not for me (dst %s)",
				net_sprint_ipv6_addr(&hdr->dst));
			return NET_DROP;
		}

		if (iface != iface_test) {
			NET_DBG("DROP: wrong interface %d (%p), expecting %d (%p)",
				net_if_get_by_iface(iface_test), iface_test,
				net_if_get_by_iface(iface), iface);
			return NET_DROP;
		}

		/* Hop limit fields is decremented, RFC2473 chapter 3.1 and
		 * RFC4213 chapter 3.3
		 */
		hdr->hop_limit--;
		(void)net_pkt_set_data(pkt, &access);

		net_pkt_set_iface(pkt, iface);

		net_pkt_cursor_restore(pkt, &hdr_start);

		/* Ensure the loopback flag is no longer set */
		net_pkt_set_loopback(pkt, false);

		return net_ipv6_input(pkt);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		NET_PKT_DATA_ACCESS_DEFINE(access, struct net_ipv4_hdr);
		struct net_ipv4_hdr *hdr;
		struct net_if *iface_test;
		uint16_t sum;

		net_pkt_cursor_backup(pkt, &hdr_start);

		hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &access);
		if (!hdr) {
			return NET_DROP;
		}

		iface_test = net_if_ipv4_select_src_iface((struct in_addr *)hdr->dst);
		if (iface_test == NULL) {
			NET_DBG("DROP: not for me (dst %s)",
				net_sprint_ipv4_addr(&hdr->dst));
			return NET_DROP;
		}

		if (iface != iface_test) {
			NET_DBG("DROP: wrong interface %d (%p), expecting %d (%p)",
				net_if_get_by_iface(iface_test), iface_test,
				net_if_get_by_iface(iface), iface);
			return NET_DROP;
		}

		/* TTL fields is decremented, RFC2003 chapter 3.1 */
		hdr->ttl--;

		/* Recalculate the checksum because TTL was changed */
		hdr->chksum = 0U;

		sum = calc_chksum(0, access.data, access.size);
		sum = (sum == 0U) ? 0xffff : htons(sum);

		hdr->chksum = ~sum;

		(void)net_pkt_set_data(pkt, &access);

		net_pkt_set_iface(pkt, iface);

		net_pkt_cursor_restore(pkt, &hdr_start);

		/* Ensure the loopback flag is no longer set */
		net_pkt_set_loopback(pkt, false);

		return net_ipv4_input(pkt);
	}

	return NET_CONTINUE;
}

static int interface_attach(struct net_if *iface, struct net_if *lower_iface)
{
	struct ipip_context *ctx;

	if (net_if_get_by_iface(iface) < 0) {
		return -ENOENT;
	}

	k_mutex_lock(&lock, K_FOREVER);

	ctx = net_if_get_device(iface)->data;
	ctx->attached_to = lower_iface;
	ctx->iface = iface;

	if (lower_iface == NULL) {
		ctx->is_used = false;
	} else {
		ctx->is_used = true;

		if (IS_ENABLED(CONFIG_NET_IPV6) && ctx->family == AF_INET6) {
			struct net_if_addr *ifaddr;
			struct in6_addr iid;
			int ret;

			/* RFC4213 chapter 3.7 */
			ret = net_ipv6_addr_generate_iid(iface, NULL,
				 COND_CODE_1(CONFIG_NET_IPV6_IID_STABLE,
					     ((uint8_t *)&iface->config.ip.ipv6->network_counter),
					     (NULL)),
				 COND_CODE_1(CONFIG_NET_IPV6_IID_STABLE,
					     (sizeof(iface->config.ip.ipv6->network_counter)),
					     (0U)),
				 0,
				 &iid,
				 net_if_get_link_addr(iface));
			if (ret < 0) {
				NET_WARN("IPv6 IID generation issue (%d)", ret);
			}

			ifaddr = net_if_ipv6_addr_add(iface, &iid, NET_ADDR_AUTOCONF, 0);
			if (!ifaddr) {
				NET_ERR("Cannot add %s address to interface %p",
					net_sprint_ipv6_addr(&iid),
					iface);
			}
		}
	}

	k_mutex_unlock(&lock);

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

			net_if_ipv6_set_hop_limit(iface, 64);

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
	.attach = interface_attach,
	.set_config = interface_set_config,
	.get_config = interface_get_config,
};

#define NET_IPIP_INTERFACE_INIT(x, _)					\
	static struct ipip_context ipip_context_data_##x = {		\
	};								\
	NET_VIRTUAL_INTERFACE_INIT_INSTANCE(ipip_##x,			\
					    "IP_TUNNEL" #x,		\
					    x,				\
					    virt_dev_init,		\
					    NULL,			\
					    &ipip_context_data_##x,	\
					    NULL, /* config */		\
					    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
					    &ipip_iface_api,		\
					    IPIPV4_MTU)

LISTIFY(CONFIG_NET_L2_IPIP_TUNNEL_COUNT, NET_IPIP_INTERFACE_INIT, (;), _);

#define INIT_IPIP_CONTEXT_PTR(x, _)					\
	[x] = &ipip_context_data_##x

static struct ipip_context *ipip_ctx[] = {
	LISTIFY(CONFIG_NET_L2_IPIP_TUNNEL_COUNT, INIT_IPIP_CONTEXT_PTR, (,), _)
};

#define INIT_IPIP_CONTEXT_IFACE(x, _)					\
	ipip_context_data_##x.iface = NET_IF_GET(ipip_##x, x)

static void init_context_iface(void)
{
	static bool init_done;

	if (init_done) {
		return;
	}

	init_done = true;

	LISTIFY(CONFIG_NET_L2_IPIP_TUNNEL_COUNT, INIT_IPIP_CONTEXT_IFACE, (;), _);
}

struct net_if *net_ipip_get_virtual_interface(struct net_if *input_iface)
{
	struct net_if *iface = NULL;

	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(ipip_ctx, i) {
		if (ipip_ctx[i] == NULL || !ipip_ctx[i]->is_used) {
			continue;
		}

		if (input_iface == ipip_ctx[i]->attached_to) {
			iface = ipip_ctx[i]->iface;
			goto out;
		}
	}

out:
	k_mutex_unlock(&lock);

	return iface;
}
