/** @file
 * @brief LLMNR responder
 *
 * This listens to LLMNR queries and responds to them.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_LLMNR_RESPONDER)
#define SYS_LOG_DOMAIN "llmnr"
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
#include <net/udp.h>

#include "dns_pack.h"
#include "ipv6.h"

#include "net_private.h"

#define LLMNR_LISTEN_PORT 5355

#define LLMNR_TTL CONFIG_LLMNR_RESPONDER_TTL /* In seconds */

static struct net_context *ipv4;
static struct net_context *ipv6;

#define BUF_ALLOC_TIMEOUT K_MSEC(100)

/* This value is recommended by RFC 1035 */
#define DNS_RESOLVER_MAX_BUF_SIZE	512
#define DNS_RESOLVER_MIN_BUF		1
#define DNS_RESOLVER_BUF_CTR	(DNS_RESOLVER_MIN_BUF + \
				 CONFIG_LLMNR_RESOLVER_ADDITIONAL_BUF_CTR)

NET_BUF_POOL_DEFINE(llmnr_dns_msg_pool, DNS_RESOLVER_BUF_CTR,
		    DNS_RESOLVER_MAX_BUF_SIZE, 0, NULL);

#if defined(CONFIG_NET_IPV6)
static void create_ipv6_addr(struct sockaddr_in6 *addr)
{
	addr->sin6_family = AF_INET6;
	addr->sin6_port = htons(LLMNR_LISTEN_PORT);

	/* Well known IPv6 ff02::1:3 address */
	net_ipv6_addr_create(&addr->sin6_addr,
			     0xff02, 0, 0, 0, 0, 0, 0x01, 0x03);
}

static void create_ipv6_dst_addr(struct net_pkt *pkt,
				 struct sockaddr_in6 *addr)
{
	struct net_udp_hdr *udp_hdr, hdr;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);

	addr->sin6_family = AF_INET6;
	addr->sin6_port = udp_hdr->src_port;

	net_ipaddr_copy(&addr->sin6_addr, &NET_IPV6_HDR(pkt)->src);
}
#endif

#if defined(CONFIG_NET_IPV4)
static void create_ipv4_addr(struct sockaddr_in *addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(LLMNR_LISTEN_PORT);

	/* Well known IPv4 224.0.0.252 address */
	addr->sin_addr.s_addr = htonl(0xE00000FC);
}

static void create_ipv4_dst_addr(struct net_pkt *pkt,
				 struct sockaddr_in *addr)
{
	struct net_udp_hdr *udp_hdr, hdr;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);

	addr->sin_family = AF_INET;
	addr->sin_port = udp_hdr->src_port;

	net_ipaddr_copy(&addr->sin_addr, &NET_IPV4_HDR(pkt)->src);
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
		NET_DBG("Cannot bind to LLMNR %s port (%d)",
			local_addr->sa_family == AF_INET ?
			"IPv4" : "IPv6", ret);
		return ret;
	}

	return ret;
}

#define append(pkt, type, value)					\
	do {								\
		if (!net_pkt_append_##type##_timeout(pkt, value,	\
						     BUF_ALLOC_TIMEOUT)) { \
			ret = -ENOMEM;					\
			goto drop;					\
		}							\
	} while (0)

#define append_all(pkt, size, value)					\
	do {								\
		if (!net_pkt_append_all(pkt, size, value,		\
					BUF_ALLOC_TIMEOUT)) {		\
			ret = -ENOMEM;					\
			goto drop;					\
		}							\
	} while (0)


static int setup_dns_hdr(struct net_pkt *pkt, u16_t answers, u16_t dns_id)
{
	u16_t flags;
	int ret;

	/* See RFC 1035, ch 4.1.1 and RFC 4795 ch 2.1.1 for header details */

	flags = BIT(15);  /* This is response */

	append(pkt, be16, dns_id);
	append(pkt, be16, flags);   /* Flags and codes */
	append(pkt, be16, 1);       /* Question count is set to 1, see
				     * (RFC 4795 ch 2.1.1)
				     */
	append(pkt, be16, answers); /* Answer RR count */
	append(pkt, be16, 0);       /* Authority RR count */
	append(pkt, be16, 0);       /* Additional RR count */

	ret = 0;
drop:
	return ret;
}

static int add_question(struct net_pkt *pkt, enum dns_rr_type qtype,
			struct net_buf *query)
{
	char *dot = query->data;
	char *prev = NULL;
	int ret;

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

