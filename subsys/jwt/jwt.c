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
 * Return the JWS "alg" name (RFC 7518 section 3.1) for a PSA signature
 * algorithm used with a given key, or NULL when the pair has no
 * equivalent.  The key size matters because RFC 7518 ties each ECDSA
 * name to a single curve and hash.
 */
static const char *jwt_alg_name(const psa_algorithm_t alg, const psa_key_type_t key_type,
				const size_t key_bits)
{
	const psa_algorithm_t hash = PSA_ALG_SIGN_GET_HASH(alg);

	if (PSA_KEY_TYPE_IS_RSA(key_type)) {
		const bool pkcs1 = PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg);

		/* RFC 7518 requires a salt as long as the hash. */
		if (!pkcs1 && !PSA_ALG_IS_RSA_PSS_STANDARD_SALT(alg)) {
			return NULL;
		}

		switch (hash) {
		case PSA_ALG_SHA_256:
			return pkcs1 ? "RS256" : "PS256";
		case PSA_ALG_SHA_384:
			return pkcs1 ? "RS384" : "PS384";
		case PSA_ALG_SHA_512:
			return pkcs1 ? "RS512" : "PS512";
		default:
			return NULL;
		}
	}

	if (PSA_KEY_TYPE_IS_ECC(key_type) && PSA_ALG_IS_ECDSA(alg) &&
	    PSA_KEY_TYPE_ECC_GET_FAMILY(key_type) == PSA_ECC_FAMILY_SECP_R1) {
		if (key_bits == 256 && hash == PSA_ALG_SHA_256) {
			return "ES256";
		}
		if (key_bits == 384 && hash == PSA_ALG_SHA_384) {
			return "ES384";
		}
		if (key_bits == 521 && hash == PSA_ALG_SHA_512) {
			return "ES512";
		}
	}

	return NULL;
}

/*
 * Add the JWT header to the buffer.
 */
static int jwt_add_header(struct jwt_builder *builder, const char *alg)
{
	const struct jwt_header header = {.alg = alg, .typ = "JWT"};
	const int res = json_obj_encode(jwt_header_descr, ARRAY_SIZE(jwt_header_descr), &header,
					base64_append_bytes, builder);
	if (res < 0) {
		return res;
	}

	base64_flush(builder);

	return builder->overflowed ? -ENOMEM : 0;
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

/* Number of base64 characters needed to encode len bytes, without padding. */
static size_t base64_encoded_len(const size_t len)
{
	const size_t tail = len % 3;

	return (len / 3) * 4 + (tail == 0 ? 0 : tail + 1);
}

int jwt_sign(struct jwt_builder *builder)
{
	if (builder->overflowed) {
		return -ENOMEM;
	}

	/* Room for the separator, the encoded signature and the terminator. */
	if (builder->len < base64_encoded_len(builder->sig_size) + 2) {
		builder->overflowed = true;
		return -ENOMEM;
	}

	/*
	 * Sign into the end of the free space and encode from there.  Base64
	 * turns 3 bytes into 4 characters, so the write position stays behind
	 * the bytes that are still to be read.
	 */
	unsigned char *const sig = (unsigned char *)builder->buf + builder->len - builder->sig_size;
	size_t sig_len = 0;
	const psa_status_t status =
		psa_sign_message(builder->key_id, builder->alg, (const uint8_t *)builder->base,
				 builder->buf - builder->base, sig, builder->sig_size, &sig_len);

	if (status != PSA_SUCCESS) {
		return -EINVAL;
	}

	base64_outch(builder, '.');
	base64_append_bytes((const char *)sig, sig_len, builder);
	base64_flush(builder);

	return builder->overflowed ? -ENOMEM : 0;
}

int jwt_init_builder(struct jwt_builder *builder, char *buffer, const size_t buffer_size,
		     const psa_key_id_t key_id, const psa_algorithm_t alg)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	if (psa_get_key_attributes(key_id, &attr) != PSA_SUCCESS) {
		return -EINVAL;
	}

	const psa_key_type_t key_type = psa_get_key_type(&attr);
	const size_t key_bits = psa_get_key_bits(&attr);

	psa_reset_key_attributes(&attr);

	const char *const alg_name = jwt_alg_name(alg, key_type, key_bits);

	if (alg_name == NULL) {
		return -ENOTSUP;
	}

	builder->base = buffer;
	builder->buf = buffer;
	builder->len = buffer_size;
	builder->overflowed = false;
	builder->pending = 0;
	builder->key_id = key_id;
	builder->alg = alg;
	builder->sig_size = PSA_SIGN_OUTPUT_SIZE(key_type, key_bits, alg);

	return jwt_add_header(builder, alg_name);
}
