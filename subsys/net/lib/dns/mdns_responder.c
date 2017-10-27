/** @file
 * @brief mDNS responder
 *
 * This listens to mDNS queries and responds to them.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_MDNS_RESPONDER)
#define SYS_LOG_DOMAIN "mdns"
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <init.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>

#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/dns_resolve.h>

#include "dns_pack.h"
#include "ipv6.h"

#include "net_private.h"

#define MDNS_LISTEN_PORT 5353

#define MDNS_TTL CONFIG_MDNS_RESPONDER_TTL /* In seconds */

static struct net_context *ipv4;
static struct net_context *ipv6;

#define BUF_ALLOC_TIMEOUT MSEC(100)

/* This value is recommended by RFC 1035 */
#define DNS_RESOLVER_MAX_BUF_SIZE	512
#define DNS_RESOLVER_MIN_BUF		1
#define DNS_RESOLVER_BUF_CTR	(DNS_RESOLVER_MIN_BUF + \
				 CONFIG_MDNS_RESOLVER_ADDITIONAL_BUF_CTR)

NET_BUF_POOL_DEFINE(dns_msg_pool, DNS_RESOLVER_BUF_CTR,
		    DNS_RESOLVER_MAX_BUF_SIZE, 0, NULL);

#if defined(CONFIG_NET_IPV6)
static void create_ipv6_addr(struct sockaddr_in6 *addr)
{
	addr->sin6_family = AF_INET6;
	addr->sin6_port = htons(MDNS_LISTEN_PORT);

	/* Well known IPv6 ff02::fb address */
	net_ipv6_addr_create(&addr->sin6_addr,
			     0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);
}
#endif

#if defined(CONFIG_NET_IPV4)
static void create_ipv4_addr(struct sockaddr_in *addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(MDNS_LISTEN_PORT);

	/* Well known IPv4 224.0.0.251 address */
	addr->sin_addr.s_addr = htonl(0xE00000FB);
}
#endif

static struct net_context *get_ctx(sa_family_t family)
{
	struct net_context *ctx;
	int ret;

	ret = net_context_get(family, SOCK_DGRAM, IPPROTO_UDP, &ctx);
	if (ret < 0) {
		NET_DBG("Cannot get context (%d)", ret);
		return NULL;
	}

	return ctx;
}

static int bind_ctx(struct net_context *ctx,
		    struct sockaddr *local_addr,
		    socklen_t addrlen)
{
	int ret;

	if (!ctx) {
		return -EINVAL;
	}

	ret = net_context_bind(ctx, local_addr, addrlen);
	if (ret < 0) {
		NET_DBG("Cannot bind to mDNS %s port (%d)",
			local_addr->sa_family == AF_INET ?
			"IPv4" : "IPv6", ret);
		return ret;
	}

	return ret;
}

static void setup_dns_hdr(struct net_pkt *pkt, u16_t answers)
{
	u16_t flags;

	/* See RFC 1035, ch 4.1.1 for header details */

	flags = BIT(15);  /* This is response */
	flags |= BIT(10); /* Authoritative Answer */

	net_pkt_append_be16(pkt, 0);       /* Identifier, RFC 6762 ch 18.1 */
	net_pkt_append_be16(pkt, flags);   /* Flags and codes */
	net_pkt_append_be16(pkt, 0);       /* Question count */
	net_pkt_append_be16(pkt, answers); /* Answer RR count */
	net_pkt_append_be16(pkt, 0);       /* Authority RR count */
	net_pkt_append_be16(pkt, 0);       /* Additional RR count */
}

static int add_answer(struct net_pkt *pkt, enum dns_rr_type qtype,
		      struct net_buf *query, u32_t ttl,
		      u16_t addr_len, u8_t *addr)
{
	char *dot = query->data;
	char *prev = NULL;

	while ((dot = strchr(dot, '.'))) {
		if (!prev) {
			prev = dot++;
			continue;
		}

		*prev = dot - prev - 1;
		prev = dot++;
	}

	if (prev) {
		*prev = strlen(prev) - 1;
	}

	if (!net_pkt_append_all(pkt, query->len + 1, query->data,
				BUF_ALLOC_TIMEOUT)) {
		return -ENOMEM;
	}

	net_pkt_append_be16(pkt, qtype);

	/* Bit 15 tells to flush the cache */
	net_pkt_append_be16(pkt, DNS_CLASS_IN | BIT(15));

	net_pkt_append_be32(pkt, ttl);
	net_pkt_append_be16(pkt, addr_len);

	if (!net_pkt_append_all(pkt, addr_len, addr,
				BUF_ALLOC_TIMEOUT)) {
		return -ENOMEM;
	}

	return 0;
}

