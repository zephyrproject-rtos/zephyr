/* keys_psa.h - Bluetooth host keys in PSA persistent storage */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_KEYS_PSA_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_KEYS_PSA_H_

#include <stdbool.h>
#include <stdint.h>

#include <psa/crypto.h>

/** @cond INTERNAL_HIDDEN */

/** Size of the keys handled by this module (LTK and IRK) */
#define BT_KEYS_PSA_KEY_SIZE 16

/**
 * @brief Initialize PSA Crypto for this module
 *
 * Must be called once before any other function of this module, which do not initialize it.
 * It is safe to call when PSA Crypto is already initialized.
 *
 * @retval 0    Success.
 * @retval -EIO PSA failure.
 */
int bt_keys_psa_init(void);

/**
 * @brief Store a key as a new PSA persistent key
 *
 * A key ID that is not in use is allocated from the Bluetooth host key ID range.
 *
 * @param key Key value.
 * @param id  Where to store the ID of the new PSA key.
 *
 * @retval 0       Success.
 * @retval -ENOMEM No free key ID.
 * @retval -EIO    PSA failure.
 */
int bt_keys_psa_put(const uint8_t key[BT_KEYS_PSA_KEY_SIZE], psa_key_id_t *id);

/**
 * @brief Read a key stored with @ref bt_keys_psa_put
 *
 * The ID is marked as in use, so it is not allocated again.
 *
 * @param id  ID of the PSA key.
 * @param key Where to store the key value.
 *
 * @retval 0        Success.
 * @retval -EINVAL  The ID is not in the Bluetooth host key ID range.
 * @retval -ENOENT  The key does not exist.
 * @retval -EIO     PSA failure.
 */
int bt_keys_psa_get(psa_key_id_t id, uint8_t key[BT_KEYS_PSA_KEY_SIZE]);

/**
 * @brief Check that a PSA key holds a given key value
 *
 * @param id  ID of the PSA key.
 * @param key Key value to compare with.
 *
 * @return true if the PSA key exists and holds @p key, false otherwise.
 */
bool bt_keys_psa_matches(psa_key_id_t id, const uint8_t key[BT_KEYS_PSA_KEY_SIZE]);

/**
 * @brief Destroy a PSA key and release its ID
 *
 * A null ID, an ID outside of the Bluetooth host key ID range and a key that does not
 * exist are ignored.
 *
 * @param id ID of the PSA key.
 */
void bt_keys_psa_del(psa_key_id_t id);

/**
 * @brief Mark all key IDs as not in use
 *
 * The PSA keys are not destroyed.
 */
void bt_keys_psa_reset(void);

/** @endcond */

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_KEYS_PSA_H_ */
