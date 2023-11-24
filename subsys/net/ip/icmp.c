/** @file
 * @brief ICMP related functions
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Use highest log level if both IPv4 and IPv6 are defined */
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)

#if CONFIG_NET_ICMPV4_LOG_LEVEL > CONFIG_NET_ICMPV6_LOG_LEVEL
#define ICMP_LOG_LEVEL CONFIG_NET_ICMPV4_LOG_LEVEL
#else
#define ICMP_LOG_LEVEL CONFIG_NET_ICMPV6_LOG_LEVEL
#endif

#elif defined(CONFIG_NET_IPV4)
#define ICMP_LOG_LEVEL CONFIG_NET_ICMPV4_LOG_LEVEL
#elif defined(CONFIG_NET_IPV6)
#define ICMP_LOG_LEVEL CONFIG_NET_ICMPV6_LOG_LEVEL
#else
#define ICMP_LOG_LEVEL LOG_LEVEL_INF
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_icmp, ICMP_LOG_LEVEL);

#include <errno.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/slist.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/icmp.h>

#include "net_private.h"
#include "icmpv6.h"
#include "icmpv4.h"
#include "ipv4.h"
#include "ipv6.h"
#include "net_stats.h"

static K_MUTEX_DEFINE(lock);
static sys_slist_t handlers = SYS_SLIST_STATIC_INIT(&handlers);

#if defined(CONFIG_NET_OFFLOADING_SUPPORT)
static sys_slist_t offload_handlers = SYS_SLIST_STATIC_INIT(&offload_handlers);
#endif

#define PKT_WAIT_TIME K_SECONDS(1)

int net_icmp_init_ctx(struct net_icmp_ctx *ctx, uint8_t type, uint8_t code,
		      net_icmp_handler_t handler)
{
	if (ctx == NULL || handler == NULL) {
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(struct net_icmp_ctx));

	ctx->handler = handler;
	ctx->type = type;
	ctx->code = code;

	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_prepend(&handlers, &ctx->node);

	k_mutex_unlock(&lock);

	return 0;
}

static void set_offload_handler(struct net_if *iface,
				net_icmp_handler_t handler)
{
	struct net_icmp_offload *offload;

	if (!IS_ENABLED(CONFIG_NET_OFFLOADING_SUPPORT)) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

#if defined(CONFIG_NET_OFFLOADING_SUPPORT)
	SYS_SLIST_FOR_EACH_CONTAINER(&offload_handlers, offload, node) {
		if (offload->iface == iface) {
			offload->handler = handler;
			break;
		}
	}
#else
	ARG_UNUSED(offload);
#endif

	k_mutex_unlock(&lock);
}

int net_icmp_cleanup_ctx(struct net_icmp_ctx *ctx)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_find_and_remove(&handlers, &ctx->node);

	k_mutex_unlock(&lock);

	set_offload_handler(ctx->iface, NULL);

	memset(ctx, 0, sizeof(struct net_icmp_ctx));

	return 0;
}

