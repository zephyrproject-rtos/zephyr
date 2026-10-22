/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TLS_CREDENTIALS_KEYGEN_TEST_COMMON_H_
#define TLS_CREDENTIALS_KEYGEN_TEST_COMMON_H_

#include <zephyr/net/tls_credentials_keygen.h>
#include <zephyr/psa/key_ids.h>

#define TEST_SEC_TAG   42
#define TEST_SEC_TAG_2 43
/* Outside of the PSA user key ID range, as used by nRF Cloud. */
#define TEST_SEC_TAG_LARGE 16842753

#define KEY_ID_MIN   ZEPHYR_PSA_TLS_CREDENTIALS_KEYGEN_KEY_ID_RANGE_BEGIN
#define KEY_ID_COUNT ZEPHYR_PSA_TLS_CREDENTIALS_KEYGEN_KEY_ID_RANGE_SIZE
/* A key of another user of PSA, outside of the reserved range. */
#define FOREIGN_KEY_ID ZEPHYR_PSA_APPLICATION_KEY_ID_RANGE_BEGIN

#define P256 TLS_CREDENTIAL_KEYGEN_ECDSA_SECP256R1
#define P384 TLS_CREDENTIAL_KEYGEN_ECDSA_SECP384R1

/* Delete the keys and registrations of the test sec tags. */
void reset_state(void);

/* Key ID of the generated key of a sec tag. Fails the test if there is none. */
psa_key_id_t key_id_get(sec_tag_t tag);

#endif /* TLS_CREDENTIALS_KEYGEN_TEST_COMMON_H_ */
