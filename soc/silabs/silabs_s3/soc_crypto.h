/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crypto management functions for Silicon Labs Series 3 devices
 *
 */

#ifndef ZEPHYR_SOC_SILABS_SILABS_S3_SOC_CRYPTO_H_
#define ZEPHYR_SOC_SILABS_SILABS_S3_SOC_CRYPTO_H_

#include <zephyr/kernel.h>

/**
 * @defgroup silabs_s3_crypto Silicon Labs Series 3 Crypto
 * @brief Crypto management for Silicon Labs Series 3 devices
 * @{
 */

/**
 * @brief Enable crypto engine.
 *
 * @param dev Crypto device instance.
 * @param yield True if the engine should be configured to support non-blocking usage.
 *
 * @return 0 on success, negative errno on failure.
 */
int soc_crypto_enable(const struct device *dev, bool yield);

/**
 * @brief Disable crypto engine.
 *
 * @param dev Crypto device instance.
 *
 * @return 0 on success, negative errno on failure.
 */
int soc_crypto_disable(const struct device *dev);

/**
 * @brief Take a mutex on a crypto engine.
 *
 * @param dev Crypto device instance.
 *
 * @return 0 on success, negative errno on failure.
 */
int soc_crypto_get(const struct device *dev);

/**
 * @brief Release a mutex on a crypto engine.
 *
 * @param dev Crypto device instance.
 *
 * @return 0 on success, negative errno on failure.
 */
int soc_crypto_put(const struct device *dev);

/**
 * @brief Busy wait for engine to become idle.
 *
 * @param dev Crypto device instance.
 *
 * @return True if the previous operation succeeded, false otherwise.
 */
bool soc_crypto_wait_busy(const struct device *dev);

/**
 * @brief Wait until the crypto operation is complete.
 *
 * If the engine is configured to support non-blocking usage, pend on a
 * semaphore until the operation is complete. If non-blocking usage isn't
 * enabled, do nothing.
 *
 * @param dev Crypto device instance.
 *
 * @return 0 on success, negative errno on failure.
 */
void soc_crypto_wait(const struct device *dev);

/**
 * @brief Get HAL handle.
 *
 * @param dev Crypto device instance.
 *
 * @return Pointer to HAL structure.
 */
struct sx_regs *soc_crypto_get_regs(const struct device *dev);

/**
 * @brief Get crypto engine state.
 *
 * @param      dev   Crypto device instance.
 * @param[out] fetch Location to store fetcher state.
 * @param[out] push  Location to store pusher state.
 */
void soc_crypto_get_state(const struct device *dev, uint32_t *fetch, uint32_t *push);

/**
 * @brief Set crypto engine state.
 *
 * @param dev   Crypto device instance.
 * @param fetch Fetcher state to set.
 * @param push  Pusher state to set.
 */
void soc_crypto_set_state(const struct device *dev, uint32_t fetch, uint32_t push);

/** @} */

#endif /* ZEPHYR_SOC_SILABS_SILABS_S3_SOC_CRYPTO_H_ */
