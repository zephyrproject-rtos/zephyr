/** @file
 * @brief DNS resolve API
 *
 * An API for applications to do DNS query.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_DNS_RESOLVE)
#define SYS_LOG_DOMAIN "dns/resolve"
#define NET_LOG_ENABLED 1
#endif

#include <zephyr/types.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/dns_resolve.h>
#include "dns_pack.h"

static int dns_write(struct dns_resolve_context *ctx,
		     int server_idx,
		     int query_idx,
		     struct net_buf *dns_data,
		     struct net_buf *dns_qname);

#define DNS_BUF_TIMEOUT 500 /* ms */

/* RFC 1035, 3.1. Name space definitions
 * To simplify implementations, the total length of a domain name (i.e.,
 * label octets and label length octets) is restricted to 255 octets or
 * less.
 */
#define DNS_MAX_NAME_LEN	255

#define DNS_QUERY_MAX_SIZE	(DNS_MSG_HEADER_SIZE + DNS_MAX_NAME_LEN + \
				 DNS_QTYPE_LEN + DNS_QCLASS_LEN)

/* This value is recommended by RFC 1035 */
#define DNS_RESOLVER_MAX_BUF_SIZE	512
#define DNS_RESOLVER_MIN_BUF	1
#define DNS_RESOLVER_BUF_CTR	(DNS_RESOLVER_MIN_BUF + \
				 CONFIG_DNS_RESOLVER_ADDITIONAL_BUF_CTR)

/* Compressed RR uses a pointer to another RR. So, min size is 12 bytes without
 * considering RR payload.
 * See https://tools.ietf.org/html/rfc1035#section-4.1.4
 */
#define DNS_ANSWER_PTR_LEN	12

/* See dns_unpack_answer, and also see:
 * https://tools.ietf.org/html/rfc1035#section-4.1.2
 */
#define DNS_QUERY_POS		0x0c

#define DNS_IPV4_LEN		sizeof(struct in_addr)
#define DNS_IPV6_LEN		sizeof(struct in6_addr)

NET_BUF_POOL_DEFINE(dns_msg_pool, DNS_RESOLVER_BUF_CTR,
		    DNS_RESOLVER_MAX_BUF_SIZE, 0, NULL);

NET_BUF_POOL_DEFINE(dns_qname_pool, DNS_RESOLVER_BUF_CTR, DNS_MAX_NAME_LEN,
		    0, NULL);

static struct dns_resolve_context dns_default_ctx;

int dns_resolve_init(struct dns_resolve_context *ctx, const char *servers[])
{
#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 local_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
	};
#endif
#if defined(CONFIG_NET_IPV4)
	struct sockaddr_in local_addr4 = {
		.sin_family = AF_INET,
		.sin_port = 0,
	};