#if defined(CONFIG_NET_IPV4)
static int send_icmpv4_echo_request(struct net_icmp_ctx *ctx,
				    struct net_if *iface,
				    struct in_addr *dst,
				    struct net_icmp_ping_params *params,
				    void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv4_access,
					      struct net_icmpv4_echo_req);
	int ret = -ENOBUFS;
	struct net_icmpv4_echo_req *echo_req;
	const struct in_addr *src;
	struct net_pkt *pkt;

	if (!iface->config.ip.ipv4) {
		return -ENETUNREACH;
	}

	src = net_if_ipv4_select_src_addr(iface, dst);

	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(struct net_icmpv4_echo_req)
					+ params->data_size,
					AF_INET, IPPROTO_ICMP,
					PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	if (!IS_ENABLED(CONFIG_NET_ALLOW_ANY_PRIORITY) &&
	    params->priority >= NET_MAX_PRIORITIES) {
		NET_ERR("Priority %d is too large, maximum allowed is %d",
			params->priority, NET_MAX_PRIORITIES - 1);
		ret = -EINVAL;
		goto drop;
	}

	if (params->priority < 0) {
		net_pkt_set_ip_dscp(pkt, net_ipv4_get_dscp(params->tc_tos));
		net_pkt_set_ip_ecn(pkt, net_ipv4_get_ecn(params->tc_tos));
	} else {
		net_pkt_set_priority(pkt, params->priority);
	}

	if (net_ipv4_create(pkt, src, dst) ||
	    net_icmpv4_create(pkt, NET_ICMPV4_ECHO_REQUEST, 0)) {
		goto drop;
	}

	echo_req = (struct net_icmpv4_echo_req *)net_pkt_get_data(
							pkt, &icmpv4_access);
	if (!echo_req) {
		goto drop;
	}

	echo_req->identifier = htons(params->identifier);
	echo_req->sequence   = htons(params->sequence);

	net_pkt_set_data(pkt, &icmpv4_access);

	if (params->data != NULL && params->data_size > 0) {
		net_pkt_write(pkt, params->data, params->data_size);
	} else if (params->data == NULL && params->data_size > 0) {
		/* Generate payload. */
		if (params->data_size >= sizeof(uint32_t)) {
			uint32_t time_stamp = htonl(k_cycle_get_32());

			net_pkt_write(pkt, &time_stamp, sizeof(time_stamp));
			params->data_size -= sizeof(time_stamp);
		}

		for (size_t i = 0; i < params->data_size; i++) {
			net_pkt_write_u8(pkt, (uint8_t)i);
		}
	} else {
		/* No payload. */
	}

	net_pkt_cursor_init(pkt);

	net_ipv4_finalize(pkt, IPPROTO_ICMP);

	NET_DBG("Sending ICMPv4 Echo Request type %d from %s to %s",
		NET_ICMPV4_ECHO_REQUEST,
		net_sprint_ipv4_addr(src),
		net_sprint_ipv4_addr(dst));

	ctx->user_data = user_data;
	ctx->iface = iface;

	if (net_send_data(pkt) >= 0) {
		net_stats_update_icmp_sent(iface);
		return 0;
	}

	net_stats_update_icmp_drop(iface);

	ret = -EIO;

drop:
	net_pkt_unref(pkt);

	return ret;

}
#else
static int send_icmpv4_echo_request(struct net_icmp_ctx *ctx,
				    struct net_if *iface,
				    struct in_addr *dst,
				    struct net_icmp_ping_params *params,
				    void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(iface);
	ARG_UNUSED(dst);
	ARG_UNUSED(params);

	return -ENOTSUP;
}
#endif

#if defined(CONFIG_NET_IPV6)
static int send_icmpv6_echo_request(struct net_icmp_ctx *ctx,
				    struct net_if *iface,
				    struct in6_addr *dst,
				    struct net_icmp_ping_params *params,
				    void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv6_access,
					      struct net_icmpv6_echo_req);
	int ret = -ENOBUFS;
	struct net_icmpv6_echo_req *echo_req;
	const struct in6_addr *src;
	struct net_pkt *pkt;

	if (!iface->config.ip.ipv6) {
		return -ENETUNREACH;
	}

	src = net_if_ipv6_select_src_addr(iface, dst);

	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(struct net_icmpv6_echo_req)
					+ params->data_size,
					AF_INET6, IPPROTO_ICMPV6,
					PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	if (!IS_ENABLED(CONFIG_NET_ALLOW_ANY_PRIORITY) &&
	    params->priority >= NET_MAX_PRIORITIES) {
		NET_ERR("Priority %d is too large, maximum allowed is %d",
			params->priority, NET_MAX_PRIORITIES - 1);
		ret = -EINVAL;
		goto drop;
	}

	if (params->priority < 0) {
		net_pkt_set_ip_dscp(pkt, net_ipv6_get_dscp(params->tc_tos));
		net_pkt_set_ip_ecn(pkt, net_ipv6_get_ecn(params->tc_tos));
	} else {
		net_pkt_set_priority(pkt, params->priority);
	}

	if (net_ipv6_create(pkt, src, dst) ||
	    net_icmpv6_create(pkt, NET_ICMPV6_ECHO_REQUEST, 0)) {
		goto drop;
	}

	echo_req = (struct net_icmpv6_echo_req *)net_pkt_get_data(
							pkt, &icmpv6_access);
	if (!echo_req) {
		goto drop;
	}

	echo_req->identifier = htons(params->identifier);
	echo_req->sequence   = htons(params->sequence);

	net_pkt_set_data(pkt, &icmpv6_access);

	if (params->data != NULL && params->data_size > 0) {
		net_pkt_write(pkt, params->data, params->data_size);
	} else if (params->data == NULL && params->data_size > 0) {
		/* Generate payload. */
		if (params->data_size >= sizeof(uint32_t)) {
			uint32_t time_stamp = htonl(k_cycle_get_32());

			net_pkt_write(pkt, &time_stamp, sizeof(time_stamp));
			params->data_size -= sizeof(time_stamp);
		}

		for (size_t i = 0; i < params->data_size; i++) {
			net_pkt_write_u8(pkt, (uint8_t)i);
		}
	} else {
		/* No payload. */
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_ICMPV6);

	NET_DBG("Sending ICMPv6 Echo Request type %d from %s to %s",
		NET_ICMPV6_ECHO_REQUEST,
		net_sprint_ipv6_addr(src),
		net_sprint_ipv6_addr(dst));

	ctx->user_data = user_data;
	ctx->iface = iface;

	if (net_send_data(pkt) >= 0) {
		net_stats_update_icmp_sent(iface);
		return 0;
	}

	net_stats_update_icmp_drop(iface);

	ret = -EIO;

