/** @file
 * @brief mDNS responder
 *
 * This listens to mDNS queries and responds to them.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mdns_responder, CONFIG_MDNS_RESPONDER_LOG_LEVEL);

#include <zephyr/zephyr.h>
#include <zephyr/init.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/igmp.h>

#include "dns_sd.h"
#include "dns_pack.h"
#include "ipv6.h"

#include "net_private.h"

#define MDNS_LISTEN_PORT 5353

#define MDNS_TTL CONFIG_MDNS_RESPONDER_TTL /* In seconds */

#if defined(CONFIG_NET_IPV4)
static struct net_context *ipv4;
static struct sockaddr_in local_addr4;
#endif
#if defined(CONFIG_NET_IPV6)
static struct net_context *ipv6;
#endif

static struct net_mgmt_event_callback mgmt_cb;

#define BUF_ALLOC_TIMEOUT K_MSEC(100)

/* This value is recommended by RFC 1035 */
#define DNS_RESOLVER_MAX_BUF_SIZE	512
#define DNS_RESOLVER_MIN_BUF		2
#define DNS_RESOLVER_BUF_CTR	(DNS_RESOLVER_MIN_BUF + \
				 CONFIG_MDNS_RESOLVER_ADDITIONAL_BUF_CTR)

#ifndef CONFIG_NET_TEST
static int setup_dst_addr(struct net_context *ctx, struct net_pkt *pkt,
			  struct sockaddr *dst, socklen_t *dst_len);
#endif /* CONFIG_NET_TEST */

NET_BUF_POOL_DEFINE(mdns_msg_pool, DNS_RESOLVER_BUF_CTR,
		    DNS_RESOLVER_MAX_BUF_SIZE, 0, NULL);

static void create_ipv6_addr(struct sockaddr_in6 *addr)
{
	addr->sin6_family = AF_INET6;
	addr->sin6_port = htons(MDNS_LISTEN_PORT);

	/* Well known IPv6 ff02::fb address */
	net_ipv6_addr_create(&addr->sin6_addr,
			     0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);
}

static void create_ipv4_addr(struct sockaddr_in *addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(MDNS_LISTEN_PORT);

	/* Well known IPv4 224.0.0.251 address */
	addr->sin_addr.s_addr = htonl(0xE00000FB);
}

static void mdns_iface_event_handler(struct net_mgmt_event_callback *cb,
				     uint32_t mgmt_event, struct net_if *iface)

{
	if (mgmt_event == NET_EVENT_IF_UP) {
#if defined(CONFIG_NET_IPV4)
		int ret = net_ipv4_igmp_join(iface, &local_addr4.sin_addr);

		if (ret < 0) {
			NET_DBG("Cannot add IPv4 multicast address to iface %p",
				iface);
		}
#endif /* defined(CONFIG_NET_IPV4) */
	}
}

int setup_dst_addr(struct net_context *ctx, struct net_pkt *pkt,
		   struct sockaddr *dst, socklen_t *dst_len)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) &&
	    net_pkt_family(pkt) == AF_INET) {
		create_ipv4_addr(net_sin(dst));
		*dst_len = sizeof(struct sockaddr_in);
		net_context_set_ipv4_ttl(ctx, 255);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   net_pkt_family(pkt) == AF_INET6) {
		create_ipv6_addr(net_sin6(dst));
		*dst_len = sizeof(struct sockaddr_in6);
		net_context_set_ipv6_hop_limit(ctx, 255);
	} else {
		return -EPFNOSUPPORT;
	}

	return 0;
}

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

