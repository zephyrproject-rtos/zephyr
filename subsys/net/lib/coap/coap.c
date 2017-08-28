/*
 * Copyright (c) 2017 Intel Corporation
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
#include <net/net_ip.h>

#include <net/coap.h>

struct option_context {
	u8_t *buf;
	int delta;
	int used; /* size used of options */
	int buflen;
};

#define COAP_VERSION 1

#define COAP_MARKER 0xFF

#define BASIC_HEADER_SIZE 4

static u8_t coap_option_header_get_delta(u8_t buf)
{
	return (buf & 0xF0) >> 4;
}

static u8_t coap_option_header_get_len(u8_t buf)
{
	return buf & 0x0F;
}

static void coap_option_header_set_delta(u8_t *buf, u8_t delta)
{
	*buf = (delta & 0xF) << 4;
}

static void coap_option_header_set_len(u8_t *buf, u8_t len)
{
	*buf |= (len & 0xF);
}

static int decode_delta(int num, const u8_t *buf, s16_t buflen,
			u16_t *decoded)
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

static int coap_parse_option(const struct coap_packet *cpkt,
			     struct option_context *context,
			     u8_t **value, u16_t *vlen)
{
	u16_t delta, len;
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

static int coap_parse_options(struct coap_packet *cpkt, unsigned int offset)
{
	struct option_context context = {
		.delta = 0,
		.used = 0,
		.buflen = cpkt->pkt->frags->len - offset,
		.buf = &cpkt->pkt->frags->data[offset] };

	while (true) {
		int r = coap_parse_option(cpkt, &context, NULL, NULL);

		if (r < 0) {
			return -EINVAL;
		}

		if (r == 0) {
			break;
		}
	}
	return context.used;
}

static u8_t coap_header_get_tkl(const struct coap_packet *cpkt)
{
	return cpkt->pkt->frags->data[0] & 0xF;
}

static int coap_get_header_len(const struct coap_packet *cpkt)
{
	unsigned int hdrlen;
	u8_t tkl;

	hdrlen = BASIC_HEADER_SIZE;

	if (cpkt->pkt->frags->len < hdrlen) {
		return -EINVAL;
	}

	tkl = coap_header_get_tkl(cpkt);

	/* Token lenghts 9-15 are reserved. */
	if (tkl > 8) {
		return -EINVAL;
	}

	if (cpkt->pkt->frags->len < hdrlen + tkl) {
		return -EINVAL;
	}

	return hdrlen + tkl;
}

int coap_packet_parse(struct coap_packet *cpkt, struct net_pkt *pkt)
{
	int optlen, hdrlen;

	if (!pkt || !pkt->frags) {
		return -EINVAL;
	}

	memset(cpkt, 0, sizeof(*cpkt));
	cpkt->pkt = pkt;

	hdrlen = coap_get_header_len(cpkt);
	if (hdrlen < 0) {
		return -EINVAL;
	}

	optlen = coap_parse_options(cpkt, hdrlen);
	if (optlen < 0) {
		return -EINVAL;
	}

	if (pkt->frags->len < hdrlen + optlen) {
		return -EINVAL;
	}

	if (pkt->frags->len <= hdrlen + optlen + 1) {
		cpkt->start = NULL;
		return 0;
	}

	cpkt->start = pkt->frags->data + hdrlen + optlen + 1;
	cpkt->total_size = pkt->frags->len;

	return 0;
}

static int delta_encode(int num, u8_t *value, u8_t *buf, size_t buflen)
{
	u16_t v;

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

static int coap_option_encode(struct option_context *context, u16_t code,
			      const void *value, u16_t len)
{
	int delta, offset, r;
	u8_t data;

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

int coap_packet_init(struct coap_packet *cpkt,
		     struct net_pkt *pkt)
{
	void *data;

	if (!pkt || !pkt->frags) {
		return -EINVAL;
	}

	if (net_buf_tailroom(pkt->frags) < BASIC_HEADER_SIZE) {
		return -ENOMEM;
	}

	memset(cpkt, 0, sizeof(*cpkt));
	cpkt->total_size = net_buf_tailroom(pkt->frags);

	data = net_buf_add(pkt->frags, BASIC_HEADER_SIZE);

	/*
	 * As some header data is built by OR operations, we zero
	 * the header to be sure.
	 */
	memset(data, 0, BASIC_HEADER_SIZE);
	cpkt->pkt = pkt;

	return 0;
}

int coap_pending_init(struct coap_pending *pending,
		      const struct coap_packet *request,
		      const struct sockaddr *addr)
{
	memset(pending, 0, sizeof(*pending));
	pending->id = coap_header_get_id(request);
	memcpy(&pending->addr, addr, sizeof(*addr));

	/* Will increase the reference count when the pending is cycled */
	pending->pkt = request->pkt;

	return 0;
}

struct coap_pending *coap_pending_next_unused(
	struct coap_pending *pendings, size_t len)
{
	struct coap_pending *p;
	size_t i;

	for (i = 0, p = pendings; i < len; i++, p++) {
		if (p->timeout == 0 && !p->pkt) {
			return p;
		}
	}

	return NULL;
}

struct coap_reply *coap_reply_next_unused(
	struct coap_reply *replies, size_t len)
{
	struct coap_reply *r;
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
	if (addr->sa_family == AF_UNSPEC) {
		return true;
	}

	if (addr->sa_family == AF_INET6) {
		return net_is_ipv6_addr_unspecified(
			&(net_sin6(addr)->sin6_addr));
	} else if (addr->sa_family == AF_INET) {
		return net_sin(addr)->sin_addr.s4_addr32[0] == 0;
	}

	return false;
}

struct coap_observer *coap_observer_next_unused(
	struct coap_observer *observers, size_t len)
{
	struct coap_observer *o;
	size_t i;

	for (i = 0, o = observers; i < len; i++, o++) {
		if (is_addr_unspecified(&o->addr)) {
			return o;
		}
	}

	return NULL;
}

struct coap_pending *coap_pending_received(
	const struct coap_packet *response,
	struct coap_pending *pendings, size_t len)
{
	struct coap_pending *p;
	u16_t resp_id = coap_header_get_id(response);
	size_t i;

	for (i = 0, p = pendings; i < len; i++, p++) {
		if (!p->timeout) {
			continue;
		}

		if (resp_id != p->id) {
			continue;
		}

		coap_pending_clear(p);
		return p;
	}

	return NULL;
}

struct coap_pending *coap_pending_next_to_expire(
	struct coap_pending *pendings, size_t len)
{
	struct coap_pending *p, *found = NULL;
	size_t i;

	for (i = 0, p = pendings; i < len; i++, p++) {
		if (p->timeout && (!found || found->timeout < p->timeout)) {
			found = p;
		}
	}

	return found;
}

#define LAST_TIMEOUT (2345 * 4)

static s32_t next_timeout(s32_t previous)
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

bool coap_pending_cycle(struct coap_pending *pending)
{
	s32_t old = pending->timeout;
	bool cont;

	pending->timeout = next_timeout(pending->timeout);

	/* If the timeout changed, it's not the last, continue... */
	cont = (old != pending->timeout);
	if (cont) {
		/*
		 * When it it is the last retransmission, the buffer
		 * will be destroyed when it is transmitted.
		 */
		net_pkt_ref(pending->pkt);
	}

	return cont;
}

void coap_pending_clear(struct coap_pending *pending)
{
	pending->timeout = 0;
	net_pkt_unref(pending->pkt);
	pending->pkt = NULL;
}

static bool uri_path_eq(const struct coap_packet *cpkt,
			const char * const *path)
{
	struct coap_option options[16];
	u16_t count = 16;
	int i, r;

	r = coap_find_options(cpkt, COAP_OPTION_URI_PATH, options, count);
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

static coap_method_t method_from_code(const struct coap_resource *resource,
				      u8_t code)
{
	switch (code) {
	case COAP_METHOD_GET:
		return resource->get;
	case COAP_METHOD_POST:
		return resource->post;
	case COAP_METHOD_PUT:
		return resource->put;
	case COAP_METHOD_DELETE:
		return resource->del;
	default:
		return NULL;
	}
}

static bool is_request(const struct coap_packet *cpkt)
{
	u8_t code = coap_header_get_code(cpkt);

	return !(code & ~COAP_REQUEST_MASK);
}

int coap_handle_request(struct coap_packet *cpkt,
			struct coap_resource *resources,
			const struct sockaddr *from)
{
	struct coap_resource *resource;

	if (!is_request(cpkt)) {
		return 0;
	}

	for (resource = resources; resource && resource->path; resource++) {
		coap_method_t method;
		u8_t code;

		/* FIXME: deal with hierarchical resources */
		if (!uri_path_eq(cpkt, resource->path)) {
			continue;
		}

		code = coap_header_get_code(cpkt);
		method = method_from_code(resource, code);

		if (!method) {
			return 0;
		}

		return method(resource, cpkt, from);
	}

	return -ENOENT;
}

unsigned int coap_option_value_to_int(const struct coap_option *option)
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

static int get_observe_option(const struct coap_packet *cpkt)
{
	struct coap_option option = {};
	u16_t count = 1;
	int r;

	r = coap_find_options(cpkt, COAP_OPTION_OBSERVE, &option, count);
	if (r <= 0) {
		return -ENOENT;
	}

	return coap_option_value_to_int(&option);
}

struct coap_reply *coap_response_received(
	const struct coap_packet *response,
	const struct sockaddr *from,
	struct coap_reply *replies, size_t len)
{
	struct coap_reply *r;
	const u8_t *token;
	u16_t id;
	u8_t tkl;
	size_t i;

	id = coap_header_get_id(response);
	token = coap_header_get_token(response, &tkl);

	for (i = 0, r = replies; i < len; i++, r++) {
		int age;

		if ((r->id == 0) && (r->tkl == 0)) {
			continue;
		}

		/* Piggybacked must match id when token is empty */
		if ((r->id != id) && (tkl == 0)) {
			continue;
		}

		/* Separate response must only match token */
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

void coap_reply_init(struct coap_reply *reply,
		     const struct coap_packet *request)
{
	const u8_t *token;
	u8_t tkl;
	int age;

	reply->id = coap_header_get_id(request);
	token = coap_header_get_token(request, &tkl);

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

void coap_reply_clear(struct coap_reply *reply)
{
	reply->id = 0;
	reply->tkl = 0;
	reply->reply = NULL;
}

int coap_resource_notify(struct coap_resource *resource)
{
	struct coap_observer *o;

	resource->age++;

	if (!resource->notify) {
		return -ENOENT;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&resource->observers, o, list) {
		resource->notify(resource, o);
	}

	return 0;
}

bool coap_request_is_observe(const struct coap_packet *request)
{
	return get_observe_option(request) == 0;
}

void coap_observer_init(struct coap_observer *observer,
			const struct coap_packet *request,
			const struct sockaddr *addr)
{
	const u8_t *token;
	u8_t tkl;

	token = coap_header_get_token(request, &tkl);

	if (tkl > 0) {
		memcpy(observer->token, token, tkl);
	}

	observer->tkl = tkl;

	net_ipaddr_copy(&observer->addr, addr);
}

bool coap_register_observer(struct coap_resource *resource,
			    struct coap_observer *observer)
{
	bool first;

	sys_slist_append(&resource->observers, &observer->list);

	first = resource->age == 0;
	if (first) {
		resource->age = 2;
	}

	return first;
}

void coap_remove_observer(struct coap_resource *resource,
			  struct coap_observer *observer)
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
	if (a->sa_family != b->sa_family) {
		return false;
	}

	if (a->sa_family == AF_INET) {
		const struct sockaddr_in *a4 = net_sin(a);
		const struct sockaddr_in *b4 = net_sin(b);

		if (a4->sin_port != b4->sin_port) {
			return false;
		}

		return net_ipv4_addr_cmp(&a4->sin_addr, &b4->sin_addr);
	}

	if (b->sa_family == AF_INET6) {
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

struct coap_observer *coap_find_observer_by_addr(
	struct coap_observer *observers, size_t len,
	const struct sockaddr *addr)
{
	size_t i;

	for (i = 0; i < len; i++) {
		struct coap_observer *o = &observers[i];

		if (sockaddr_equal(&o->addr, addr)) {
			return o;
		}
	}

	return NULL;
}

u8_t *coap_packet_get_payload(struct coap_packet *cpkt, u16_t *len)
{
	u8_t *appdata = cpkt->pkt->frags->data;
	u16_t appdatalen = cpkt->pkt->frags->len;

	if (!cpkt || !len) {
		return NULL;
	}

	if (!cpkt->start) {
		if (appdatalen + 1 >= cpkt->total_size) {
			return NULL;
		}

		appdata[appdatalen] = COAP_MARKER;
		cpkt->pkt->frags->len += 1;

		cpkt->start = appdata + cpkt->pkt->frags->len;
	}

	*len = appdata + cpkt->total_size - cpkt->start;

	return cpkt->start;
}

int coap_packet_set_used(struct coap_packet *cpkt, u16_t len)
{
	if ((cpkt->pkt->frags->len + len) >
	    net_buf_tailroom(cpkt->pkt->frags)) {
		return -ENOMEM;
	}

	cpkt->pkt->frags->len += len;

	return 0;
}

int coap_add_option(struct coap_packet *cpkt, u16_t code,
		    const void *value, u16_t len)
{
	struct net_buf *frag = cpkt->pkt->frags;
	struct option_context context = { .delta = 0,
					  .used = 0 };
	int r, offset;

	if (cpkt->start) {
		return -EINVAL;
	}

	offset = coap_get_header_len(cpkt);
	if (offset < 0) {
		return -EINVAL;
	}

	/* We check for options in all the 'used' space. */
	context.buflen = frag->len - offset;
	context.buf = frag->data + offset;

	while (context.delta <= code) {
		r = coap_parse_option(cpkt, &context, NULL, NULL);
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

int coap_add_option_int(struct coap_packet *cpkt, u16_t code,
			unsigned int val)
{
	u8_t data[4], len;

	if (val == 0) {
		data[0] = 0;
		len = 0;
	} else if (val < 0xFF) {
		data[0] = (u8_t) val;
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

	return coap_add_option(cpkt, code, data, len);
}

int coap_find_options(const struct coap_packet *cpkt, u16_t code,
		      struct coap_option *options, u16_t veclen)
{
	struct net_buf *frag = cpkt->pkt->frags;
	struct option_context context = { .delta = 0,
					  .used = 0 };
	int hdrlen, count = 0;
	u16_t len;

	hdrlen = coap_get_header_len(cpkt);
	if (hdrlen < 0) {
		return -EINVAL;
	}

	context.buflen = frag->len - hdrlen;
	context.buf = (u8_t *)frag->data + hdrlen;

	while (context.delta <= code && count < veclen) {
		int used = coap_parse_option(cpkt, &context,
					     (u8_t **)&options[count].value,
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

u8_t coap_header_get_version(const struct coap_packet *cpkt)
{
	return (cpkt->pkt->frags->data[0] & 0xC0) >> 6;
}

u8_t coap_header_get_type(const struct coap_packet *cpkt)
{
	return (cpkt->pkt->frags->data[0] & 0x30) >> 4;
}

u8_t coap_header_get_code(const struct coap_packet *cpkt)
{
	return cpkt->pkt->frags->data[1];
}

const u8_t *coap_header_get_token(const struct coap_packet *cpkt,
				     u8_t *len)
{
	struct net_buf *frag = cpkt->pkt->frags;
	u8_t tkl = coap_header_get_tkl(cpkt);

	if (len) {
		*len = 0;
	}

	if (tkl == 0) {
		return NULL;
	}

	if (len) {
		*len = tkl;
	}

	return (u8_t *)frag->data + BASIC_HEADER_SIZE;
}

u8_t coap_header_get_code(const struct coap_packet *cpkt)
{
	u8_t code = coap_header_get_code(cpkt);

	switch (code) {
	/* Methods are encoded in the code field too */
	case COAP_METHOD_GET:
	case COAP_METHOD_POST:
	case COAP_METHOD_PUT:
	case COAP_METHOD_DELETE:

	/* All the defined response codes */
	case COAP_RESPONSE_CODE_OK:
	case COAP_RESPONSE_CODE_CREATED:
	case COAP_RESPONSE_CODE_DELETED:
	case COAP_RESPONSE_CODE_VALID:
	case COAP_RESPONSE_CODE_CHANGED:
	case COAP_RESPONSE_CODE_CONTENT:
	case COAP_RESPONSE_CODE_CONTINUE:
	case COAP_RESPONSE_CODE_BAD_REQUEST:
	case COAP_RESPONSE_CODE_UNAUTHORIZED:
	case COAP_RESPONSE_CODE_BAD_OPTION:
	case COAP_RESPONSE_CODE_FORBIDDEN:
	case COAP_RESPONSE_CODE_NOT_FOUND:
	case COAP_RESPONSE_CODE_NOT_ALLOWED:
	case COAP_RESPONSE_CODE_NOT_ACCEPTABLE:
	case COAP_RESPONSE_CODE_INCOMPLETE:
	case COAP_RESPONSE_CODE_PRECONDITION_FAILED:
	case COAP_RESPONSE_CODE_REQUEST_TOO_LARGE:
	case COAP_RESPONSE_CODE_UNSUPPORTED_CONTENT_FORMAT:
	case COAP_RESPONSE_CODE_INTERNAL_ERROR:
	case COAP_RESPONSE_CODE_NOT_IMPLEMENTED:
	case COAP_RESPONSE_CODE_BAD_GATEWAY:
	case COAP_RESPONSE_CODE_SERVICE_UNAVAILABLE:
	case COAP_RESPONSE_CODE_GATEWAY_TIMEOUT:
	case COAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED:
	case COAP_CODE_EMPTY:
		return code;
	default:
		return COAP_CODE_EMPTY;
	}
}

u16_t coap_header_get_id(const struct coap_packet *cpkt)
{
	return sys_get_be16(&cpkt->pkt->frags->data[2]);
}

void coap_header_set_version(struct coap_packet *cpkt, u8_t ver)
{
	cpkt->pkt->frags->data[0] |= (ver & 0x3) << 6;
}

void coap_header_set_type(struct coap_packet *cpkt, u8_t type)
{
	cpkt->pkt->frags->data[0] |= (type & 0x3) << 4;
}

int coap_header_set_token(struct coap_packet *cpkt, const u8_t *token,
			  u8_t tokenlen)
{
	struct net_buf *frag = cpkt->pkt->frags;
	u8_t *appdata = frag->data;

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

void coap_header_set_code(struct coap_packet *cpkt, u8_t code)
{
	cpkt->pkt->frags->data[1] = code;
}

void coap_header_set_id(struct coap_packet *cpkt, u16_t id)
{
	sys_put_be16(id, &cpkt->pkt->frags->data[2]);
}

int coap_block_transfer_init(struct coap_block_context *ctx,
			      enum coap_block_size block_size,
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

int coap_add_block1_option(struct coap_packet *cpkt,
			    struct coap_block_context *ctx)
{
	u16_t bytes = coap_block_size_to_bytes(ctx->block_size);
	unsigned int val = 0;
	int r;

	if (is_request(cpkt)) {
		SET_BLOCK_SIZE(val, ctx->block_size);
		SET_MORE(val, ctx->current + bytes < ctx->total_size);
		SET_NUM(val, ctx->current / bytes);
	} else {
		SET_BLOCK_SIZE(val, ctx->block_size);
		SET_NUM(val, ctx->current / bytes);
	}

	r = coap_add_option_int(cpkt, COAP_OPTION_BLOCK1, val);

	return r;
}

int coap_add_block2_option(struct coap_packet *cpkt,
			    struct coap_block_context *ctx)
{
	int r, val = 0;
	u16_t bytes = coap_block_size_to_bytes(ctx->block_size);

	if (is_request(cpkt)) {
		SET_BLOCK_SIZE(val, ctx->block_size);
		SET_NUM(val, ctx->current / bytes);
	} else {
		SET_BLOCK_SIZE(val, ctx->block_size);
		SET_MORE(val, ctx->current + bytes < ctx->total_size);
		SET_NUM(val, ctx->current / bytes);
	}

	r = coap_add_option_int(cpkt, COAP_OPTION_BLOCK2, val);

	return r;
}

int coap_add_size1_option(struct coap_packet *cpkt,
			   struct coap_block_context *ctx)
{
	return coap_add_option_int(cpkt, COAP_OPTION_SIZE1, ctx->total_size);
}

int coap_add_size2_option(struct coap_packet *cpkt,
			   struct coap_block_context *ctx)
{
	return coap_add_option_int(cpkt, COAP_OPTION_SIZE2, ctx->total_size);
}

static int get_block_option(const struct coap_packet *cpkt, u16_t code)
{
	struct coap_option option;
	unsigned int val;
	int count = 1;

	count = coap_find_options(cpkt, code, &option, count);
	if (count <= 0) {
		return -ENOENT;
	}

	val = coap_option_value_to_int(&option);

	return val;
}

static int update_descriptive_block(struct coap_block_context *ctx,
				    int block, int size)
{
	size_t new_current = GET_NUM(block) << (GET_BLOCK_SIZE(block) + 4);

	if (block == -ENOENT) {
		return 0;
	}

	if (size && ctx->total_size && ctx->total_size != size) {
		return -EINVAL;
	}

	if (ctx->current > 0 && GET_BLOCK_SIZE(block) > ctx->block_size) {
		return -EINVAL;
	}

	if (ctx->total_size && new_current > ctx->total_size) {
		return -EINVAL;
	}

	if (size) {
		ctx->total_size = size;
	}
	ctx->current = new_current;
	ctx->block_size = min(GET_BLOCK_SIZE(block), ctx->block_size);

	return 0;
}

static int update_control_block1(struct coap_block_context *ctx,
				     int block, int size)
{
	size_t new_current = GET_NUM(block) << (GET_BLOCK_SIZE(block) + 4);

	if (block == -ENOENT) {
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

static int update_control_block2(struct coap_block_context *ctx,
				 int block, int size)
{
	size_t new_current = GET_NUM(block) << (GET_BLOCK_SIZE(block) + 4);

	if (block == -ENOENT) {
		return 0;
	}

	if (GET_MORE(block)) {
		return -EINVAL;
	}

	if (GET_NUM(block) > 0 && GET_BLOCK_SIZE(block) != ctx->block_size) {
		return -EINVAL;
	}

	ctx->current = new_current;
	ctx->block_size = min(GET_BLOCK_SIZE(block), ctx->block_size);

	return 0;
}

int coap_update_from_block(const struct coap_packet *cpkt,
			   struct coap_block_context *ctx)
{
	int r, block1, block2, size1, size2;

	block1 = get_block_option(cpkt, COAP_OPTION_BLOCK1);
	block2 = get_block_option(cpkt, COAP_OPTION_BLOCK2);
	size1 = get_block_option(cpkt, COAP_OPTION_SIZE1);
	size2 = get_block_option(cpkt, COAP_OPTION_SIZE2);

	size1 = size1 == -ENOENT ? 0 : size1;
	size2 = size2 == -ENOENT ? 0 : size2;

	if (is_request(cpkt)) {
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

size_t coap_next_block(const struct coap_packet *cpkt,
		       struct coap_block_context *ctx)
{
	int block;
	if (is_request(cpkt)) {
		block = get_block_option(cpkt, COAP_OPTION_BLOCK1);
	} else {
		block = get_block_option(cpkt, COAP_OPTION_BLOCK2);
	}

	if (!GET_MORE(block)) {
		return 0;
	}

	ctx->current += coap_block_size_to_bytes(ctx->block_size);

	return ctx->current;
}

u8_t *coap_next_token(void)
{
	static u32_t rand[2];

	rand[0] = sys_rand32_get();
	rand[1] = sys_rand32_get();

	return (u8_t *) rand;
}
