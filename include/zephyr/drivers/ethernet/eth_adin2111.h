/*
 * Copyright (c) 2023 PHOENIX CONTACT Electronics GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETH_ADIN2111_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETH_ADIN2111_H__

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Locks device access
 *
 * @param[in] dev ADIN2111 device.
 * @param timeout Waiting period to lock the device,
 *                or one of the special values K_NO_WAIT and
 *                K_FOREVER.
 *
 * @retval 0 Device locked.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int eth_adin2111_lock(const struct device *dev, k_timeout_t timeout);

/**
 * @brief Unlocks device access
 *
 * @param[in] dev ADIN2111 device.
 *
 * @retval 0 Device unlocked.
 * @retval -EPERM The current thread does not own the device lock.
 * @retval -EINVAL The device is not locked.
 */
int eth_adin2111_unlock(const struct device *dev);

/**
 * @brief Writes host MAC interface register over SPI
 *
 * @note The caller is responsible for device lock.
 *       Shall not be called from ISR.
 *
 * @param[in] dev ADIN2111 device.
 * @param reg Register address.
 * @param val Value to write.
 *
 * @retval 0 Successful write.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin2111_reg_write(const struct device *dev, const uint16_t reg, uint32_t val);

/**
 * @brief Reads host MAC interface register over SPI
 *
 * @note The caller is responsible for device lock.
 *       Shall not be called from ISR.
 *
 * @param[in] dev ADIN2111 device.
 * @param reg Register address.
 * @param[out] val Read value output.
 *
 * @retval 0 Successful write.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin2111_reg_read(const struct device *dev, const uint16_t reg, uint32_t *val);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_ADIN2111_H__ */
