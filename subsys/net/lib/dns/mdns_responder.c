/** @file
 * @brief mDNS responder
 *
 * This listens to mDNS queries and responds to them.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mdns_responder, CONFIG_MDNS_RESPONDER_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>

#include <zephyr/net/mld.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/igmp.h>

#include "dns_sd.h"
#include "dns_pack.h"
#include "ipv6.h"
#include "../../ip/net_stats.h"

#include "net_private.h"

/*
 * GCC complains about struct sockaddr accesses due to the various
 * address-family-specific variants being of differing sizes. Let's not
 * mess with code (which looks correct), just silence the compiler.
 */
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif

extern void dns_dispatcher_svc_handler(struct net_socket_service_event *pev);

#define MDNS_LISTEN_PORT 5353

#define MDNS_TTL CONFIG_MDNS_RESPONDER_TTL /* In seconds */

#if defined(CONFIG_NET_IPV4)
static struct mdns_responder_context v4_ctx[MAX_IPV4_IFACE_COUNT];

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(v4_svc, dns_dispatcher_svc_handler,
				      MDNS_MAX_IPV4_IFACE_COUNT);
#endif

#if defined(CONFIG_NET_IPV6)
static struct mdns_responder_context v6_ctx[MAX_IPV6_IFACE_COUNT];

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(v6_svc, dns_dispatcher_svc_handler,
				      MDNS_MAX_IPV6_IFACE_COUNT);
#endif

static struct net_mgmt_event_callback mgmt_cb;
static const struct dns_sd_rec *external_records;
static size_t external_records_count;

#define BUF_ALLOC_TIMEOUT K_MSEC(100)

#ifndef CONFIG_NET_TEST
static int setup_dst_addr(int sock, sa_family_t family,
			  struct sockaddr *dst, socklen_t *dst_len);
#endif /* CONFIG_NET_TEST */

#define DNS_RESOLVER_MIN_BUF		2
#define DNS_RESOLVER_BUF_CTR	(DNS_RESOLVER_MIN_BUF + \
				 CONFIG_MDNS_RESOLVER_ADDITIONAL_BUF_CTR)

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
		ARRAY_FOR_EACH(v4_ctx, i) {
			int ret = net_ipv4_igmp_join(iface,
					&net_sin(&v4_ctx[i].dispatcher.local_addr)->sin_addr,
					NULL);
			if (ret < 0) {
				NET_DBG("Cannot add IPv4 multicast address to iface %d",
					net_if_get_by_iface(iface));
			}
		}
#endif /* defined(CONFIG_NET_IPV4) */
	}
}

static int set_ttl_hop_limit(int sock, int level, int option, int new_limit)
{
	return zsock_setsockopt(sock, level, option, &new_limit, sizeof(new_limit));
}

int setup_dst_addr(int sock, sa_family_t family,
		   struct sockaddr *dst, socklen_t *dst_len)
{
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		create_ipv4_addr(net_sin(dst));
		*dst_len = sizeof(struct sockaddr_in);

		ret = set_ttl_hop_limit(sock, IPPROTO_IP, IP_MULTICAST_TTL, 255);
		if (ret < 0) {
			NET_DBG("Cannot set %s multicast %s (%d)", "IPv4", "TTL", ret);
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		create_ipv6_addr(net_sin6(dst));
		*dst_len = sizeof(struct sockaddr_in6);

		ret = set_ttl_hop_limit(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, 255);
		if (ret < 0) {
			NET_DBG("Cannot set %s multicast %s (%d)", "IPv6", "hoplimit", ret);
		}
	} else {
		return -EPFNOSUPPORT;
	}

	return 0;
}

