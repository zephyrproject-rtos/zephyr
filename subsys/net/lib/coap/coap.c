/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <random/rand32.h>
#include <sys/atomic.h>
#include <sys/util.h>

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <sys/math_extras.h>

#include <net/net_ip.h>
#include <net/net_core.h>
#include <net/coap.h>

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

/* CoAP Payload Marker */
#define COAP_MARKER		0xFF

#define BASIC_HEADER_SIZE	4

/* The CoAP message ID that is incremented each time coap_next_id() is called. */
static uint16_t message_id;

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

int coap_packet_init(struct coap_packet *cpkt, uint8_t *data, uint16_t max_len,
		     uint8_t ver, uint8_t type, uint8_t token_len,
		     const uint8_t *token, uint8_t code, uint16_t id)
{
	uint8_t hdr;
	bool res;

	if (!cpkt || !data || !max_len) {
		return -EINVAL;
	}

	memset(cpkt, 0, sizeof(*cpkt));

	cpkt->data = data;
	cpkt->offset = 0U;
	cpkt->max_len = max_len;
	cpkt->delta = 0U;

	hdr = (ver & 0x3) << 6;
	hdr |= (type & 0x3) << 4;
	hdr |= token_len & 0xF;

	res = append_u8(cpkt, hdr);
	if (!res) {
		return -EINVAL;
	}

	res = append_u8(cpkt, code);
	if (!res) {
		return -EINVAL;
	}

	res = append_be16(cpkt, id);
	if (!res) {
		return -EINVAL;
	}

	if (token && token_len) {
		res = append(cpkt, token, token_len);
		if (!res) {
			return -EINVAL;
		}
	}

	/* Header length : (version + type + tkl) + code + id + [token] */
	cpkt->hdr_len = 1 + 1 + 2 + token_len;

	return 0;
}

int coap_ack_init(struct coap_packet *cpkt, const struct coap_packet *req,
		  uint8_t *data, uint16_t max_len, uint8_t code)
{
	uint16_t id;
	uint8_t ver;
	uint8_t tkl;
	uint8_t token[COAP_TOKEN_MAX_LEN];

	ver = coap_header_get_version(req);
	id = coap_header_get_id(req);
	tkl = code ? coap_header_get_token(req, token) : 0;

	return coap_packet_init(cpkt, data, max_len, ver, COAP_TYPE_ACK, tkl,
				token, code, id);
}

static void option_header_set_delta(uint8_t *opt, uint8_t delta)
{
	*opt = (delta & 0xF) << 4;
}

static void option_header_set_len(uint8_t *opt, uint8_t len)
{
	*opt |= (len & 0xF);
}

