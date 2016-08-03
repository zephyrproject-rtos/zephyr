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

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <misc/byteorder.h>
#include <net/ip_buf.h>

#include "zoap.h"

struct option_context {
	uint8_t *buf;
	int delta;
	int used; /* size used of options */
	int buflen;
};

#define COAP_VERSION 1

#define COAP_MARKER 0xFF

#define BASIC_HEADER_SIZE 4

static uint8_t coap_option_header_get_delta(uint8_t buf)
{
	return (buf & 0xF0) >> 4;
}

static uint8_t coap_option_header_get_len(uint8_t buf)
{
	return buf & 0x0F;
}

static void coap_option_header_set_delta(uint8_t *buf, uint8_t delta)
{
	*buf |= (delta & 0xF) << 4;
}

static void coap_option_header_set_len(uint8_t *buf, uint8_t len)
{
	*buf |= (len & 0xF);
}

static int decode_delta(int num, const uint8_t *buf, int16_t buflen,
			uint16_t *decoded)
{
	int hdrlen = 0;

	switch (num) {
	case 13:
		if (buflen < 1) {
			return -EINVAL;
		}

		num = *buf + 13;
		hdrlen += 1;
		break;
	case 14:
		if (buflen < 2) {
			return -EINVAL;
		}

		num = sys_be16_to_cpu((uint16_t)*buf) + 269;
		hdrlen += 2;
		break;
	case 15:
		return -EINVAL;
	}

	*decoded = num;

	return hdrlen;
}

static int coap_parse_option(const struct zoap_packet *pkt,
			     struct option_context *context,
			     uint8_t **value, uint16_t *vlen)
{
	uint16_t delta, len;
	int r;

	if (context->buflen < 1) {
		return 0;
	}

	/* This indicates that options have ended */
	if (context->buf[0] == COAP_MARKER) {
		return 0;
	}

	delta = coap_option_header_get_delta(context->buf[0]);
	len = coap_option_header_get_len(context->buf[0]);
	context->buf += 1;
	context->used += 1;
	context->buflen -= 1;

	/* In case 'delta' doesn't fit the option fixed header. */
	r = decode_delta(delta, context->buf, context->buflen, &delta);
	if (r < 0) {
		return -EINVAL;
	}

	context->buf += r;
	context->used += r;
	context->buflen -= r;

	/* In case 'len' doesn't fit the option fixed header. */
	r = decode_delta(len, context->buf, context->buflen, &len);
	if (r < 0) {
		return -EINVAL;
	}

	if (context->buflen < r + len) {
		return -EINVAL;
	}

	if (value) {
		*value = context->buf + r;
	}

	if (vlen) {
		*vlen = len;
	}

	context->buf += r + len;
	context->used += r + len;
	context->buflen -= r + len;

	context->delta += delta;

	return context->used;
}

static int coap_parse_options(struct zoap_packet *pkt, unsigned int offset)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);
	struct option_context context = {
		.delta = 0,
		.used = 0,
		.buflen = ip_buf_appdatalen(buf) - offset,
		.buf = &appdata[offset] };

	while (true) {
		int r = coap_parse_option(pkt, &context, NULL, NULL);

		if (r < 0) {
			return -EINVAL;
		}

		if (r == 0) {
			break;
		}
	}
	return context.used;
}

static uint8_t coap_header_get_tkl(const struct zoap_packet *pkt)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);

	return appdata[0] & 0xF;
}

static int coap_get_header_len(const struct zoap_packet *pkt)
{
	struct net_buf *buf = pkt->buf;
	unsigned int hdrlen;
	uint8_t tkl;

	hdrlen = BASIC_HEADER_SIZE;

	if (ip_buf_appdatalen(buf) < hdrlen) {
		return -EINVAL;
	}

	tkl = coap_header_get_tkl(pkt);

	/* Token lenghts 9-15 are reserved. */
	if (tkl > 8) {
		return -EINVAL;
	}

	if (net_buf_tailroom(buf) < hdrlen + tkl) {
		return -EINVAL;
	}

	return hdrlen + tkl;
}

