/*
 * Copyright (C) 2018 Linaro Ltd
 * Copyright (C) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <errno.h>

#include <zephyr/data/jwt.h>
#include <zephyr/data/json.h>

#include "jwt.h"

#if defined(CONFIG_JWT_SIGN_RSA)
#define JWT_SIGNATURE_LEN 256
#else /* CONFIG_JWT_SIGN_ECDSA */
#define JWT_SIGNATURE_LEN 64
#endif

/*
 * Base64URL encoding is typically done by lookup into a 64-byte static
 * array.  As an experiment, lets look at both code size and time for
 * one that does the character encoding computationally.  Like the
 * array version, this doesn't do bounds checking, and assumes the
 * passed value has been masked.
 *
 * On Cortex-M, this function is 34 bytes of code, which is only a
 * little more than half of the size of the lookup table.
 */
#if 1
static int b64url_char(int value)
{
	if (value < 26) {
		return value + 'A';
	} else if (value < 52) {
		return value + 'a' - 26;
	} else if (value < 62) {
		return value + '0' - 52;
	} else if (value == 62) {
		return '-';
	} else {
		return '_';
	}
}
#else
static const char b64url_encode_table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static inline int b64url_char(int value)
{
	return b64url_encode_table[value];
}
#endif

#if 1
static int b64url_val(int ch)
{
	if (('A' <= ch) && ('Z' >= ch)) {
		return ch - 'A';
	} else if (('a' <= ch) && ('z' >= ch)) {
		return ch + 26 - 'a';
	} else if (('0' <= ch) && ('9' >= ch)) {
		return ch + 52 - '0';
	} else if (ch == '-') {
		return 62;
	} else if (ch == '_') {
		return 63;
	} else {
		return 64;
	}
}
#else
/* clang-format off */
static const char b64url_decode_table[256] = {
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
	64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 63,
	64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};
/* clang-format on */
static inline int b64url_val(int ch)
{
	return b64url_decode_table[ch];
}
#endif

/*
 * Add a single character to the jwt buffer.  Detects overflow, and
 * always keeps the buffer null terminated.
 */
static void base64url_encode_outch(struct jwt_builder *st, char ch)
{
	if (st->overflowed) {
		return;
	}

	if (st->len < 2) {
		st->overflowed = true;
		return;
	}

	*st->buf++ = ch;
	st->len--;
	*st->buf = 0;
}

/*
 * Flush any pending base64 character data out.  If we have all three
 * bytes are present, this will generate 4 characters, otherwise it
 * may generate fewer.
 */
static void base64url_encode_flush(struct jwt_builder *st)
{
	if (st->pending < 1) {
		return;
	}

	base64url_encode_outch(st, b64url_char(st->wip[0] >> 2));
	base64url_encode_outch(st, b64url_char(((st->wip[0] & 0x03) << 4) | (st->wip[1] >> 4)));

	if (st->pending >= 2) {
		base64url_encode_outch(st,
				       b64url_char(((st->wip[1] & 0x0f) << 2) | (st->wip[2] >> 6)));
	}
	if (st->pending >= 3) {
		base64url_encode_outch(st, b64url_char(st->wip[2] & 0x3f));
	}

	st->pending = 0;
	memset(st->wip, 0, 3);
}

static void base64url_encode_addbyte(struct jwt_builder *st, uint8_t byte)
{
	st->wip[st->pending++] = byte;
	if (st->pending == 3) {
		base64url_encode_flush(st);
	}
}

static int base64url_encode_append_bytes(const char *bytes, size_t len, void *data)
{
	struct jwt_builder *st = data;

	while (len-- > 0) {
		base64url_encode_addbyte(st, *bytes++);
	}

	return 0;
}

/*
 * Base64URL decoding
 */
static int base64url_decode(char *dst, size_t dlen, const char *src, size_t slen)
{
	char *bp = dst;

	if (dlen < (((slen * 3) + 3) >> 2)) {
		return -ENOSPC;
	}

	while (slen > 4) {
		(*bp++) = ((b64url_val(src[0]) & 0x3f) << 2) | ((b64url_val(src[1]) & 0x30) >> 4);
		(*bp++) = ((b64url_val(src[1]) & 0x0f) << 4) | ((b64url_val(src[2]) & 0x3c) >> 2);
		(*bp++) = ((b64url_val(src[2]) & 0x03) << 6) | (b64url_val(src[3]) & 0x3f);
		src += 4;
		slen -= 4;
	}

	if (slen > 1) {
		(*bp++) = ((b64url_val(src[0]) & 0x3f) << 2) | ((b64url_val(src[1]) & 0x30) >> 4);
	}
	if (slen > 2) {
		(*bp++) = ((b64url_val(src[1]) & 0x0f) << 4) | ((b64url_val(src[2]) & 0x3c) >> 2);
	}
	if (slen > 3) {
		(*bp++) = ((b64url_val(src[2]) & 0x03) << 6) | (b64url_val(src[3]) & 0x3f);
	}

	*bp = 0;

	return bp - dst;
}

struct jwt_payload {
	int32_t exp;
	int32_t iat;
	const char *aud;
};

static struct json_obj_descr jwt_payload_desc[] = {
	JSON_OBJ_DESCR_PRIM(struct jwt_payload, aud, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct jwt_payload, exp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct jwt_payload, iat, JSON_TOK_NUMBER),
};