#endif
	struct sockaddr *local_addr = NULL;
	socklen_t addr_len = 0;
	int i = 0, idx = 0;
	u16_t port = 0;
	int ret, count;

	if (!ctx) {
		return -ENOENT;
	}

	if (!servers || !*servers) {
		return -ENOENT;
	}

	if (ctx->is_used) {
		return -ENOTEMPTY;
	}

	memset(ctx, 0, sizeof(*ctx));

	for (i = 0; i < CONFIG_DNS_RESOLVER_MAX_SERVERS && servers[i]; i++) {
		int j;
		char *ptr;

		if (*servers[i] == '[') {
#if defined(CONFIG_NET_IPV6)
			/* IPv6 address with port number */
			struct in6_addr *addr;
			char server[INET6_ADDRSTRLEN + 1];
			int end;

			ptr = strstr(servers[i], "]:");
			if (!ptr) {
				continue;
			}

			end = min(INET6_ADDRSTRLEN, ptr - (servers[i] + 1));
			memcpy(server, servers[i] + 1, end);
			server[end] = '\0';

			addr = &net_sin6(&ctx->servers[idx].dns_server)->
				sin6_addr;

			ret = net_addr_pton(AF_INET6, server, addr);
			if (ret < 0 && ret != -EINVAL) {
				return ret;
			}

			if (ret == -EINVAL) {
				continue;
			}

			port = strtol(ptr + 2, NULL, 10);

			net_sin6(&ctx->servers[idx].dns_server)->sin6_port =
				htons(port);

			NET_DBG("[%d] IPv6 server %s port %d", idx, server,
				port);

			ctx->servers[idx++].dns_server.family = AF_INET6;
#endif /* CONFIG_NET_IPV6 */

			continue;
		}

		count = j = 0;
		while (servers[i][j]) {
			if (servers[i][j] == ':') {
				count++;
			}

			j++;
		}

		if (count == 1) {
#if defined(CONFIG_NET_IPV4)
			/* IPv4 address with port number */
			char server[NET_IPV4_ADDR_LEN + 1];
			struct in_addr *addr;
			int end;

			ptr = strstr(servers[i], ":");
			if (!ptr) {
				continue;
			}

			end = min(NET_IPV4_ADDR_LEN, ptr - servers[i]);
			memcpy(server, servers[i], end);
			server[end] = '\0';

			addr = &net_sin(&ctx->servers[idx].dns_server)->
				sin_addr;
			ret = net_addr_pton(AF_INET, server, addr);
			if (ret < 0 && ret != -EINVAL) {
				NET_ERR("Cannot set DNS server %s", server);
				return ret;
			}

			if (ret == -EINVAL) {
				continue;
			}

			port = strtol(ptr + 1, NULL, 10);

			net_sin(&ctx->servers[idx].dns_server)->sin_port =
				htons(port);

			NET_DBG("[%d] IPv4 server %s port %d", idx, server,
				port);

			ctx->servers[idx++].dns_server.family = AF_INET;
#endif /* CONFIG_NET_IPV4 */

			continue;
		}

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
		/* First try IPv4 address */
		ret = net_addr_pton(AF_INET, servers[i],
				    &net_sin(&ctx->servers[idx].dns_server)->
				    sin_addr);
		if (ret < 0 && ret != -EINVAL) {
			return ret;
		}

		if (!ret) {
			net_sin(&ctx->servers[idx].dns_server)->sin_port =
				htons(53);

			NET_DBG("[%d] IPv4 server %s port %d", idx, servers[i],
				53);

			ctx->servers[idx++].dns_server.family = AF_INET;

		} else if (ret == -EINVAL) {
			/* Then the address must be IPv6 based */
			ret = net_addr_pton(AF_INET6, servers[i],
				&net_sin6(&ctx->servers[idx].dns_server)->
					    sin6_addr);
			if (ret < 0 && ret != -EINVAL) {
				return ret;
			}

			if (ret == -EINVAL) {
				continue;
			}

			net_sin6(&ctx->servers[idx].dns_server)->sin6_port =
				htons(53);

			NET_DBG("[%d] IPv6 server %s port %d", idx, servers[i],
				53);

			ctx->servers[idx++].dns_server.family = AF_INET6;
		}
#endif

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
		ret = net_addr_pton(AF_INET, servers[i],
				    &net_sin(&ctx->servers[idx].dns_server)->
				    sin_addr);
		if (ret < 0 && ret != -EINVAL) {
			return ret;
		}

		if (ret == -EINVAL) {
			continue;
		}

		NET_DBG("[%d] IPv4 server %s port %d", idx, servers[i], 53);

		net_sin(&ctx->servers[idx].dns_server)->sin_port = htons(53);
		ctx->servers[idx++].dns_server.family = AF_INET;
#endif /* IPv4 && !IPv6 */

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
		ret = net_addr_pton(AF_INET6, servers[i],
			  &net_sin6(&ctx->servers[idx].dns_server)->sin6_addr);
		if (ret < 0 && ret != -EINVAL) {
			return ret;
		}

		if (ret == -EINVAL) {
			continue;
		}

		NET_DBG("[%d] IPv6 server %s port %d", idx, servers[i], 53);

		net_sin6(&ctx->servers[idx].dns_server)->sin6_port = htons(53);
		ctx->servers[idx++].dns_server.family = AF_INET6;
#endif /* IPv6 && !IPv4 */
	}

	for (i = 0, count = 0; i < CONFIG_DNS_RESOLVER_MAX_SERVERS &&
		     ctx->servers[i].dns_server.family; i++) {

		if (ctx->servers[i].dns_server.family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
			local_addr = (struct sockaddr *)&local_addr6;
			addr_len = sizeof(struct sockaddr_in6);
#else
			continue;
#endif
		}

		if (ctx->servers[i].dns_server.family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			local_addr = (struct sockaddr *)&local_addr4;
			addr_len = sizeof(struct sockaddr_in);
#else
			continue;
#endif
		}

		ret = net_context_get(ctx->servers[i].dns_server.family,
				      SOCK_DGRAM, IPPROTO_UDP,
				      &ctx->servers[i].net_ctx);
		if (ret < 0) {
			NET_DBG("Cannot get net_context (%d)", ret);
			return ret;
		}

		ret = net_context_bind(ctx->servers[i].net_ctx,
				       local_addr, addr_len);
		if (ret < 0) {
			NET_DBG("Cannot bind DNS context (%d)", ret);
			return ret;
		}

		count++;
	}

	if (count == 0) {
		/* No servers defined */
		NET_DBG("No DNS servers defined.");
		return -EINVAL;
	}

	ctx->is_used = true;
	ctx->buf_timeout = DNS_BUF_TIMEOUT;

	return 0;
}

