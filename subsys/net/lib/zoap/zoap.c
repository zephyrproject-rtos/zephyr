/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <misc/byteorder.h>
#include <net/buf.h>
#include <net/nbuf.h>
#include <net/net_ip.h>

#include <net/zoap.h>

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
	*buf = (delta & 0xF) << 4;
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

		num = sys_get_be16(buf) + 269;
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
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;
	struct option_context context = {
		.delta = 0,
		.used = 0,
		.buflen = frag->len - offset,
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
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;

	return appdata[0] & 0xF;
}

static int coap_get_header_len(const struct zoap_packet *pkt)
{
	struct net_buf *frag = pkt->buf->frags;
	unsigned int hdrlen;
	uint8_t tkl;

	hdrlen = BASIC_HEADER_SIZE;

	if (frag->len < hdrlen) {
		return -EINVAL;
	}

	tkl = coap_header_get_tkl(pkt);

	/* Token lenghts 9-15 are reserved. */
	if (tkl > 8) {
		return -EINVAL;
	}

	if (frag->len < hdrlen + tkl) {
		return -EINVAL;
	}

	return hdrlen + tkl;
}

int zoap_packet_parse(struct zoap_packet *pkt, struct net_buf *buf)
{
	struct net_buf *frag = buf->frags;
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

	if (frag->len < hdrlen + optlen) {
		return -EINVAL;
	}

	if (frag->len <= hdrlen + optlen + 1) {
		pkt->start = NULL;
		return 0;
	}

	pkt->start = frag->data + hdrlen + optlen + 1;

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
	struct net_buf *frag = buf->frags;
	void *data;

	if (!buf) {
		return -EINVAL;
	}

	if (net_buf_tailroom(frag) < BASIC_HEADER_SIZE) {
		return -ENOMEM;
	}

	memset(pkt, 0, sizeof(*pkt));

	data = net_buf_add(frag, BASIC_HEADER_SIZE);

	/*
	 * As some header data is built by OR operations, we zero
	 * the header to be sure.
	 */
	memset(data, 0, BASIC_HEADER_SIZE);

	pkt->buf = buf;

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

static inline bool is_addr_unspecified(const struct sockaddr *addr)
{
	if (addr->family == AF_UNSPEC) {
		return true;
	}

	if (addr->family == AF_INET6) {
		return net_is_ipv6_addr_unspecified(
			&(net_sin6(addr)->sin6_addr));
	} else if (addr->family == AF_INET) {
		return net_sin(addr)->sin_addr.s4_addr32[0] == 0;
	}

	return false;
}

struct zoap_observer *zoap_observer_next_unused(
	struct zoap_observer *observers, size_t len)
{
	struct zoap_observer *o;
	size_t i;

	for (i = 0, o = observers; i < len; i++, o++) {
		if (is_addr_unspecified(&o->addr)) {
			return o;
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

static int32_t next_timeout(int32_t previous)
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
	int32_t old = pending->timeout;

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

		if (memcmp(options[i].value, path[i], len)) {
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

static bool is_request(struct zoap_packet *pkt)
{
	uint8_t code = zoap_header_get_code(pkt);

	return !(code & ~ZOAP_REQUEST_MASK);
}

int zoap_handle_request(struct zoap_packet *pkt,
			struct zoap_resource *resources,
			const struct sockaddr *from)
{
	struct zoap_resource *resource;

	if (!is_request(pkt)) {
		return 0;
	}

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

unsigned int zoap_option_value_to_int(const struct zoap_option *option)
{
	switch (option->len) {
	case 0:
		return 0;
	case 1:
		return option->value[0];
	case 2:
		return (option->value[1] << 0) | (option->value[0] << 8);
	case 3:
		return (option->value[2] << 0) | (option->value[1] << 8) |
			(option->value[0] << 16);
	case 4:
		return (option->value[2] << 0) | (option->value[2] << 8) |
			(option->value[1] << 16) | (option->value[0] << 24);
	default:
		return 0;
	}

	return 0;
}

static int get_observe_option(const struct zoap_packet *pkt)
{
	struct zoap_option option = {};
	uint16_t count = 1;
	int r;

	r = zoap_find_options(pkt, ZOAP_OPTION_OBSERVE, &option, count);
	if (r <= 0) {
		return -ENOENT;
	}

	return zoap_option_value_to_int(&option);
}

struct zoap_reply *zoap_response_received(
	const struct zoap_packet *response,
	const struct sockaddr *from,
	struct zoap_reply *replies, size_t len)
{
	struct zoap_reply *r;
	const uint8_t *token;
	uint8_t tkl;
	size_t i;

	token = zoap_header_get_token(response, &tkl);

	for (i = 0, r = replies; i < len; i++, r++) {
		int age;

		if (r->tkl != tkl) {
			continue;
		}

		if (tkl > 0 && memcmp(r->token, token, tkl)) {
			continue;
		}

		age = get_observe_option(response);
		if (age > 0) {
			/*
			 * age == 2 means that the notifications wrapped,
			 * or this is the first one
			 */
			if (r->age > age && age != 2) {
				continue;
			}

			r->age = age;
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
	int age;

	token = zoap_header_get_token(request, &tkl);

	if (tkl > 0) {
		memcpy(reply->token, token, tkl);
	}
	reply->tkl = tkl;

	age = get_observe_option(request);

	/* It means that the request enabled observing a resource */
	if (age == 0) {
		reply->age = 2;
	}
}

void zoap_reply_clear(struct zoap_reply *reply)
{
	reply->reply = NULL;
}

int zoap_resource_notify(struct zoap_resource *resource)
{
	sys_snode_t *node;

	resource->age++;

	if (!resource->notify) {
		return -ENOENT;
	}

	SYS_SLIST_FOR_EACH_NODE(&resource->observers, node) {
		struct zoap_observer *o = (struct zoap_observer *) node;

		resource->notify(resource, o);
	}

	return 0;
}

bool zoap_request_is_observe(const struct zoap_packet *request)
{
	return get_observe_option(request) == 0;
}

void zoap_observer_init(struct zoap_observer *observer,
			const struct zoap_packet *request,
			const struct sockaddr *addr)
{
	const uint8_t *token;
	uint8_t tkl;

	token = zoap_header_get_token(request, &tkl);

	if (tkl > 0) {
		memcpy(observer->token, token, tkl);
	}

	observer->tkl = tkl;

	net_ipaddr_copy(&observer->addr, addr);
}

bool zoap_register_observer(struct zoap_resource *resource,
			    struct zoap_observer *observer)
{
	bool first;

	sys_slist_append(&resource->observers, &observer->list);

	first = resource->age == 0;
	if (first) {
		resource->age = 2;
	}

	return first;
}

void zoap_remove_observer(struct zoap_resource *resource,
			  struct zoap_observer *observer)
{
	sys_slist_find_and_remove(&resource->observers, &observer->list);
}

static bool sockaddr_equal(const struct sockaddr *a,
			   const struct sockaddr *b)
{
	/*
	 * FIXME: Should we consider ipv6-mapped ipv4 addresses as equal to
	 * ipv4 addresses?
	 */
	if (a->family != b->family) {
		return false;
	}

	if (a->family == AF_INET) {
		const struct sockaddr_in *a4 = net_sin(a);
		const struct sockaddr_in *b4 = net_sin(b);

		if (a4->sin_port != b4->sin_port) {
			return false;
		}

		return net_ipv4_addr_cmp(&a4->sin_addr, &b4->sin_addr);
	}

	if (b->family == AF_INET6) {
		const struct sockaddr_in6 *a6 = net_sin6(a);
		const struct sockaddr_in6 *b6 = net_sin6(b);

		if (a6->sin6_scope_id != b6->sin6_scope_id) {
			return false;
		}

		if (a6->sin6_port != b6->sin6_port) {
			return false;
		}

		return net_ipv6_addr_cmp(&a6->sin6_addr, &b6->sin6_addr);
	}

	/* Invalid address family */
	return false;
}

struct zoap_observer *zoap_find_observer_by_addr(
	struct zoap_observer *observers, size_t len,
	const struct sockaddr *addr)
{
	size_t i;

	for (i = 0; i < len; i++) {
		struct zoap_observer *o = &observers[i];

		if (sockaddr_equal(&o->addr, addr)) {
			return o;
		}
	}

	return NULL;
}

uint8_t *zoap_packet_get_payload(struct zoap_packet *pkt, uint16_t *len)
{
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;
	uint16_t appdatalen = frag->len;

	if (len) {
		*len = 0;
	}

	if (!pkt->start) {
		if (appdatalen + 1 > net_buf_tailroom(frag)) {
			return NULL;
		}

		appdata[appdatalen] = COAP_MARKER;
		frag->len += 1;

		pkt->start = appdata + frag->len;
	}

	if (len) {
		*len = net_buf_tailroom(frag);
	}

	return pkt->start;
}

int zoap_packet_set_used(struct zoap_packet *pkt, uint16_t len)
{
	struct net_buf *frag = pkt->buf->frags;

	if (frag->len + len > net_buf_tailroom(frag)) {
		return -ENOMEM;
	}

	frag->len += len;

	return 0;
}

struct net_buf *zoap_packet_get_buf(struct zoap_packet *pkt)
{
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *str;

	if (!pkt->start) {
		str = net_buf_add(frag, 1);
		*str = COAP_MARKER;
		pkt->start = str + 1;
	}

	return frag;
}

int zoap_add_option(struct zoap_packet *pkt, uint16_t code,
		    const void *value, uint16_t len)
{
	struct net_buf *frag = pkt->buf->frags;
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
	context.buflen = frag->len - offset;
	context.buf = frag->data + offset;

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
	context.buflen = net_buf_tailroom(frag) - (offset + context.used);

	r = coap_option_encode(&context, code, value, len);
	if (r < 0) {
		return -EINVAL;
	}

	frag->len += r;

	return 0;
}

int zoap_add_option_int(struct zoap_packet *pkt, uint16_t code,
			unsigned int val)
{
	uint8_t data[4], len;

	if (val == 0) {
		data[0] = 0;
		len = 0;
	} else if (val < 0xFF) {
		data[0] = (uint8_t) val;
		len = 1;
	} else if (val < 0xFFFF) {
		sys_put_be16(val, data);
		len = 2;
	} else if (val < 0xFFFFFF) {
		sys_put_be16(val, data);
		data[2] = val >> 16;
		len = 3;
	} else {
		sys_put_be32(val, data);
		len = 4;
	}

	return zoap_add_option(pkt, code, data, len);
}

int zoap_find_options(const struct zoap_packet *pkt, uint16_t code,
		      struct zoap_option *options, uint16_t veclen)
{
	struct net_buf *frag = pkt->buf->frags;
	struct option_context context = { .delta = 0,
					  .used = 0 };
	int hdrlen, count = 0;
	uint16_t len;

	hdrlen = coap_get_header_len(pkt);
	if (hdrlen < 0) {
		return -EINVAL;
	}

	context.buflen = frag->len - hdrlen;
	context.buf = (uint8_t *)frag->data + hdrlen;

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
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;
	uint8_t byte = appdata[0];

	return (byte & 0xC0) >> 6;
}

uint8_t zoap_header_get_type(const struct zoap_packet *pkt)
{
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;
	uint8_t byte = appdata[0];

	return (byte & 0x30) >> 4;
}

uint8_t coap_header_get_code(const struct zoap_packet *pkt)
{
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;

	return appdata[1];
}

const uint8_t *zoap_header_get_token(const struct zoap_packet *pkt,
				     uint8_t *len)
{
	struct net_buf *frag = pkt->buf->frags;
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

	return (uint8_t *)frag->data + BASIC_HEADER_SIZE;
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
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;

	return sys_get_be16(&appdata[2]);
}

void zoap_header_set_version(struct zoap_packet *pkt, uint8_t ver)
{
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;

	appdata[0] |= (ver & 0x3) << 6;
}

void zoap_header_set_type(struct zoap_packet *pkt, uint8_t type)
{
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;

	appdata[0] |= (type & 0x3) << 4;
}

int zoap_header_set_token(struct zoap_packet *pkt, const uint8_t *token,
			  uint8_t tokenlen)
{
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;

	if (net_buf_tailroom(frag) < BASIC_HEADER_SIZE + tokenlen) {
		return -EINVAL;
	}

	if (tokenlen > 8) {
		return -EINVAL;
	}

	frag->len += tokenlen;

	appdata[0] |= tokenlen & 0xF;

	memcpy(frag->data + BASIC_HEADER_SIZE, token, tokenlen);

	return 0;
}

void zoap_header_set_code(struct zoap_packet *pkt, uint8_t code)
{
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;

	appdata[1] = code;
}

void zoap_header_set_id(struct zoap_packet *pkt, uint16_t id)
{
	struct net_buf *frag = pkt->buf->frags;
	uint8_t *appdata = frag->data;

	sys_put_be16(id, &appdata[2]);
}

int zoap_block_transfer_init(struct zoap_block_context *ctx,
			      enum zoap_block_size block_size,
			      size_t total_size)
{
	ctx->block_size = block_size;
	ctx->total_size = total_size;
	ctx->current = 0;

	return 0;
}

#define GET_BLOCK_SIZE(v) (((v) & 0x7))
#define GET_MORE(v) (!!((v) & 0x08))
#define GET_NUM(v) ((v) >> 4)

#define SET_BLOCK_SIZE(v, b) (v |= ((b) & 0x07))
#define SET_MORE(v, m) ((v) |= (m) ? 0x08 : 0x00)
#define SET_NUM(v, n) ((v) |= ((n) << 4))

int zoap_add_block1_option(struct zoap_packet *pkt,
			    struct zoap_block_context *ctx)
{
	uint16_t bytes = zoap_block_size_to_bytes(ctx->block_size);
	unsigned int val = 0;
	int r;

	if (is_request(pkt)) {
		SET_BLOCK_SIZE(val, ctx->block_size);
		SET_MORE(val, ctx->current + bytes < ctx->total_size);
		SET_NUM(val, ctx->current / bytes);
	} else {
		SET_BLOCK_SIZE(val, ctx->block_size);
		SET_NUM(val, ctx->current / bytes);
	}

	r = zoap_add_option_int(pkt, ZOAP_OPTION_BLOCK1, val);

	return r;
}

int zoap_add_block2_option(struct zoap_packet *pkt,
			    struct zoap_block_context *ctx)
{
	int r, val = 0;
	uint16_t bytes = zoap_block_size_to_bytes(ctx->block_size);

	if (is_request(pkt)) {
		SET_BLOCK_SIZE(val, ctx->block_size);
		SET_NUM(val, ctx->current / bytes);
	} else {
		SET_BLOCK_SIZE(val, ctx->block_size);
		SET_MORE(val, ctx->current + bytes < ctx->total_size);
		SET_NUM(val, ctx->current / bytes);
	}

	r = zoap_add_option_int(pkt, ZOAP_OPTION_BLOCK2, val);

	return r;
}

int zoap_add_size1_option(struct zoap_packet *pkt,
			   struct zoap_block_context *ctx)
{
	return zoap_add_option_int(pkt, ZOAP_OPTION_SIZE1, ctx->total_size);
}

int zoap_add_size2_option(struct zoap_packet *pkt,
			   struct zoap_block_context *ctx)
{
	return zoap_add_option_int(pkt, ZOAP_OPTION_SIZE2, ctx->total_size);
}

struct block_transfer {
	int num;
	int block_size;
	bool more;
};

static unsigned int get_block_option(struct zoap_packet *pkt, uint16_t code)
{
	struct zoap_option option;
	unsigned int val;
	int count = 1;

	count = zoap_find_options(pkt, code, &option, count);
	if (count <= 0)
		return 0;

	val = zoap_option_value_to_int(&option);

	return val;
}

static int update_descriptive_block(struct zoap_block_context *ctx,
				    int block, int size)
{
	size_t new_current = GET_NUM(block) << (GET_BLOCK_SIZE(block) + 4);

	if (block == 0) {
		return -ENOENT;
	}

	if (size && ctx->total_size && ctx->total_size != size) {
		return -EINVAL;
	}

	if (ctx->block_size > 0 && GET_BLOCK_SIZE(block) > ctx->block_size) {
		return -EINVAL;
	}

	if (ctx->total_size && new_current > ctx->total_size) {
		return -EINVAL;
	}

	if (size) {
		ctx->total_size = size;
	}
	ctx->current = new_current;
	ctx->block_size = GET_BLOCK_SIZE(block);

	return 0;
}

static int update_control_block1(struct zoap_block_context *ctx,
				     int block, int size)
{
	size_t new_current = GET_NUM(block) << (GET_BLOCK_SIZE(block) + 4);

	if (block == 0) {
		return 0;
	}

	if (new_current != ctx->current) {
		return -EINVAL;
	}

	if (GET_BLOCK_SIZE(block) > ctx->block_size) {
		return -EINVAL;
	}

	ctx->block_size = GET_BLOCK_SIZE(block);
	ctx->total_size = size;

	return 0;
}

static int update_control_block2(struct zoap_block_context *ctx,
				 int block, int size)
{
	size_t new_current = GET_NUM(block) << (GET_BLOCK_SIZE(block) + 4);

	if (block == 0) {
		return 0;
	}

	if (GET_MORE(block)) {
		return -EINVAL;
	}

	if (GET_NUM(block) > 0 && GET_BLOCK_SIZE(block) != ctx->block_size) {
		return -EINVAL;
	}

	ctx->current = new_current;
	ctx->block_size = GET_BLOCK_SIZE(block);
	ctx->total_size = size;

	return 0;
}

int zoap_update_from_block(struct zoap_packet *pkt,
			   struct zoap_block_context *ctx)
{
	unsigned int block1, block2, size1, size2;
	int r;

	block1 = get_block_option(pkt, ZOAP_OPTION_BLOCK1);
	block2 = get_block_option(pkt, ZOAP_OPTION_BLOCK2);
	size1 = get_block_option(pkt, ZOAP_OPTION_SIZE1);
	size2 = get_block_option(pkt, ZOAP_OPTION_SIZE2);

	if (is_request(pkt)) {
		r = update_control_block2(ctx, block2, size2);
		if (r) {
			return r;
		}

		return update_descriptive_block(ctx, block1, size1);
	}

	r = update_control_block1(ctx, block1, size1);
	if (r) {
		return r;
	}

	return update_descriptive_block(ctx, block2, size2);
}

size_t zoap_next_block(struct zoap_block_context *ctx)
{
	if (ctx->current >= ctx->total_size) {
		return 0;
	}

	ctx->current += zoap_block_size_to_bytes(ctx->block_size);

	return ctx->current;
}

uint8_t *zoap_next_token(void)
{
	static uint32_t rand[2];

	rand[0] = sys_rand32_get();
	rand[1] = sys_rand32_get();

	return (uint8_t *) rand;
}
