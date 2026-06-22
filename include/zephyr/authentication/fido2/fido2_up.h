/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FIDO2 user presence abstraction API.
 */

#ifndef ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_FIDO2_UP_H_
#define ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_FIDO2_UP_H_

#include <zephyr/kernel.h>

/**
 * @brief FIDO2 user presence
 * @ingroup fido2
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check for user presence.
 *
 * Blocks up to CONFIG_FIDO2_UP_TIMEOUT_MS waiting for a physical user gesture.
 *
 * @retval 0 User presence confirmed.
 * @retval -ETIMEDOUT Timeout expired without user interaction.
 * @retval -ECANCELED Canceled user interaction.
 * @retval -errno On other failure.
 */
int fido2_up_wait(void);

/**
 * @brief Cancel a pending user presence wait.
 *
 * Called when the cancel command is received from a transport.
 */
void fido2_up_cancel(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_FIDO2_UP_H_ */