static inline int get_cb_slot(struct dns_resolve_context *ctx)
{
	int i;

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (!ctx->queries[i].cb) {
			return i;
		}
	}

	return -ENOENT;
}

static inline int get_slot_by_id(struct dns_resolve_context *ctx,
				 u16_t dns_id)
{
	int i;

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (ctx->queries[i].cb && ctx->queries[i].id == dns_id) {
			return i;
		}
	}

	return -ENOENT;
}

static int dns_read(struct dns_resolve_context *ctx,
		    struct net_pkt *pkt,
		    struct net_buf *dns_data,
		    u16_t *dns_id,
		    struct net_buf *dns_cname,
		    struct dns_addrinfo *info)
{
	/* Helper struct to track the dns msg received from the server */
	struct dns_msg_t dns_msg;
	u32_t ttl; /* RR ttl, so far it is not passed to caller */
	u8_t *src, *addr;
	int address_size;
	/* index that points to the current answer being analyzed */
	int answer_ptr;
	int data_len;
	int offset;
	int items;
	int ret;
	int server_idx, query_idx;

	data_len = min(net_pkt_appdatalen(pkt), DNS_RESOLVER_MAX_BUF_SIZE);
	offset = net_pkt_get_len(pkt) - data_len;

	/* TODO: Instead of this temporary copy, just use the net_pkt directly.
	 */
	ret = net_frag_linear_copy(dns_data, pkt->frags, offset, data_len);
	if (ret < 0) {
		ret = DNS_EAI_MEMORY;
		goto quit;
	}

	dns_msg.msg = dns_data->data;
	dns_msg.msg_size = data_len;

	/* The dns_unpack_response_header() has design flaw as it expects
	 * dns id to be given instead of returning the id to the caller.
	 * In our case we would like to get it returned instead so that we
	 * can match the DNS query that we sent. When dns_read() is called,
	 * we do not know what the DNS id is yet.
	 */
	*dns_id = dns_unpack_header_id(dns_msg.msg);

	query_idx = get_slot_by_id(ctx, *dns_id);
	if (query_idx < 0) {
		ret = DNS_EAI_SYSTEM;
		goto quit;
	}

	if (dns_header_rcode(dns_msg.msg) == DNS_HEADER_REFUSED) {
		ret = DNS_EAI_FAIL;
		goto quit;
	}

	ret = dns_unpack_response_header(&dns_msg, *dns_id);
	if (ret < 0) {
		ret = DNS_EAI_FAIL;
		goto quit;
	}

	if (dns_header_qdcount(dns_msg.msg) != 1) {
		ret = DNS_EAI_FAIL;
		goto quit;
	}

	ret = dns_unpack_response_query(&dns_msg);
	if (ret < 0) {
		ret = DNS_EAI_FAIL;
		goto quit;
	}

	if (ctx->queries[query_idx].query_type == DNS_QUERY_TYPE_A) {
		address_size = DNS_IPV4_LEN;
		addr = (u8_t *)&net_sin(&info->ai_addr)->sin_addr;
		info->ai_family = AF_INET;
		info->ai_addr.family = AF_INET;
		info->ai_addrlen = sizeof(struct sockaddr_in);
	} else if (ctx->queries[query_idx].query_type == DNS_QUERY_TYPE_AAAA) {
		address_size = DNS_IPV6_LEN;
		addr = (u8_t *)&net_sin6(&info->ai_addr)->sin6_addr;
		info->ai_family = AF_INET6;
		info->ai_addr.family = AF_INET6;
		info->ai_addrlen = sizeof(struct sockaddr_in6);
	} else {
		ret = DNS_EAI_FAMILY;
		goto quit;
	}

