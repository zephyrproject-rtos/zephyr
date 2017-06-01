/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <misc/byteorder.h>
#include <net/buf.h>
#include <net/net_pkt.h>

#include <misc/printk.h>

#include <net/zoap.h>
#include <net/zoap_link_format.h>

#define PKT_WAIT_TIME K_SECONDS(1)
/* CoAP End Of Options Marker */
#define COAP_MARKER	0xFF

static bool match_path_uri(const char * const *path,
			   const char *uri, u16_t len)
{
	const char * const *p = NULL;
	int i, j, k, plen;

	if (!path) {
		return false;
	}

	if (len <= 1 || uri[0] != '/') {
		return false;
	}

	p = path;
	plen = *p ? strlen(*p) : 0;
	j = 0;

	if (plen == 0) {
		return false;
	}

	/* Go through uri and try to find a matching path */
	for (i = 1; i < len; i++) {
		while (*p) {
			plen = strlen(*p);

			k = i;

			for (j = 0; j < plen; j++) {
				if (uri[k] == '*') {
					if ((k + 1) == len) {
						return true;
					}
				}

				if (uri[k] != (*p)[j]) {
					goto next;
				}

				k++;
			}

			if (i == (k - 1) && j == plen) {
				return true;
			}

			if (k == len && j == plen) {
				return true;
			}

		next:
			p++;
		}
	}

	/* Did we find the resource or not */
	if (i == len && !*p) {
		return false;
	}

	return true;
}

static bool match_attributes(const char * const *attributes,
			     const struct zoap_option *query)
{
	const char * const *attr;

	/* FIXME: deal with the case when there are multiple options in a
	 * query, for example: 'rt=lux temperature', if I want to list
	 * resources with resource type lux or temperature.
	 */
	for (attr = attributes; attr && *attr; attr++) {
		u16_t attr_len = strlen(*attr);

		if (query->len != attr_len) {
			continue;
		}

		if (!strncmp((char *) query->value, *attr, attr_len)) {
			return true;
		}
	}

	return false;
}

static bool match_queries_resource(const struct zoap_resource *resource,
				   const struct zoap_option *query,
				   int num_queries)
{
	struct zoap_core_metadata *meta = resource->user_data;
	const char * const *attributes = NULL;
	const int href_len = strlen("href");

	if (num_queries == 0) {
		return true;
	}

	if (meta && meta->attributes) {
		attributes = meta->attributes;
	}

	if (!attributes) {
		return false;
	}

	if (query->len > href_len + 1 &&
	    !strncmp((char *) query->value, "href", href_len)) {
		/* The stuff after 'href=' */
		const char *uri = (char *) query->value + href_len + 1;
		u16_t uri_len  = query->len - (href_len + 1);

		return match_path_uri(resource->path, uri, uri_len);
	}

	return match_attributes(attributes, query);
}