	append_all(pkt, query->len + 1, query->data);
	append(pkt, be16, qtype);
	append(pkt, be16, DNS_CLASS_IN);

	ret = 0;
drop:
	return ret;
}

static int add_answer(struct net_pkt *pkt, enum dns_rr_type qtype,
		      struct net_buf *query, u32_t ttl,
		      u16_t addr_len, const u8_t *addr)
{
	int ret;

	ret = add_question(pkt, qtype, query);
	if (ret < 0) {
		return ret;
	}

	append(pkt, be32, ttl);
	append(pkt, be16, addr_len);
	append_all(pkt, addr_len, addr);

	ret = 0;
drop:
	return ret;
}

static struct net_pkt *create_answer(struct net_context *ctx,
				     sa_family_t family,
				     enum dns_rr_type qtype,
				     struct net_buf *query,
				     u16_t dns_id,
				     u16_t addr_len, const u8_t *addr)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_get_tx(ctx, BUF_ALLOC_TIMEOUT);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_family(pkt, family);

	if (setup_dns_hdr(pkt, 1, dns_id) < 0) {
		goto drop;
	}

	ret = add_question(pkt, qtype, query);
	if (ret < 0) {
		goto drop;
	}

	ret = add_answer(pkt, qtype, query, LLMNR_TTL, addr_len, addr);
	if (ret < 0) {
		goto drop;
	}

	return pkt;

drop:
	net_pkt_unref(pkt);
	return NULL;
}

#if defined(CONFIG_NET_IPV4)
static const u8_t *get_ipv4_src(struct net_if *iface, struct in_addr *dst)
{
	const struct in_addr *addr;

	addr = net_if_ipv4_select_src_addr(iface, dst);
	if (!addr || net_is_ipv4_addr_unspecified(addr)) {
		return NULL;
	}

	return (const u8_t *)addr;
}
#endif

#if defined(CONFIG_NET_IPV6)
static const u8_t *get_ipv6_src(struct net_if *iface, struct in6_addr *dst)
{
	const struct in6_addr *addr;

	addr = net_if_ipv6_select_src_addr(iface, dst);
	if (!addr || net_is_ipv6_addr_unspecified(addr)) {
		return NULL;
	}

	return (const u8_t *)addr;
}
#endif

#if defined(CONFIG_NET_IPV4)
static int create_ipv4_answer(struct net_context *ctx,
			      struct net_pkt *pkt,
			      enum dns_rr_type qtype,
			      struct net_buf *query,
			      u16_t dns_id,
			      struct sockaddr *dst,
			      socklen_t *dst_len,
			      struct net_pkt **reply)
{
	const u8_t *addr;
	int addr_len;

	create_ipv4_dst_addr(pkt, net_sin(dst));
	*dst_len = sizeof(struct sockaddr_in);

	if (qtype == DNS_RR_TYPE_A) {
		/* Select proper address according to destination */
		addr = get_ipv4_src(net_pkt_iface(pkt),
				    &net_sin(dst)->sin_addr);
		if (!addr) {
			return -ENOENT;
		}

		addr_len = sizeof(struct in_addr);

	} else if (qtype == DNS_RR_TYPE_AAAA) {
#if defined(CONFIG_NET_IPV6)
		addr = get_ipv6_src(net_pkt_iface(pkt),
				    &NET_IPV6_HDR(pkt)->src);
		if (!addr) {
			return -ENOENT;
		}

		addr_len = sizeof(struct in6_addr);
#else
		addr_len = 0;
#endif
	} else {
		return -EINVAL;
	}

	*reply = create_answer(ctx, AF_INET, qtype, query, dns_id, addr_len,
			       addr);
	if (!*reply) {
		return -ENOMEM;
	}

	net_pkt_set_ipv4_ttl(*reply, 255);

	return 0;
}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV6)
static int create_ipv6_answer(struct net_context *ctx,
			      struct net_pkt *pkt,
			      enum dns_rr_type qtype,
			      struct net_buf *query,
			      u16_t dns_id,
			      struct sockaddr *dst,
			      socklen_t *dst_len,
			      struct net_pkt **reply)
{
	const u8_t *addr;
	int addr_len;

	create_ipv6_dst_addr(pkt, net_sin6(dst));
	*dst_len = sizeof(struct sockaddr_in6);

