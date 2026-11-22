/*
 * Copyright (c) 2026 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nordic CRACEN peripheral access arbitration
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_NORDIC_CRACEN_NORDIC_CRACEN_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_NORDIC_CRACEN_NORDIC_CRACEN_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Nordic CRACEN peripheral
 * @defgroup nordic_cracen Nordic CRACEN peripheral
 * @ingroup misc_interfaces
 * @{
 */

/**
 * @brief Acquire exclusive access to the CRACEN
 *
 * The synchronisation mechanism is selected with Kconfig:
 *  - @kconfig{CONFIG_NORDIC_CRACEN_SYNC_NONE}: no synchronisation is
 *    performed. @a timeout is ignored and the call always succeeds. The
 *    application must guarantee that the CRACEN is never used concurrently.
 *  - @kconfig{CONFIG_NORDIC_CRACEN_SYNC_IRQ_LOCK}: interrupts are locked until
 *    @ref nordic_cracen_release is called. @a timeout is ignored and the call
 *    always succeeds. Usable from any context.
 *  - @kconfig{CONFIG_NORDIC_CRACEN_SYNC_SEMAPHORE}: a semaphore is taken until
 *    @ref nordic_cracen_release is called. From interrupt context, or before
 *    the kernel has started, @a timeout is treated as @ref K_NO_WAIT.
 *
 * Access is not recursive, @ref nordic_cracen_release must be called before
 * the CRACEN is acquired again.
 *
 * Power for the individual CRACEN modules is managed by the nrfx CRACEN
 * library around each hardware operation.
 *
 * @param dev CRACEN device
 * @param timeout Time to wait for the CRACEN to become available
 *
 * @retval 0 on success
 * @retval -EBUSY CRACEN is in use and @a timeout is @ref K_NO_WAIT
 * @retval -EAGAIN Waiting for the CRACEN timed out
 */
int nordic_cracen_acquire(const struct device *dev, k_timeout_t timeout);

/**
 * @brief Release access to the CRACEN
 *
 * @param dev CRACEN device
 */
void nordic_cracen_release(const struct device *dev);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_NORDIC_CRACEN_NORDIC_CRACEN_H_ */
