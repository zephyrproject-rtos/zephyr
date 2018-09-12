/*
 * Copyright (c) 2017 Intel Corporation
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

#include <misc/byteorder.h>
#include <net/buf.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>

#include <net/coap.h>

struct option_context {
	u16_t delta;
	u16_t offset;
	struct net_buf *frag;
};

#define COAP_VERSION 1

#define COAP_MARKER 0xFF

#define BASIC_HEADER_SIZE 4

#define PKT_WAIT_TIME K_SECONDS(1)

/* Values as per RFC 7252, section-3.1.
 *
 * Option Delta/Length: 4-bit unsigned integer. A value between 0 and
 * 12 indicates the Option Delta/Length.  Three values are reserved for
 * special constructs:
 * 13: An 8-bit unsigned integer precedes the Option Value and indicates
 *     the Option Delta/Length minus 13.
 * 14: A 16-bit unsigned integer in network byte order precedes the
 *     Option Value and indicates the Option Delta/Length minus 269.
 * 15: Reserved for future use.
 */
#define COAP_OPTION_NO_EXT 12 /* Option's Delta/Length without extended data */
#define COAP_OPTION_EXT_13 13
#define COAP_OPTION_EXT_14 14
#define COAP_OPTION_EXT_15 15
#define COAP_OPTION_EXT_269 269

static u8_t option_header_get_delta(u8_t opt)
{
	return (opt & 0xF0) >> 4;
}

static u8_t option_header_get_len(u8_t opt)
{
	return opt & 0x0F;
}

static void option_header_set_delta(u8_t *opt, u8_t delta)
{
	*opt = (delta & 0xF) << 4;
}

static void option_header_set_len(u8_t *opt, u8_t len)
{
	*opt |= (len & 0xF);
}

static u16_t get_coap_packet_len(struct net_pkt *pkt)
{
	u16_t len;

	/* TODO: verify with smaller packets */
	len = net_pkt_get_len(pkt)
	      - net_pkt_ip_hdr_len(pkt)
	      - net_pkt_ipv6_ext_len(pkt)
	      - NET_UDPH_LEN;

	return len;
}

static int check_frag_read_status(const struct net_buf *frag, u16_t offset)
{
	if (!frag && offset == 0xffff) {
		return -EINVAL;
	} else if (!frag && offset == 0) {
		return 0;
	}

	return 1;
}

static int decode_delta(struct option_context *context, u16_t opt,
			u16_t *opt_ext, u16_t *hdr_len)
{
	int ret = 0;

	if (opt == COAP_OPTION_EXT_13) {
		u8_t val;

		*hdr_len = 1;
		context->frag = net_frag_read_u8(context->frag,
						 context->offset,
						 &context->offset,
						 &val);
		ret = check_frag_read_status(context->frag,
					     context->offset);
		if (ret < 0) {
			return -EINVAL;
		}

		opt = val + COAP_OPTION_EXT_13;
	} else if (opt == COAP_OPTION_EXT_14) {
		u16_t val;

		*hdr_len = 2;
		context->frag = net_frag_read_be16(context->frag,
						   context->offset,
						   &context->offset,
						   &val);
		ret = check_frag_read_status(context->frag,
					     context->offset);
		if (ret < 0) {
			return -EINVAL;
		}

		opt = val + COAP_OPTION_EXT_269;
	} else if (opt == COAP_OPTION_EXT_15) {
		return -EINVAL;
	}

	*opt_ext = opt;

	return ret;
}