static void setup_dns_hdr(uint8_t *buf, uint16_t answers)
{
	uint16_t offset;
	uint16_t flags;

	/* See RFC 1035, ch 4.1.1 for header details */

	flags = BIT(15);  /* This is response */
	flags |= BIT(10); /* Authoritative Answer */

	UNALIGNED_PUT(0, (uint16_t *)(buf)); /* Identifier, RFC 6762 ch 18.1 */
	offset = DNS_HEADER_ID_LEN;

	UNALIGNED_PUT(htons(flags), (uint16_t *)(buf+offset));
	offset += DNS_HEADER_FLAGS_LEN;

	UNALIGNED_PUT(0, (uint16_t *)(buf + offset));
	offset += DNS_QDCOUNT_LEN;

	UNALIGNED_PUT(htons(answers), (uint16_t *)(buf + offset));
	offset += DNS_ANCOUNT_LEN;

	UNALIGNED_PUT(0, (uint16_t *)(buf + offset));
	offset += DNS_NSCOUNT_LEN;

	UNALIGNED_PUT(0, (uint16_t *)(buf + offset));
}

static void add_answer(struct net_buf *query, enum dns_rr_type qtype,
		       uint32_t ttl, uint16_t addr_len, uint8_t *addr)
{
	char *dot = query->data + DNS_MSG_HEADER_SIZE;
	char *prev = NULL;
	uint16_t offset;

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

	/* terminator byte (0x00) */
	query->len += 1;

	offset = DNS_MSG_HEADER_SIZE + query->len;
	UNALIGNED_PUT(htons(qtype), (uint16_t *)(query->data+offset));

	/* Bit 15 tells to flush the cache */
	offset += DNS_QTYPE_LEN;
	UNALIGNED_PUT(htons(DNS_CLASS_IN | BIT(15)),
		      (uint16_t *)(query->data+offset));


	offset += DNS_QCLASS_LEN;
	UNALIGNED_PUT(htonl(ttl), query->data + offset);

	offset += DNS_TTL_LEN;
	UNALIGNED_PUT(htons(addr_len), query->data + offset);

	offset += DNS_RDLENGTH_LEN;
	memcpy(query->data + offset, addr, addr_len);
}

static int create_answer(struct net_context *ctx,
			 struct net_buf *query,
			 enum dns_rr_type qtype,
			 uint16_t addr_len, uint8_t *addr)
{
	/* Prepare the response into the query buffer: move the name
	 * query buffer has to get enough free space: dns_hdr + answer
	 */
	if ((query->size - query->len) < (DNS_MSG_HEADER_SIZE +
					  DNS_QTYPE_LEN + DNS_QCLASS_LEN +
					  DNS_TTL_LEN + DNS_RDLENGTH_LEN +
					  addr_len)) {
		return -ENOBUFS;
	}

	memmove(query->data + DNS_MSG_HEADER_SIZE, query->data, query->len);

	setup_dns_hdr(query->data, 1);

	add_answer(query, qtype, MDNS_TTL, addr_len, addr);

	query->len += DNS_MSG_HEADER_SIZE +
		DNS_QTYPE_LEN + DNS_QCLASS_LEN +
		DNS_TTL_LEN + DNS_RDLENGTH_LEN + addr_len;

	return 0;
}

static int send_response(struct net_context *ctx,
			 struct net_pkt *pkt,
			 union net_ip_header *ip_hdr,
			 struct net_buf *query,
			 enum dns_rr_type qtype)
{
	struct sockaddr dst;
	socklen_t dst_len;
	int ret;