static struct net_pkt *create_answer(struct net_context *ctx,
				     sa_family_t family,
				     enum dns_rr_type qtype,
				     struct net_buf *query,
				     u16_t addr_len, u8_t *addr)
{
	struct net_pkt *pkt;

	pkt = net_pkt_get_tx(ctx, BUF_ALLOC_TIMEOUT);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_family(pkt, family);

	setup_dns_hdr(pkt, 1);

	add_answer(pkt, qtype, query, MDNS_TTL, addr_len, addr);

	return pkt;
}

static int send_response(struct net_context *ctx, struct net_pkt *pkt,
			 struct net_buf *query,
			 enum dns_rr_type qtype)
{
	struct net_pkt *reply;
	struct sockaddr dst;
	socklen_t dst_len;
	int ret;

	if (qtype == DNS_RR_TYPE_A) {
#if defined(CONFIG_NET_IPV4)
		struct in_addr *addr;

		/* For IPv4 we take the first address in the interface */
		addr = &net_pkt_iface(pkt)->ipv4.unicast[0].address.in_addr;

		create_ipv4_addr(net_sin(&dst));
		dst_len = sizeof(struct sockaddr_in);

		reply = create_answer(ctx, AF_INET, qtype, query,
				      sizeof(struct in_addr), (u8_t *)addr);
		if (!reply) {
			return -ENOMEM;
		}

		net_pkt_set_ipv4_ttl(reply, 255);
#else /* CONFIG_NET_IPV4 */
		return -EPFNOSUPPORT;
#endif /* CONFIG_NET_IPV4 */

	} else if (qtype == DNS_RR_TYPE_AAAA) {
#if defined(CONFIG_NET_IPV6)
		const struct in6_addr *addr;

		addr = net_if_ipv6_select_src_addr(net_pkt_iface(pkt),
						   &NET_IPV6_HDR(pkt)->src);

		create_ipv6_addr(net_sin6(&dst));
		dst_len = sizeof(struct sockaddr_in6);

		reply = create_answer(ctx, AF_INET6, qtype, query,
				      sizeof(struct in6_addr), (u8_t *)addr);
		if (!reply) {
			return -ENOMEM;
		}

		net_pkt_set_ipv6_hop_limit(reply, 255);
#else /* CONFIG_NET_IPV6 */
		return -EPFNOSUPPORT;
#endif /* CONFIG_NET_IPV6 */

	} else {
		/* TODO: support also service PTRs */
		return -EINVAL;
	}

	ret = net_context_sendto(reply, &dst, dst_len, NULL, K_NO_WAIT,
				 NULL, NULL);
	if (ret < 0) {
		NET_DBG("Cannot send mDNS reply (%d)", ret);
		net_pkt_unref(reply);
	}

	return ret;
}

static int dns_read(struct net_context *ctx,
		    struct net_pkt *pkt,
		    struct net_buf *dns_data,
		    struct dns_addrinfo *info)
{
	/* Helper struct to track the dns msg received from the server */
	const char *hostname = net_hostname_get();
	int hostname_len = strlen(hostname);
	struct net_buf *result;
	struct dns_msg_t dns_msg;
	int data_len;
	int queries;
	int offset;
	int ret;

	data_len = min(net_pkt_appdatalen(pkt), DNS_RESOLVER_MAX_BUF_SIZE);
	offset = net_pkt_get_len(pkt) - data_len;

	/* Store the DNS query name into a temporary net_buf. This means
	 * that largest name we can resolve is CONFIG_NET_BUF_DATA_SIZE
	 * which typically is 128 bytes. This is done using net_buf so that
	 * we do not increase the stack usage of RX thread.
	 */
	result = net_pkt_get_data(ctx, BUF_ALLOC_TIMEOUT);
	if (!result) {
		ret = -ENOMEM;
		goto quit;
	}

	/* TODO: Instead of this temporary copy, just use the net_pkt directly.
	 */
	ret = net_frag_linear_copy(dns_data, pkt->frags, offset, data_len);
	if (ret < 0) {
		goto quit;
	}

	dns_msg.msg = dns_data->data;
	dns_msg.msg_size = data_len;

	ret = mdns_unpack_query_header(&dns_msg, NULL);
	if (ret < 0) {
		ret = -EINVAL;
		goto quit;
	}

	queries = ret;