int zoap_packet_parse(struct zoap_packet *pkt, struct net_buf *buf)
{
	int optlen, hdrlen;

	if (!buf) {
		return -EINVAL;
	}

	memset(pkt, 0, sizeof(*pkt));
	pkt->buf = buf;

	hdrlen = coap_get_header_len(pkt);
	if (hdrlen < 0) {
		return -EINVAL;
	}

	optlen = coap_parse_options(pkt, hdrlen);
	if (optlen < 0) {
		return -EINVAL;
	}

	if (ip_buf_appdatalen(buf) < hdrlen + optlen) {
		return -EINVAL;
	}

	if (ip_buf_appdatalen(buf) <= hdrlen + optlen + 1) {
		pkt->start = NULL;
		return 0;
	}

	pkt->start = ip_buf_appdata(buf) + hdrlen + optlen + 1;

	return 0;
}

static int delta_encode(int num, uint8_t *value, uint8_t *buf, size_t buflen)
{
	uint16_t v;

	if (num < 13) {
		*value = num;
		return 0;
	}

	if (num < 269) {
		if (buflen < 1) {
			return -EINVAL;
		}

		*value = 13;
		*buf = num - 13;
		return 1;
	}

	if (buflen < 2) {
		return -EINVAL;
	}

	*value = 14;

	v = sys_cpu_to_be16(num - 269);
	memcpy(buf, &v, sizeof(v));

	return 2;
}

static int coap_option_encode(struct option_context *context, uint16_t code,
			      const void *value, uint16_t len)
{
	int delta, offset, r;
	uint8_t data;

	delta = code - context->delta;

	offset = 1;

	r = delta_encode(delta, &data, context->buf + offset,
			 context->buflen - offset);

	if (r < 0) {
		return -EINVAL;
	}

	offset += r;
	coap_option_header_set_delta(context->buf, data);

	r = delta_encode(len, &data, context->buf + offset,
			 context->buflen - offset);
	if (r < 0) {
		return -EINVAL;
	}

	offset += r;
	coap_option_header_set_len(context->buf, data);

	if (context->buflen < offset + len) {
		return -EINVAL;
	}

	memcpy(context->buf + offset, value, len);

	return offset + len;
}

int zoap_packet_init(struct zoap_packet *pkt,
		      struct net_buf *buf)
{
	if (!buf) {
		return -EINVAL;
	}

	if (net_buf_tailroom(buf) < BASIC_HEADER_SIZE) {
		return -ENOMEM;
	}

	memset(pkt, 0, sizeof(*pkt));
	memset(ip_buf_appdata(buf), 0, net_buf_tailroom(buf));

	pkt->buf = buf;
	ip_buf_appdatalen(buf) = BASIC_HEADER_SIZE;

	return 0;
}

int zoap_pending_init(struct zoap_pending *pending,
		       const struct zoap_packet *request)
{
	memset(pending, 0, sizeof(*pending));
	memcpy(&pending->request, request, sizeof(*request));

	/* FIXME: keeping a reference to the original net_buf is necessary? */

	return 0;
}

struct zoap_pending *zoap_pending_next_unused(
	struct zoap_pending *pendings, size_t len)
{
	struct zoap_pending *p;
	size_t i;

	for (i = 0, p = pendings; i < len; i++, p++) {
		struct zoap_packet *pkt = &p->request;

		if (p->timeout == 0 && !pkt->buf) {
			return p;
		}
	}

	return NULL;
}

struct zoap_reply *zoap_reply_next_unused(
	struct zoap_reply *replies, size_t len)
{
	struct zoap_reply *r;
	size_t i;

	for (i = 0, r = replies; i < len; i++, r++) {
		if (!r->reply) {
			return r;
		}
	}

	return NULL;
}

