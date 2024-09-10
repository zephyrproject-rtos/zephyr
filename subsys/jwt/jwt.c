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
static int base64_char(int value)
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
static const char b64_table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static inline int base64_char(int value)
{
	return b64_table[value];
}
#endif

/*
 * Add a single character to the jwt buffer.  Detects overflow, and
 * always keeps the buffer null terminated.
 */
static void base64_outch(struct jwt_builder *st, char ch)
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
static void base64_flush(struct jwt_builder *st)
{
	if (st->pending < 1) {
		return;
	}

	base64_outch(st, base64_char(st->wip[0] >> 2));
	base64_outch(st, base64_char(((st->wip[0] & 0x03) << 4) |
				(st->wip[1] >> 4)));

	if (st->pending >= 2) {
		base64_outch(st, base64_char(((st->wip[1] & 0x0f) << 2) |
				(st->wip[2] >> 6)));
	}
	if (st->pending >= 3) {
		base64_outch(st, base64_char(st->wip[2] & 0x3f));
	}

	st->pending = 0;
	memset(st->wip, 0, 3);
}

static void base64_addbyte(struct jwt_builder *st, uint8_t byte)
{
	st->wip[st->pending++] = byte;
	if (st->pending == 3) {
		base64_flush(st);
	}
}

static int base64_append_bytes(const char *bytes, size_t len,
			 void *data)
{
	struct jwt_builder *st = data;

	while (len-- > 0) {
		base64_addbyte(st, *bytes++);
	}

	return 0;
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

int jwt_add_payload(struct jwt_builder *builder,
		     int32_t exp,
		     int32_t iat,
		     const char *aud)
{
	struct jwt_payload payload = {
		.exp = exp,
		.iat = iat,
		.aud = aud,
	};

	base64_outch(builder, '.');
	int res = json_obj_encode(jwt_payload_desc,
				  ARRAY_SIZE(jwt_payload_desc),
				  &payload, base64_append_bytes, builder);

	base64_flush(builder);
	return res;
}

int jwt_sign(struct jwt_builder *builder,
	     const char *der_key,
	     size_t der_key_len)
{
	int ret;
	unsigned char sig[JWT_SIGNATURE_LEN];

	ret = jwt_sign_impl(builder, der_key, der_key_len, sig, sizeof(sig));
	if (ret < 0) {
		return ret;
	}

	base64_outch(builder, '.');
	base64_append_bytes(sig, sizeof(sig), builder);
	base64_flush(builder);

	return builder->overflowed ? -ENOMEM : 0;
}

int jwt_init_builder(struct jwt_builder *builder,
		     char *buffer,
		     size_t buffer_size)
{
	builder->base = buffer;
	builder->buf = buffer;
	builder->len = buffer_size;
	builder->overflowed = false;
	builder->pending = 0;

	return jwt_add_header(builder);
}
