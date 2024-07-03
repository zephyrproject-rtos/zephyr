/** @file
 * @brief LLMNR responder
 *
 * This listens to LLMNR queries and responds to them.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_llmnr_responder, CONFIG_LLMNR_RESPONDER_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/udp.h>
#include <zephyr/net/igmp.h>

#include "dns_pack.h"
#include "ipv6.h"

#include "net_private.h"

#define LLMNR_LISTEN_PORT 5355

#define LLMNR_TTL CONFIG_LLMNR_RESPONDER_TTL /* In seconds */

#if defined(CONFIG_NET_IPV4)
static int ipv4;
static struct sockaddr_in local_addr4;
#endif

#if defined(CONFIG_NET_IPV6)
static int ipv6;
#endif

static struct net_mgmt_event_callback mgmt_cb;

#define BUF_ALLOC_TIMEOUT K_MSEC(100)

/* This value is recommended by RFC 1035 */
#define DNS_RESOLVER_MAX_BUF_SIZE	512
#define DNS_RESOLVER_MIN_BUF		2
#define DNS_RESOLVER_BUF_CTR	(DNS_RESOLVER_MIN_BUF + \
				 CONFIG_LLMNR_RESOLVER_ADDITIONAL_BUF_CTR)

#if (IS_ENABLED(CONFIG_NET_IPV6) && IS_ENABLED(CONFIG_NET_IPV4))
#define LLMNR_MAX_POLL 2
#else
#define LLMNR_MAX_POLL 1
#endif

/* Socket polling for each server connection */
static struct zsock_pollfd fds[LLMNR_MAX_POLL];

static void svc_handler(struct k_work *work);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(svc_llmnr, NULL, svc_handler, LLMNR_MAX_POLL);

NET_BUF_POOL_DEFINE(llmnr_msg_pool, DNS_RESOLVER_BUF_CTR,
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

static void create_ipv6_dst_addr(struct sockaddr_in6 *src_addr,
				 struct sockaddr_in6 *addr)
{
	addr->sin6_family = AF_INET6;
	addr->sin6_port = src_addr->sin6_port;

	net_ipv6_addr_copy_raw((uint8_t *)&addr->sin6_addr,
			       (uint8_t *)&src_addr->sin6_addr);
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

static void create_ipv4_dst_addr(struct sockaddr_in *src_addr,
				 struct sockaddr_in *addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = src_addr->sin_port;

	net_ipv4_addr_copy_raw((uint8_t *)&addr->sin_addr,
			       (uint8_t *)&src_addr->sin_addr);
}
#endif

static void llmnr_iface_event_handler(struct net_mgmt_event_callback *cb,
				      uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IF_UP) {
#if defined(CONFIG_NET_IPV4)
		int ret = net_ipv4_igmp_join(iface, &local_addr4.sin_addr, NULL);

		if (ret < 0) {
			NET_DBG("Cannot add IPv4 multicast address to iface %p",
				iface);
		}
#endif /* defined(CONFIG_NET_IPV4) */
	}
}

static int get_socket(sa_family_t family)
{
	int ret;

	ret = zsock_socket(family, SOCK_DGRAM, IPPROTO_UDP);
	if (ret < 0) {
		NET_DBG("Cannot get context (%d)", ret);
	}

	return ret;
}

static int bind_ctx(int sock,
		    struct sockaddr *local_addr,
		    socklen_t addrlen)
{
	int ret;

	if (sock < 0) {
		return -EINVAL;
	}

	ret = zsock_bind(sock, local_addr, addrlen);
	if (ret < 0) {
		NET_DBG("Cannot bind to %s %s port (%d)", "LLMNR",
			local_addr->sa_family == AF_INET ?
			"IPv4" : "IPv6", ret);
		return ret;
	}

	return ret;
}

static void setup_dns_hdr(uint8_t *buf, uint16_t answers, uint16_t dns_id)
{
	uint16_t offset;
	uint16_t flags;

	/* See RFC 1035, ch 4.1.1 and RFC 4795 ch 2.1.1 for header details */

	flags = BIT(15);  /* This is response */

	UNALIGNED_PUT(htons(dns_id), (uint16_t *)(buf));
	offset = DNS_HEADER_ID_LEN;

	UNALIGNED_PUT(htons(flags), (uint16_t *)(buf+offset));
	offset += DNS_HEADER_FLAGS_LEN;

	UNALIGNED_PUT(htons(1), (uint16_t *)(buf + offset));
	offset += DNS_QDCOUNT_LEN;

	UNALIGNED_PUT(htons(answers), (uint16_t *)(buf + offset));
	offset += DNS_ANCOUNT_LEN;

	UNALIGNED_PUT(0, (uint16_t *)(buf + offset));
	offset += DNS_NSCOUNT_LEN;

	UNALIGNED_PUT(0, (uint16_t *)(buf + offset));
}

static void add_question(struct net_buf *query, enum dns_rr_type qtype)
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

	offset = DNS_MSG_HEADER_SIZE + query->len + 1;
	UNALIGNED_PUT(htons(qtype), (uint16_t *)(query->data+offset));

	offset += DNS_QTYPE_LEN;
	UNALIGNED_PUT(htons(DNS_CLASS_IN), (uint16_t *)(query->data+offset));
}