	if (qtype == DNS_RR_TYPE_AAAA) {
		addr = get_ipv6_src(net_pkt_iface(pkt),
				    &NET_IPV6_HDR(pkt)->src);
		if (!addr) {
			return -ENOENT;
		}

		addr_len = sizeof(struct in6_addr);
	} else if (qtype == DNS_RR_TYPE_A) {
#if defined(CONFIG_NET_IPV4)
		addr = get_ipv4_src(net_pkt_iface(pkt),
				    &NET_IPV4_HDR(pkt)->src);
		if (!addr) {
			return -ENOENT;
		}

		addr_len = sizeof(struct in_addr);
#else
		addr_len = 0;
#endif
	} else {
		return -EINVAL;
	}

	*reply = create_answer(ctx, AF_INET6, qtype, query, dns_id,
			       addr_len, addr);
	if (!*reply) {
		return -ENOMEM;
	}

	net_pkt_set_ipv6_hop_limit(*reply, 255);

	return 0;
}
#endif /* CONFIG_NET_IPV6 */

static int send_response(struct net_context *ctx, struct net_pkt *pkt,
			 struct net_buf *query, enum dns_rr_type qtype,
			 u16_t dns_id)
{
	struct net_pkt *reply;
	struct sockaddr dst;
	socklen_t dst_len;
	int ret;

	if (net_pkt_family(pkt) == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		ret = create_ipv4_answer(ctx, pkt, qtype, query, dns_id,
					 &dst, &dst_len, &reply);
		if (ret < 0) {
			return ret;
		}
#else /* CONFIG_NET_IPV4 */
		return -EPFNOSUPPORT;
#endif /* CONFIG_NET_IPV4 */

	} else if (net_pkt_family(pkt) == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		ret = create_ipv6_answer(ctx, pkt, qtype, query, dns_id,
					 &dst, &dst_len, &reply);
		if (ret < 0) {
			return ret;
		}
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
		NET_DBG("Cannot send LLMNR reply to %s (%d)",
			net_pkt_family(pkt) == AF_INET ?
			net_sprint_ipv4_addr(&net_sin(&dst)->sin_addr) :
			net_sprint_ipv6_addr(&net_sin6(&dst)->sin6_addr),
			ret);

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
	u16_t dns_id = 0;
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

	ret = llmnr_unpack_query_header(&dns_msg, &dns_id);
	if (ret < 0) {
		ret = -EINVAL;
		goto quit;
	}

	queries = ret;

	NET_DBG("Received %d %s from %s (id 0x%04x)", queries,
		queries > 1 ? "queries" : "query",
		net_pkt_family(pkt) == AF_INET ?
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src) :
		net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src), dns_id);

	do {
		enum dns_rr_type qtype;
		enum dns_class qclass;

		(void)memset(result->data, 0, net_buf_tailroom(result));
		result->len = 0;

		ret = dns_unpack_query(&dns_msg, result, &qtype, &qclass);
		if (ret < 0) {
			goto quit;
		}

		NET_DBG("[%d] query %s/%s label %s (%d bytes)", queries,
			qtype == DNS_RR_TYPE_A ? "A" : "AAAA", "IN",
			result->data, ret);

		/* If the query matches to our hostname, then send reply */
		if (!strncasecmp(hostname, result->data + 1, hostname_len) &&
		    (result->len - 1) >= hostname_len) {
			NET_DBG("LLMNR query to our hostname %s", hostname);
			send_response(ctx, pkt, result, qtype, dns_id);
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

	dns_data = net_buf_alloc(&llmnr_dns_msg_pool, BUF_ALLOC_TIMEOUT);
	if (!dns_data) {
		goto quit;
	}

	ret = dns_read(ctx, pkt, dns_data, &info);
	if (ret < 0 && ret != -EINVAL) {
		NET_DBG("LLMNR read failed (%d)", ret);
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
	{
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
			NET_WARN("Cannot receive IPv6 LLMNR data (%d)", ret);
			net_context_put(ipv6);
		} else {
			ok++;
		}
	}
ipv6_out:
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	{
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
			NET_WARN("Cannot receive IPv4 LLMNR data (%d)", ret);
			net_context_put(ipv4);
		} else {
			ok++;
		}
	}
ipv4_out:
#endif /* CONFIG_NET_IPV4 */

	if (!ok) {
		NET_WARN("Cannot start LLMNR responder");
	}

	return !ok;
}

static int llmnr_responder_init(struct device *device)
{
	ARG_UNUSED(device);

	return init_listener();
}

SYS_INIT(llmnr_responder_init, APPLICATION, CONFIG_LLMNR_RESPONDER_INIT_PRIO);
