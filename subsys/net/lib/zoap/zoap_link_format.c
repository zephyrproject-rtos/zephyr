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

	for (p = path; p && *p; ) {
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

	for (attr = attributes; attr && *attr; ) {
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

	r = format_attributes(attributes, buf);
	if (r < 0) {
		return r;
	}

	return r;
}

static bool match_path_uri(const char * const *path,
			   const char *uri, u16_t len)
{
	const char * const *p = NULL;
	int i, j, plen;

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

	for (i = 1; i < len; i++) {
		if (!*p) {
			return false;
		}

		if (!p) {
			p++;
			plen = *p ? strlen(*p) : 0;
			j = 0;
		}

		if (j == plen && uri[i] == '/') {
			p = NULL;
			continue;
		}

		if (uri[i] == '*' && i + 1 == len) {
			return true;
		}

		if (uri[i] != (*p)[j]) {
			return false;
		}

		j++;
	}

	return true;
}

static bool match_attributes(const char * const *attributes,
			     const struct zoap_option *query)
{
	const char * const *attr;

	/*
	 * FIXME: deal with the case when there are multiple options in a
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

	pkt = net_pkt_get_tx(context, K_FOREVER);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_pkt_get_data(context, K_FOREVER);
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
	u8_t tkl, format = 40; /* application/link-format */
	int r;

	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	/*
	 * Per RFC 6690, Section 4.1, only one (or none) query parameter may be
	 * provided, use the first if multiple.
	 */
	r = zoap_find_options(request, ZOAP_OPTION_URI_QUERY, &query, 1);
	if (r < 0) {
		return r;
	}

	num_queries = r;

	context = net_pkt_context(request->pkt);

	pkt = net_pkt_get_tx(context, K_FOREVER);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_pkt_get_data(context, K_FOREVER);
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

	/* FIXME: In mesh kind of scenarios sending bulk (multiple fragments)
	 * response to farthest node (over multiple hops) is not a good idea.
	 * Send discovery response block by block.
	 */
	while (resource++ && resource->path) {
		struct net_buf *temp;
		u8_t *str;

		if (!match_queries_resource(resource, &query, num_queries)) {
			continue;
		}

		if (!response.start) {
			temp = response.pkt->frags;
			str = net_buf_add(temp, 1);
			*str = 0xFF;
			response.start = str + 1;
		} else {
			temp = net_pkt_get_data(context, K_FOREVER);
			if (!temp) {
				net_pkt_unref(pkt);
				return -ENOMEM;
			}

			net_pkt_frag_add(pkt, temp);
		}

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