	ret = setup_dst_addr(ctx, pkt, &dst, &dst_len);
	if (ret < 0) {
		NET_DBG("unable to set up the response address");
		return ret;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && qtype == DNS_RR_TYPE_A) {
		const struct in_addr *addr;

		addr = net_if_ipv4_select_src_addr(net_pkt_iface(pkt),
						   (struct in_addr *)ip_hdr->ipv4->src);

		ret = create_answer(ctx, query, qtype,
				      sizeof(struct in_addr), (uint8_t *)addr);
		if (ret != 0) {
			return ret;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && qtype == DNS_RR_TYPE_AAAA) {
		const struct in6_addr *addr;

		addr = net_if_ipv6_select_src_addr(net_pkt_iface(pkt),
						   (struct in6_addr *)ip_hdr->ipv6->src);

		ret = create_answer(ctx, query, qtype,
				      sizeof(struct in6_addr), (uint8_t *)addr);
		if (ret != 0) {
			return -ENOMEM;
		}
	} else {
		/* TODO: support also service PTRs */
		return -EINVAL;
	}

	ret = net_context_sendto(ctx, query->data, query->len, &dst,
				 dst_len, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		NET_DBG("Cannot send mDNS reply (%d)", ret);
	}

	return ret;
}

static const char *qtype_to_string(int qtype)
{
	switch (qtype) {
	case DNS_RR_TYPE_A: return "A";
	case DNS_RR_TYPE_CNAME: return "CNAME";
	case DNS_RR_TYPE_PTR: return "PTR";
	case DNS_RR_TYPE_TXT: return "TXT";
	case DNS_RR_TYPE_AAAA: return "AAAA";
	case DNS_RR_TYPE_SRV: return "SRV";
	default: return "<unknown type>";
	}
}

static void send_sd_response(struct net_context *ctx,
		 struct net_pkt *pkt, union net_ip_header *ip_hdr,
		 struct dns_msg_t *dns_msg, struct net_buf *result)
{
	int ret;
	const struct dns_sd_rec *record;
	/* filter must be zero-initialized for "wildcard" port */
	struct dns_sd_rec filter = {0};
	struct sockaddr dst;
	socklen_t dst_len;
	bool service_type_enum = false;
	const struct in6_addr *addr6 = NULL;
	const struct in_addr *addr4 = NULL;
	char instance_buf[DNS_SD_SERVICE_MAX_SIZE + 1];
	char service_buf[DNS_SD_SERVICE_MAX_SIZE + 1];
	char proto_buf[DNS_SD_PROTO_SIZE + 1];
	char domain_buf[DNS_SD_DOMAIN_MAX_SIZE + 1];
	char *label[4];
	size_t size[] = {
		ARRAY_SIZE(instance_buf),
		ARRAY_SIZE(service_buf),
		ARRAY_SIZE(proto_buf),
		ARRAY_SIZE(domain_buf),
	};
	size_t n = ARRAY_SIZE(label);

	BUILD_ASSERT(ARRAY_SIZE(label) == ARRAY_SIZE(size), "");

	/*
	 * work around for bug in compliance scripts which say that the array
	 * should be static const (incorrect)
	 */
	label[0] = instance_buf;
	label[1] = service_buf;
	label[2] = proto_buf;
	label[3] = domain_buf;

	/* This actually is used but the compiler doesn't see that */
	ARG_UNUSED(record);

	ret = setup_dst_addr(ctx, pkt, &dst, &dst_len);
	if (ret < 0) {
		NET_DBG("unable to set up the response address");
		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		/* Look up the local IPv4 address */
		addr4 = net_if_ipv4_select_src_addr(net_pkt_iface(pkt),
						    (struct in_addr *)ip_hdr->ipv4->src);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		/* Look up the local IPv6 address */
		addr6 = net_if_ipv6_select_src_addr(net_pkt_iface(pkt),
						    (struct in6_addr *)ip_hdr->ipv6->src);
	}

	ret = dns_sd_query_extract(dns_msg->msg,
		dns_msg->msg_size, &filter, label, size, &n);
	if (ret < 0) {
		NET_DBG("unable to extract query (%d)", ret);
		return;
	}

	if (IS_ENABLED(CONFIG_MDNS_RESPONDER_DNS_SD_SERVICE_TYPE_ENUMERATION)
		&& dns_sd_is_service_type_enumeration(&filter)) {

		/*
		 * RFC 6763, Section 9
		 *
		 * A DNS query for PTR records with the name
		 * "_services._dns-sd._udp.<Domain>" yields a set of PTR records,
		 * where the rdata of each PTR record is the two-label <Service> name,
		 * plus the same domain, e.g., "_http._tcp.<Domain>".
		 */
		dns_sd_create_wildcard_filter(&filter);
		service_type_enum = true;
	}