drop:
	net_pkt_unref(pkt);

	return ret;
}
#else
static int send_icmpv6_echo_request(struct net_icmp_ctx *ctx,
				    struct net_if *iface,
				    struct in6_addr *dst,
				    struct net_icmp_ping_params *params,
				    void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(iface);
	ARG_UNUSED(dst);
	ARG_UNUSED(params);

	return -ENOTSUP;
}
#endif

static struct net_icmp_ping_params *get_default_params(void)
{
	static struct net_icmp_ping_params params = { 0 };

	params.identifier = sys_rand32_get();

	return &params;
}

static int get_offloaded_ping_handler(struct net_if *iface,
				      net_icmp_offload_ping_handler_t *ping_handler)
{
	struct net_icmp_offload *offload;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_OFFLOADING_SUPPORT)) {
		return -ENOTSUP;
	}

	if (iface == NULL) {
		return -EINVAL;
	}

	if (!net_if_is_offloaded(iface)) {
		return -ENOENT;
	}

	ret = -ENOENT;

	k_mutex_lock(&lock, K_FOREVER);

#if defined(CONFIG_NET_OFFLOADING_SUPPORT)
	SYS_SLIST_FOR_EACH_CONTAINER(&offload_handlers, offload, node) {
		if (offload->iface == iface) {
			*ping_handler = offload->ping_handler;
			ret = 0;
			break;
		}
	}
#else
	ARG_UNUSED(offload);
#endif

	k_mutex_unlock(&lock);

	return ret;
}

int net_icmp_send_echo_request(struct net_icmp_ctx *ctx,
			       struct net_if *iface,
			       struct sockaddr *dst,
			       struct net_icmp_ping_params *params,
			       void *user_data)
{
	if (ctx == NULL || dst == NULL) {
		return -EINVAL;
	}

	if (iface == NULL) {
		if (IS_ENABLED(CONFIG_NET_IPV4) && dst->sa_family == AF_INET) {
			iface = net_if_ipv4_select_src_iface(&net_sin(dst)->sin_addr);
		} else if (IS_ENABLED(CONFIG_NET_IPV6) && dst->sa_family == AF_INET6) {
			iface = net_if_ipv6_select_src_iface(&net_sin6(dst)->sin6_addr);
		}

		if (iface == NULL) {
			return -ENOENT;
		}
	}

	if (IS_ENABLED(CONFIG_NET_OFFLOADING_SUPPORT) && net_if_is_offloaded(iface)) {
		net_icmp_offload_ping_handler_t ping_handler = NULL;
		int ret;

		ret = get_offloaded_ping_handler(iface, &ping_handler);
		if (ret < 0) {
			return ret;
		}

		if (ping_handler == NULL) {
			NET_ERR("No ping handler set");
			return -ENOENT;
		}

		set_offload_handler(iface, ctx->handler);

		return ping_handler(ctx, iface, dst, params, user_data);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && dst->sa_family == AF_INET) {
		if (params == NULL) {
			params = get_default_params();
		}

		return send_icmpv4_echo_request(ctx, iface, &net_sin(dst)->sin_addr,
						params, user_data);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && dst->sa_family == AF_INET6) {
		if (params == NULL) {
			params = get_default_params();
		}

		return send_icmpv6_echo_request(ctx, iface, &net_sin6(dst)->sin6_addr,
						params, user_data);
	}

	return -ENOENT;
}

