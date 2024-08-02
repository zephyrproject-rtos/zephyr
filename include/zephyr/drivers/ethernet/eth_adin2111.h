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

/**
 * @brief Update host MAC interface register over SPI
 *
 * @note The caller is responsible for device lock.
 *       Shall not be called from ISR.
 *
 * @param[in] dev ADIN2111 device.
 * @param reg Register address.
 * @param mask Bitmask for bits that may be modified.
 * @param data Data to apply in the masked range.
 *
 * @retval 0 Successful write.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin2111_reg_update(const struct device *dev, const uint16_t reg,
			    uint32_t mask, uint32_t data);

/**
 * @brief Reset both the MAC and PHY.
 *
 * @param[in] dev ADIN2111 device.
 * @param delay Delay in milliseconds.
 *
 * @note The caller is responsible for device lock.
 *       Shall not be called from ISR.
 *
 * @retval 0 Successful write.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin2111_sw_reset(const struct device *dev, uint16_t delay);

/**
 * @brief Reset the MAC device. Note that PHY 1 must be out of software power-down for the MAC
 *        subsystem reset to take effect.
 *
 * @note The caller is responsible for device lock.
 *       Shall not be called from ISR.
 *
 * @param[in] dev ADIN2111 device.
 *
 * @retval 0 Successful write.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin2111_mac_reset(const struct device *dev);

/**
 * @brief Enable/disable the forwarding (to host) of broadcast frames. Frames who's DA
 *        doesn't match are dropped.
 *
 * @note The caller is responsible for device lock.
 *       Shall not be called from ISR.
 *
 * @param[in] dev ADIN2111 device.
 * @param enable Set to 0 to disable and to nonzero to enable.
 *
 * @retval 0 Successful write.
 * @retval <0 Error, a negative errno code.
 */
int eth_adin2111_broadcast_filter(const struct device *dev, bool enable);

/**
 * @brief Get the port-related net_if reference.
 *
 * @param[in] dev ADIN2111 device.
 * @param port_idx Port index.
 *
 * @retval a struct net_if pointer, or NULL on error.
 */
struct net_if *eth_adin2111_get_iface(const struct device *dev, const uint16_t port_idx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_ADIN2111_H__ */
