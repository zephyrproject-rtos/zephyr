/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/dns_client.h>
#include "dns_pack.h"

#include <drivers/rand32.h>
#include <net/buf.h>
#include <net/nbuf.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

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
#define DNS_RESOLVER_QUERIES	(1 + CONFIG_DNS_RESOLVER_ADDITIONAL_QUERIES)

/* Compressed RR uses a pointer to another RR. So, min size is 12 bytes without
 * considering RR payload.
 * See https://tools.ietf.org/html/rfc1035#section-4.1.4
 */
#define DNS_ANSWER_PTR_LEN	12

/* See dns_unpack_answer, and also see:
 * https://tools.ietf.org/html/rfc1035#section-4.1.2
 */
#define DNS_QUERY_POS		0x0c

#define DNS_IPV4_LEN		4
#define DNS_IPV6_LEN		16

NET_BUF_POOL_DEFINE(dns_msg_pool, DNS_RESOLVER_BUF_CTR,
		    DNS_RESOLVER_MAX_BUF_SIZE, 0, NULL);

NET_BUF_POOL_DEFINE(dns_qname_pool, DNS_RESOLVER_BUF_CTR, DNS_MAX_NAME_LEN,
		    0, NULL);

int dns_init(struct dns_context *ctx)
{
	k_sem_init(&ctx->rx_sem, 0, UINT_MAX);

	return 0;
}

static
int dns_write(struct dns_context *ctx, struct net_buf *dns_data,
	      uint16_t dns_id, struct net_buf *dns_qname);

static
int dns_read(struct dns_context *ctx, struct net_buf *dns_data, uint16_t dns_id,
	     struct net_buf *cname);

/* net_context_recv callback */
static
void cb_recv(struct net_context *net_ctx, struct net_buf *buf, int status,
	     void *data);

/*
 * Note about the DNS transaction identifier:
 * The transaction identifier is randomized according to:
 * http://www.cisco.com/c/en/us/about/security-center/dns-best-practices.html#3
 * Here we assume that even after the cast, dns_id = sys_rand32_get(), there is
 * enough entropy :)
 */
int dns_resolve(struct dns_context *ctx)
{
	struct net_buf *dns_data = NULL;
	struct net_buf *dns_qname = NULL;
	uint16_t dns_id;
	int rc;
	int i;

	k_sem_reset(&ctx->rx_sem);

	dns_id = sys_rand32_get();

	net_context_recv(ctx->net_ctx, cb_recv, K_NO_WAIT, ctx);

	dns_data = net_buf_alloc(&dns_msg_pool, ctx->timeout);
	if (dns_data == NULL) {
		rc = -ENOMEM;
		goto exit_resolve;
	}

	dns_qname = net_buf_alloc(&dns_qname_pool, ctx->timeout);
	if (dns_qname == NULL) {
		rc = -ENOMEM;
		goto exit_resolve;
	}

	rc = dns_msg_pack_qname(&dns_qname->len, dns_qname->data,
				DNS_MAX_NAME_LEN, ctx->name);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_resolve;
	}

	i = 0;
	do {
		rc = dns_write(ctx, dns_data, dns_id, dns_qname);
		if (rc != 0) {
			goto exit_resolve;
		}

		rc = dns_read(ctx, dns_data, dns_id, dns_qname);
		if (rc != 0) {
			goto exit_resolve;
		}

		/* Server response includes at least one IP address */
		if (ctx->items > 0) {
			break;
		}
	} while (++i < DNS_RESOLVER_QUERIES);

	if (ctx->items > 0) {
		rc = 0;
	} else {
		rc = -EINVAL;
	}

exit_resolve:
	/* dns_data may be NULL, however net_nbuf_unref supports that */
	net_nbuf_unref(dns_data);
	net_nbuf_unref(dns_qname);

	/* uninstall the callback */
	net_context_recv(ctx->net_ctx, NULL, 0, NULL);

	return rc;
}

static
int dns_write(struct dns_context *ctx, struct net_buf *dns_data,
	      uint16_t dns_id, struct net_buf *dns_qname)
{
	struct net_buf *tx;
	int server_addr_len;
	int rc;

	rc = dns_msg_pack_query(dns_data->data, &dns_data->len, dns_data->size,
				dns_qname->data, dns_qname->len, dns_id,
				(enum dns_rr_type)ctx->query_type);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_write;
	}

	tx = net_nbuf_get_tx(ctx->net_ctx, K_FOREVER);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_write;
	}

	rc = net_nbuf_append(tx, dns_data->len, dns_data->data, K_FOREVER);
	if (rc != true) {
		rc = -ENOMEM;
		goto exit_write;
	}

	if (ctx->dns_server->family == AF_INET) {
		server_addr_len = sizeof(struct sockaddr_in);
	} else {
		server_addr_len = sizeof(struct sockaddr_in6);
	}

	/* tx and dns_data buffers will be dereferenced after this call */
	rc = net_context_sendto(tx, ctx->dns_server, server_addr_len, NULL,
				ctx->timeout, NULL, NULL);
	if (rc != 0) {
		rc = -EIO;
		goto exit_write;
	}

	rc = 0;