	/* while loop to traverse the response */
	answer_ptr = DNS_QUERY_POS;
	items = 0;
	server_idx = 0;
	while (server_idx < dns_header_ancount(dns_msg.msg)) {
		ret = dns_unpack_answer(&dns_msg, answer_ptr, &ttl);
		if (ret < 0) {
			ret = DNS_EAI_FAIL;
			goto quit;
		}

		switch (dns_msg.response_type) {
		case DNS_RESPONSE_IP:
			if (dns_msg.response_length < address_size) {
				/* it seems this is a malformed message */
				ret = DNS_EAI_FAIL;
				goto quit;
			}

			src = dns_msg.msg + dns_msg.response_position;

			memcpy(addr, src, address_size);

			ctx->queries[query_idx].cb(DNS_EAI_INPROGRESS, info,
					ctx->queries[query_idx].user_data);
			items++;
			break;

		case DNS_RESPONSE_CNAME_NO_IP:
			/* Instead of using the QNAME at DNS_QUERY_POS,
			 * we will use this CNAME
			 */
			answer_ptr = dns_msg.response_position;
			break;

		default:
			ret = DNS_EAI_FAIL;
			goto quit;
		}

		/* Update the answer offset to point to the next RR (answer) */
		dns_msg.answer_offset += DNS_ANSWER_PTR_LEN;
		dns_msg.answer_offset += dns_msg.response_length;

		server_idx++;
	}

	/* No IP addresses were found, so we take the last CNAME to generate
	 * another query. Number of additional queries is controlled via Kconfig
	 */
	if (items == 0) {
		if (dns_msg.response_type == DNS_RESPONSE_CNAME_NO_IP) {
			u16_t pos = dns_msg.response_position;

			ret = dns_copy_qname(dns_cname->data, &dns_cname->len,
					     dns_cname->size, &dns_msg, pos);
			if (ret < 0) {
				ret = DNS_EAI_SYSTEM;
				goto quit;
			}

			ret = DNS_EAI_AGAIN;
			goto finished;
		}
	}

	if (items == 0) {
		ret = DNS_EAI_NODATA;
	} else {
		ret = DNS_EAI_ALLDONE;
	}

	/* Marks the end of the results */
	ctx->queries[query_idx].cb(ret, NULL,
				   ctx->queries[query_idx].user_data);

	if (k_delayed_work_remaining_get(&ctx->queries[query_idx].timer) > 0) {
		k_delayed_work_cancel(&ctx->queries[query_idx].timer);
	}

	ctx->queries[query_idx].cb = NULL;

	net_pkt_unref(pkt);

	return 0;

finished:
	dns_resolve_cancel(ctx, *dns_id);

quit:
	net_pkt_unref(pkt);

	return ret;
}

static void cb_recv(struct net_context *net_ctx,
		    struct net_pkt *pkt,
		    int status,
		    void *user_data)
{
	struct dns_resolve_context *ctx = user_data;
	struct dns_addrinfo info = { 0 };
	struct net_buf *dns_cname = NULL;
	struct net_buf *dns_data = NULL;
	u16_t dns_id = 0;
	int ret, i;

	ARG_UNUSED(net_ctx);

	if (status) {
		ret = DNS_EAI_SYSTEM;
		goto quit;
	}

	dns_data = net_buf_alloc(&dns_msg_pool, ctx->buf_timeout);
	if (!dns_data) {
		ret = DNS_EAI_MEMORY;
		goto quit;
	}

	dns_cname = net_buf_alloc(&dns_qname_pool, ctx->buf_timeout);
	if (!dns_cname) {
		ret = DNS_EAI_MEMORY;
		goto quit;
	}

	ret = dns_read(ctx, pkt, dns_data, &dns_id, dns_cname, &info);
	if (!ret) {
		/* We called the callback already in dns_read() if there
		 * was no errors.
		 */
		goto free_buf;
	}

	/* Query again if we got CNAME */
	if (ret == DNS_EAI_AGAIN) {
		int failure = 0;
		int j;

		i = get_slot_by_id(ctx, dns_id);
		if (i < 0) {
			goto free_buf;
		}

		for (j = 0; j < CONFIG_DNS_RESOLVER_MAX_SERVERS; j++) {
			if (!ctx->servers[j].net_ctx) {
				continue;
			}

			ret = dns_write(ctx, j, i, dns_data, dns_cname);
			if (ret < 0) {
				failure++;
			}
		}

		if (failure) {
			NET_DBG("DNS cname query failed %d times", failure);

			if (failure == j) {
				ret = DNS_EAI_SYSTEM;
				goto quit;
			}
		}

		goto free_buf;
	}

quit:
	i = get_slot_by_id(ctx, dns_id);
	if (i < 0) {
		goto free_buf;
	}

	if (k_delayed_work_remaining_get(&ctx->queries[i].timer) > 0) {
		k_delayed_work_cancel(&ctx->queries[i].timer);
	}

	ctx->queries[i].cb(ret, &info, ctx->queries[i].user_data);
	ctx->queries[i].cb = NULL;

free_buf:
	if (dns_data) {
		net_buf_unref(dns_data);
	}

	if (dns_cname) {
		net_buf_unref(dns_cname);
	}
}