static int add_answer(struct net_buf *query, uint32_t ttl,
		       uint16_t addr_len, const uint8_t *addr)
{
	const uint16_t q_len = query->len + 1 + DNS_QTYPE_LEN + DNS_QCLASS_LEN;
	uint16_t offset = DNS_MSG_HEADER_SIZE + q_len;

	memcpy(query->data + offset, query->data + DNS_MSG_HEADER_SIZE, q_len);
	offset += q_len;

	UNALIGNED_PUT(htonl(ttl), query->data + offset);
	offset += DNS_TTL_LEN;

	UNALIGNED_PUT(htons(addr_len), query->data + offset);
	offset += DNS_RDLENGTH_LEN;

	memcpy(query->data + offset, addr, addr_len);

	return offset + addr_len;
}

static int create_answer(enum dns_rr_type qtype,
			 struct net_buf *query,
			 uint16_t dns_id,
			 uint16_t addr_len, const uint8_t *addr)
{
	/* Prepare the response into the query buffer: move the name
	 * query buffer has to get enough free space: dns_hdr + query + answer
	 */
	if ((net_buf_max_len(query) - query->len) < (DNS_MSG_HEADER_SIZE +
					  (DNS_QTYPE_LEN + DNS_QCLASS_LEN) * 2 +
					  DNS_TTL_LEN + DNS_RDLENGTH_LEN +
					  addr_len + query->len)) {
		return -ENOBUFS;
	}

	memmove(query->data + DNS_MSG_HEADER_SIZE, query->data, query->len);

	setup_dns_hdr(query->data, 1, dns_id);

	add_question(query, qtype);

	query->len = add_answer(query, LLMNR_TTL, addr_len, addr);

	return 0;
}

#if defined(CONFIG_NET_IPV4)
static const uint8_t *get_ipv4_src(struct net_if *iface, struct in_addr *dst)
{
	const struct in_addr *addr;

	addr = net_if_ipv4_select_src_addr(iface, dst);
	if (!addr || net_ipv4_is_addr_unspecified(addr)) {
		return NULL;
	}

	return (const uint8_t *)addr;
}
#endif

#if defined(CONFIG_NET_IPV6)
static const uint8_t *get_ipv6_src(struct net_if *iface, struct in6_addr *dst)
{
	const struct in6_addr *addr;

	addr = net_if_ipv6_select_src_addr(iface, dst);
	if (!addr || net_ipv6_is_addr_unspecified(addr)) {
		return NULL;
	}

	return (const uint8_t *)addr;
}
#endif

static int set_ttl_hop_limit(int sock, int level, int option, int new_limit)
{
	return zsock_setsockopt(sock, level, option, &new_limit, sizeof(new_limit));
}

