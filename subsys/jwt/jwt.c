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

#include <psa/crypto.h>

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
static int base64_char(const int value)
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

/*
 * Add a single character to the jwt buffer.  Detects overflow, and
 * always keeps the buffer null terminated.
 */
static void base64_outch(struct jwt_builder *st, const char ch)
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
	base64_outch(st, base64_char(((st->wip[0] & 0x03) << 4) | (st->wip[1] >> 4)));

	if (st->pending >= 2) {
		base64_outch(st, base64_char(((st->wip[1] & 0x0f) << 2) | (st->wip[2] >> 6)));
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

static int base64_append_bytes(const char *bytes, size_t len, void *data)
{
	struct jwt_builder *st = data;

	while (len-- > 0) {
		base64_addbyte(st, *bytes++);
	}

	return 0;
}

struct jwt_header {
	const char *alg;
	const char *typ;
};

static struct json_obj_descr jwt_header_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct jwt_header, alg, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct jwt_header, typ, JSON_TOK_STRING)};

/*
 * Add the JWT header to the buffer.
 */
static int jwt_add_header(struct jwt_builder *builder, const char *alg, const char *typ)
{
	const struct jwt_header header = {.alg = alg, .typ = typ};
	const int res = json_obj_encode(jwt_header_descr, ARRAY_SIZE(jwt_header_descr), &header,
					base64_append_bytes, builder);
	if (res < 0) {
		return res;
	}

	base64_flush(builder);

	return 0;
}

int jwt_add_payload(struct jwt_builder *builder, const void *payload,
		    const struct json_obj_descr *payload_json, const size_t payload_json_len)
{
	base64_outch(builder, '.');
	const int res = json_obj_encode(payload_json, payload_json_len, payload,
					base64_append_bytes, builder);

	base64_flush(builder);
	return res;
}

int jwt_sign(struct jwt_builder *builder, const psa_key_id_t key_id, const psa_algorithm_t alg)
{
	unsigned char sig[PSA_SIGNATURE_MAX_SIZE];

	size_t sig_len_out = 0;
	const int ret = psa_sign_message(key_id, alg, (const uint8_t *)builder->base,
					 builder->buf - builder->base, sig, PSA_SIGNATURE_MAX_SIZE,
					 &sig_len_out);
	if (ret != PSA_SUCCESS) {
		return -EINVAL;
	}

	base64_outch(builder, '.');
	base64_append_bytes(sig, sig_len_out, builder);
	base64_flush(builder);

	return builder->overflowed ? -ENOMEM : 0;
}

int jwt_init_builder(struct jwt_builder *builder, char *buffer, const size_t buffer_size,
		     const char *alg, const char *typ)
{
	builder->base = buffer;
	builder->buf = buffer;
	builder->len = buffer_size;
	builder->overflowed = false;
	builder->pending = 0;

	return jwt_add_header(builder, alg, typ);
}
