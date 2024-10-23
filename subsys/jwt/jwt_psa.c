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
#include <psa/crypto.h>

#include "jwt.h"

int jwt_sign_impl(struct jwt_builder *builder, const unsigned char *der_key, size_t der_key_len,
		  unsigned char *sig, size_t sig_size)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	size_t sig_len_out;
	psa_algorithm_t alg;
	int ret;

#if defined(CONFIG_JWT_SIGN_ECDSA)
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
#else  /* CONFIG_JWT_SIGN_RSA */
	psa_set_key_type(&attr, PSA_KEY_TYPE_RSA_KEY_PAIR);
	psa_set_key_algorithm(&attr, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256));
	alg = PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256);
#endif /* CONFIG_JWT_SIGN_ECDSA || CONFIG_JWT_SIGN_RSA */
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);

	status = psa_import_key(&attr, der_key, der_key_len, &key_id);
	if (status != PSA_SUCCESS) {
		return -EINVAL;
	}

	status = psa_sign_message(key_id, alg,
				  builder->base, builder->buf - builder->base,
				  sig, sig_size, &sig_len_out);
	ret = (status == PSA_SUCCESS) ? 0 : -EINVAL;

	psa_destroy_key(key_id);

	return ret;
}
