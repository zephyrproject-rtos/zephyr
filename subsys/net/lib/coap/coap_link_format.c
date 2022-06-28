/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <stdlib.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>

#include <zephyr/sys/printk.h>

#include <zephyr/net/coap.h>
#include <zephyr/net/coap_link_format.h>

static inline bool append_u8(struct coap_packet *cpkt, uint8_t data)
{
	if (!cpkt) {
		return false;
	}

	if (cpkt->max_len - cpkt->offset < 1) {
		return false;
	}

	cpkt->data[cpkt->offset++] = data;

	return true;
}

static inline bool append_be16(struct coap_packet *cpkt, uint16_t data)
{
	if (!cpkt) {
		return false;
	}

	if (cpkt->max_len - cpkt->offset < 2) {
		return false;
	}

	cpkt->data[cpkt->offset++] = data >> 8;
	cpkt->data[cpkt->offset++] = (uint8_t) data;

	return true;
}

static inline bool append(struct coap_packet *cpkt, const uint8_t *data, uint16_t len)
{
	if (!cpkt || !data) {
		return false;
	}

	if (cpkt->max_len - cpkt->offset < len) {
		return false;
	}

	memcpy(cpkt->data + cpkt->offset, data, len);
	cpkt->offset += len;

	return true;
}

static bool match_path_uri(const char * const *path,
			   const char *uri, uint16_t len)
{
	const char * const *p = NULL;
	int i, j, k, plen;

	if (!path) {
		return false;
	}

	if (len <= 1U || uri[0] != '/') {
		return false;
	}

	p = path;
	plen = *p ? strlen(*p) : 0;

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
			     const struct coap_option *query)
{
	const char * const *attr;

	/* FIXME: deal with the case when there are multiple options in a
	 * query, for example: 'rt=lux temperature', if I want to list
	 * resources with resource type lux or temperature.
	 */
	for (attr = attributes; attr && *attr; attr++) {
		uint16_t attr_len = strlen(*attr);

		if (query->len != attr_len) {
			continue;
		}

		if (!strncmp((char *) query->value, *attr, attr_len)) {
			return true;
		}
	}

	return false;
}

static bool match_queries_resource(const struct coap_resource *resource,
				   const struct coap_option *query,
				   int num_queries)
{
	struct coap_core_metadata *meta = resource->user_data;
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
		uint16_t uri_len  = query->len - (href_len + 1);

		return match_path_uri(resource->path, uri, uri_len);
	}

	return match_attributes(attributes, query);
}

#if defined(CONFIG_COAP_WELL_KNOWN_BLOCK_WISE)

#define MAX_BLOCK_WISE_TRANSFER_SIZE 2048

enum coap_block_size default_block_size(void)
{
	switch (CONFIG_COAP_WELL_KNOWN_BLOCK_WISE_SIZE) {
	case 16:
		return COAP_BLOCK_16;
	case 32:
		return COAP_BLOCK_32;
	case 64:
		return COAP_BLOCK_64;
	case 128:
		return COAP_BLOCK_128;
	case 256:
		return COAP_BLOCK_256;
	case 512:
		return COAP_BLOCK_512;
	case 1024:
		return COAP_BLOCK_1024;
	}

	return COAP_BLOCK_64;
}

static bool append_to_coap_pkt(struct coap_packet *response,
			       const char *str, uint16_t len,
			       uint16_t *remaining, size_t *offset,
			       size_t current)
{
	uint16_t pos = 0U;
	bool res;

	if (!*remaining) {
		return true;
	}

	if (*offset < current) {
		pos = current - *offset;

		if (len >= pos) {
			len -= pos;
			*offset += pos;
		} else {
			*offset += len;
			return true;
		}
	}

	if (len > *remaining) {
		len = *remaining;
	}

	res = append(response, str + pos, len);

	*remaining -= len;
	*offset += len;

	return res;
}