static uint8_t encode_extended_option(uint16_t num, uint8_t *opt, uint16_t *ext)
{
	if (num < COAP_OPTION_EXT_13) {
		*opt = num;
		*ext = 0U;

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

static int encode_option(struct coap_packet *cpkt, uint16_t code,
			 const uint8_t *value, uint16_t len)
{
	uint16_t delta_ext; /* Extended delta */
	uint16_t len_ext; /* Extended length */
	uint8_t opt; /* delta | len */
	uint8_t opt_delta;
	uint8_t opt_len;
	uint8_t delta_size;
	uint8_t len_size;
	bool res;

	delta_size = encode_extended_option(code, &opt_delta, &delta_ext);
	len_size = encode_extended_option(len, &opt_len, &len_ext);

	option_header_set_delta(&opt, opt_delta);
	option_header_set_len(&opt, opt_len);

	res = append_u8(cpkt, opt);
	if (!res) {
		return -EINVAL;
	}

	if (delta_size == 1U) {
		res = append_u8(cpkt, (uint8_t)delta_ext);
		if (!res) {
			return -EINVAL;
		}
	} else if (delta_size == 2U) {
		res = append_be16(cpkt, delta_ext);
		if (!res) {
			return -EINVAL;
		}
	}

	if (len_size == 1U) {
		res = append_u8(cpkt, (uint8_t)len_ext);
		if (!res) {
			return -EINVAL;
		}
	} else if (len_size == 2U) {
		res = append_be16(cpkt, len_ext);
		if (!res) {
			return -EINVAL;
		}
	}

	if (len && value) {
		res = append(cpkt, value, len);
		if (!res) {
			return -EINVAL;
		}
	}

	return  (1 + delta_size + len_size + len);
}

/* TODO Add support for inserting options in proper place
 * and modify other option's delta accordingly.
 */
int coap_packet_append_option(struct coap_packet *cpkt, uint16_t code,
			      const uint8_t *value, uint16_t len)
{
	int r;

	if (!cpkt) {
		return -EINVAL;
	}

	if (len && !value) {
		return -EINVAL;
	}

	if (code < cpkt->delta) {
		NET_ERR("Options should be in ascending order");
		return -EINVAL;
	}

	/* Calculate delta, if this option is not the first one */
	if (cpkt->opt_len) {
		code = (code == cpkt->delta) ? 0 : code - cpkt->delta;
	}

	r = encode_option(cpkt, code, value, len);
	if (r < 0) {
		return -EINVAL;
	}

	cpkt->opt_len += r;
	cpkt->delta += code;

	return 0;
}

int coap_append_option_int(struct coap_packet *cpkt, uint16_t code,
			   unsigned int val)
{
	uint8_t data[4], len;

	if (val == 0U) {
		data[0] = 0U;
		len = 0U;
	} else if (val < 0xFF) {
		data[0] = (uint8_t) val;
		len = 1U;
	} else if (val < 0xFFFF) {
		sys_put_be16(val, data);
		len = 2U;
	} else if (val < 0xFFFFFF) {
		sys_put_be16(val, &data[1]);
		data[0] = val >> 16;
		len = 3U;
	} else {
		sys_put_be32(val, data);
		len = 4U;
	}

	return coap_packet_append_option(cpkt, code, data, len);
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
		return (option->value[3] << 0) | (option->value[2] << 8) |
			(option->value[1] << 16) | (option->value[0] << 24);
	default:
		return 0;
	}

	return 0;
}

int coap_packet_append_payload_marker(struct coap_packet *cpkt)
{
	return append_u8(cpkt, COAP_MARKER) ? 0 : -EINVAL;
}

int coap_packet_append_payload(struct coap_packet *cpkt, const uint8_t *payload,
			       uint16_t payload_len)
{
	return append(cpkt, payload, payload_len) ? 0 : -EINVAL;
}

uint8_t *coap_next_token(void)
{
	static uint32_t rand[
		ceiling_fraction(COAP_TOKEN_MAX_LEN, sizeof(uint32_t))];

	for (size_t i = 0; i < ARRAY_SIZE(rand); ++i) {
		rand[i] = sys_rand32_get();
	}

	return (uint8_t *) rand;
}

static uint8_t option_header_get_delta(uint8_t opt)
{
	return (opt & 0xF0) >> 4;
}

static uint8_t option_header_get_len(uint8_t opt)
{
	return opt & 0x0F;
}

static int read_u8(uint8_t *data, uint16_t offset, uint16_t *pos,
		   uint16_t max_len, uint8_t *value)
{
	if (max_len - offset < 1) {
		return -EINVAL;
	}

	*value = data[offset++];
	*pos = offset;

	return max_len - offset;
}

static int read_be16(uint8_t *data, uint16_t offset, uint16_t *pos,
		     uint16_t max_len, uint16_t *value)
{
	if (max_len - offset < 2) {
		return -EINVAL;
	}

	*value = data[offset++] << 8;
	*value |= data[offset++];
	*pos = offset;

	return max_len - offset;
}

static int read(uint8_t *data, uint16_t offset, uint16_t *pos,
		uint16_t max_len, uint16_t len, uint8_t *value)
{
	if (max_len - offset < len) {
		return -EINVAL;
	}

	memcpy(value, data + offset, len);
	offset += len;
	*pos = offset;

	return max_len - offset;
}

static int decode_delta(uint8_t *data, uint16_t offset, uint16_t *pos, uint16_t max_len,
			uint16_t opt, uint16_t *opt_ext, uint16_t *hdr_len)
{
	int ret = 0;

	if (opt == COAP_OPTION_EXT_13) {
		uint8_t val;

		*hdr_len = 1U;

		ret = read_u8(data, offset, pos, max_len, &val);
		if (ret < 0) {
			return -EINVAL;
		}

		opt = val + COAP_OPTION_EXT_13;
	} else if (opt == COAP_OPTION_EXT_14) {
		uint16_t val;

		*hdr_len = 2U;

		ret = read_be16(data, offset, pos, max_len, &val);
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

static int parse_option(uint8_t *data, uint16_t offset, uint16_t *pos,
			uint16_t max_len, uint16_t *opt_delta, uint16_t *opt_len,
			struct coap_option *option)
{
	uint16_t hdr_len;
	uint16_t delta;
	uint16_t len;
	uint8_t opt;
	int r;

	r = read_u8(data, offset, pos, max_len, &opt);
	if (r < 0) {
		return r;
	}

	*opt_len += 1U;

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
		r = decode_delta(data, *pos, pos, max_len,
				 delta, &delta, &hdr_len);
		if ((r < 0) || (r == 0 && len > COAP_OPTION_NO_EXT)) {
			return -EINVAL;
		}

		if (u16_add_overflow(*opt_len, hdr_len, opt_len)) {
			return -EINVAL;
		}
	}

	if (len > COAP_OPTION_NO_EXT) {
		/* In case 'len' doesn't fit the option fixed header. */
		r = decode_delta(data, *pos, pos, max_len,
				 len, &len, &hdr_len);
		if (r < 0) {
			return -EINVAL;
		}

		if (u16_add_overflow(*opt_len, hdr_len, opt_len)) {
			return -EINVAL;
		}
	}

	if (u16_add_overflow(*opt_delta, delta, opt_delta) ||
	    u16_add_overflow(*opt_len, len, opt_len)) {
		return -EINVAL;
	}

	if (r == 0 && len != 0U) {
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
			return -EINVAL;
		}

		option->delta = *opt_delta;
		option->len = len;
		r = read(data, *pos, pos, max_len, len, &option->value[0]);
		if (r < 0) {
			return -EINVAL;
		}
	} else {
		if (u16_add_overflow(*pos, len, pos)) {
			return -EINVAL;
		}

		r = max_len - *pos;
	}

	return r;
}

int coap_packet_parse(struct coap_packet *cpkt, uint8_t *data, uint16_t len,
		      struct coap_option *options, uint8_t opt_num)
{
	uint16_t opt_len;
	uint16_t offset;
	uint16_t delta;
	uint8_t num;
	uint8_t tkl;
	int ret;

	if (!cpkt || !data) {
		return -EINVAL;
	}

	if (len < BASIC_HEADER_SIZE) {
		return -EINVAL;
	}

	if (options) {
		memset(options, 0, opt_num * sizeof(struct coap_option));
	}

	cpkt->data = data;
	cpkt->offset = 0U;
	cpkt->max_len = len;
	cpkt->opt_len = 0U;
	cpkt->hdr_len = 0U;
	cpkt->delta = 0U;

	/* Token lengths 9-15 are reserved. */
	tkl = cpkt->data[0] & 0x0f;
	if (tkl > 8) {
		return -EINVAL;
	}

	cpkt->hdr_len = BASIC_HEADER_SIZE + tkl;
	if (cpkt->hdr_len > len) {
		return -EINVAL;
	}

	cpkt->offset = cpkt->hdr_len;
	if (cpkt->hdr_len == len) {
		return 0;
	}

	offset = cpkt->offset;
	opt_len = 0U;
	delta = 0U;
	num = 0U;

	while (1) {
		struct coap_option *option;

		option = num < opt_num ? &options[num++] : NULL;
		ret = parse_option(cpkt->data, offset, &offset, cpkt->max_len,
				   &delta, &opt_len, option);
		if (ret < 0) {
			return ret;
		} else if (ret == 0) {
			break;
		}
	}

	cpkt->opt_len = opt_len;
	cpkt->delta = delta;
	cpkt->offset = offset;

	return 0;
}

int coap_find_options(const struct coap_packet *cpkt, uint16_t code,
		      struct coap_option *options, uint16_t veclen)
{
	uint16_t opt_len;
	uint16_t offset;
	uint16_t delta;
	uint8_t num;
	int r;

	offset = cpkt->hdr_len;
	opt_len = 0U;
	delta = 0U;
	num = 0U;

	while (delta <= code && num < veclen) {
		r = parse_option(cpkt->data, offset, &offset,
				 cpkt->max_len, &delta, &opt_len,
				 &options[num]);
		if (r < 0) {
			return -EINVAL;
		}

		if (code == options[num].delta) {
			num++;
		}

		if (r == 0) {
			break;
		}
	}

	return num;
}

uint8_t coap_header_get_version(const struct coap_packet *cpkt)
{
	if (!cpkt || !cpkt->data) {
		return 0;
	}

	return (cpkt->data[0] & 0xC0) >> 6;
}

uint8_t coap_header_get_type(const struct coap_packet *cpkt)
{
	if (!cpkt || !cpkt->data) {
		return 0;
	}

	return (cpkt->data[0] & 0x30) >> 4;
}

static uint8_t __coap_header_get_code(const struct coap_packet *cpkt)
{
	if (!cpkt || !cpkt->data) {
		return 0;
	}

	return cpkt->data[1];
}

uint8_t coap_header_get_token(const struct coap_packet *cpkt, uint8_t *token)
{
	uint8_t tkl;

	if (!cpkt || !cpkt->data) {
		return 0;
	}

	tkl = cpkt->data[0] & 0x0f;
	if (tkl) {
		memcpy(token, cpkt->data + BASIC_HEADER_SIZE, tkl);
	}

	return tkl;
}

uint8_t coap_header_get_code(const struct coap_packet *cpkt)
{
	uint8_t code = __coap_header_get_code(cpkt);

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

uint16_t coap_header_get_id(const struct coap_packet *cpkt)
{
	if (!cpkt || !cpkt->data) {
		return 0;
	}

	return (cpkt->data[2] << 8) | cpkt->data[3];
}

const uint8_t *coap_packet_get_payload(const struct coap_packet *cpkt, uint16_t *len)
{
	int payload_len;

	if (!cpkt || !len) {
		return NULL;
	}

	payload_len = cpkt->max_len - cpkt->hdr_len - cpkt->opt_len;
	if (payload_len > 0) {
		*len = payload_len;
	} else {
		*len = 0U;
	}

	return !(*len) ? NULL :
		cpkt->data + cpkt->hdr_len + cpkt->opt_len;
}

static bool uri_path_eq(const struct coap_packet *cpkt,
			const char * const *path,
			struct coap_option *options,
			uint8_t opt_num)
{
	uint8_t i;
	uint8_t j = 0U;

	for (i = 0U; i < opt_num && path[j]; i++) {
		if (options[i].delta != COAP_OPTION_URI_PATH) {
			continue;
		}

		if (IS_ENABLED(CONFIG_COAP_URI_WILDCARD) && strlen(path[j]) == 1) {
			if (*path[j] == '+') {
				/* Single-level wildcard */
				j++;
				continue;
			} else if (*path[j] == '#') {
				/* Multi-level wildcard */
				return true;
			}
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
				      uint8_t code)
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
	uint8_t code = coap_header_get_code(cpkt);

	return !(code & ~COAP_REQUEST_MASK);
}

int coap_handle_request(struct coap_packet *cpkt,
			struct coap_resource *resources,
			struct coap_option *options,
			uint8_t opt_num,
			struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_resource *resource;

	if (!is_request(cpkt)) {
		return 0;
	}

	/* FIXME: deal with hierarchical resources */
	for (resource = resources; resource && resource->path; resource++) {
		coap_method_t method;
		uint8_t code;

		if (!uri_path_eq(cpkt, resource->path, options, opt_num)) {
			continue;
		}

		code = coap_header_get_code(cpkt);
		method = method_from_code(resource, code);
		if (!method) {
			return -EPERM;
		}

		return method(resource, cpkt, addr, addr_len);
	}

	NET_DBG("%d", __LINE__);
	return -ENOENT;
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
	uint16_t bytes = coap_block_size_to_bytes(ctx->block_size);
	unsigned int val = 0U;
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
	uint16_t bytes = coap_block_size_to_bytes(ctx->block_size);

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

int coap_get_option_int(const struct coap_packet *cpkt, uint16_t code)
{
	struct coap_option option = {};
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
	ctx->block_size = MIN(GET_BLOCK_SIZE(block), ctx->block_size);

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
	ctx->block_size = MIN(GET_BLOCK_SIZE(block), ctx->block_size);

	return 0;
}

int coap_update_from_block(const struct coap_packet *cpkt,
			   struct coap_block_context *ctx)
{
	int r, block1, block2, size1, size2;

	block1 = coap_get_option_int(cpkt, COAP_OPTION_BLOCK1);
	block2 = coap_get_option_int(cpkt, COAP_OPTION_BLOCK2);
	size1 = coap_get_option_int(cpkt, COAP_OPTION_SIZE1);
	size2 = coap_get_option_int(cpkt, COAP_OPTION_SIZE2);

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
		block = coap_get_option_int(cpkt, COAP_OPTION_BLOCK1);
	} else {
		block = coap_get_option_int(cpkt, COAP_OPTION_BLOCK2);
	}

	if (!GET_MORE(block)) {
		return 0;
	}

	ctx->current += coap_block_size_to_bytes(ctx->block_size);

	return ctx->current;
}

int coap_pending_init(struct coap_pending *pending,
		      const struct coap_packet *request,
		      const struct sockaddr *addr,
		      uint8_t retries)
{
	memset(pending, 0, sizeof(*pending));

	pending->id = coap_header_get_id(request);

	memcpy(&pending->addr, addr, sizeof(*addr));

	pending->data = request->data;
	pending->len = request->offset;
	pending->t0 = k_uptime_get_32();
	pending->retries = retries;

	return 0;
}

struct coap_pending *coap_pending_next_unused(
	struct coap_pending *pendings, size_t len)
{
	struct coap_pending *p;
	size_t i;

	for (i = 0, p = pendings; i < len; i++, p++) {
		if (p->timeout == 0) {
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
		return net_ipv6_is_addr_unspecified(
			&(net_sin6(addr)->sin6_addr));
	} else if (addr->sa_family == AF_INET) {
		return net_sin(addr)->sin_addr.s4_addr32[0] == 0U;
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
	uint16_t resp_id = coap_header_get_id(response);
	size_t i;

	for (i = 0, p = pendings; i < len; i++, p++) {
		if (!p->timeout) {
			continue;
		}

		if (resp_id != p->id) {
			continue;
		}

		return p;
	}

	return NULL;
}

struct coap_pending *coap_pending_next_to_expire(
	struct coap_pending *pendings, size_t len)
{
	struct coap_pending *p, *found = NULL;
	size_t i;
	uint32_t expiry, min_expiry;

	for (i = 0, p = pendings; i < len; i++, p++) {
		if (!p->timeout) {
			continue;
		}

		expiry = p->t0 + p->timeout;

		if (!found || (int32_t)(expiry - min_expiry) < 0) {
			min_expiry = expiry;
			found = p;
		}
	}

	return found;
}

static uint32_t init_ack_timeout(void)
{
#if defined(CONFIG_COAP_RANDOMIZE_ACK_TIMEOUT)
	const uint32_t max_ack = CONFIG_COAP_INIT_ACK_TIMEOUT_MS *
				 COAP_DEFAULT_ACK_RANDOM_FACTOR;
	const uint32_t min_ack = CONFIG_COAP_INIT_ACK_TIMEOUT_MS;

	/* Randomly generated initial ACK timeout
	 * ACK_TIMEOUT < INIT_ACK_TIMEOUT < ACK_TIMEOUT * ACK_RANDOM_FACTOR
	 * Ref: https://tools.ietf.org/html/rfc7252#section-4.8
	 */
	return min_ack + (sys_rand32_get() % (max_ack - min_ack));
#else
	return CONFIG_COAP_INIT_ACK_TIMEOUT_MS;
#endif /* defined(CONFIG_COAP_RANDOMIZE_ACK_TIMEOUT) */
}

bool coap_pending_cycle(struct coap_pending *pending)
{
	if (pending->timeout == 0) {
		/* Initial transmission. */
		pending->timeout = init_ack_timeout();

		return true;
	}

	if (pending->retries == 0) {
		return false;
	}

	pending->t0 += pending->timeout;
	pending->timeout = pending->timeout << 1;
	pending->retries--;

	return true;
}

void coap_pending_clear(struct coap_pending *pending)
{
	pending->timeout = 0;
	pending->data = NULL;
}

void coap_pendings_clear(struct coap_pending *pendings, size_t len)
{
	struct coap_pending *p;
	size_t i;

	for (i = 0, p = pendings; i < len; i++, p++) {
		coap_pending_clear(p);
	}
}

struct coap_reply *coap_response_received(
	const struct coap_packet *response,
	const struct sockaddr *from,
	struct coap_reply *replies, size_t len)
{
	struct coap_reply *r;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint8_t tkl;
	size_t i;

	id = coap_header_get_id(response);
	tkl = coap_header_get_token(response, token);

	for (i = 0, r = replies; i < len; i++, r++) {
		int age;

		if ((r->id == 0U) && (r->tkl == 0U)) {
			continue;
		}

		/* Piggybacked must match id when token is empty */
		if ((r->id != id) && (tkl == 0U)) {
			continue;
		}

		if (tkl > 0 && memcmp(r->token, token, tkl)) {
			continue;
		}

		age = coap_get_option_int(response, COAP_OPTION_OBSERVE);
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
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl;
	int age;

	reply->id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	if (tkl > 0) {
		memcpy(reply->token, token, tkl);
	}

	reply->tkl = tkl;

	age = coap_get_option_int(request, COAP_OPTION_OBSERVE);

	/* It means that the request enabled observing a resource */
	if (age == 0) {
		reply->age = 2;
	}
}

void coap_reply_clear(struct coap_reply *reply)
{
	(void)memset(reply, 0, sizeof(*reply));
}

void coap_replies_clear(struct coap_reply *replies, size_t len)
{
	struct coap_reply *r;
	size_t i;

	for (i = 0, r = replies; i < len; i++, r++) {
		coap_reply_clear(r);
	}
}

int coap_resource_notify(struct coap_resource *resource)
{
	struct coap_observer *o;

	if (!resource->notify) {
		return -ENOENT;
	}

	resource->age++;

	SYS_SLIST_FOR_EACH_CONTAINER(&resource->observers, o, list) {
		resource->notify(resource, o);
	}

	return 0;
}

bool coap_request_is_observe(const struct coap_packet *request)
{
	return coap_get_option_int(request, COAP_OPTION_OBSERVE) == 0;
}

void coap_observer_init(struct coap_observer *observer,
			const struct coap_packet *request,
			const struct sockaddr *addr)
{
	observer->tkl = coap_header_get_token(request, observer->token);

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

/**
 * @brief Internal initialization function for CoAP library.
 *
 * Called by the network layer init procedure. Seeds the CoAP @message_id with a
 * random number in accordance with recommendations in CoAP specification.
 *
 * @note This function is not exposed in a public header, as it's for internal
 * use and should therefore not be exposed to applications.
 *
 * @return N/A
 */
void net_coap_init(void)
{
	/* Initialize message_id to a random number */
	message_id = (uint16_t)sys_rand32_get();
}

uint16_t coap_next_id(void)
{
	return message_id++;
}