exit_write:
	return rc;
}

static
void cb_recv(struct net_context *net_ctx, struct net_buf *buf, int status,
	     void *data)
{
	struct dns_context *ctx = (struct dns_context *)data;

	ARG_UNUSED(net_ctx);

	if (status != 0) {
		return;
	}

	ctx->rx_buf = buf;
	k_sem_give(&ctx->rx_sem);
}

static
int dns_read(struct dns_context *ctx, struct net_buf *dns_data, uint16_t dns_id,
	     struct net_buf *cname)
{
	/* helper struct to track the dns msg received from the server */
	struct dns_msg_t dns_msg;
	uint8_t *addresses;
	/* RR ttl, so far it is not passed to caller */
	uint32_t ttl;
	uint8_t *src;
	uint8_t *dst;
	int address_size;
	/* index that points to the current answer being analyzed */
	int answer_ptr;
	int data_len;
	int offset;
	int rc;
	int i;

	/* The cast is applied on address.ipv4, however we can also apply it on
	 * address.ipv6 and we will get the same result.
	 */
	addresses = (uint8_t *)ctx->address.ipv4;

	if (ctx->elements <= 0) {
		rc = -EINVAL;
		goto exit_error;
	}

	ctx->rx_buf = NULL;

	/* Block until timeout or data is received, see the 'cb_recv' routine */
	k_sem_take(&ctx->rx_sem, ctx->timeout);

	/* If data is received, rx_buf is set inside 'cb_recv'. Otherwise,
	 * k_sem_take will expire while ctx->rx_buf is still NULL
	 */
	if (!ctx->rx_buf) {
		rc = -EIO;
		goto exit_error;
	}

	data_len = min(net_nbuf_appdatalen(ctx->rx_buf),
		       DNS_RESOLVER_MAX_BUF_SIZE);
	offset = net_buf_frags_len(ctx->rx_buf) - data_len;

	rc = net_nbuf_linear_copy(dns_data, ctx->rx_buf, offset, data_len);
	if (rc != 0) {
		rc = -ENOMEM;
		goto exit_error;
	}

	k_sem_give(&ctx->rx_sem);

	dns_msg.msg = dns_data->data;
	dns_msg.msg_size = data_len;

	rc = dns_unpack_response_header(&dns_msg, dns_id);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_error;
	}

	if (dns_header_qdcount(dns_msg.msg) != 1) {
		rc = -EINVAL;
		goto exit_error;
	}

	rc = dns_unpack_response_query(&dns_msg);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_error;
	}

	if (ctx->query_type == DNS_QUERY_TYPE_A) {
		address_size = DNS_IPV4_LEN;
	} else {
		address_size = DNS_IPV6_LEN;
	}

	/* while loop to traverse the response */
	answer_ptr = DNS_QUERY_POS;
	ctx->items = 0;
	i = 0;
	while (i < dns_header_ancount(dns_msg.msg)) {
		rc = dns_unpack_answer(&dns_msg, answer_ptr, &ttl);
		if (rc != 0) {
			rc = -EINVAL;
			goto exit_error;
		}

		switch (dns_msg.response_type) {
		case DNS_RESPONSE_IP:
			if (dns_msg.response_length < address_size) {
				/* it seems this is a malformed message */
				rc = -EINVAL;
				goto exit_error;
			}

			src = dns_msg.msg + dns_msg.response_position;
			dst = addresses + ctx->items * address_size;
			memcpy(dst, src, address_size);

			ctx->items += 1;
			if (ctx->items >= ctx->elements) {
				/* elements is always >= 1, so it is assumed
				 * that at least one address was returned.
				 */
				goto exit_ok;
			}

			break;

		case DNS_RESPONSE_CNAME_NO_IP:
			/* Instead of using the QNAME at DNS_QUERY_POS,
			 * we will use this CNAME
			 */
			answer_ptr = dns_msg.response_position;
			break;

		default:
			rc = -EINVAL;
			goto exit_error;
		}

		/* Update the answer offset to point to the next RR (answer) */
		dns_msg.answer_offset += DNS_ANSWER_PTR_LEN;
		dns_msg.answer_offset += dns_msg.response_length;

		++i;
	}

	/* No IP addresses were found, so we take the last CNAME to generate
	 * another query. Number of additional queries is controlled via Kconfig
	 */
	if (ctx->items == 0) {
		if (dns_msg.response_type == DNS_RESPONSE_CNAME_NO_IP) {
			uint16_t pos = dns_msg.response_position;

			rc = dns_copy_qname(cname->data, &cname->len,
					    cname->size, &dns_msg, pos);
			if (rc != 0) {
				goto exit_error;
			}

		}
	}

exit_ok:
	rc = 0;

exit_error:
	net_nbuf_unref(ctx->rx_buf);

	return rc;
}