#if defined(CONFIG_NET_IPV4)
static int create_ipv4_answer(int sock,
			      struct sockaddr_in *src_addr,
			      enum dns_rr_type qtype,
			      struct net_buf *query,
			      uint16_t dns_id,
			      struct sockaddr *dst,
			      socklen_t *dst_len)
{
	const uint8_t *addr;
	int addr_len;

	create_ipv4_dst_addr(src_addr, net_sin(dst));

	*dst_len = sizeof(struct sockaddr_in);

	/* Select proper address according to destination */
	addr = get_ipv4_src(NULL, &net_sin(dst)->sin_addr);
	if (!addr) {
		return -ENOENT;
	}

	addr_len = sizeof(struct in_addr);

	if (create_answer(qtype, query, dns_id, addr_len, addr)) {
		return -ENOMEM;
	}

	(void)set_ttl_hop_limit(sock, IPPROTO_IP, IP_MULTICAST_TTL, 255);

	return 0;
}
#endif /* CONFIG_NET_IPV4 */

static int create_ipv6_answer(int sock,
			      struct sockaddr_in6 *src_addr,
			      enum dns_rr_type qtype,
			      struct net_buf *query,
			      uint16_t dns_id,
			      struct sockaddr *dst,
			      socklen_t *dst_len)
{
#if defined(CONFIG_NET_IPV6)
	const uint8_t *addr;
	int addr_len;

	create_ipv6_dst_addr(src_addr, net_sin6(dst));

	*dst_len = sizeof(struct sockaddr_in6);

	addr = get_ipv6_src(NULL, &src_addr->sin6_addr);
	if (!addr) {
		return -ENOENT;
	}

	addr_len = sizeof(struct in6_addr);

	if (create_answer(qtype, query, dns_id, addr_len, addr)) {
		return -ENOMEM;
	}

	(void)set_ttl_hop_limit(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, 255);

#endif /* CONFIG_NET_IPV6 */
	return 0;
}

