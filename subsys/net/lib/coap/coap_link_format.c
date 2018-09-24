/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_COAP)
#define SYS_LOG_DOMAIN "coap"
#define NET_LOG_ENABLED 1
#endif

#include <stdlib.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <flat_buffer_util.h>

#include <misc/byteorder.h>
#include <misc/printk.h>

#include <net/coap.h>
#include <net/coap_link_format.h>

#define PKT_WAIT_TIME K_SECONDS(1)

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
			     const struct coap_option *query)
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
		u16_t uri_len  = query->len - (href_len + 1);

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

static int append_to_buf(struct fbuf_ctx *fbuf, const char *str, u16_t len,
			 u16_t *remaining, size_t *offset, size_t current)
{
	int ret;
	u16_t pos = 0;

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

	ret = fbuf_append(fbuf, (u8_t *)(str + pos), len);

	*remaining -= len;
	*offset += len;

	return ret;
}

static int format_uri(const char * const *path,
		      struct fbuf_ctx *fbuf,
		      u16_t *remaining, size_t *offset,
		      size_t current, bool *more)
{
	static const char prefix[] = "</";
	const char * const *p;
	int r;

	if (!path) {
		return -EINVAL;
	}

	r = append_to_buf(fbuf, &prefix[0], sizeof(prefix) - 1, remaining,
			  offset, current);
	if (r < 0) {
		return r;
	}

	if (!*remaining) {
		*more = true;
		return 0;
	}

	for (p = path; *p; ) {
		u16_t path_len = strlen(*p);

		r = append_to_buf(fbuf, *p, path_len, remaining, offset,
				  current);
		if (r < 0) {
			return r;
		}

		if (!*remaining) {
			*more = true;
			return 0;
		}

		p++;
		if (!*p) {
			continue;
		}

		r = append_to_buf(fbuf, "/", 1, remaining, offset, current);
		if (r < 0) {
			return r;
		}

		if (!*remaining) {
			*more = true;
			return 0;
		}

	}

	r = append_to_buf(fbuf, ">", 1, remaining, offset, current);
	if (r < 0) {
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
			     struct fbuf_ctx *fbuf,
			     u16_t *remaining, size_t *offset,
			     size_t current, bool *more)
{
	const char * const *attr;
	bool res;

	if (!attributes) {
		goto terminator;
	}

	for (attr = attributes; *attr; ) {
		int attr_len = strlen(*attr);

		res = append_to_buf(fbuf, *attr, attr_len, remaining, offset,
				    current);
		if (!res) {
			return -ENOMEM;
		}

		if (!*remaining) {
			*more = true;
			return 0;
		}

		attr++;
		if (!*attr) {
			continue;
		}

		res = append_to_buf(fbuf, ";", 1, remaining, offset, current);
		if (!res) {
			return -ENOMEM;
		}

		if (!*remaining) {
			*more = true;
			return 0;
		}
	}

terminator:
	res = append_to_buf(fbuf, ";", 1, remaining, offset, current);
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

static int format_resource(const struct coap_resource *resource,
			   struct fbuf_ctx *fbuf,
			   u16_t *remaining, size_t *offset,
			   size_t current, bool *more)
{
	struct coap_core_metadata *meta = resource->user_data;
	const char * const *attributes = NULL;
	int r;

	r = format_uri(resource->path, fbuf, remaining, offset, current,
		       more);
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

	return format_attributes(attributes, fbuf, remaining, offset,
				 current, more);
}

int clear_more_flag(struct coap_packet *cpkt)
{
	int r;
	u16_t offset;
	u8_t opt;
	u8_t delta;
	u8_t len;

	offset = cpkt->offset;
	r = fbuf_skip(&cpkt->fbuf, &offset, cpkt->hdr_len);
	if (r < 0) {
		return -EINVAL;
	}

	delta = 0;
	/* Note: coap_well_known_core_get() added Option (delta and len) witho
	 * out any extended options so parsing will not consider at the moment.
	 */
	while (1) {
		r = fbuf_read_u8(&cpkt->fbuf, &offset, &opt);
		if (r < 0) {
			return -EINVAL;
		}

		delta += ((opt & 0xF0) >> 4);
		len = (opt & 0xF);

		if (delta == COAP_OPTION_BLOCK2) {
			break;
		}

		r = fbuf_skip(&cpkt->fbuf, &offset, len);
		if (r < 0) {
			return -EINVAL;
		}
	}

	/* As per RFC 7959 Sec 2.2 : NUM filed can be on 0-3 bytes.
	 * Skip NUM field to update M bit.
	 */
	if (len > 1) {
		r = fbuf_skip(&cpkt->fbuf, &offset, len - 1);
		if (r < 0) {
			return -EINVAL;
		}
	}

	cpkt->fbuf.buf[offset] = cpkt->fbuf.buf[offset] & 0xF7;

	return 0;
}

int coap_well_known_core_get(struct coap_resource *resource,
			     struct coap_packet *request,
			     struct coap_packet *response,
			     u8_t *buf, u16_t buf_size)
{
	static struct coap_block_context ctx;
	struct coap_option query;
	unsigned int num_queries;
	size_t offset;
	u8_t token[8];
	u16_t remaining;
	u16_t id;
	u8_t tkl;
	u8_t format;
	int r;
	bool more = false;

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

	r = coap_packet_init(response, buf, buf_size, 1, COAP_TYPE_ACK,
			     tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		goto end;
	}

	format = 40; /* application/link-format */

	r = coap_packet_append_option(response, COAP_OPTION_CONTENT_FORMAT,
				      &format, sizeof(format));
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

		r = format_resource(resource, &response->fbuf, &remaining,
				    &offset, ctx.current, &more);
		if (r < 0) {
			goto end;
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

static int format_uri(const char * const *path, struct fbuf_ctx *fbuf)
{
	const char * const *p;
	int r;
	u8_t *prefix = "</";
	u8_t slash = '\\', grthan = '>';

	if (!path) {
		return -EINVAL;
	}

	r = fbuf_append(fbuf, prefix, strlen(prefix));
	if (r < 0) {
		return r;
	}

	for (p = path; *p; ) {
		r = fbuf_append(fbuf, (u8_t *)*p, strlen(*p));
		if (r < 0) {
			return r;
		}

		p++;
		if (*p) {
			r = fbuf_append(fbuf, &slash, 1);
			if (r < 0) {
				return r;
			}
		}
	}

	r = fbuf_append(fbuf, &grthan, 1);

	return r;
}

static int format_attributes(const char * const *attributes,
			     struct fbuf_ctx *fbuf)
{
	int r;
	const char * const *attr;
	u8_t semicolon = ';';

	if (!attributes) {
		goto terminator;
	}

	for (attr = attributes; *attr; ) {
		r = fbuf_append(fbuf, (u8_t *)*attr, strlen(*attr));
		if (r < 0) {
			return r;
		}

		attr++;
		if (*attr) {
			r = fbuf_append(fbuf, &semicolon, 1);
			if (r < 0) {
				return r;
			}
		}
	}

terminator:
	r = fbuf_append(fbuf, &semicolon, 1);

	return r;
}

static int format_resource(const struct coap_resource *resource,
			   struct fbuf_ctx *fbuf)
{
	struct coap_core_metadata *meta = resource->user_data;
	const char * const *attributes = NULL;
	int r;

	r = format_uri(resource->path, fbuf);
	if (r < 0) {
		return r;
	}

	if (meta && meta->attributes) {
		attributes = meta->attributes;
	}

	return format_attributes(attributes, fbuf);
}

int coap_well_known_core_get(struct coap_resource *resource,
			     struct coap_packet *request,
			     struct coap_packet *response,
			     u8_t *buf, u16_t buf_size)
{
	struct coap_option query;
	u8_t token[8];
	u16_t id;
	u8_t tkl;
	u8_t format;
	u8_t num_queries;
	int r;

	if (!resource || !request || !response) {
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

	r = coap_packet_init(response, buf, buf_size, 1, COAP_TYPE_ACK,
			     tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		return r;
	}

	format = 40; /* application/link-format */
	r = coap_packet_append_option(response, COAP_OPTION_CONTENT_FORMAT,
				      &format, sizeof(format));
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

		r = format_resource(resource, &response->fbuf);
		if (r < 0) {
			return r;
		}
	}

	return 0;
}
#endif

/* Exposing some of the APIs to CoAP unit tests in tests/net/lib/coap */
#if defined(CONFIG_COAP_TEST_API_ENABLE)
bool _coap_match_path_uri(const char * const *path,
			  const char *uri, u16_t len)
{
	return match_path_uri(path, uri, len);
}
#endif