static int parse_option(const struct coap_packet *cpkt,
			struct option_context *context,
			struct coap_option *option,
			u16_t *opt_len)
{
	u16_t hdr_len;
	u16_t delta;
	u16_t len;
	u8_t opt;
	int r;

	context->frag = net_frag_read_u8(context->frag,
					 context->offset,
					 &context->offset,
					 &opt);
	r = check_frag_read_status(context->frag, context->offset);
	if (r < 0) {
		return r;
	}

	*opt_len += 1;

	/* This indicates that options have ended */
	if (opt == COAP_MARKER) {
		/* packet w/ marker but no payload is malformed */
		return r > 0 ? 0 : -EINVAL;
	}

	delta = option_header_get_delta(opt);
	len = option_header_get_len(opt);

	/* r == 0 means no more data to read from fragment, but delta
	 * field shows that packet should contain more data, it must
	 * be a malformed packet.
	 */
	if (r == 0 && delta > COAP_OPTION_NO_EXT) {
		return -EINVAL;
	}

	if (delta > COAP_OPTION_NO_EXT) {
		/* In case 'delta' doesn't fit the option fixed header. */
		r = decode_delta(context, delta, &delta, &hdr_len);
		if ((r < 0) || (r == 0 && len > COAP_OPTION_NO_EXT)) {
			return -EINVAL;
		}

		*opt_len += hdr_len;
	}

	if (len > COAP_OPTION_NO_EXT) {
		/* In case 'len' doesn't fit the option fixed header. */
		r = decode_delta(context, len, &len, &hdr_len);
		if (r < 0) {
			return -EINVAL;
		}

		*opt_len += hdr_len;
	}

	*opt_len += len;

	if (r == 0) {
		if (len == 0) {
			context->delta += delta;
			return r;
		}

		/* r == 0 means no more data to read from fragment, but len
		 * field shows that packet should contain more data, it must
		 * be a malformed packet.
		 */
		return -EINVAL;
	}

	if (option) {
		/*
		 * Make sure the option data will fit into the value field of
		 * coap_option.
		 * NOTE: To expand the size of the value field set:
		 * CONFIG_COAP_EXTENDED_OPTIONS_LEN=y
		 * CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE=<size>
		 */
		if (len > sizeof(option->value)) {
			NET_ERR("%u is > sizeof(coap_option->value)(%zu)!",
				len, sizeof(option->value));
			return -ENOMEM;
		}

		option->delta = context->delta + delta;
		option->len = len;
		context->frag = net_frag_read(context->frag, context->offset,
					      &context->offset, len,
					      &option->value[0]);
	} else {
		context->frag = net_frag_skip(context->frag, context->offset,
					      &context->offset, len);
	}

	r = check_frag_read_status(context->frag, context->offset);
	if (r < 0) {
		return r;
	}

	context->delta += delta;

	return r;
}

static int parse_options(const struct coap_packet *cpkt,
			 struct coap_option *options, u8_t opt_num)
{
	struct option_context context = {
					.delta = 0,
					.frag = NULL,
					.offset = 0
					};
	u16_t opt_len;
	u8_t num;
	int r;

	/* Skip CoAP header */
	context.frag = net_frag_skip(cpkt->frag, cpkt->offset,
				     &context.offset, cpkt->hdr_len);
	r = check_frag_read_status(context.frag, context.offset);
	if (r <= 0) {
		return r;
	}

	num = 0;
	opt_len = 0;

	while (true) {
		struct coap_option *option;

		option = num < opt_num ? &options[num++] : NULL;
		r = parse_option(cpkt, &context, option, &opt_len);
		if (r <= 0) {
			break;
		}
	}

	if (r < 0) {
		return r;
	}

	return opt_len;
}

static u8_t get_header_tkl(const struct coap_packet *cpkt)
{
	struct net_buf *frag;
	u16_t offset;
	u8_t tkl;

	frag = net_frag_read_u8(cpkt->frag, cpkt->offset, &offset, &tkl);

	return tkl & 0xF;
}

static u16_t get_pkt_len(const struct coap_packet *cpkt)
{
	struct net_buf *frag;
	u16_t len;

	len = cpkt->frag->len - cpkt->offset;

	for (frag = cpkt->frag->frags; frag; frag = frag->frags) {
		len += frag->len;
	}

	return len;
}

