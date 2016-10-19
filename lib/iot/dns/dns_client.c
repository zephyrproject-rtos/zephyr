/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iot/dns_client.h>
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

static struct nano_fifo dns_msg_fifo;
static NET_BUF_POOL(dns_msg_pool, DNS_RESOLVER_BUF_CTR,
		    DNS_RESOLVER_MAX_BUF_SIZE, &dns_msg_fifo, NULL, 0);

static struct nano_fifo dns_qname_fifo;
static NET_BUF_POOL(dns_qname_pool, DNS_RESOLVER_BUF_CTR,
		    DNS_MAX_NAME_LEN, &dns_qname_fifo, NULL, 0);

int dns_init(void)
{
	net_buf_pool_init(dns_msg_pool);
	net_buf_pool_init(dns_qname_pool);

	return 0;
}

static
int dns_write(struct net_context *ctx, struct net_buf *dns_data,
	      uint32_t timeout, uint16_t dns_id, enum dns_query_type type,
	      struct net_buf *dns_qname, struct sockaddr *dns_server);

static
int dns_read(struct net_context *ctx, struct net_buf *dns_data,
	     uint32_t timeout, uint16_t dns_id, enum dns_query_type type,
	     uint8_t *addresses, int *items, int elements,
	     uint8_t *cname, uint16_t *cname_len);

/*
 * Note about the DNS transaction identifier:
 * The transaction identifier is randomized according to:
 * http://www.cisco.com/c/en/us/about/security-center/dns-best-practices.html#3
 * Here we assume that even after the cast, dns_id = sys_rand32_get(), there is
 * enough entropy :)
 */
static
int dns_resolve(struct net_context *ctx, uint8_t *addresses, int *items,
		int elements, char *name, enum dns_query_type type,
		struct sockaddr *dns_server, uint32_t timeout)
{
	struct net_buf *dns_data = NULL;
	struct net_buf *dns_qname = NULL;
	uint16_t dns_id;
	int rc;
	int i;

	dns_id = sys_rand32_get();

	dns_data = net_buf_get_timeout(&dns_msg_fifo, 0, timeout);
	if (dns_data == NULL) {
		rc = -ENOMEM;
		goto exit_resolve;
	}

	dns_qname = net_buf_get_timeout(&dns_qname_fifo, 0, timeout);
	if (dns_qname == NULL) {
		rc = -ENOMEM;
		goto exit_resolve;
	}

	rc = dns_msg_pack_qname(&dns_qname->len, dns_qname->data,
				DNS_MAX_NAME_LEN, name);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_resolve;
	}

	i = 0;
	do {
		rc = dns_write(ctx, dns_data, timeout, dns_id, type, dns_qname,
			       dns_server);
		if (rc != 0) {
			goto exit_resolve;
		}

		rc = dns_read(ctx, dns_data, timeout, dns_id, type,
			      addresses, items, elements, dns_qname->data,
			      &dns_qname->len);
		if (rc != 0) {
			goto exit_resolve;
		}

		/* Server response includes at least one IP address */
		if (*items > 0) {
			break;
		}
	} while (++i < DNS_RESOLVER_QUERIES);

	rc = 0;
	if (*items <= 0) {
		rc = -EINVAL;
	}

exit_resolve:
	/* dns_data may be NULL, however net_nbuf_unref supports that */
	net_nbuf_unref(dns_data);
	net_nbuf_unref(dns_qname);

	return rc;
}

int dns4_resolve(struct net_context *ctx, struct in_addr *addresses,
		 int *items, int elements, char *name,
		 struct sockaddr *dns_server, uint32_t timeout)
{
	return dns_resolve(ctx, (uint8_t *)addresses, items, elements,
			   name, DNS_QUERY_TYPE_A, dns_server, timeout);
}

int dns6_resolve(struct net_context *ctx, struct in6_addr *addresses,
		 int *items, int elements, char *name,
		 struct sockaddr *dns_server, uint32_t timeout)
{
	return dns_resolve(ctx, (uint8_t *)addresses, items, elements,
			   name, DNS_QUERY_TYPE_AAAA, dns_server, timeout);
}

static
int dns_write(struct net_context *ctx, struct net_buf *dns_data,
	      uint32_t timeout, uint16_t dns_id, enum dns_query_type type,
	      struct net_buf *dns_qname, struct sockaddr *dns_server)
{
	struct net_buf *tx;

