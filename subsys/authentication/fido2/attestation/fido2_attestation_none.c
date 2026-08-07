/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/authentication/fido2/fido2_attestation.h>
#include <zephyr/authentication/fido2/fido2_types.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

int fido2_attestation_sign(const uint8_t *auth_data, size_t auth_data_len,
			   const uint8_t *client_data_hash, uint32_t credential_key_id,
			   struct fido2_attestation_result *result)
{
	ARG_UNUSED(auth_data);
	ARG_UNUSED(auth_data_len);
	ARG_UNUSED(client_data_hash);
	ARG_UNUSED(credential_key_id);

	result->fmt = FIDO2_ATTESTATION_FMT_NONE;
	result->alg = 0;
	result->sig = NULL;
	result->sig_len = 0;
	result->x5c = NULL;
	result->x5c_len = 0;

	return 0;
}