static int get_header_len(struct coap_packet *cpkt)
{
	u8_t tkl;
	u16_t len;
	int hdrlen;

	len = get_pkt_len(cpkt);

	hdrlen = BASIC_HEADER_SIZE;

	if (len < hdrlen) {
		return -EINVAL;
	}

	tkl = get_header_tkl(cpkt);

	/* Token lenghts 9-15 are reserved. */
	if (tkl > 8) {
		return -EINVAL;
	}

	if (len < hdrlen + tkl) {
		return -EINVAL;
	}

	cpkt->hdr_len =  hdrlen + tkl;

	return 0;
}

int coap_packet_parse(struct coap_packet *cpkt, struct net_pkt *pkt,
		      struct coap_option *options, u8_t opt_num)
{
	int ret;

	if (!cpkt || !pkt || !pkt->frags) {
		return -EINVAL;
	}

	cpkt->pkt = pkt;
	cpkt->hdr_len = 0;
	cpkt->opt_len = 0;

	cpkt->frag = net_frag_skip(pkt->frags, 0, &cpkt->offset,
				   net_pkt_ip_hdr_len(pkt) +
				   NET_UDPH_LEN +
				   net_pkt_ipv6_ext_len(pkt));
	ret = check_frag_read_status(cpkt->frag, cpkt->offset);
	if (ret <= 0) {
		return ret;
	}

	ret = get_header_len(cpkt);
	if (ret < 0) {
		return -EINVAL;
	}

	ret = parse_options(cpkt, options, opt_num);
	if (ret < 0) {
		return -EINVAL;
	}

	cpkt->opt_len = ret;

	return 0;
}

#define append(pkt, type, value)					\
	do {								\
		if (!net_pkt_append_##type##_timeout(pkt, value,	\
						     PKT_WAIT_TIME)) {	\
			return -ENOMEM;					\
		}							\
	} while (0)

#define append_all(pkt, size, value)					\
	do {								\
		if (!net_pkt_append_all(pkt, size, value,		\
					PKT_WAIT_TIME)) {		\
			return -ENOMEM;					\
		}							\
	} while (0)

int coap_packet_init(struct coap_packet *cpkt, struct net_pkt *pkt,
		     u8_t ver, u8_t type, u8_t tokenlen,
		     u8_t *token, u8_t code, u16_t id)
{
	u8_t hdr;
	bool res;

	if (!cpkt || !pkt || !pkt->frags) {
		return -EINVAL;
	}

	(void)memset(cpkt, 0, sizeof(*cpkt));
	cpkt->pkt = pkt;
	cpkt->frag = pkt->frags;
	cpkt->offset = 0;
	cpkt->last_delta = 0;

	hdr = (ver & 0x3) << 6;
	hdr |= (type & 0x3) << 4;
	hdr |= tokenlen & 0xF;

	append(pkt, u8, hdr);
	append(pkt, u8, code);
	append(pkt, be16, id);

	if (token && tokenlen) {
		res = net_pkt_append_all(pkt, tokenlen, token, PKT_WAIT_TIME);
		if (!res) {
			return -ENOMEM;
		}
	}

	/* Header length : (version + type + tkl) + code + id + [token] */
	cpkt->hdr_len = 1 + 1 + 2 + tokenlen;

	return 0;
}