	int server_addr_len;
	int rc;

	rc = dns_msg_pack_query(dns_data->data, &dns_data->len, dns_data->size,
				dns_qname->data, dns_qname->len, dns_id,
				(enum dns_rr_type)type);
	if (rc != 0) {
		rc = -EINVAL;
		goto exit_write;
	}

	tx = net_nbuf_get_tx(ctx);
	if (tx == NULL) {
		rc = -ENOMEM;
		goto exit_write;
	}

	rc = net_nbuf_append(tx, dns_data->len, dns_data->data);
	if (rc != true) {
		rc = -ENOMEM;
		goto exit_write;
	}

	if (dns_server->family == AF_INET) {
		server_addr_len = sizeof(struct sockaddr_in);
	} else {
		server_addr_len = sizeof(struct sockaddr_in6);
	}

	/* tx and dns_data buffers will be dereferenced after this call */
	rc = net_context_sendto(tx, dns_server, server_addr_len, NULL,
				timeout, NULL, NULL);
	if (rc != 0) {
		rc = -EIO;
		goto exit_write;
	}

	rc = 0;

exit_write:
	return rc;
}

static
void cb_recv(struct net_context *context, struct net_buf *buf, int status,
		void *user_data)
{
	struct net_buf **data;

	ARG_UNUSED(context);

	if (status != 0) {
		return;
	}

	data = (struct net_buf **)user_data;
	*data = buf;
}

static
int dns_recv(struct net_context *ctx, struct net_buf **buf, uint32_t timeout)
{
	int rc;

	rc = net_context_recv(ctx, cb_recv, timeout, buf);
	if (rc != 0) {
		return -EIO;
	}

	return 0;
}

static
int nbuf_copy(struct net_buf *dst, struct net_buf *src, int offset,
	      int len);

static
int dns_read(struct net_context *ctx, struct net_buf *dns_data,
	     uint32_t timeout, uint16_t dns_id, enum dns_query_type type,
	     uint8_t *addresses, int *items, int elements,
	     uint8_t *cname, uint16_t *cname_len)
{
	struct net_buf *rx = NULL;
	/* helper struct to track the dns msg received from the server */
	struct dns_msg_t dns_msg;
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

	if (elements <= 0) {
		rc = -EINVAL;
		goto exit_error;
	}

	rc = dns_recv(ctx, &rx, timeout);
	if (rc != 0) {
		rc = -EIO;
		goto exit_error;
	}

	data_len = min(net_nbuf_appdatalen(rx), DNS_RESOLVER_MAX_BUF_SIZE);
	offset = net_buf_frags_len(rx) - data_len;

	if (nbuf_copy(dns_data, rx, offset, data_len) != 0) {
		rc = -ENOMEM;
		goto exit_error;
	}

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

	if (type == DNS_QUERY_TYPE_A) {
		address_size = DNS_IPV4_LEN;
	} else {
		address_size = DNS_IPV6_LEN;
	}

	/* while loop to traverse the response */
	answer_ptr = DNS_QUERY_POS;
	*items = 0;
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
			dst = (uint8_t *)addresses + *items * address_size;
			memcpy(dst, src, address_size);

			*items += 1;
			if (*items >= elements) {
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
	if (*items == 0 && dns_msg.response_type == DNS_RESPONSE_CNAME_NO_IP) {
		src = dns_msg.msg + dns_msg.response_position;
		*cname_len = dns_msg.response_length;

		memcpy(cname, src, *cname_len);
	}

exit_ok:
	rc = 0;

exit_error:
	net_nbuf_unref(rx);
	return rc;
}

static
int nbuf_copy(struct net_buf *dst, struct net_buf *src, int offset,
	      int len)
{
	int copied;
	int to_copy;

	/* find the right fragment to start copying from */
	while (src && offset >= src->len) {
		offset -= src->len;
		src = src->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	copied = 0;
	while (src && len > 0) {
		to_copy = min(len, src->len - offset);
		memcpy(dst->data + copied, src->data + offset, to_copy);

		copied += to_copy;
		len -= to_copy;
		src = src->frags;
		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	if (len > 0) {
		return -ENOMEM;
	}

	return 0;
}