static bool match_response(const struct zoap_packet *request,
			   const struct zoap_packet *response)
{
	const uint8_t *req_token, *resp_token;
	uint8_t req_tkl, resp_tkl;

	if (zoap_header_get_id(request) != zoap_header_get_id(response)) {
		return false;
	}

	req_token = zoap_header_get_token(request, &req_tkl);
	resp_token = zoap_header_get_token(response, &resp_tkl);

	if (req_tkl != resp_tkl) {
		return false;
	}

	return req_tkl == 0 || memcmp(req_token, resp_token, req_tkl) == 0;
}

struct zoap_pending *zoap_pending_received(
	const struct zoap_packet *response,
	struct zoap_pending *pendings, size_t len)
{
	struct zoap_pending *p;
	size_t i;

	for (i = 0, p = pendings; i < len; i++, p++) {
		struct zoap_packet *req = &p->request;

		if (!p->timeout) {
			continue;
		}

		if (!match_response(req, response)) {
			continue;
		}

		zoap_pending_clear(p);
		return p;
	}

	return NULL;
}

struct zoap_pending *zoap_pending_next_to_expire(
	struct zoap_pending *pendings, size_t len)
{
	struct zoap_pending *p, *found = NULL;
	size_t i;

	for (i = 0, p = pendings; i < len; i++, p++) {
		if (p->timeout && (!found || found->timeout < p->timeout)) {
			found = p;
		}
	}

	return found;
}

#define LAST_TIMEOUT (2345 * 4)

static uint16_t next_timeout(uint16_t previous)
{
	switch (previous) {
	case 0:
		return 2345;
	case 2345:
		return 2345 * 2;
	case (2345 * 2):
		return LAST_TIMEOUT;
	case LAST_TIMEOUT:
		return LAST_TIMEOUT;
	}

	return 2345;
}

bool zoap_pending_cycle(struct zoap_pending *pending)
{
	uint16_t old = pending->timeout;

	pending->timeout = next_timeout(pending->timeout);

	return old != pending->timeout;
}

void zoap_pending_clear(struct zoap_pending *pending)
{
	pending->timeout = 0;
}

static bool uri_path_eq(const struct zoap_packet *pkt,
			const char * const *path)
{
	struct zoap_option options[16];
	uint16_t count = 16;
	int i, r;

	r = zoap_find_options(pkt, ZOAP_OPTION_URI_PATH, options, count);
	if (r < 0) {
		return false;
	}

	count = r;

	for (i = 0; i < count && path[i]; i++) {
		size_t len;

		len = strlen(path[i]);

		if (options[i].len != len) {
			return false;
		}

		if (strncmp(options[i].value, path[i], len)) {
			return false;
		}
	}

	return i == count && !path[i];
}

static zoap_method_t method_from_code(const struct zoap_resource *resource,
				       uint8_t code)
{
	switch (code) {
	case ZOAP_METHOD_GET:
		return resource->get;
	case ZOAP_METHOD_POST:
		return resource->post;
	case ZOAP_METHOD_PUT:
		return resource->put;
	case ZOAP_METHOD_DELETE:
		return resource->del;
	default:
		return NULL;
	}
}

int zoap_handle_request(struct zoap_packet *pkt,
			 struct zoap_resource *resources,
			 const void *from)
{
	struct zoap_resource *resource;

	for (resource = resources; resource && resource->path; resource++) {
		zoap_method_t method;
		uint8_t code;

		/* FIXME: deal with hierarchical resources */
		if (!uri_path_eq(pkt, resource->path)) {
			continue;
		}

		code = zoap_header_get_code(pkt);
		method = method_from_code(resource, code);

		if (!method) {
			return 0;
		}

		return method(resource, pkt, from);
	}

	return -ENOENT;
}

struct zoap_reply *zoap_response_received(
	const struct zoap_packet *response, const void *from,
	struct zoap_reply *replies, size_t len)
{
	struct zoap_reply *r;
	size_t i;

	for (i = 0, r = replies; i < len; i++, r++) {
		const uint8_t *token;
		uint8_t tkl;

		token = zoap_header_get_token(response, &tkl);

		if (r->tkl != tkl) {
			continue;
		}

		if (tkl > 0 && memcmp(r->token, token, tkl)) {
			continue;
		}

		r->reply(response, r, from);
		return r;
	}

