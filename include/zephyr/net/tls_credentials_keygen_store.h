/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Key store interface of the TLS credentials on-device key generation.
 */

#ifndef ZEPHYR_INCLUDE_NET_TLS_CREDENTIALS_KEYGEN_STORE_H_
#define ZEPHYR_INCLUDE_NET_TLS_CREDENTIALS_KEYGEN_STORE_H_

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/net/tls_credentials_keygen.h>
#include <psa/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TLS credentials on-device key generation key store
 * @defgroup tls_credentials_keygen_store TLS credentials on-device key generation key store
 * @since 4.5
 * @version 0.1.0
 * @ingroup tls_credentials_keygen
 * @{
 *
 * The key store decides where the generated keys are stored and which PSA key IDs they get.
 * With @kconfig{CONFIG_TLS_CREDENTIALS_KEYGEN_KEY_STORE_DEFAULT}, the keys are persistent keys in
 * the default PSA key location, with key IDs from the range that @c zephyr/psa/key_ids.h reserves.
 * With @kconfig{CONFIG_TLS_CREDENTIALS_KEYGEN_KEY_STORE_CUSTOM}, the application or the vendor
 * implements the functions below, for example to store the keys in a hardware key location.
 *
 * The functions are called with the TLS credentials locked.
 *
 * The keys of one key store do not belong to another key store. Delete the keys with
 * @ref tls_credential_keygen_delete before the key store is changed. After a change, remove the
 * registration of an earlier key with @ref tls_credential_delete, and destroy the key with
 * psa_destroy_key().
 */

/**
 * @brief Set the lifetime of a key that is generated.
 *
 * The type, size, algorithm and usage of the key are already set. The key store sets only the
 * lifetime, which also gives the location of the key. It must not change the other attributes,
 * because TLS, the CSR and other users of the key need them. If the key location cannot store
 * such a key, the key store returns -ENOTSUP.
 *
 * @param type Type of the key pair.
 * @param attr Attributes of the key.
 *
 * @retval 0 On success.
 * @retval -ENOTSUP The key store does not support the key type.
 */
int tls_credential_keygen_store_attr_set(enum tls_credential_keygen_type type,
					 psa_key_attributes_t *attr);

/**
 * @brief Allocate a key ID for a key that is generated.
 *
 * A key with an ID of the key store that is not in @p used is left over, for example after its
 * registration was removed with @ref tls_credential_delete. The key store destroys it, so that
 * its key ID can be used again. @ref tls_credential_keygen_store_id_pick does that.
 *
 * @param tag        Security tag the key is generated for.
 * @param type       Type of the key pair.
 * @param used       Key IDs that security tags are registered for. Each of them belongs to the
 *                   key store, see @ref tls_credential_keygen_store_id_is_own.
 * @param used_count Number of entries in @p used.
 * @param key_id     Allocated key ID.
 *
 * @retval 0 On success.
 * @retval -ENOSPC No free key ID.
 */
int tls_credential_keygen_store_id_alloc(sec_tag_t tag, enum tls_credential_keygen_type type,
					 const psa_key_id_t *used, size_t used_count,
					 psa_key_id_t *key_id);

/**
 * @brief Pick a free key ID for a key store.
 *
 * Returns the first key ID of the key store that is not in @p used. If a key exists with that
 * key ID, it is left over and is destroyed. A key store can call it from
 * @ref tls_credential_keygen_store_id_alloc.
 *
 * @param count      Number of key IDs of the key store.
 * @param key_id_at  Returns the key ID with an index from 0 to @p count - 1.
 * @param used       Key IDs that security tags are registered for.
 * @param used_count Number of entries in @p used.
 * @param key_id     Free key ID.
 *
 * @retval 0 On success.
 * @retval -ENOSPC No free key ID.
 */
int tls_credential_keygen_store_id_pick(size_t count, psa_key_id_t (*key_id_at)(size_t index),
					const psa_key_id_t *used, size_t used_count,
					psa_key_id_t *key_id);

/**
 * @brief Check whether a key ID belongs to the key store.
 *
 * The key generation only uses and deletes keys with an ID of the key store.
 *
 * @param key_id PSA key ID.
 *
 * @retval true The key ID belongs to the key store.
 * @retval false Otherwise.
 */
bool tls_credential_keygen_store_id_is_own(psa_key_id_t key_id);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_TLS_CREDENTIALS_KEYGEN_STORE_H_ */
