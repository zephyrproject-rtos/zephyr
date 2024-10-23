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
#include <zephyr/random/random.h>

#include <tinycrypt/ctr_prng.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/ecc_dsa.h>
#include <tinycrypt/constants.h>

#include "jwt.h"

static TCCtrPrng_t prng_state;
static bool prng_init;

static const char personalize[] = "zephyr:drivers/jwt/jwt.c";

static int setup_prng(void)
{
	if (prng_init) {
		return 0;
	}
	prng_init = true;

	uint8_t entropy[TC_AES_KEY_SIZE + TC_AES_BLOCK_SIZE];

	sys_rand_get(entropy, sizeof(entropy));

	int res = tc_ctr_prng_init(&prng_state, (const uint8_t *)&entropy, sizeof(entropy),
				   personalize, sizeof(personalize));

	return res == TC_CRYPTO_SUCCESS ? 0 : -EINVAL;
}

/* This function is declared in
 * modules/crypto/tinycrypt/lib/include/tinycrypt/ecc_platform_specific.h.
 *
 * TinyCrypt expects this function to be implemented somewhere when using the
 * ECC module.
 */
int default_CSPRNG(uint8_t *dest, unsigned int size)
{
	int res = tc_ctr_prng_generate(&prng_state, NULL, 0, dest, size);
	return res;
}

int jwt_sign_impl(struct jwt_builder *builder, const unsigned char *der_key, size_t der_key_len,
		  unsigned char *sig, size_t sig_size)
{
	struct tc_sha256_state_struct ctx;
	uint8_t hash[32];
	int res;

	ARG_UNUSED(sig_size);

	tc_sha256_init(&ctx);
	tc_sha256_update(&ctx, builder->base, builder->buf - builder->base);
	tc_sha256_final(hash, &ctx);

	res = setup_prng();

	if (res != 0) {
		return res;
	}

	/* Note that tinycrypt only supports P-256. */
	res = uECC_sign(der_key, hash, sizeof(hash), sig, &curve_secp256r1);
	if (res != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}
