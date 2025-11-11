/*
 * Copyright (C) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <errno.h>

#include <zephyr/data/jwt.h>
#include <zephyr/data/json.h>

#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/sha256.h>
#include <zephyr/random/random.h>

#include "jwt.h"

static int csprng_wrapper(void *ctx, unsigned char *dest, size_t size)
{
	ARG_UNUSED(ctx);

	return sys_csrand_get((void *)dest, size);
}

int jwt_sign_impl(struct jwt_builder *builder, const unsigned char *der_key, size_t der_key_len,
		  unsigned char *sig, size_t sig_size)
{
	int res;
	mbedtls_pk_context ctx;
	size_t sig_len_out;

	mbedtls_pk_init(&ctx);

	res = mbedtls_pk_parse_key(&ctx, der_key, der_key_len, NULL, 0, csprng_wrapper, NULL);
	if (res != 0) {
		return res;
	}

	uint8_t hash[32];

	/*
	 * The '0' indicates to mbedtls to do a SHA256, instead of
	 * 224.
	 */
	res = mbedtls_sha256(builder->base, builder->buf - builder->base, hash, 0);
	if (res != 0) {
		return res;
	}

	res = mbedtls_pk_sign(&ctx, MBEDTLS_MD_SHA256, hash, sizeof(hash), sig, sig_size,
			      &sig_len_out, csprng_wrapper, NULL);
	return res;
}