	return NULL;
}

void zoap_reply_init(struct zoap_reply *reply,
		      const struct zoap_packet *request)
{
	const uint8_t *token;
	uint8_t tkl;

	token = zoap_header_get_token(request, &tkl);

	if (tkl > 0) {
		memcpy(reply->token, token, tkl);
	}
	reply->tkl = tkl;
}

void zoap_reply_clear(struct zoap_reply *reply)
{
	reply->reply = NULL;
}

uint8_t *zoap_packet_get_payload(struct zoap_packet *pkt, uint16_t *len)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);

	if (len) {
		*len = 0;
	}

	if (!pkt->start) {
		if (ip_buf_appdatalen(buf) + 1 > net_buf_tailroom(buf)) {
			return NULL;
		}

		appdata[ip_buf_appdatalen(buf)] = COAP_MARKER;
		ip_buf_appdatalen(buf) += 1;

		pkt->start = appdata + ip_buf_appdatalen(buf);
	}

	if (len) {
		*len = net_buf_tailroom(buf) - ip_buf_appdatalen(buf);
	}

	return pkt->start;
}

int zoap_packet_set_used(struct zoap_packet *pkt, uint16_t len)
{
	struct net_buf *buf = pkt->buf;

	if (ip_buf_appdatalen(buf) + len > net_buf_tailroom(buf)) {
		return -ENOMEM;
	}

	ip_buf_appdatalen(buf) += len;

	return 0;
}

int zoap_add_option(struct zoap_packet *pkt, uint16_t code,
		     const void *value, uint16_t len)
{
	struct net_buf *buf = pkt->buf;
	struct option_context context = { .delta = 0,
					  .used = 0 };
	int r, offset;

	if (pkt->start) {
		return -EINVAL;
	}

	offset = coap_get_header_len(pkt);
	if (offset < 0) {
		return -EINVAL;
	}

	/* We check for options in all the 'used' space. */
	context.buflen = ip_buf_appdatalen(buf) - offset;
	context.buf = ip_buf_appdata(buf) + offset;

	while (context.delta <= code) {
		r = coap_parse_option(pkt, &context, NULL, NULL);
		if (r < 0) {
			return -ENOENT;
		}

		if (r == 0) {
			break;
		}

		/* If the new option code is out of order. */
		if (code < context.delta) {
			return -EINVAL;
		}
	}
	/* We can now add options using all the available space. */
	context.buflen = net_buf_tailroom(buf) - (offset + context.used);

	r = coap_option_encode(&context, code, value, len);
	if (r < 0) {
		return -EINVAL;
	}

	ip_buf_appdatalen(buf) += r;

	return 0;
}

int zoap_find_options(const struct zoap_packet *pkt, uint16_t code,
		       struct zoap_option *options, uint16_t veclen)
{
	struct net_buf *buf = pkt->buf;
	struct option_context context = { .delta = 0,
					  .used = 0 };
	int hdrlen, count = 0;
	uint16_t len;

	hdrlen = coap_get_header_len(pkt);
	if (hdrlen < 0) {
		return -EINVAL;
	}

	context.buflen = ip_buf_appdatalen(buf) - hdrlen;
	context.buf = (uint8_t *)ip_buf_appdata(buf) + hdrlen;

	while (context.delta <= code && count < veclen) {
		int used = coap_parse_option(pkt, &context,
					     (uint8_t **)&options[count].value,
					     &len);
		options[count].len = len;
		if (used < 0) {
			return -ENOENT;
		}

		if (used == 0) {
			break;
		}

		if (code != context.delta) {
			continue;
		}

		count++;
	}

	return count;
}

uint8_t zoap_header_get_version(const struct zoap_packet *pkt)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);
	uint8_t byte = appdata[0];

	return (byte & 0xC0) >> 6;
}

uint8_t zoap_header_get_type(const struct zoap_packet *pkt)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);
	uint8_t byte = appdata[0];

	return (byte & 0x30) >> 4;
}

