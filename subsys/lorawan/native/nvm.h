/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_NATIVE_NVM_H_
#define SUBSYS_LORAWAN_NATIVE_NVM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Persist the next counter value and reserve a DevNonce for native OTAA.
 *
 * Called only by the native MAC engine after NVM initialization and restore.
 * Calls must be serialized. On failure, the output is unchanged and no join
 * request may be transmitted. A storage error requires a reload before retry.
 *
 * @param nonce Reserved DevNonce.
 * @return 0 on success, or a negative error code otherwise.
 */
int lwan_nvm_dev_nonce_reserve(uint16_t *nonce);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_NVM_H_ */