static int dns_write(struct dns_resolve_context *ctx,
		     int server_idx,
		     int query_idx,
		     struct net_buf *dns_data,
		     struct net_buf *dns_qname)
{
	enum dns_query_type query_type;
	struct net_context *net_ctx;
	struct sockaddr *server;
	struct net_pkt *pkt;
	int server_addr_len;
	u16_t dns_id;
	int ret;

	net_ctx = ctx->servers[server_idx].net_ctx;
	server = &ctx->servers[server_idx].dns_server;
	dns_id = ctx->queries[query_idx].id;
	query_type = ctx->queries[query_idx].query_type;

	ret = dns_msg_pack_query(dns_data->data, &dns_data->len, dns_data->size,
				 dns_qname->data, dns_qname->len, dns_id,
				 (enum dns_rr_type)query_type);
	if (ret < 0) {
		ret = -EINVAL;
		goto quit;
	}

	pkt = net_pkt_get_tx(net_ctx, ctx->buf_timeout);
	if (!pkt) {
		ret = -ENOMEM;
		goto quit;
	}

	ret = net_pkt_append_all(pkt, dns_data->len, dns_data->data,
			      ctx->buf_timeout);
	if (ret < 0) {
		ret = -ENOMEM;
		goto quit;
	}

	ret = net_context_recv(net_ctx, cb_recv, K_NO_WAIT, ctx);
	if (ret < 0) {
		NET_DBG("Couldn't receive from socket (%d)", ret);
		net_pkt_unref(pkt);
		goto quit;
	}

	if (server->family == AF_INET) {
		server_addr_len = sizeof(struct sockaddr_in);
	} else {
		server_addr_len = sizeof(struct sockaddr_in6);
	}

	ret = net_context_sendto(pkt, server, server_addr_len, NULL,
				 K_NO_WAIT, NULL, NULL);
	if (ret < 0) {
		NET_DBG("Cannot send query (%d)", ret);
		net_pkt_unref(pkt);
		goto quit;
	}

	ret = k_delayed_work_submit(&ctx->queries[query_idx].timer,
				    ctx->queries[query_idx].timeout);
	if (ret < 0) {
		NET_DBG("[%u] cannot submit work to server idx %d for id %u "
			"timeout %u ret %d",
			query_idx, server_idx, dns_id,
			ctx->queries[query_idx].timeout, ret);
		goto quit;
	} else {
		NET_DBG("[%u] submitting work to server idx %d for id %u "
			"timeout %u",
			query_idx, server_idx, dns_id,
			ctx->queries[query_idx].timeout);
	}

	ret = 0;

quit:
	return ret;
}

int dns_resolve_cancel(struct dns_resolve_context *ctx, u16_t dns_id)
{
	int i;

	i = get_slot_by_id(ctx, dns_id);
	if (i < 0) {
		return -ENOENT;
	}

	NET_DBG("Cancelling DNS req %u", dns_id);

	if (k_delayed_work_remaining_get(&ctx->queries[i].timer) > 0) {
		k_delayed_work_cancel(&ctx->queries[i].timer);
	}

	ctx->queries[i].cb(DNS_EAI_CANCELED, NULL, ctx->queries[i].user_data);
	ctx->queries[i].cb = NULL;

	return 0;
}

static void query_timeout(struct k_work *work)
{
	struct dns_pending_query *pending_query =
		CONTAINER_OF(work, struct dns_pending_query, timer);

	NET_DBG("Query timeout DNS req %u", pending_query->id);

	dns_resolve_cancel(pending_query->ctx, pending_query->id);
}

