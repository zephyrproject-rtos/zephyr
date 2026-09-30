/*
 * Copyright (c) 2026 Vaisala Oyj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zephyr specific additions to the TF-M NS interface
 */

#ifndef ZEPHYR_MODULES_TRUSTED_FIRMWARE_M_INTERFACE_TFM_NS_INTERFACE_ZEPHYR_H_
#define ZEPHYR_MODULES_TRUSTED_FIRMWARE_M_INTERFACE_TFM_NS_INTERFACE_ZEPHYR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Take the TF-M NS interface lock and never release it
 *
 * Waits for a Secure call in progress on behalf of another thread to complete
 * and blocks any further Secure calls from other threads. Secure calls made by
 * the calling thread keep working, as the lock is recursive. Meant to be
 * called right before the system is reset.
 *
 * Does nothing in ISR and pre-kernel context, where Secure calls bypass the
 * lock and blocking is not allowed.
 */
void tfm_ns_interface_lock_forever(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_TRUSTED_FIRMWARE_M_INTERFACE_TFM_NS_INTERFACE_ZEPHYR_H_ */
