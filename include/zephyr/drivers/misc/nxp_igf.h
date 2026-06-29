/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_IGF_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_IGF_H_

/**
 * @file
 * @brief NXP Input Glitch Filter (IGF) device-specific API.
 *
 * The IGF channels are configured statically from devicetree and applied by
 * the driver at init time. The only runtime operations exposed are reading and
 * clearing a channel's status flags. These functions are not ISR-safe and have
 * no internal locking; callers must serialize concurrent access to the same
 * device. Operations on distinct channels do not interfere.
 */

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name IGF channel status flags
 *
 * Bit positions match the IGF MSR (Module Status Register) fields, i.e. the
 * IGF_MSR_*_MASK definitions in the fsl_igf register header.
 * @{
 */
/** Filtered output level (MSR.FLO). */
#define NXP_IGF_STATUS_OUTPUT_LEVEL    BIT(0)
/** Filter input level (MSR.FLI). */
#define NXP_IGF_STATUS_INPUT_LEVEL     BIT(1)
/** Noise detected after a falling edge (MSR.FNDET). */
#define NXP_IGF_STATUS_FALL_NOISE      BIT(2)
/** Noise detected after a rising edge (MSR.RNDET). */
#define NXP_IGF_STATUS_RISE_NOISE      BIT(3)
/** Filter is processing an edge (MSR.FEDGE). */
#define NXP_IGF_STATUS_PROCESSING_EDGE BIT(4)
/** Filter is waiting for an edge (MSR.WEDGE). */
#define NXP_IGF_STATUS_WAITING_EDGE    BIT(5)
/** @} */

/**
 * @brief Read the status flags of an IGF channel.
 *
 * @param dev IGF device instance.
 * @param channel Channel index (0..31).
 * @param flags Output: the channel's MSR status flags (NXP_IGF_STATUS_*).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p channel is out of range or @p flags is NULL.
 * @retval -ENODEV if the device is not ready.
 */
int nxp_igf_get_status(const struct device *dev, uint32_t channel, uint32_t *flags);

/**
 * @brief Clear status flags of an IGF channel.
 *
 * @param dev IGF device instance.
 * @param channel Channel index (0..31).
 * @param mask Mask of status flags to clear (NXP_IGF_STATUS_*).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p channel is out of range.
 * @retval -ENODEV if the device is not ready.
 */
int nxp_igf_clear_status(const struct device *dev, uint32_t channel, uint32_t mask);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_IGF_H_ */