static int get_socket(sa_family_t family)
{
	int ret;

	ret = zsock_socket(family, SOCK_DGRAM, IPPROTO_UDP);
	if (ret < 0) {
		ret = -errno;
		NET_DBG("Cannot get socket (%d)", ret);
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

static int create_answer(int sock,
			 struct net_buf *query,
			 enum dns_rr_type qtype,
			 uint16_t addr_len, uint8_t *addr)
{
	/* Prepare the response into the query buffer: move the name
	 * query buffer has to get enough free space: dns_hdr + answer
	 */
	if ((net_buf_max_len(query) - query->len) < (DNS_MSG_HEADER_SIZE +
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

static int send_response(int sock,
			 sa_family_t family,
			 struct sockaddr *src_addr,
			 size_t addrlen,
			 struct net_buf *query,
			 enum dns_rr_type qtype)
{
	struct net_if *iface;
	socklen_t dst_len;
	int ret;
	COND_CODE_1(IS_ENABLED(CONFIG_NET_IPV6),
		    (struct sockaddr_in6), (struct sockaddr_in)) dst;

	ret = setup_dst_addr(sock, family, (struct sockaddr *)&dst, &dst_len);
	if (ret < 0) {
		NET_DBG("unable to set up the response address");
		return ret;
	}

	if (family == AF_INET6) {
		iface = net_if_ipv6_select_src_iface(&net_sin6(src_addr)->sin6_addr);
	} else {
		iface = net_if_ipv4_select_src_iface(&net_sin(src_addr)->sin_addr);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && qtype == DNS_RR_TYPE_A) {
		const struct in_addr *addr;

		if (family == AF_INET) {
			addr = net_if_ipv4_select_src_addr(iface,
							   &net_sin(src_addr)->sin_addr);
		} else {
			struct sockaddr_in tmp_addr;

			create_ipv4_addr(&tmp_addr);
			addr = net_if_ipv4_select_src_addr(iface, &tmp_addr.sin_addr);
		}

		ret = create_answer(sock, query, qtype, sizeof(struct in_addr), (uint8_t *)addr);
		if (ret != 0) {
			return ret;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && qtype == DNS_RR_TYPE_AAAA) {
		const struct in6_addr *addr;

		if (family == AF_INET6) {
			addr = net_if_ipv6_select_src_addr(iface,
							   &net_sin6(src_addr)->sin6_addr);
		} else {
			struct sockaddr_in6 tmp_addr;

			create_ipv6_addr(&tmp_addr);
			addr = net_if_ipv6_select_src_addr(iface, &tmp_addr.sin6_addr);
		}

		ret = create_answer(sock, query, qtype, sizeof(struct in6_addr), (uint8_t *)addr);
		if (ret != 0) {
			return -ENOMEM;
		}
	} else {
		/* TODO: support also service PTRs */
		return -EINVAL;
	}

	ret = zsock_sendto(sock, query->data, query->len, 0,
			   (struct sockaddr *)&dst, dst_len);
	if (ret < 0) {
		NET_DBG("Cannot send %s reply (%d)", "mDNS", ret);
	} else {
		net_stats_update_dns_sent(iface);
	}

	return ret;
}

static void send_sd_response(int sock,
			     sa_family_t family,
			     struct sockaddr *src_addr,
			     size_t addrlen,
			     struct dns_msg_t *dns_msg,
			     struct net_buf *result)
{
	struct net_if *iface;
	socklen_t dst_len;
	int ret;
	const struct dns_sd_rec *record;
	/* filter must be zero-initialized for "wildcard" port */
	struct dns_sd_rec filter = {0};
	bool service_type_enum = false;
	const struct in6_addr *addr6 = NULL;
	const struct in_addr *addr4 = NULL;
	char instance_buf[DNS_SD_INSTANCE_MAX_SIZE + 1];
	char service_buf[DNS_SD_SERVICE_MAX_SIZE + 1];
	/* Depending on segment count in the query, third buffer could hold
	 * either protocol or domain, use larger size.
	 */
	char proto_buf[DNS_SD_DOMAIN_MAX_SIZE + 1];
	char domain_buf[DNS_SD_DOMAIN_MAX_SIZE + 1];
	char *label[4];
	size_t size[] = {
		ARRAY_SIZE(instance_buf),
		ARRAY_SIZE(service_buf),
		ARRAY_SIZE(proto_buf),
		ARRAY_SIZE(domain_buf),
	};
	size_t n = ARRAY_SIZE(label);
	size_t rec_num;
	size_t ext_rec_num = external_records_count;
	COND_CODE_1(IS_ENABLED(CONFIG_NET_IPV6),
		    (struct sockaddr_in6), (struct sockaddr_in)) dst;

	BUILD_ASSERT(ARRAY_SIZE(label) == ARRAY_SIZE(size), "");

	/*
	 * work around for bug in compliance scripts which say that the array
	 * should be static const (incorrect)
	 */
	label[0] = instance_buf;
	label[1] = service_buf;
	label[2] = proto_buf;
	label[3] = domain_buf;

	ret = setup_dst_addr(sock, family, (struct sockaddr *)&dst, &dst_len);
	if (ret < 0) {
		NET_DBG("unable to set up the response address");
		return;
	}

	if (family == AF_INET6) {
		iface = net_if_ipv6_select_src_iface(&net_sin6(src_addr)->sin6_addr);
	} else {
		iface = net_if_ipv4_select_src_iface(&net_sin(src_addr)->sin_addr);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		/* Look up the local IPv4 address */
		if (family == AF_INET) {
			addr4 = net_if_ipv4_select_src_addr(iface,
							    &net_sin(src_addr)->sin_addr);
		} else {
			struct sockaddr_in tmp_addr;

			create_ipv4_addr(&tmp_addr);
			addr4 = net_if_ipv4_select_src_addr(iface, &tmp_addr.sin_addr);
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		/* Look up the local IPv6 address */
		if (family == AF_INET6) {
			addr6 = net_if_ipv6_select_src_addr(iface,
							    &net_sin6(src_addr)->sin6_addr);
		} else {
			struct sockaddr_in6 tmp_addr;

			create_ipv6_addr(&tmp_addr);
			addr6 = net_if_ipv6_select_src_addr(iface, &tmp_addr.sin6_addr);
		}
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

	DNS_SD_COUNT(&rec_num);

	while (rec_num > 0 || ext_rec_num > 0) {
		/*
		 * The loop will always iterate over all entries, it can be done
		 * backwards for simplicity
		 */
		if (rec_num > 0) {
			DNS_SD_GET(rec_num - 1, &record);
			rec_num--;
		} else {
			record = &external_records[ext_rec_num - 1];
			ext_rec_num--;
		}

		/* Checks validity and then compare */
		if (dns_sd_rec_match(record, &filter)) {
			NET_DBG("matched query: %s.%s.%s.%s port: %u",
				record->instance, record->service,
				record->proto, record->domain,
				ntohs(*(record->port)));

			/* Construct the response */
			if (service_type_enum) {
				ret = dns_sd_handle_service_type_enum(record, addr4, addr6,
						result->data, net_buf_max_len(result));
				if (ret < 0) {
					NET_DBG("dns_sd_handle_service_type_enum() failed (%d)",
						ret);
					continue;
				}
			} else {
				ret = dns_sd_handle_ptr_query(record, addr4, addr6,
						result->data, net_buf_max_len(result));
				if (ret < 0) {
					NET_DBG("dns_sd_handle_ptr_query() failed (%d)", ret);
					continue;
				}
			}

			result->len = ret;

			/* Send the response */
			ret = zsock_sendto(sock, result->data, result->len, 0,
					   (struct sockaddr *)&dst, dst_len);
			if (ret < 0) {
				NET_DBG("Cannot send %s reply (%d)", "mDNS", ret);
				continue;
			} else {
				net_stats_update_dns_sent(iface);
			}
		}
	}
}

static int dns_read(int sock,
		    struct net_buf *dns_data,
		    size_t len,
		    struct sockaddr *src_addr,
		    size_t addrlen)
{
	/* Helper struct to track the dns msg received from the server */
	const char *hostname = net_hostname_get();
	int hostname_len = strlen(hostname);
	int family = src_addr->sa_family;
	struct net_buf *result;
	struct dns_msg_t dns_msg;
	int data_len;
	int queries;
	int ret;

	data_len = MIN(len, DNS_RESOLVER_MAX_BUF_SIZE);

	/* Store the DNS query name into a temporary net_buf, which will be
	 * eventually used to send a response
	 */
	result = net_buf_alloc(&mdns_msg_pool, BUF_ALLOC_TIMEOUT);
	if (!result) {
		ret = -ENOMEM;
		goto quit;
	}

	dns_msg.msg = dns_data->data;
	dns_msg.msg_size = data_len;

	ret = mdns_unpack_query_header(&dns_msg, NULL);
	if (ret < 0) {
		goto quit;
	}

	queries = ret;

	NET_DBG("Received %d %s from %s", queries,
		queries > 1 ? "queries" : "query",
		net_sprint_addr(family,
				family == AF_INET ?
				(const void *)&net_sin(src_addr)->sin_addr :
				(const void *)&net_sin6(src_addr)->sin6_addr));

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
			dns_qtype_to_str(qtype), "IN",
			result->data, ret);

		/* If the query matches to our hostname, then send reply.
		 * We skip the first dot, and make sure there is dot after
		 * matching hostname.
		 */
		if (!strncasecmp(hostname, result->data + 1, hostname_len) &&
		    (result->len - 1) >= hostname_len &&
		    &(result->data + 1)[hostname_len] == lquery) {
			NET_DBG("%s query to our hostname %s.local", "mDNS",
				hostname);
			send_response(sock, family, src_addr, addrlen,
				      result, qtype);
		} else if (IS_ENABLED(CONFIG_MDNS_RESPONDER_DNS_SD)
			&& qtype == DNS_RR_TYPE_PTR) {
			send_sd_response(sock, family, src_addr, addrlen,
					 &dns_msg, result);
		}

	} while (--queries);

	ret = 0;

quit:
	if (result) {
		net_buf_unref(result);
	}

	return ret;
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

	if (!net_if_is_up(iface)) {
		struct net_if_mcast_addr *maddr;

		NET_DBG("Interface %d is down, not joining mcast group yet",
			net_if_get_by_iface(iface));

		maddr = net_if_ipv4_maddr_add(iface, addr);
		if (!maddr) {
			NET_DBG("Cannot add multicast address %s",
				net_sprint_ipv4_addr(addr));
		}

		return;
	}

	ret = net_ipv4_igmp_join(iface, addr, NULL);
	if (ret < 0) {
		NET_DBG("Cannot add IPv4 multicast address to iface %d",
			net_if_get_by_iface(iface));
	}
}

static void setup_ipv4_addr(struct sockaddr_in *local_addr)
{
	create_ipv4_addr(local_addr);

	net_if_foreach(iface_ipv4_cb, &local_addr->sin_addr);
}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_INTERFACE_NAME_LEN)
#define INTERFACE_NAME_LEN CONFIG_NET_INTERFACE_NAME_LEN
#else
#define INTERFACE_NAME_LEN 0
#endif

static int dispatcher_cb(void *my_ctx, int sock,
			 struct sockaddr *addr, size_t addrlen,
			 struct net_buf *dns_data, size_t len)
{
	int ret;

	ARG_UNUSED(my_ctx);

	ret = dns_read(sock, dns_data, len, addr, addrlen);
	if (ret < 0 && ret != -EINVAL && ret != -ENOENT) {
		NET_DBG("%s read failed (%d)", "mDNS", ret);
	}

	return ret;
}

static int register_dispatcher(struct mdns_responder_context *ctx,
			       const struct net_socket_service_desc *svc,
			       struct sockaddr *local,
			       int ifindex,
			       struct zsock_pollfd *fds,
			       size_t fds_len)
{
	ctx->dispatcher.type = DNS_SOCKET_RESPONDER;
	ctx->dispatcher.cb = dispatcher_cb;
	ctx->dispatcher.fds = fds;
	ctx->dispatcher.fds_len = fds_len;
	ctx->dispatcher.sock = ctx->sock;
	ctx->dispatcher.svc = svc;
	ctx->dispatcher.mdns_ctx = ctx;
	ctx->dispatcher.pair = NULL;
	ctx->dispatcher.ifindex = ifindex;

	/* Mark the fd so that "net sockets" can show it. This is needed if there
	 * is already a socket bound to same port and the dispatcher will mux
	 * the connections. Without this, the FD in "net sockets" services list will
	 * show the socket descriptor value as -1.
	 */
	svc->pev[0].event.fd = ctx->sock;

	if (IS_ENABLED(CONFIG_NET_IPV6) && local->sa_family == AF_INET6) {
		memcpy(&ctx->dispatcher.local_addr, local,
		       sizeof(struct sockaddr_in6));
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && local->sa_family == AF_INET) {
		memcpy(&ctx->dispatcher.local_addr, local,
		       sizeof(struct sockaddr_in));
	} else {
		return -ENOTSUP;
	}

	return dns_dispatcher_register(&ctx->dispatcher);
}

static int init_listener(void)
{
	int ret, ok = 0, ifindex;
	char name[INTERFACE_NAME_LEN + 1];
	struct ifreq if_req;
	struct net_if *iface;
	int iface_count, fds_pos;

	NET_IFACE_COUNT(&iface_count);
	NET_DBG("Setting %s listener to %d interface%s", "mDNS", iface_count,
		iface_count > 1 ? "s" : "");

#if defined(CONFIG_NET_IPV6)
	/* Because there is only one IPv6 socket service context for all
	 * IPv6 sockets, we must collect the sockets in one place.
	 */
	struct zsock_pollfd ipv6_fds[MAX_IPV6_IFACE_COUNT];
	struct sockaddr_in6 local_addr6;
	int v6;

	if ((iface_count > MAX_IPV6_IFACE_COUNT && MAX_IPV6_IFACE_COUNT > 0)) {
		NET_WARN("You have %d %s interfaces configured but there "
			 "are %d network interfaces in the system.",
			 MAX_IPV6_IFACE_COUNT, "IPv6", iface_count);
	}

	setup_ipv6_addr(&local_addr6);

	ARRAY_FOR_EACH(ipv6_fds, i) {
		ipv6_fds[i].fd = -1;
	}

	fds_pos = 0;

	ARRAY_FOR_EACH(v6_ctx, i) {
		ARRAY_FOR_EACH(v6_ctx[i].fds, j) {
			v6_ctx[i].fds[j].fd = -1;
		}

		v6 = get_socket(AF_INET6);
		if (v6 < 0) {
			NET_ERR("Cannot get %s socket (%d %s interfaces). Max sockets is %d",
				"IPv6", MAX_IPV6_IFACE_COUNT,
				"IPv6", CONFIG_NET_MAX_CONTEXTS);
			continue;
		}

		iface = net_if_get_by_index(i + 1);
		if (iface == NULL) {
			zsock_close(v6);
			continue;
		}

		ifindex = net_if_get_by_iface(iface);

		ret = net_if_get_name(iface, name, INTERFACE_NAME_LEN);
		if (ret < 0) {
			NET_DBG("Cannot get interface name for %d (%d)",
				ifindex, ret);
		} else {
			memset(&if_req, 0, sizeof(if_req));
			strncpy(if_req.ifr_name, name, sizeof(if_req.ifr_name) - 1);

			ret = zsock_setsockopt(v6, SOL_SOCKET, SO_BINDTODEVICE,
					       &if_req, sizeof(if_req));
			if (ret < 0) {
				NET_DBG("Cannot bind sock %d to interface %d (%d)",
					v6, ifindex, ret);
			}
		}

		v6_ctx[i].sock = v6;
		ret = -1;

		ARRAY_FOR_EACH(v6_ctx[i].fds, j) {
			if (v6_ctx[i].fds[j].fd == v6) {
				ret = 0;
				break;
			}

			if (v6_ctx[i].fds[j].fd < 0) {
				v6_ctx[i].fds[j].fd = v6;
				v6_ctx[i].fds[j].events = ZSOCK_POLLIN;
				ipv6_fds[fds_pos].fd = v6;
				ipv6_fds[fds_pos++].events = ZSOCK_POLLIN;
				ret = 0;
				break;
			}
		}

		if (ret < 0) {
			NET_DBG("Cannot set %s to socket (%d)", "polling", ret);
			zsock_close(v6);
			continue;
		}

		ret = register_dispatcher(&v6_ctx[i], &v6_svc, (struct sockaddr *)&local_addr6,
					  ifindex, ipv6_fds, ARRAY_SIZE(ipv6_fds));
		if (ret < 0) {
			NET_DBG("Cannot register %s %s socket service (%d)",
				"IPv6", "mDNS", ret);
			zsock_close(v6);
		} else {
			ok++;
		}
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	struct zsock_pollfd ipv4_fds[MAX_IPV4_IFACE_COUNT];
	struct sockaddr_in local_addr4;
	int v4;

	if ((iface_count > MAX_IPV4_IFACE_COUNT && MAX_IPV4_IFACE_COUNT > 0)) {
		NET_WARN("You have %d %s interfaces configured but there "
			 "are %d network interfaces in the system.",
			 MAX_IPV4_IFACE_COUNT, "IPv4", iface_count);
	}

	setup_ipv4_addr(&local_addr4);

	ARRAY_FOR_EACH(ipv4_fds, i) {
		ipv4_fds[i].fd = -1;
	}

	fds_pos = 0;

	ARRAY_FOR_EACH(v4_ctx, i) {
		ARRAY_FOR_EACH(v4_ctx[i].fds, j) {
			v4_ctx[i].fds[j].fd = -1;
		}

		v4 = get_socket(AF_INET);
		if (v4 < 0) {
			NET_ERR("Cannot get %s socket (%d %s interfaces). Max sockets is %d",
				"IPv4", MAX_IPV4_IFACE_COUNT,
				"IPv4", CONFIG_NET_MAX_CONTEXTS);
			continue;
		}

		iface = net_if_get_by_index(i + 1);
		if (iface == NULL) {
			zsock_close(v4);
			continue;
		}

		ifindex = net_if_get_by_iface(iface);

		ret = net_if_get_name(iface, name, INTERFACE_NAME_LEN);
		if (ret < 0) {
			NET_DBG("Cannot get interface name for %d (%d)",
				ifindex, ret);
		} else {
			memset(&if_req, 0, sizeof(if_req));
			strncpy(if_req.ifr_name, name, sizeof(if_req.ifr_name) - 1);

			ret = zsock_setsockopt(v4, SOL_SOCKET, SO_BINDTODEVICE,
					       &if_req, sizeof(if_req));
			if (ret < 0) {
				NET_DBG("Cannot bind sock %d to interface %d (%d)",
					v4, ifindex, ret);
			}
		}

		v4_ctx[i].sock = v4;
		ret = -1;

		ARRAY_FOR_EACH(v4_ctx[i].fds, j) {
			if (v4_ctx[i].fds[j].fd == v4) {
				ret = 0;
				break;
			}

			if (v4_ctx[i].fds[j].fd < 0) {
				v4_ctx[i].fds[j].fd = v4;
				v4_ctx[i].fds[j].events = ZSOCK_POLLIN;
				ipv4_fds[fds_pos].fd = v4;
				ipv4_fds[fds_pos++].events = ZSOCK_POLLIN;
				ret = 0;
				break;
			}
		}

		if (ret < 0) {
			NET_DBG("Cannot set %s to socket (%d)", "polling", ret);
			zsock_close(v4);
			continue;
		}

		ret = register_dispatcher(&v4_ctx[i], &v4_svc, (struct sockaddr *)&local_addr4,
					  ifindex, ipv4_fds, ARRAY_SIZE(ipv4_fds));
		if (ret < 0) {
			NET_DBG("Cannot register %s %s socket service (%d)",
				"IPv4", "mDNS", ret);
			zsock_close(v4);
		} else {
			ok++;
		}
	}
#endif /* CONFIG_NET_IPV4 */

	if (!ok) {
		NET_WARN("Cannot start %s responder", "mDNS");
	}

	return !ok;
}

static int mdns_responder_init(void)
{
	external_records = NULL;
	external_records_count = 0;

	net_mgmt_init_event_callback(&mgmt_cb, mdns_iface_event_handler,
				     NET_EVENT_IF_UP);

	net_mgmt_add_event_callback(&mgmt_cb);

	return init_listener();
}

int mdns_responder_set_ext_records(const struct dns_sd_rec *records, size_t count)
{
	if (records == NULL || count == 0) {
		return -EINVAL;
	}

	external_records = records;
	external_records_count = count;

	return 0;
}

void mdns_init_responder(void)
{
	(void)mdns_responder_init();
}