static int icmp_call_handlers(struct net_pkt *pkt,
			      struct net_icmp_ip_hdr *ip_hdr,
			      struct net_icmp_hdr *icmp_hdr)
{
	struct net_icmp_ctx *ctx;
	int ret = -ENOENT;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&handlers, ctx, node) {
		if (ctx->type == icmp_hdr->type &&
		    (ctx->code == icmp_hdr->code || ctx->code == 0U)) {
			/* Do not use a handler that is expecting data from different
			 * network interface we sent the request.
			 */
			if (ctx->iface != NULL && ctx->iface != net_pkt_iface(pkt)) {
				continue;
			}

			ret = ctx->handler(ctx, pkt, ip_hdr, icmp_hdr, ctx->user_data);
			if (ret < 0) {
				goto out;
			}
		}
	}

out:
	k_mutex_unlock(&lock);

	return ret;
}


int net_icmp_call_ipv4_handlers(struct net_pkt *pkt,
				struct net_ipv4_hdr *ipv4_hdr,
				struct net_icmp_hdr *icmp_hdr)
{
	struct net_icmp_ip_hdr ip_hdr;

	ip_hdr.ipv4 = ipv4_hdr;
	ip_hdr.family = AF_INET;

	return icmp_call_handlers(pkt, &ip_hdr, icmp_hdr);
}

int net_icmp_call_ipv6_handlers(struct net_pkt *pkt,
				struct net_ipv6_hdr *ipv6_hdr,
				struct net_icmp_hdr *icmp_hdr)
{
	struct net_icmp_ip_hdr ip_hdr;

	ip_hdr.ipv6 = ipv6_hdr;
	ip_hdr.family = AF_INET6;

	return icmp_call_handlers(pkt, &ip_hdr, icmp_hdr);
}

int net_icmp_register_offload_ping(struct net_icmp_offload *ctx,
				   struct net_if *iface,
				   net_icmp_offload_ping_handler_t ping_handler)
{
	if (!IS_ENABLED(CONFIG_NET_OFFLOADING_SUPPORT)) {
		return -ENOTSUP;
	}

	if (iface == NULL) {
		return -EINVAL;
	}

	if (!net_if_is_offloaded(iface)) {
		return -ENOENT;
	}

	memset(ctx, 0, sizeof(struct net_icmp_offload));

	ctx->ping_handler = ping_handler;
	ctx->iface = iface;

	k_mutex_lock(&lock, K_FOREVER);

#if defined(CONFIG_NET_OFFLOADING_SUPPORT)
	sys_slist_prepend(&offload_handlers, &ctx->node);
#endif

	k_mutex_unlock(&lock);

	return 0;
}

int net_icmp_unregister_offload_ping(struct net_icmp_offload *ctx)
{
	if (!IS_ENABLED(CONFIG_NET_OFFLOADING_SUPPORT)) {
		return -ENOTSUP;
	}

	if (ctx == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

#if defined(CONFIG_NET_OFFLOADING_SUPPORT)
	sys_slist_find_and_remove(&offload_handlers, &ctx->node);
#endif

	k_mutex_unlock(&lock);

	memset(ctx, 0, sizeof(struct net_icmp_offload));

	return 0;
}

int net_icmp_get_offload_rsp_handler(struct net_icmp_offload *ctx,
				     net_icmp_handler_t *resp_handler)
{
	if (!IS_ENABLED(CONFIG_NET_OFFLOADING_SUPPORT)) {
		return -ENOTSUP;
	}

	if (ctx == NULL) {
		return -EINVAL;
	}

	*resp_handler = ctx->handler;

	return 0;
}