int dns_resolve_name(struct dns_resolve_context *ctx,
		     const char *query,
		     enum dns_query_type type,
		     u16_t *dns_id,
		     dns_resolve_cb_t cb,
		     void *user_data,
		     s32_t timeout)
{
	struct net_buf *dns_data;
	struct net_buf *dns_qname = NULL;
	int ret, i, j = 0;
	int failure = 0;

	if (!ctx || !ctx->is_used || !query || !cb) {
		return -EINVAL;
	}

	/* Timeout cannot be 0 as we cannot resolve name that fast.
	 */
	if (timeout == K_NO_WAIT) {
		return -EINVAL;
	}

	i = get_cb_slot(ctx);
	if (i < 0) {
		return -EAGAIN;
	}

	ctx->queries[i].cb = cb;
	ctx->queries[i].timeout = timeout;
	ctx->queries[i].query = query;
	ctx->queries[i].query_type = type;
	ctx->queries[i].user_data = user_data;
	ctx->queries[i].ctx = ctx;

	k_delayed_work_init(&ctx->queries[i].timer, query_timeout);

	dns_data = net_buf_alloc(&dns_msg_pool, ctx->buf_timeout);
	if (!dns_data) {
		ret = -ENOMEM;
		goto quit;
	}

	dns_qname = net_buf_alloc(&dns_qname_pool, ctx->buf_timeout);
	if (!dns_qname) {
		ret = -ENOMEM;
		goto quit;
	}

	ret = dns_msg_pack_qname(&dns_qname->len, dns_qname->data,
				DNS_MAX_NAME_LEN, ctx->queries[i].query);
	if (ret < 0) {
		goto quit;
	}

	ctx->queries[i].id = sys_rand32_get();

	/* Do this immediately after calculating the Id so that the unit
	 * test will work properly.
	 */
	if (dns_id) {
		*dns_id = ctx->queries[i].id;

		NET_DBG("DNS id will be %u", *dns_id);
	}


	for (j = 0; j < CONFIG_DNS_RESOLVER_MAX_SERVERS; j++) {
		if (!ctx->servers[j].net_ctx) {
			continue;
		}

		ret = dns_write(ctx, j, i, dns_data, dns_qname);
		if (ret < 0) {
			failure++;
			continue;
		}

		/* Do one concurrent query only for each name resolve.
		 * TODO: Change the i (query index) to do multiple concurrent
		 *       to each server.
		 */
		break;
	}

	if (failure) {
		NET_DBG("DNS query failed %d times", failure);

		if (failure == j) {
			ret = -ENOENT;
			goto quit;
		}
	}

	ret = 0;

quit:
	if (ret < 0) {
		if (k_delayed_work_remaining_get(&ctx->queries[i].timer) > 0) {
			k_delayed_work_cancel(&ctx->queries[i].timer);
		}

		ctx->queries[i].cb = NULL;

		if (dns_id) {
			*dns_id = 0;
		}
	}

	if (dns_data) {
		net_buf_unref(dns_data);
	}

	if (dns_qname) {
		net_buf_unref(dns_qname);
	}

	return ret;
}

int dns_resolve_close(struct dns_resolve_context *ctx)
{
	int i;

	if (!ctx->is_used) {
		return -ENOENT;
	}

	for (i = 0; i < CONFIG_DNS_RESOLVER_MAX_SERVERS; i++) {
		if (ctx->servers[i].net_ctx) {
			net_context_put(ctx->servers[i].net_ctx);
		}
	}

	return 0;
}

struct dns_resolve_context *dns_resolve_get_default(void)
{
	return &dns_default_ctx;
}

void dns_init_resolver(void)
{
#if defined(CONFIG_DNS_SERVER_IP_ADDRESSES)
	static const char *dns_servers[CONFIG_DNS_RESOLVER_MAX_SERVERS + 1];
	int count = CONFIG_DNS_RESOLVER_MAX_SERVERS;
	int ret;

	if (count > 5) {
		count = 5;
	}

	switch (count) {
	case 5:
		dns_servers[4] = CONFIG_DNS_SERVER5;
		/* fallthrough */
	case 4:
		dns_servers[3] = CONFIG_DNS_SERVER4;
		/* fallthrough */
	case 3:
		dns_servers[2] = CONFIG_DNS_SERVER3;
		/* fallthrough */
	case 2:
		dns_servers[1] = CONFIG_DNS_SERVER2;
		/* fallthrough */
	case 1:
		dns_servers[0] = CONFIG_DNS_SERVER1;
		/* fallthrough */
	case 0:
		break;
	}

	dns_servers[CONFIG_DNS_RESOLVER_MAX_SERVERS] = NULL;

	ret = dns_resolve_init(dns_resolve_get_default(), dns_servers);
	if (ret < 0) {
		NET_WARN("Cannot initialize DNS resolver (%d)", ret);
	}
#endif
}