static int format_uri(const char * const *path,
		      struct coap_packet *response,
		      uint16_t *remaining, size_t *offset,
		      size_t current, bool *more)
{
	static const char prefix[] = "</";
	const char * const *p;
	bool res;

	if (!path) {
		return -EINVAL;
	}

	res = append_to_coap_pkt(response, &prefix[0], sizeof(prefix) - 1,
				 remaining, offset, current);
	if (!res) {
		return -ENOMEM;
	}

	if (!*remaining) {
		*more = true;
		return 0;
	}

	for (p = path; *p; ) {
		uint16_t path_len = strlen(*p);

		res = append_to_coap_pkt(response, *p, path_len, remaining,
					 offset, current);
		if (!res) {
			return -ENOMEM;
		}

		if (!*remaining) {
			*more = true;
			return 0;
		}

		p++;
		if (!*p) {
			continue;
		}

		res = append_to_coap_pkt(response, "/", 1, remaining, offset,
					 current);
		if (!res) {
			return -ENOMEM;
		}

		if (!*remaining) {
			*more = true;
			return 0;
		}

	}

	res = append_to_coap_pkt(response, ">", 1, remaining, offset, current);
	if (!res) {
		return -ENOMEM;
	}

	if (!*remaining) {
		*more = true;
		return 0;
	}

	*more = false;

	return 0;
}

static int format_attributes(const char * const *attributes,
			     struct coap_packet *response,
			     uint16_t *remaining, size_t *offset,
			     size_t current, bool *more)
{
	const char * const *attr;
	bool res;

	if (!attributes) {
		*more = false;
		return 0;
	}

	for (attr = attributes; *attr; attr++) {
		int attr_len;

		res = append_to_coap_pkt(response, ";", 1,
					 remaining, offset, current);
		if (!res) {
			return -ENOMEM;
		}

		if (!*remaining) {
			*more = true;
			return 0;
		}

		attr_len = strlen(*attr);

		res = append_to_coap_pkt(response, *attr, attr_len,
					 remaining, offset, current);
		if (!res) {
			return -ENOMEM;
		}

		if (*(attr + 1) && !*remaining) {
			*more = true;
			return 0;
		}
	}

	*more = false;
	return 0;
}

static int format_resource(const struct coap_resource *resource,
			   struct coap_packet *response,
			   uint16_t *remaining, size_t *offset,
			   size_t current, bool *more)
{
	struct coap_core_metadata *meta = resource->user_data;
	const char * const *attributes = NULL;
	int r;

	r = format_uri(resource->path, response, remaining,
		       offset, current, more);
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

	return format_attributes(attributes, response, remaining, offset,
				 current, more);
}

/* coap_well_known_core_get() added Option (delta and len) with
 * out any extended options so this function will not consider Extended
 * options at the moment.
 */
int clear_more_flag(struct coap_packet *cpkt)
{
	uint16_t offset;
	uint8_t opt;
	uint8_t delta;
	uint8_t len;

	offset = cpkt->hdr_len;
	delta = 0U;

	while (1) {
		opt = cpkt->data[offset++];

		delta += ((opt & 0xF0) >> 4);
		len = (opt & 0xF);

		if (delta == COAP_OPTION_BLOCK2) {
			break;
		}

		offset += len;
	}

	/* As per RFC 7959 Sec 2.2 : NUM filed can be on 0-3 bytes.
	 * Skip NUM field to update M bit.
	 */
	if (len > 1) {
		offset = offset + len - 1;
	}

	cpkt->data[offset] = cpkt->data[offset] & 0xF7;

	return 0;
}

int coap_well_known_core_get(struct coap_resource *resource,
			      struct coap_packet *request,
			      struct coap_packet *response,
			      uint8_t *data, uint16_t len)
{
	static struct coap_block_context ctx;
	struct coap_option query;
	unsigned int num_queries;
	size_t offset;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t remaining;
	uint16_t id;
	uint8_t tkl;
	int r;
	bool more = false;

	if (!resource || !request || !response || !data || !len) {
		return -EINVAL;
	}

	if (ctx.total_size == 0) {
		/* We have to iterate through resources and it's attributes,
		 * total size is unknown, so initialize it to
		 * MAX_BLOCK_WISE_TRANSFER_SIZE and update it according to
		 * offset.
		 */
		coap_block_transfer_init(&ctx, default_block_size(),
					 MAX_BLOCK_WISE_TRANSFER_SIZE);
	}

	r = coap_update_from_block(request, &ctx);
	if (r < 0) {
		goto end;
	}

	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	/* Per RFC 6690, Section 4.1, only one (or none) query parameter may be
	 * provided, use the first if multiple.
	 */
	r = coap_find_options(request, COAP_OPTION_URI_QUERY, &query, 1);
	if (r < 0) {
		goto end;
	}

	num_queries = r;

	r = coap_packet_init(response, data, len, COAP_VERSION_1, COAP_TYPE_ACK,
			     tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		goto end;
	}

	r = coap_append_option_int(response, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_APP_LINK_FORMAT);
	if (r < 0) {
		goto end;
	}

	r = coap_append_block2_option(response, &ctx);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload_marker(response);
	if (r < 0) {
		goto end;
	}

	offset = 0;
	remaining = coap_block_size_to_bytes(ctx.block_size);

	while (resource++ && resource->path) {
		if (!remaining) {
			more = true;
			break;
		}

		if (!match_queries_resource(resource, &query, num_queries)) {
			continue;
		}

		r = format_resource(resource, response, &remaining, &offset,
				    ctx.current, &more);
		if (r < 0) {
			goto end;
		}

		if ((resource + 1) && (resource + 1)->path) {
			r = append_to_coap_pkt(response, ",", 1, &remaining,
					       &offset, ctx.current);
			if (!r) {
				goto end;
			}
		}
	}

	/* Offset is the total size now, but block2 option is already
	 * appended. So update only 'more' flag.
	 */
	if (!more) {
		ctx.total_size = offset;
		r = clear_more_flag(response);
	}

end:
	/* So it's a last block, reset context */
	if (!more) {
		(void)memset(&ctx, 0, sizeof(ctx));
	}

	return r;
}