/*
 * Add the JWT header to the buffer.
 */
static int jwt_add_header(struct jwt_builder *builder)
{
	/*
	 * Pre-computed JWT header
	 * Use https://www.base64encode.org/ for update
	 */
	const char jwt_header[] =
#ifdef CONFIG_JWT_SIGN_RSA
		/* {"alg":"RS256","typ":"JWT"} */
		"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9";
#else /* CONFIG_JWT_SIGN_ECDSA */
		/* {"alg":"ES256","typ":"JWT"} */
		"eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9";
#endif
	int jwt_header_len = ARRAY_SIZE(jwt_header);

	if (jwt_header_len > builder->len) {
		builder->overflowed = true;
		return -ENOSPC;
	}
	strcpy(builder->buf, jwt_header);
	builder->buf += jwt_header_len - 1;
	builder->len -= jwt_header_len - 1;
	return 0;
}

int jwt_add_payload(struct jwt_builder *builder, int32_t expt, int32_t iat, const char *aud)
{
	struct jwt_payload payload = {
		.exp = expt,
		.iat = iat,
		.aud = aud,
	};

	base64url_encode_outch(builder, '.');
	int res = json_obj_encode(jwt_payload_desc, ARRAY_SIZE(jwt_payload_desc), &payload,
				  base64url_encode_append_bytes, builder);

	base64url_encode_flush(builder);
	return res;
}

int jwt_sign(struct jwt_builder *builder, const char *der_key, size_t der_key_len)
{
	int ret;
	unsigned char sig[JWT_SIGNATURE_LEN];

	ret = jwt_sign_impl(builder, der_key, der_key_len, sig, sizeof(sig));
	if (ret < 0) {
		return ret;
	}

	base64url_encode_outch(builder, '.');
	base64url_encode_append_bytes(sig, sizeof(sig), builder);
	base64url_encode_flush(builder);

	return builder->overflowed ? -ENOMEM : 0;
}

int jwt_init_builder(struct jwt_builder *builder, char *buffer, size_t buffer_size)
{
	builder->base = buffer;
	builder->buf = buffer;
	builder->len = buffer_size;
	builder->overflowed = false;
	builder->pending = 0;

	return jwt_add_header(builder);
}

int jwt_parse_payload(struct jwt_parser *parser, int32_t *expt, int32_t *iat, char *aud)
{
	struct jwt_payload payload;
	int res;

	res = base64url_decode(parser->buf, parser->buf_len, parser->payload, parser->payload_len);
	if (res < 0) {
		return res;
	}

	res = json_obj_parse(parser->buf, res, jwt_payload_desc, ARRAY_SIZE(jwt_payload_desc),
			     &payload);
	if (res == (1 << ARRAY_SIZE(jwt_payload_desc)) - 1) {
		*expt = payload.exp;
		*iat = payload.iat;
		strcpy(aud, payload.aud);
		res = 0;
	} else if (res >= 0) {
		res = -EINVAL;
	}

	return res;
}

int jwt_verify(struct jwt_parser *parser, const char *der_key, size_t der_key_len)
{
	struct jwt_builder builder;
	char *builder_header;
	size_t builder_header_len;
	char *builder_sign;
	size_t builder_sign_len;
	int res;

	/*
	 * Check JWT header
	 */

	res = jwt_init_builder(&builder, parser->buf, parser->buf_len);
	if (res != 0) {
		return res;
	}
	builder_header = builder.base;
	builder_header_len = builder.buf - builder.base;
	if (parser->header_len != builder_header_len) {
		return -EINVAL;
	}
	res = memcmp(parser->header, builder_header, builder_header_len);
	if (res != 0) {
		return -EINVAL;
	}

	/*
	 * Copy the payload
	 */

	if (builder.len < (parser->payload_len + 1)) {
		return -ENOSPC;
	}
	/* -1/+1 to also copy the dot */
	memcpy(builder.buf, parser->payload - 1, parser->payload_len + 1);
	builder.buf += parser->payload_len + 1;
	builder.buf[0] = '\0';
	builder.len -= parser->payload_len + 1;
	builder_sign = builder.buf + 1;

	/*
	 * Check JWT signature
	 */

	res = base64url_decode(builder_sign, builder.len - 1, parser->sign, parser->sign_len);
	if (res < 0) {
		return res;
	}
	builder_sign_len = res;
	return jwt_verify_impl(&builder, der_key, der_key_len, builder_sign, builder_sign_len);
}

int jwt_init_parser(struct jwt_parser *parser, const char *token, char *buffer, size_t buffer_size)
{
	char *first_dot;
	char *last_dot;

	if (buffer_size < (strlen(token) + 1)) {
		return -ENOSPC;
	}

	parser->buf = buffer;
	parser->buf_len = buffer_size;

	first_dot = strchr(token, '.');
	if (first_dot == NULL) {
		return -EINVAL;
	}

	last_dot = strrchr(token, '.');
	if (last_dot == NULL) {
		return -EINVAL;
	}

	if (first_dot == last_dot) {
		return -EINVAL;
	}

	parser->header = token;
	parser->header_len = first_dot - parser->header;

	parser->payload = first_dot + 1;
	parser->payload_len = last_dot - parser->payload;

	parser->sign = last_dot + 1;
	parser->sign_len = strlen(parser->sign);

	return 0;
}