uint8_t coap_header_get_code(const struct zoap_packet *pkt)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);

	return appdata[1];
}

const uint8_t *zoap_header_get_token(const struct zoap_packet *pkt,
				      uint8_t *len)
{
	struct net_buf *buf = pkt->buf;
	uint8_t tkl = coap_header_get_tkl(pkt);

	if (len) {
		*len = 0;
	}

	if (tkl == 0) {
		return NULL;
	}

	if (len) {
		*len = tkl;
	}

	return (uint8_t *)ip_buf_appdata(buf) + BASIC_HEADER_SIZE;
}

uint8_t zoap_header_get_code(const struct zoap_packet *pkt)
{
	uint8_t code = coap_header_get_code(pkt);

	switch (code) {
	/* Methods are encoded in the code field too */
	case ZOAP_METHOD_GET:
	case ZOAP_METHOD_POST:
	case ZOAP_METHOD_PUT:
	case ZOAP_METHOD_DELETE:

	/* All the defined response codes */
	case ZOAP_RESPONSE_CODE_OK:
	case ZOAP_RESPONSE_CODE_CREATED:
	case ZOAP_RESPONSE_CODE_DELETED:
	case ZOAP_RESPONSE_CODE_VALID:
	case ZOAP_RESPONSE_CODE_CHANGED:
	case ZOAP_RESPONSE_CODE_CONTENT:
	case ZOAP_RESPONSE_CODE_BAD_REQUEST:
	case ZOAP_RESPONSE_CODE_UNAUTHORIZED:
	case ZOAP_RESPONSE_CODE_BAD_OPTION:
	case ZOAP_RESPONSE_CODE_FORBIDDEN:
	case ZOAP_RESPONSE_CODE_NOT_FOUND:
	case ZOAP_RESPONSE_CODE_NOT_ALLOWED:
	case ZOAP_RESPONSE_CODE_NOT_ACCEPTABLE:
	case ZOAP_RESPONSE_CODE_PRECONDITION_FAILED:
	case ZOAP_RESPONSE_CODE_REQUEST_TOO_LARGE:
	case ZOAP_RESPONSE_CODE_INTERNAL_ERROR:
	case ZOAP_RESPONSE_CODE_NOT_IMPLEMENTED:
	case ZOAP_RESPONSE_CODE_BAD_GATEWAY:
	case ZOAP_RESPONSE_CODE_SERVICE_UNAVAILABLE:
	case ZOAP_RESPONSE_CODE_GATEWAY_TIMEOUT:
	case ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED:
	case ZOAP_CODE_EMPTY:
		return code;
	default:
		return ZOAP_CODE_EMPTY;
	}
}

uint16_t zoap_header_get_id(const struct zoap_packet *pkt)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);

	return sys_get_be16(&appdata[2]);
}

void zoap_header_set_version(struct zoap_packet *pkt, uint8_t ver)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);

	appdata[0] |= (ver & 0x3) << 6;
}

void zoap_header_set_type(struct zoap_packet *pkt, uint8_t type)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);

	appdata[0] |= (type & 0x3) << 4;
}

int zoap_header_set_token(struct zoap_packet *pkt, const uint8_t *token,
			   uint8_t tokenlen)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);

	if (net_buf_tailroom(buf) < BASIC_HEADER_SIZE + tokenlen) {
		return -EINVAL;
	}

	if (tokenlen > 8) {
		return -EINVAL;
	}

	ip_buf_appdatalen(buf) += tokenlen;
	appdata[0] |= tokenlen & 0xF;

	memcpy(ip_buf_appdata(buf) + BASIC_HEADER_SIZE, token, tokenlen);

	return 0;
}

void zoap_header_set_code(struct zoap_packet *pkt, uint8_t code)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);

	appdata[1] = code;
}

void zoap_header_set_id(struct zoap_packet *pkt, uint16_t id)
{
	struct net_buf *buf = pkt->buf;
	uint8_t *appdata = ip_buf_appdata(buf);

	sys_put_be16(id, &appdata[2]);
}