#else

static int format_uri(const char * const *path, struct coap_packet *response)
{
	const char * const *p;
	char *prefix = "</";
	bool res;

	if (!path) {
		return -EINVAL;
	}

	res = append(response, (uint8_t *) prefix, strlen(prefix));
	if (!res) {
		return -ENOMEM;
	}

	for (p = path; *p; ) {
		res = append(response, (uint8_t *) *p, strlen(*p));
		if (!res) {
			return -ENOMEM;
		}

		p++;
		if (*p) {
			res = append_u8(response, (uint8_t) '/');
			if (!res) {
				return -ENOMEM;
			}
		}
	}

	res = append_u8(response, (uint8_t) '>');
	if (!res) {
		return -ENOMEM;
	}

	return 0;
}

static int format_attributes(const char * const *attributes,
			     struct coap_packet *response)
{
	const char * const *attr;
	bool res;

	if (!attributes) {
		return 0;
	}

	for (attr = attributes; *attr; attr++) {
		res = append_u8(response, (uint8_t) ';');
		if (!res) {
			return -ENOMEM;
		}

		res = append(response, (uint8_t *) *attr, strlen(*attr));
		if (!res) {
			return -ENOMEM;
		}
	}

	return 0;
}

static int format_resource(const struct coap_resource *resource,
			   struct coap_packet *response)
{
	struct coap_core_metadata *meta = resource->user_data;
	const char * const *attributes = NULL;
	int r;

	r = format_uri(resource->path, response);
	if (r < 0) {
		return r;
	}

	if (meta && meta->attributes) {
		attributes = meta->attributes;
	}

	return format_attributes(attributes, response);
}

int coap_well_known_core_get(struct coap_resource *resource,
			     struct coap_packet *request,
			     struct coap_packet *response,
			     uint8_t *data, uint16_t len)
{
	struct coap_option query;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint8_t tkl;
	uint8_t num_queries;
	int r;

	if (!resource || !request || !response || !data || !len) {
		return -EINVAL;
	}

	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	/* Per RFC 6690, Section 4.1, only one (or none) query parameter may be
	 * provided, use the first if multiple.
	 */
	r = coap_find_options(request, COAP_OPTION_URI_QUERY, &query, 1);
	if (r < 0) {
		return r;
	}

	num_queries = r;

	r = coap_packet_init(response, data, len, COAP_VERSION_1, COAP_TYPE_ACK,
			     tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		return r;
	}

	r = coap_append_option_int(response, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_APP_LINK_FORMAT);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_packet_append_payload_marker(response);
	if (r < 0) {
		return -EINVAL;
	}

	while (resource++ && resource->path) {
		if (!match_queries_resource(resource, &query, num_queries)) {
			continue;
		}

		r = format_resource(resource, response);
		if (r < 0) {
			return r;
		}

		if ((resource + 1) && (resource + 1)->path) {
			r = append_u8(response, (uint8_t) ',');
			if (!r) {
				return -ENOMEM;
			}
		}
	}

	return 0;
}
#endif

/* Exposing some of the APIs to CoAP unit tests in tests/net/lib/coap */
#if defined(CONFIG_COAP_TEST_API_ENABLE)
bool _coap_match_path_uri(const char * const *path,
			  const char *uri, uint16_t len)
{
	return match_path_uri(path, uri, len);
}
#endif