	DNS_SD_FOREACH(record) {
		/* Checks validity and then compare */
		if (dns_sd_rec_match(record, &filter)) {
			NET_DBG("matched query: %s.%s.%s.%s port: %u",
				record->instance, record->service,
				record->proto, record->domain,
				ntohs(*(record->port)));

			/* Construct the response */
			if (service_type_enum) {
				ret = dns_sd_handle_service_type_enum(record, addr4, addr6,
					result->data, result->size);
				if (ret < 0) {
					NET_DBG("dns_sd_handle_service_type_enum() failed (%d)",
						ret);
					continue;
				}
			} else {
				ret = dns_sd_handle_ptr_query(record, addr4, addr6,
					result->data, result->size);
				if (ret < 0) {
					NET_DBG("dns_sd_handle_ptr_query() failed (%d)", ret);
					continue;
				}
			}

			result->len = ret;

			/* Send the response */
			ret = net_context_sendto(ctx, result->data,
				result->len, &dst, dst_len, NULL,
				K_NO_WAIT, NULL);
			if (ret < 0) {
				NET_DBG("Cannot send mDNS reply (%d)", ret);
				continue;
			}
		}
	}
}

static int dns_read(struct net_context *ctx,
		    struct net_pkt *pkt,
		    union net_ip_header *ip_hdr,
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
	int ret;

	data_len = MIN(net_pkt_remaining_data(pkt), DNS_RESOLVER_MAX_BUF_SIZE);

	/* Store the DNS query name into a temporary net_buf, which will be
	 * eventually used to send a response
	 */
	result = net_buf_alloc(&mdns_msg_pool, BUF_ALLOC_TIMEOUT);
	if (!result) {
		ret = -ENOMEM;
		goto quit;
	}

	/* TODO: Instead of this temporary copy, just use the net_pkt directly.
	 */
	ret = net_pkt_read(pkt, dns_data->data, data_len);
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
		uint8_t *lquery;

		(void)memset(result->data, 0, net_buf_tailroom(result));
		result->len = 0U;

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
			qtype_to_string(qtype), "IN",
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
			send_response(ctx, pkt, ip_hdr, result, qtype);
		} else if (IS_ENABLED(CONFIG_MDNS_RESPONDER_DNS_SD)
			&& qtype == DNS_RR_TYPE_PTR) {
			send_sd_response(ctx, pkt, ip_hdr, &dns_msg, result);
		}

	} while (--queries);

	ret = 0;

quit:
	if (result) {
		net_buf_unref(result);
	}

	return ret;
}

static void recv_cb(struct net_context *net_ctx,
		    struct net_pkt *pkt,
		    union net_ip_header *ip_hdr,
		    union net_proto_header *proto_hdr,
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

	dns_data = net_buf_alloc(&mdns_msg_pool, BUF_ALLOC_TIMEOUT);
	if (!dns_data) {
		goto quit;
	}

	ret = dns_read(ctx, pkt, ip_hdr, dns_data, &info);
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
	int ret;

	ret = net_ipv4_igmp_join(iface, addr);
	if (ret < 0) {
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
		setup_ipv4_addr(&local_addr4);

		ipv4 = get_ctx(AF_INET);

		ret = bind_ctx(ipv4, (struct sockaddr *)&local_addr4,
			       sizeof(local_addr4));
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

static int mdns_responder_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	net_mgmt_init_event_callback(&mgmt_cb, mdns_iface_event_handler,
				     NET_EVENT_IF_UP);

	net_mgmt_add_event_callback(&mgmt_cb);

	return init_listener();
}

SYS_INIT(mdns_responder_init, APPLICATION, CONFIG_MDNS_RESPONDER_INIT_PRIO);
