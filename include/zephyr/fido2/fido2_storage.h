/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FIDO2 credential storage API.
 */

#ifndef ZEPHYR_INCLUDE_FIDO2_FIDO2_STORAGE_H_
#define ZEPHYR_INCLUDE_FIDO2_FIDO2_STORAGE_H_

#include <zephyr/fido2/fido2_types.h>
#include <stddef.h>

/**
 * @brief FIDO2 credential storage
 * @ingroup fido2
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback for enumerating credentials.
 * @param cred		The current credential
 * @param user_data Opaque user context
 * @retval 0        Continue enumeration
 * @retval non-zero Stop enumeration; value is propagated to fido2_storage_iterate()
 */
typedef int (*fido2_storage_iterate_cb_t)(const struct fido2_credential *cred, void *user_data);

/** @brief Storage backend API */
struct fido2_storage_api {
	/** Initialize the storage backend */
	int (*init)(void);
	/** Store a credential */
	int (*store)(const struct fido2_credential *cred);
	/** Load a credential by ID */
	int (*load)(const uint8_t *cred_id, size_t cred_id_len, struct fido2_credential *cred);
	/** Remove a credential by ID */
	int (*remove)(const uint8_t *cred_id, size_t cred_id_len);
	/** Find credentials by relying party ID hash */
	int (*find_by_rp)(const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
			  struct fido2_credential *creds, size_t max_creds, size_t *count);
	/** Enumerate unique Relying Party IDs from stored credentials */
	int (*enumerate_rps)(size_t offset, struct fido2_credential *creds, size_t max_creds,
			     size_t *count);
	/** Iterate all stored credentials */
	int (*iterate)(fido2_storage_iterate_cb_t cb, void *user_data);
	/** Atomically increment and return the signature counter */
	int (*sign_count_increment)(const uint8_t *cred_id, size_t cred_id_len,
				    uint32_t *new_count);
	/** Wipe all stored credentials and PIN state */
	int (*wipe_all)(void);
	/** Store PIN hash */
	int (*pin_set)(const uint8_t pin_hash[FIDO2_PIN_HASH_SIZE]);
	/** Load stored PIN hash */
	int (*pin_get)(uint8_t pin_hash[FIDO2_PIN_HASH_SIZE]);
	/** Get remaining PIN retry count */
	int (*pin_retries_get)(uint8_t *retries);
	/** Decrement PIN retry counter */
	int (*pin_retries_decrement)(void);
	/** Reset PIN retry counter to maximum */
	int (*pin_retries_reset)(void);
};

/**
 * @brief FIDO2 storage backend instance.
 *
 * A storage backend must provide exactly one definition of this symbol.
 * Multiple definitions will cause a link error.
 */
extern const struct fido2_storage_api fido2_storage_backend;

/**
 * @brief Initialize credential storage.
 * @retval 0 On success
 * @retval -errno On failure
 */
int fido2_storage_init(void);

/**
 * @brief Store a credential.
 * @param cred Credential to store
 * @retval 0 On success
 * @retval -ENOSPC If storage is full
 */
int fido2_storage_store(const struct fido2_credential *cred);

/**
 * @brief Load a credential by ID.
 * @param cred_id Credential ID bytes
 * @param cred_id_len Length of credential ID
 * @param cred Output credential structure
 * @retval 0 On success
 * @retval -ENOENT If not found
 */
int fido2_storage_load(const uint8_t *cred_id, size_t cred_id_len, struct fido2_credential *cred);

/**
 * @brief Delete a credential.
 * @param cred_id Credential ID bytes
 * @param cred_id_len Length of credential ID
 * @retval 0 On success
 * @retval -ENOENT If not found
 */
int fido2_storage_remove(const uint8_t *cred_id, size_t cred_id_len);

/**
 * @brief Find all credentials for a relying party.
 * @param rp_id_hash SHA-256 of the relying party ID
 * @param creds Output array
 * @param max_creds Array capacity
 * @param count Number of credentials found
 * @retval 0 on Success
 */
int fido2_storage_find_by_rp(const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
			     struct fido2_credential *creds, size_t max_creds, size_t *count);

/**
 * @brief Enumerate unique Relying Party IDs from stored credentials.
 * @param offset Starting index for enumeration
 * @param creds Output array for credentials containing unique RP IDs
 * @param max_creds Capacity of the output array
 * @param count Number of unique RPs found
 * @retval 0 On success
 * @retval -errno On failure
 */
int fido2_storage_enumerate_rps(size_t offset, struct fido2_credential *creds, size_t max_creds,
				size_t *count);

/**
 * @brief Iterate over all stored credentials.
 * @param cb Callback function to invoke for each credential
 * @param user_data Opaque context passed to the callback
 * @retval 0        On success or if all credentials were visited
 * @retval non-zero If the callback stopped iteration early (callback's return value)
 * @retval -errno   On storage failure
 */
int fido2_storage_iterate(fido2_storage_iterate_cb_t cb, void *user_data);

/**
 * @brief Atomically increment and return the signature counter.
 * @param cred_id     Credential ID bytes
 * @param cred_id_len Length of credential ID
 * @param new_count   Output: counter value after increment
 * @retval 0	      On success
 * @retval -ENOENT    If credential not found
 */
int fido2_storage_sign_count_increment(const uint8_t *cred_id, size_t cred_id_len,
				       uint32_t *new_count);

/** @brief Wipe all stored credentials and PIN state */
int fido2_storage_wipe_all(void);

/* PIN storage */

/**
 * @brief Store PIN hash
 * Stores LEFT(SHA-256(pin), 16) as per CTAP2 specs
 * @param pin_hash First 16 bytes of SHA-256(pin).
 */
int fido2_storage_pin_set(const uint8_t pin_hash[FIDO2_PIN_HASH_SIZE]);

/** @brief Load stored PIN hash */
int fido2_storage_pin_get(uint8_t pin_hash[FIDO2_PIN_HASH_SIZE]);

/** @brief Get remaining PIN retries */
int fido2_storage_pin_retries_get(uint8_t *retries);

/** @brief Decrement PIN retry counter */
int fido2_storage_pin_retries_decrement(void);

/** @brief Reset PIN retry counter to max */
int fido2_storage_pin_retries_reset(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_FIDO2_FIDO2_STORAGE_H_ */
