/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/authentication/fido2/fido2_attestation.h>
#include <zephyr/authentication/fido2/fido2_types.h>

#include "../fido2_crypto.h"

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

static uint8_t self_attest_sig[FIDO2_ECDSA_SIG_MAX_SIZE];

int fido2_attestation_sign(const uint8_t *auth_data, size_t auth_data_len,
			   const uint8_t *client_data_hash, uint32_t credential_key_id,
			   struct fido2_attestation_result *result)
{
	uint8_t sig_input_hash[FIDO2_SHA256_SIZE];
	size_t sig_len;
	int ret;

	ret = fido2_crypto_hash_authdata(auth_data, auth_data_len, client_data_hash,
					 sig_input_hash);
	if (ret) {
		LOG_ERR("Self-attestation hash failed: %d", ret);
		return ret;
	}

	ret = fido2_crypto_sign(credential_key_id, sig_input_hash, self_attest_sig,
				sizeof(self_attest_sig), &sig_len);
	if (ret) {
		LOG_ERR("Self-attestation sign failed: %d", ret);
		return ret;
	}

	result->fmt = FIDO2_ATTESTATION_FMT_PACKED;
	result->alg = FIDO2_COSE_ES256;
	result->sig_len = sig_len;
	result->sig = self_attest_sig;
	result->x5c = NULL;
	result->x5c_len = 0;

	return 0;
}
