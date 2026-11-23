/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/net/tls_credentials_keygen_store.h>
#include <zephyr/psa/key_ids.h>
#include <zephyr/sys/util.h>
#include <psa/crypto.h>

#define KEY_ID_MIN   ZEPHYR_PSA_TLS_CREDENTIALS_KEYGEN_KEY_ID_RANGE_BEGIN
#define KEY_ID_COUNT ZEPHYR_PSA_TLS_CREDENTIALS_KEYGEN_KEY_ID_RANGE_SIZE

int tls_credential_keygen_store_attr_set(enum tls_credential_keygen_type type,
					 psa_key_attributes_t *attr)
{
	ARG_UNUSED(type);

	psa_set_key_lifetime(attr, PSA_KEY_LIFETIME_PERSISTENT);

	return 0;
}

bool tls_credential_keygen_store_id_is_own(psa_key_id_t key_id)
{
	return key_id >= KEY_ID_MIN && key_id - KEY_ID_MIN < KEY_ID_COUNT;
}

static psa_key_id_t key_id_at(size_t index)
{
	return KEY_ID_MIN + index;
}

int tls_credential_keygen_store_id_alloc(sec_tag_t tag, enum tls_credential_keygen_type type,
					 const psa_key_id_t *used, size_t used_count,
					 psa_key_id_t *key_id)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(type);

	return tls_credential_keygen_store_id_pick(KEY_ID_COUNT, key_id_at, used, used_count,
						   key_id);
}