static int send_error_response(struct zoap_resource *resource,
			       struct zoap_packet *request,
			       const struct sockaddr *from)
{
	struct net_context *context;
	struct zoap_packet response;
	struct net_pkt *pkt;
	struct net_buf *frag;
	u16_t id;
	int r;

	id = zoap_header_get_id(request);

	context = net_pkt_context(request->pkt);

	pkt = net_pkt_get_tx(context, PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_pkt_get_data(context, PKT_WAIT_TIME);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&response, pkt);
	if (r < 0) {
		net_pkt_unref(pkt);
		return r;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_BAD_REQUEST);
	zoap_header_set_id(&response, id);

	r = net_context_sendto(pkt, from, sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

#if defined(CONFIG_ZOAP_WELL_KNOWN_BLOCK_WISE)

#define MAX_BLOCK_WISE_TRANSFER_SIZE 2048

enum zoap_block_size default_block_size(void)
{
	switch (CONFIG_ZOAP_WELL_KNOWN_BLOCK_WISE_SIZE) {
	case 16:
		return ZOAP_BLOCK_16;
	case 32:
		return ZOAP_BLOCK_32;
	case 64:
		return ZOAP_BLOCK_64;
	case 128:
		return ZOAP_BLOCK_128;
	case 256:
		return ZOAP_BLOCK_256;
	case 512:
		return ZOAP_BLOCK_512;
	case 1024:
		return ZOAP_BLOCK_1024;
	}

	return ZOAP_BLOCK_64;
}

static void add_to_net_buf(struct net_buf *buf, const char *str, u16_t len,
			   u16_t *remaining, size_t *offset, size_t current)
{
	u16_t pos;
	char *ptr;

	if (!*remaining) {
		return;
	}

	pos = 0;

	if (*offset < current) {
		pos = current - *offset;

		if (len >= pos) {
			len -= pos;
			*offset += pos;
		} else {
			*offset += len;
			return;
		}
	}

	if (len > *remaining) {
		len = *remaining;
	}

	ptr = net_buf_add(buf, len);
	strncpy(ptr, str + pos, len);

	*remaining -= len;
	*offset += len;
}

static int format_uri(const char * const *path, struct net_buf *buf,
		      u16_t *remaining, size_t *offset,
		      size_t current, bool *more)
{
	static const char prefix[] = "</";
	const char * const *p;

	if (!path) {
		return -EINVAL;
	}

	add_to_net_buf(buf, &prefix[0], sizeof(prefix) - 1,
		       remaining, offset, current);
	if (!*remaining) {
		*more = true;
		return 0;
	}

	for (p = path; *p; ) {
		u16_t path_len = strlen(*p);

		add_to_net_buf(buf, *p, path_len,
			       remaining, offset, current);
		if (!*remaining) {
			*more = true;
			return 0;
		}

		p++;
		if (!*p) {
			continue;
		}

		add_to_net_buf(buf, "/", 1,
			       remaining, offset, current);
		if (!*remaining) {
			*more = true;
			return 0;
		}

	}

	add_to_net_buf(buf, ">", 1, remaining, offset, current);
	*more = false;

	return 0;
}

static int format_attributes(const char * const *attributes,
			     struct net_buf *buf,
			     u16_t *remaining, size_t *offset,
			     size_t current, bool *more)
{
	const char * const *attr;

	if (!attributes) {
		goto terminator;
	}

	for (attr = attributes; *attr; ) {
		int attr_len = strlen(*attr);

		add_to_net_buf(buf, *attr, attr_len,
			       remaining, offset, current);
		if (!*remaining) {
			*more = true;
			return 0;
		}

		attr++;
		if (!*attr) {
			continue;
		}

		add_to_net_buf(buf, ";", 1,
			       remaining, offset, current);
		if (!*remaining) {
			*more = true;
			return 0;
		}
	}

terminator:
	add_to_net_buf(buf, ";", 1, remaining, offset, current);
	*more = false;

	return 0;
}

static int format_resource(const struct zoap_resource *resource,
			   struct net_buf *buf,
			   u16_t *remaining, size_t *offset,
			   size_t current, bool *more)
{
	struct zoap_core_metadata *meta = resource->user_data;
	const char * const *attributes = NULL;
	int r;

	r = format_uri(resource->path, buf, remaining, offset, current, more);
	if (r < 0) {
		return r;
	}

	if (!*remaining) {
		*more = true;
		return 0;
	}

	if (meta && meta->attributes) {
		attributes = meta->attributes;
	}

	return format_attributes(attributes, buf, remaining, offset, current,
				 more);
}

int _zoap_well_known_core_get(struct zoap_resource *resource,
			      struct zoap_packet *request,
			      const struct sockaddr *from)
{
	/* FIXME: Add support for concurrent connections at same time,
	 * Maintain separate Context for each client (from addr) through slist
	 * and match it with 'from' address.
	 */
	static struct zoap_block_context ctx;
	struct net_context *context;
	struct zoap_packet response;
	struct zoap_option query;
	struct net_pkt *pkt;
	struct net_buf *frag;
	unsigned int num_queries;
	size_t offset;
	const u8_t *token;
	bool more;
	u16_t remaining;
	u16_t id;
	u8_t tkl;
	u8_t format = 40; /* application/link-format */
	u8_t *str;
	int r;

	if (ctx.total_size == 0) {
		zoap_block_transfer_init(&ctx, default_block_size(),
					 MAX_BLOCK_WISE_TRANSFER_SIZE);
	}

	r = zoap_update_from_block(request, &ctx);
	if (r < 0) {
		return -EINVAL;
	}

	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	/* Per RFC 6690, Section 4.1, only one (or none) query parameter may be
	 * provided, use the first if multiple.
	 */
	r = zoap_find_options(request, ZOAP_OPTION_URI_QUERY, &query, 1);
	if (r < 0) {
		return r;
	}

	num_queries = r;

	context = net_pkt_context(request->pkt);

	pkt = net_pkt_get_tx(context, PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_pkt_get_data(context, PKT_WAIT_TIME);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&response, pkt);
	if (r < 0) {
		goto done;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CONTENT);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	r = zoap_add_option(&response, ZOAP_OPTION_CONTENT_FORMAT,
			    &format, sizeof(format));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	offset = 0;
	more = false;
	remaining = zoap_block_size_to_bytes(ctx.block_size);

	while (resource++ && resource->path) {
		struct net_buf *temp;

		if (!match_queries_resource(resource, &query, num_queries)) {
			continue;
		}

		if (!remaining) {
			more = true;
			break;
		}

		temp = net_pkt_get_data(context, PKT_WAIT_TIME);
		if (!temp) {
			net_pkt_unref(pkt);
			return -ENOMEM;
		}

		net_pkt_frag_add(pkt, temp);

		r = format_resource(resource, temp, &remaining, &offset,
				    ctx.current, &more);
		if (r < 0) {
			goto done;
		}
	}

	if (!response.pkt->frags->frags) {
		r = -ENOENT;
		goto done;
	}

	if (!more) {
		ctx.total_size = offset;
	}

	r = zoap_add_block2_option(&response, &ctx);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	str = net_buf_add(response.pkt->frags, 1);
	*str = COAP_MARKER;
	response.start = str + 1;

	net_pkt_compact(pkt);

done:
	if (r < 0) {
		net_pkt_unref(pkt);
		return send_error_response(resource, request, from);
	}

	r = net_context_sendto(pkt, from, sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	if (!more) {
		/* So it's a last block, reset context */
		memset(&ctx, 0, sizeof(ctx));
	}

	return r;
}

#else

static int format_uri(const char * const *path, struct net_buf *buf)
{
	static const char prefix[] = "</";
	const char * const *p;
	char *str;

	if (!path) {
		return -EINVAL;
	}

	str = net_buf_add(buf, sizeof(prefix) - 1);
	strncpy(str, prefix, sizeof(prefix) - 1);

	for (p = path; *p; ) {
		u16_t path_len = strlen(*p);

		str = net_buf_add(buf, path_len);
		strncpy(str, *p, path_len);

		p++;
		if (*p) {
			str = net_buf_add(buf, 1);
			*str = '/';
		}
	}

	str = net_buf_add(buf, 1);
	*str = '>';

	return 0;
}

static int format_attributes(const char * const *attributes,
			     struct net_buf *buf)
{
	const char * const *attr;
	char *str;

	if (!attributes) {
		goto terminator;
	}

	for (attr = attributes; *attr; ) {
		int attr_len = strlen(*attr);

		str = net_buf_add(buf, attr_len);
		strncpy(str, *attr, attr_len);

		attr++;
		if (*attr) {
			str = net_buf_add(buf, 1);
			*str = ';';
		}
	}

terminator:
	str = net_buf_add(buf, 1);
	*str = ';';

	return 0;
}

static int format_resource(const struct zoap_resource *resource,
			   struct net_buf *buf)
{
	struct zoap_core_metadata *meta = resource->user_data;
	const char * const *attributes = NULL;
	int r;

	r = format_uri(resource->path, buf);
	if (r < 0) {
		return r;
	}

	if (meta && meta->attributes) {
		attributes = meta->attributes;
	}

	return format_attributes(attributes, buf);
}

int _zoap_well_known_core_get(struct zoap_resource *resource,
			      struct zoap_packet *request,
			      const struct sockaddr *from)
{
	struct net_context *context;
	struct zoap_packet response;
	struct zoap_option query;
	struct net_pkt *pkt;
	struct net_buf *frag;
	const u8_t *token;
	unsigned int num_queries;
	u16_t id;
	u8_t tkl;
	u8_t format = 40; /* application/link-format */
	u8_t *str;
	int r;

	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	/* Per RFC 6690, Section 4.1, only one (or none) query parameter may be
	 * provided, use the first if multiple.
	 */
	r = zoap_find_options(request, ZOAP_OPTION_URI_QUERY, &query, 1);
	if (r < 0) {
		return r;
	}

	num_queries = r;

	context = net_pkt_context(request->pkt);

	pkt = net_pkt_get_tx(context, PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_pkt_get_data(context, PKT_WAIT_TIME);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&response, pkt);
	if (r < 0) {
		goto done;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CONTENT);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	r = zoap_add_option(&response, ZOAP_OPTION_CONTENT_FORMAT,
			    &format, sizeof(format));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = -ENOENT;

	str = net_buf_add(response.pkt->frags, 1);
	*str = COAP_MARKER;
	response.start = str + 1;

	while (resource++ && resource->path) {
		struct net_buf *temp;

		if (!match_queries_resource(resource, &query, num_queries)) {
			continue;
		}

		temp = net_pkt_get_data(context, PKT_WAIT_TIME);
		if (!temp) {
			net_pkt_unref(pkt);
			return -ENOMEM;
		}

		net_pkt_frag_add(pkt, temp);

		r = format_resource(resource, temp);
		if (r < 0) {
			goto done;
		}
	}

	net_pkt_compact(pkt);

done:
	if (r < 0) {
		net_pkt_unref(pkt);
		return send_error_response(resource, request, from);
	}

	r = net_context_sendto(pkt, from, sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}
#endif

/* Exposing some of the APIs to ZoAP unit tests in tests/net/lib/zoap */
#if defined(CONFIG_ZOAP_TEST_API_ENABLE)
bool _zoap_match_path_uri(const char * const *path,
			  const char *uri, u16_t len)
{
	return match_path_uri(path, uri, len);
}
#endif