static int send_response(int sock,
			 struct sockaddr *src_addr,
			 size_t addrlen,
			 struct net_buf *reply,
			 enum dns_rr_type qtype,
			 uint16_t dns_id)
{
	socklen_t dst_len;
	int ret;
	COND_CODE_1(IS_ENABLED(CONFIG_NET_IPV6),
		    (struct sockaddr_in6), (struct sockaddr_in)) dst;

	if (IS_ENABLED(CONFIG_NET_IPV4) && src_addr->sa_family == AF_INET) {
		ret = create_ipv4_answer(sock, (struct sockaddr_in *)src_addr,
					 qtype, reply, dns_id,
					 (struct sockaddr *)&dst, &dst_len);
		if (ret < 0) {
			return ret;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && src_addr->sa_family == AF_INET6) {
		ret = create_ipv6_answer(sock, (struct sockaddr_in6 *)src_addr,
					 qtype, reply, dns_id,
					 (struct sockaddr *)&dst, &dst_len);
		if (ret < 0) {
			return ret;
		}
	} else {
		/* TODO: support also service PTRs */
		return -EPFNOSUPPORT;
	}

	ret = zsock_sendto(sock, reply->data, reply->len, 0,
			   (struct sockaddr *)&dst, dst_len);
	if (ret < 0) {
		NET_DBG("Cannot send %s reply to %s (%d)", "LLMNR",
			src_addr->sa_family == AF_INET ?
			net_sprint_ipv4_addr(&net_sin((struct sockaddr *)&dst)->sin_addr) :
			net_sprint_ipv6_addr(&net_sin6((struct sockaddr *)&dst)->sin6_addr),
			ret);
	}

	return ret;
}

static int dns_read(int sock,
		    struct net_buf *dns_data,
		    size_t len,
		    struct sockaddr *src_addr,
		    size_t addrlen,
		    struct dns_addrinfo *info)
{
	/* Helper struct to track the dns msg received from the server */
	const char *hostname = net_hostname_get();
	int hostname_len = strlen(hostname);
	struct net_buf *result;
	struct dns_msg_t dns_msg;
	uint16_t dns_id = 0U;
	socklen_t optlen;
	int data_len;
	int queries;
	int family;
	int ret;

	data_len = MIN(len, DNS_RESOLVER_MAX_BUF_SIZE);

	/* Store the DNS query name into a temporary net_buf, which will be
	 * eventually used to send a response
	 */
	result = net_buf_alloc(&llmnr_msg_pool, BUF_ALLOC_TIMEOUT);
	if (!result) {
		ret = -ENOMEM;
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

	optlen = sizeof(int);
	(void)zsock_getsockopt(sock, SOL_SOCKET, SO_DOMAIN, &family, &optlen);

	NET_DBG("Received %d %s from %s (id 0x%04x)", queries,
		queries > 1 ? "queries" : "query",
		family == AF_INET ?
		net_sprint_ipv4_addr(&net_sin(src_addr)->sin_addr) :
		net_sprint_ipv6_addr(&net_sin6(src_addr)->sin6_addr),
		dns_id);

	do {
		enum dns_rr_type qtype;
		enum dns_class qclass;

		(void)memset(result->data, 0, net_buf_max_len(result));
		result->len = 0U;

		ret = dns_unpack_query(&dns_msg, result, &qtype, &qclass);
		if (ret < 0) {
			goto quit;
		}

		NET_DBG("[%d] query %s/%s label %s (%d bytes)", queries,
			dns_qtype_to_str(qtype), "IN",
			result->data, ret);

		/* If the query matches to our hostname, then send reply */
		if (!strncasecmp(hostname, result->data + 1, hostname_len) &&
		    (result->len - 1) >= hostname_len) {
			NET_DBG("%s query to our hostname %s", "LLMNR",
				hostname);
			ret = send_response(sock, src_addr, addrlen, result, qtype,
					    dns_id);
			if (ret < 0) {
				NET_DBG("Cannot send response (%d)", ret);
			}
		}
	} while (--queries);

	ret = 0;

quit:
	if (result) {
		net_buf_unref(result);
	}

	return ret;
}

static int recv_data(struct net_socket_service_event *pev)
{
	COND_CODE_1(IS_ENABLED(CONFIG_NET_IPV6),
		    (struct sockaddr_in6), (struct sockaddr_in)) addr;
	struct net_buf *dns_data = NULL;
	struct dns_addrinfo info = { 0 };
	size_t addrlen = sizeof(addr);
	int ret, family = AF_UNSPEC, sock_error, len;
	socklen_t optlen;

	if ((pev->event.revents & ZSOCK_POLLERR) ||
	    (pev->event.revents & ZSOCK_POLLNVAL)) {
		(void)zsock_getsockopt(pev->event.fd, SOL_SOCKET,
				       SO_DOMAIN, &family, &optlen);
		(void)zsock_getsockopt(pev->event.fd, SOL_SOCKET,
				       SO_ERROR, &sock_error, &optlen);
		NET_ERR("Receiver IPv%d socket error (%d)",
			family == AF_INET ? 4 : 6, sock_error);
		ret = DNS_EAI_SYSTEM;
		goto quit;
	}

	dns_data = net_buf_alloc(&llmnr_msg_pool, BUF_ALLOC_TIMEOUT);
	if (!dns_data) {
		ret = -ENOENT;
		goto quit;
	}

	ret = zsock_recvfrom(pev->event.fd, dns_data->data,
			     net_buf_max_len(dns_data), 0,
			     (struct sockaddr *)&addr, &addrlen);
	if (ret < 0) {
		ret = -errno;
		NET_ERR("recv failed on IPv%d socket (%d)",
			family == AF_INET ? 4 : 6, -ret);
		goto free_buf;
	}

	len = ret;

	ret = dns_read(pev->event.fd, dns_data, len,
		       (struct sockaddr *)&addr, addrlen, &info);
	if (ret < 0 && ret != -EINVAL) {
		NET_DBG("%s read failed (%d)", "LLMNR", ret);
	}

free_buf:
	net_buf_unref(dns_data);

quit:
	return ret;
}

static void svc_handler(struct k_work *work)
{
	struct net_socket_service_event *pev =
		CONTAINER_OF(work, struct net_socket_service_event, work);
	int ret;

	ret = recv_data(pev);
	if (ret < 0) {
		NET_ERR("DNS recv error (%d)", ret);
	}
}

#if defined(CONFIG_NET_IPV6)
static void iface_ipv6_cb(struct net_if *iface, void *user_data)
{
	struct in6_addr *addr = user_data;
	int ret;

	ret = net_ipv6_mld_join(iface, addr);
	if (ret < 0) {
		NET_DBG("Cannot join %s IPv6 multicast group to iface %d (%d)",
			net_sprint_ipv6_addr(addr),
			net_if_get_by_iface(iface),
			ret);
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

	ret = net_ipv4_igmp_join(iface, addr, NULL);
	if (ret < 0) {
		NET_DBG("Cannot add %s multicast address to iface %d", "IPv4",
			net_if_get_by_iface(iface));
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

	ARRAY_FOR_EACH(fds, j) {
		fds[j].fd = -1;
	}

#if defined(CONFIG_NET_IPV6)
	{
		static struct sockaddr_in6 local_addr;

		setup_ipv6_addr(&local_addr);

		ipv6 = get_socket(AF_INET6);

		ret = bind_ctx(ipv6, (struct sockaddr *)&local_addr,
			       sizeof(local_addr));
		if (ret < 0) {
			zsock_close(ipv6);
			goto ipv6_out;
		}

		ret = -ENOENT;

		ARRAY_FOR_EACH(fds, j) {
			if (fds[j].fd == ipv6) {
				ret = 0;
				break;
			}

			if (fds[j].fd < 0) {
				fds[j].fd = ipv6;
				fds[j].events = ZSOCK_POLLIN;
				ret = 0;
				break;
			}
		}

		if (ret < 0) {
			NET_DBG("Cannot set %s to socket (%d)", "polling", ret);
			zsock_close(ipv6);
			goto ipv6_out;
		}

		ret = net_socket_service_register(&svc_llmnr, fds, ARRAY_SIZE(fds), NULL);
		if (ret < 0) {
			NET_DBG("Cannot register %s %s socket service (%d)",
				"IPv6", "LLMNR", ret);
			zsock_close(ipv6);
		} else {
			ok++;
		}
	}
ipv6_out:
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	{
		setup_ipv4_addr(&local_addr4);

		ipv4 = get_socket(AF_INET);

		ret = bind_ctx(ipv4, (struct sockaddr *)&local_addr4,
			       sizeof(local_addr4));
		if (ret < 0) {
			zsock_close(ipv4);
			goto ipv4_out;
		}

		ret = -ENOENT;

		ARRAY_FOR_EACH(fds, j) {
			if (fds[j].fd == ipv4) {
				ret = 0;
				break;
			}

			if (fds[j].fd < 0) {
				fds[j].fd = ipv4;
				fds[j].events = ZSOCK_POLLIN;
				ret = 0;
				break;
			}
		}

		if (ret < 0) {
			NET_DBG("Cannot set %s to socket (%d)", "polling", ret);
			zsock_close(ipv4);
			goto ipv4_out;
		}

		ret = net_socket_service_register(&svc_llmnr, fds, ARRAY_SIZE(fds), NULL);
		if (ret < 0) {
			NET_DBG("Cannot register %s %s socket service (%d)",
				"IPv4", "LLMNR", ret);
			zsock_close(ipv4);
		} else {
			ok++;
		}
	}
ipv4_out:
#endif /* CONFIG_NET_IPV4 */

	if (!ok) {
		NET_WARN("Cannot start %s responder", "LLMNR");
	}

	return !ok;
}

static int llmnr_responder_init(void)
{
	net_mgmt_init_event_callback(&mgmt_cb, llmnr_iface_event_handler,
				     NET_EVENT_IF_UP);

	net_mgmt_add_event_callback(&mgmt_cb);

	return init_listener();
}

SYS_INIT(llmnr_responder_init, APPLICATION, CONFIG_LLMNR_RESPONDER_INIT_PRIO);