int coap_pending_init(struct coap_pending *pending,
		      const struct coap_packet *request,
		      const struct sockaddr *addr)
{
	(void)memset(pending, 0, sizeof(*pending));
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

/* TODO: random generated initial ACK timeout
 * ACK_TIMEOUT < INIT_ACK_TIMEOUT < ACK_TIMEOUT * ACK_RANDOM_FACTOR
 * where ACK_TIMEOUT = 2 and ACK_RANDOM_FACTOR = 1.5 by default
 * Ref: https://tools.ietf.org/html/rfc7252#section-4.8
 */
#define INIT_ACK_TIMEOUT	CONFIG_COAP_INIT_ACK_TIMEOUT_MS

static s32_t next_timeout(s32_t previous)
{
	switch (previous) {
	case INIT_ACK_TIMEOUT:
	case (INIT_ACK_TIMEOUT * 2):
	case (INIT_ACK_TIMEOUT * 4):
		return previous << 1;
	case (INIT_ACK_TIMEOUT * 8):
		/* equal value is returned to end retransmit */
		return previous;
	}

	/* initial or unrecognized */
	return INIT_ACK_TIMEOUT;
}

bool coap_pending_cycle(struct coap_pending *pending)
{
	s32_t old = pending->timeout;
	bool cont;

	pending->timeout = next_timeout(pending->timeout);

	/* If the timeout changed, it's not the last, continue... */
	cont = (old != pending->timeout);
	if (cont) {
		/* When it it is the last retransmission, the buffer
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
			const char * const *path,
			struct coap_option *options,
			u8_t opt_num)
{
	u8_t i;
	u8_t j = 0;

	for (i = 0; i < opt_num && path[j]; i++) {
		if (options[i].delta != COAP_OPTION_URI_PATH) {
			continue;
		}

		if (options[i].len != strlen(path[j])) {
			return false;
		}

		if (memcmp(options[i].value, path[j], options[i].len)) {
			return false;
		}

		j++;
	}

	if (path[j]) {
		return false;
	}

	for (; i < opt_num; i++) {
		if (options[i].delta == COAP_OPTION_URI_PATH) {
			return false;
		}
	}

	return true;
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
			struct coap_option *options,
			u8_t opt_num)
{
	struct coap_resource *resource;

	if (!is_request(cpkt)) {
		return 0;
	}

	/* FIXME: deal with hierarchical resources */
	for (resource = resources; resource && resource->path; resource++) {
		coap_method_t method;
		u8_t code;

		if (!uri_path_eq(cpkt, resource->path, options, opt_num)) {
			continue;
		}

		code = coap_header_get_code(cpkt);
		method = method_from_code(resource, code);
		if (!method) {
			return 0;
		}

		return method(resource, cpkt);
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
	u8_t token[8];
	u16_t id;
	u8_t tkl;
	size_t i;

	id = coap_header_get_id(response);
	tkl = coap_header_get_token(response, (u8_t *)token);

	for (i = 0, r = replies; i < len; i++, r++) {
		int age;

		if ((r->id == 0) && (r->tkl == 0)) {
			continue;
		}

		/* Piggybacked must match id when token is empty */
		if ((r->id != id) && (tkl == 0)) {
			continue;
		}

		if (tkl > 0 && memcmp(r->token, token, tkl)) {
			continue;
		}

		age = get_observe_option(response);
		if (age > 0) {
			/* age == 2 means that the notifications wrapped,
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
	u8_t token[8];
	u8_t tkl;
	int age;

	reply->id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, (u8_t *)&token);

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
	(void)memset(reply, 0, sizeof(*reply));
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
	u8_t token[8];
	u8_t tkl;

	tkl = coap_header_get_token(request, (u8_t *)&token);

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
	/* FIXME: Should we consider ipv6-mapped ipv4 addresses as equal to
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

int coap_packet_append_payload_marker(struct coap_packet *cpkt)
{
	return net_pkt_append_u8_timeout(cpkt->pkt, COAP_MARKER,
					 PKT_WAIT_TIME) ? 0 : -EINVAL;
}

int coap_packet_append_payload(struct coap_packet *cpkt, u8_t *payload,
			       u16_t payload_len)
{
	bool status;

	status = net_pkt_append_all(cpkt->pkt, payload_len, payload,
				    PKT_WAIT_TIME);

	return status ? 0 : -EINVAL;
}

static u8_t encode_extended_option(u16_t num, u8_t *opt, u16_t *ext)
{
	if (num < COAP_OPTION_EXT_13) {
		*opt = num;
		*ext = 0;

		return 0;
	} else if (num < COAP_OPTION_EXT_269) {
		*opt = COAP_OPTION_EXT_13;
		*ext = num - COAP_OPTION_EXT_13;

		return 1;
	}

	*opt = COAP_OPTION_EXT_14;
	*ext = num - COAP_OPTION_EXT_269;

	return 2;
}

static int encode_option(struct coap_packet *cpkt, u16_t code,
			 const u8_t *value, u16_t len)
{
	u16_t delta_ext; /* Extended delta */
	u16_t len_ext; /* Extended length */
	u8_t opt; /* delta | len */
	u8_t opt_delta;
	u8_t opt_len;
	u8_t delta_size;
	u8_t len_size;
	bool res;

	delta_size = encode_extended_option(code, &opt_delta, &delta_ext);
	len_size = encode_extended_option(len, &opt_len, &len_ext);

	option_header_set_delta(&opt, opt_delta);
	option_header_set_len(&opt, opt_len);

	append(cpkt->pkt, u8, opt);

	if (delta_size == 1) {
		append(cpkt->pkt, u8, (u8_t) delta_ext);
	} else if (delta_size == 2) {
		append(cpkt->pkt, be16, delta_ext);
	}

	if (len_size == 1) {
		append(cpkt->pkt, u8, (u8_t) len_ext);
	} else if (delta_size == 2) {
		append(cpkt->pkt, be16, len_ext);
	}

	if (len && value) {
		res = net_pkt_append_all(cpkt->pkt, len, value, PKT_WAIT_TIME);
		if (!res) {
			return -EINVAL;
		}
	}

	return  (1 + delta_size + len_size + len);
}

/* TODO Add support for inserting options in proper place
 * and modify other option's delta accordingly.
 */
int coap_packet_append_option(struct coap_packet *cpkt, u16_t code,
			      const u8_t *value, u16_t len)
{
	struct net_buf *frag;
	u16_t offset;
	int r;

	if (!cpkt) {
		return -EINVAL;
	}

	if (len && !value) {
		return -EINVAL;
	}

	if (code < cpkt->last_delta) {
		NET_ERR("Options should be in ascending order");
		return -EINVAL;
	}

	/* Skip CoAP packet header */
	frag = net_frag_skip(cpkt->frag, cpkt->offset, &offset, cpkt->hdr_len);
	if (!frag && offset == 0xffff) {
		return -EINVAL;
	}

	/* Calculate delta, if this option is not the first one */
	if (cpkt->opt_len) {
		code = (code == cpkt->last_delta) ? 0 :
			code - cpkt->last_delta;
	}

	r = encode_option(cpkt, code, value, len);
	if (r < 0) {
		return -EINVAL;
	}

	cpkt->opt_len += r;
	cpkt->last_delta += code;

	return 0;
}

int coap_append_option_int(struct coap_packet *cpkt, u16_t code,
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

	return coap_packet_append_option(cpkt, code, data, len);
}

int coap_find_options(const struct coap_packet *cpkt, u16_t code,
		      struct coap_option *options, u16_t veclen)
{
	struct option_context context = {
					  .delta = 0,
					  .frag = NULL,
					  .offset = 0
					};
	u16_t opt_len;
	int count;
	int r;

	if (!cpkt || !cpkt->pkt || !cpkt->pkt->frags || !cpkt->hdr_len) {
		return -EINVAL;
	}

	/* Skip CoAP header */
	context.frag = net_frag_skip(cpkt->frag, cpkt->offset,
				     &context.offset, cpkt->hdr_len);
	r = check_frag_read_status(context.frag, context.offset);
	if (r <= 0) {
		return r;
	}

	opt_len = 0;
	count = 0;

	while (context.delta <= code && count < veclen) {
		r = parse_option(cpkt, &context, &options[count], &opt_len);
		if (r < 0) {
			return -EINVAL;
		}

		if (code == options[count].delta) {
			count++;
		}

		if (r == 0) {
			break;
		}
	}

	return count;
}

u8_t coap_header_get_version(const struct coap_packet *cpkt)
{
	struct net_buf *frag;
	u16_t offset;
	u8_t version;

	frag = net_frag_read_u8(cpkt->frag, cpkt->offset, &offset, &version);
	if (!frag && offset == 0xffff) {
		return 0;
	}

	return (version & 0xC0) >> 6;
}

u8_t coap_header_get_type(const struct coap_packet *cpkt)
{
	struct net_buf *frag;
	u16_t offset;
	u8_t type;

	frag = net_frag_read_u8(cpkt->frag, cpkt->offset, &offset, &type);
	if (!frag && offset == 0xffff) {
		return 0;
	}

	return (type & 0x30) >> 4;
}

static u8_t __coap_header_get_code(const struct coap_packet *cpkt)
{
	struct net_buf *frag;
	u16_t offset;
	u8_t code;

	frag = net_frag_skip(cpkt->frag, cpkt->offset, &offset, 1);
	frag = net_frag_read_u8(frag, offset, &offset, &code);
	if (!frag && offset == 0xffff) {
		return 0;
	}

	return code;
}

u8_t coap_header_get_token(const struct coap_packet *cpkt, u8_t *token)
{
	struct net_buf *frag;
	u16_t offset;
	u8_t tkl;

	if (!cpkt || !token) {
		return 0;
	}

	tkl = get_header_tkl(cpkt);
	if (!tkl) {
		return 0;
	}

	frag = net_frag_skip(cpkt->frag, cpkt->offset, &offset,
			     BASIC_HEADER_SIZE);
	frag = net_frag_read(frag, offset, &offset, tkl, token);
	if (!frag && offset == 0xffff) {
		return 0;
	}

	return tkl;
}

u8_t coap_header_get_code(const struct coap_packet *cpkt)
{
	u8_t code = __coap_header_get_code(cpkt);

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
	struct net_buf *frag;
	u16_t offset;
	u16_t id;

	frag = net_frag_skip(cpkt->frag, cpkt->offset, &offset, 2);
	frag = net_frag_read_be16(frag, offset, &offset, &id);
	if (!frag && offset == 0xffff) {
		return 0;
	}

	return id;
}

struct net_buf *coap_packet_get_payload(const struct coap_packet *cpkt,
					u16_t *offset, u16_t *len)
{
	struct net_buf *frag = NULL;
	u16_t coap_pkt_len;

	if (!cpkt || !cpkt->pkt || !offset || !len) {
		return NULL;
	}

	*offset = 0xffff;
	*len = 0;

	coap_pkt_len = get_coap_packet_len(cpkt->pkt);

	frag = net_frag_skip(cpkt->frag, cpkt->offset, offset,
			     cpkt->hdr_len + cpkt->opt_len);
	*len = coap_pkt_len - cpkt->hdr_len - cpkt->opt_len;

	return frag;
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

int coap_append_block1_option(struct coap_packet *cpkt,
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

	r = coap_append_option_int(cpkt, COAP_OPTION_BLOCK1, val);

	return r;
}

int coap_append_block2_option(struct coap_packet *cpkt,
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

	r = coap_append_option_int(cpkt, COAP_OPTION_BLOCK2, val);

	return r;
}

int coap_append_size1_option(struct coap_packet *cpkt,
			     struct coap_block_context *ctx)
{
	return coap_append_option_int(cpkt, COAP_OPTION_SIZE1, ctx->total_size);
}

int coap_append_size2_option(struct coap_packet *cpkt,
			     struct coap_block_context *ctx)
{
	return coap_append_option_int(cpkt, COAP_OPTION_SIZE2, ctx->total_size);
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
