/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_NATIVE_CRYPTO_CRYPTO_H_
#define SUBSYS_LORAWAN_NATIVE_CRYPTO_CRYPTO_H_

#include <stddef.h>
#include <stdint.h>
#include <psa/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

int lwan_crypto_init(void);

/*
 * Import raw AES-128 key material into PSA.
 * Returns a volatile psa_key_id_t for the requested algorithm,
 * or PSA_KEY_ID_NULL on failure.
 */
psa_key_id_t lwan_crypto_import_cmac_key(const uint8_t key[16]);
psa_key_id_t lwan_crypto_import_ecb_key(const uint8_t key[16]);

/* AES-CMAC truncated to 4 bytes; key must have been imported for CMAC */
int lwan_crypto_compute_mic(psa_key_id_t cmac_key, const uint8_t *msg,
			    size_t msg_len, uint8_t mic[4]);

/* LoRaWAN uses AES encrypt (not decrypt) for join accept; in-place, 16 or 32 bytes */
int lwan_crypto_decrypt_join_accept(psa_key_id_t ecb_key,
				    uint8_t *payload, size_t len);

/*
 * Derive AppSKey and FNwkSIntKey (LoRaWAN 1.0.x) and import them as
 * PSA keys. Raw key material is zeroed after import.
 *
 * nwk_ecb_key: NwkKey imported for ECB
 * app_s_ecb:   (out) AppSKey for payload encrypt (port > 0)
 * fnwk_cmac:   (out) FNwkSIntKey for MIC
 * fnwk_ecb:    (out) FNwkSIntKey for payload encrypt (port 0)
 */
int lwan_crypto_derive_session_keys(psa_key_id_t nwk_ecb_key,
				    const uint8_t join_nonce[3],
				    const uint8_t net_id[3],
				    uint16_t dev_nonce,
				    psa_key_id_t *app_s_ecb,
				    psa_key_id_t *fnwk_cmac,
				    psa_key_id_t *fnwk_ecb);

/* AES-128 CTR payload encrypt/decrypt (XOR-symmetric); in-place */
int lwan_crypto_payload_encrypt(psa_key_id_t ecb_key, uint32_t dev_addr,
				uint32_t fcnt, uint8_t dir,
				uint8_t *payload, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_CRYPTO_CRYPTO_H_ */