	NET_DBG("Received %d %s from %s", queries,
		queries > 1 ? "queries" : "query",
		net_pkt_family(pkt) == AF_INET ?
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src) :
		net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src));

	do {
		enum dns_rr_type qtype;
		enum dns_class qclass;
		u8_t *lquery;

		memset(result->data, 0, net_buf_tailroom(result));
		result->len = 0;

		ret = dns_unpack_query(&dns_msg, result, &qtype, &qclass);
		if (ret < 0) {
			goto quit;
		}

		/* Handle only .local queries */
		lquery = strrchr(result->data + 1, '.');
		if (!lquery || memcmp(lquery, (const void *){ ".local" }, 7)) {
			continue;
		}

		NET_DBG("[%d] query %s/%s label %s (%d bytes)", queries,
			qtype == DNS_RR_TYPE_A ? "A" : "AAAA", "IN",
			result->data, ret);

		/* If the query matches to our hostname, then send reply.
		 * We skip the first dot, and make sure there is dot after
		 * matching hostname.
		 */
		if (!strncasecmp(hostname, result->data + 1, hostname_len) &&
		    (result->len - 1) >= hostname_len &&
		    &(result->data + 1)[hostname_len] == lquery) {
			NET_DBG("mDNS query to our hostname %s.local",
				hostname);
			send_response(ctx, pkt, result, qtype);
		}
	} while (--queries);

	ret = 0;

quit:
	if (result) {
		net_pkt_frag_unref(result);
	}

	return ret;
}

static void recv_cb(struct net_context *net_ctx,
		    struct net_pkt *pkt,
		    int status,
		    void *user_data)
{
	struct net_context *ctx = user_data;
	struct net_buf *dns_data = NULL;
	struct dns_addrinfo info = { 0 };
	int ret;

	ARG_UNUSED(net_ctx);
	NET_ASSERT(ctx == net_ctx);

	if (!pkt) {
		return;
	}

	if (status) {
		goto quit;
	}

	dns_data = net_buf_alloc(&dns_msg_pool, BUF_ALLOC_TIMEOUT);
	if (!dns_data) {
		goto quit;
	}

	ret = dns_read(ctx, pkt, dns_data, &info);
	if (ret < 0 && ret != -EINVAL) {
		NET_DBG("mDNS read failed (%d)", ret);
	}

	net_buf_unref(dns_data);

quit:
	net_pkt_unref(pkt);
}

#if defined(CONFIG_NET_IPV6)
static void iface_ipv6_cb(struct net_if *iface, void *user_data)
{
	struct in6_addr *addr = user_data;
	int ret;

	ret = net_ipv6_mld_join(iface, addr);
	if (ret < 0) {
		NET_DBG("Cannot join %s IPv6 multicast group (%d)",
			net_sprint_ipv6_addr(addr), ret);
	}
}

static void setup_ipv6_addr(struct sockaddr_in6 *local_addr)
{
	create_ipv6_addr(local_addr);

	net_if_foreach(iface_ipv6_cb, &local_addr->sin6_addr);
}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
static void iface_ipv4_cb(struct net_if *iface, void *user_data)
{
	struct in_addr *addr = user_data;
	struct net_if_mcast_addr *ifaddr;

	ifaddr = net_if_ipv4_maddr_add(iface, addr);
	if (!ifaddr) {
		NET_DBG("Cannot add IPv4 multicast address to iface %p",
			iface);
	}
}

static void setup_ipv4_addr(struct sockaddr_in *local_addr)
{
	create_ipv4_addr(local_addr);

	net_if_foreach(iface_ipv4_cb, &local_addr->sin_addr);
}
#endif /* CONFIG_NET_IPV4 */

static int init_listener(void)
{
	int ret, ok = 0;

#if defined(CONFIG_NET_IPV6)
	do {
		static struct sockaddr_in6 local_addr;

		setup_ipv6_addr(&local_addr);

		ipv6 = get_ctx(AF_INET6);

		ret = bind_ctx(ipv6, (struct sockaddr *)&local_addr,
			       sizeof(local_addr));
		if (ret < 0) {
			net_context_put(ipv6);
			goto ipv6_out;
		}

		ret = net_context_recv(ipv6, recv_cb, K_NO_WAIT, ipv6);
		if (ret < 0) {
			NET_WARN("Cannot receive IPv6 mDNS data (%d)", ret);
			net_context_put(ipv6);
		} else {
			ok++;
		}
	} while (0);
ipv6_out:
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	do {
		static struct sockaddr_in local_addr;

		setup_ipv4_addr(&local_addr);

		ipv4 = get_ctx(AF_INET);

		ret = bind_ctx(ipv4, (struct sockaddr *)&local_addr,
			       sizeof(local_addr));
		if (ret < 0) {
			net_context_put(ipv4);
			goto ipv4_out;
		}

		ret = net_context_recv(ipv4, recv_cb, K_NO_WAIT, ipv4);
		if (ret < 0) {
			NET_WARN("Cannot receive IPv4 mDNS data (%d)", ret);
			net_context_put(ipv4);
		} else {
			ok++;
		}
	} while (0);
ipv4_out:
#endif /* CONFIG_NET_IPV4 */

	if (!ok) {
		NET_WARN("Cannot start mDNS responder");
	}

	return !ok;
}

static int mdns_responder_init(struct device *device)
{
	ARG_UNUSED(device);

	return init_listener();
}

SYS_INIT(mdns_responder_init, APPLICATION, CONFIG_MDNS_RESPONDER_INIT_PRIO);
