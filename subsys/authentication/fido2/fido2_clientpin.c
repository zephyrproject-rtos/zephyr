/*
 * Copyright (c) 2026 Siratul Islam <siratul.islam@linux.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/authentication/fido2/fido2_storage.h>

#include "fido2_cbor.h"
#include "fido2_crypto.h"
#include "fido2_clientpin.h"

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

static uint32_t key_agreement_key_id;

static int generate_key_agreement(void)
{
	int ret;

	ret = fido2_crypto_ecdh_generate_keypair(&key_agreement_key_id);
	if (ret) {
		LOG_ERR("KeyAgreement key gen failed: %d", ret);
		return ret;
	}

	return 0;
}

int fido2_clientpin_init(void)
{
	return generate_key_agreement();
}

enum fido2_status fido2_clientpin_cmd_get_retries(uint8_t *cbor_out, size_t cbor_out_cap,
						  size_t *cbor_out_len)
{
	uint8_t retries;
	int ret;

	ret = fido2_storage_pin_retries_get(&retries);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	ret = fido2_cbor_encode_client_pin_retries(retries, false, cbor_out, cbor_out_cap,
						   cbor_out_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	return FIDO2_OK;
}

enum fido2_status fido2_clientpin_cmd_get_key_agreement(uint8_t *cbor_out, size_t cbor_out_cap,
							size_t *cbor_out_len)
{
	uint8_t pub_key[FIDO2_P256_UNCOMPRESSED_KEY_SIZE];
	size_t pub_key_len;
	int ret;

	ret = fido2_crypto_export_pubkey(key_agreement_key_id, pub_key, sizeof(pub_key),
					 &pub_key_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	ret = fido2_cbor_encode_client_pin_key_agreement(pub_key, pub_key_len, cbor_out,
							 cbor_out_cap, cbor_out_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	return FIDO2_OK;
}
