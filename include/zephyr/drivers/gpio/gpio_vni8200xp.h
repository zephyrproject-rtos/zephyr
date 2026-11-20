/*
 * Copyright (c) 2026 Sebastian Balling
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended API of the ST VNI8200XP GPIO driver.
 *
 * The outputs are used through the regular GPIO API. This header adds access
 * to the 16-bit fault frame the chip returns on every SPI write
 * (datasheet DS11044, table 17).
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_VNI8200XP_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_VNI8200XP_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** DC-DC regulation ok (FB above 90 % of nominal). Expected to be set. */
#define VNI8200XP_STATUS_FBOK       BIT(7)
/** Case over-temperature warning (not latched). */
#define VNI8200XP_STATUS_TWARN      BIT(6)
/** Parity check fail or frame length not a multiple of 8 (previous frame). */
#define VNI8200XP_STATUS_PC         BIT(5)
/** Power good diagnostic active: VCC below the power good threshold. */
#define VNI8200XP_STATUS_PG         BIT(4)
/** Mask of the four general status flags above. */
#define VNI8200XP_STATUS_FLAGS_MASK 0xF0U

/** Fault frame of the last successful SPI transfer. */
struct vni8200xp_status {
	/** FBOK, TWARN, PC, PG (upper nibble) and the frame parity (lower nibble). */
	uint8_t status;
	/** Latched channel over-temperature faults, bit n = OUT(n+1). */
	uint8_t faults;
};

/**
 * @brief Get the fault frame returned by the last successful SPI write.
 *
 * @param dev VNI8200XP device.
 * @param status Filled with the last fault frame.
 *
 * @retval 0 on success.
 * @retval -ENODATA if no valid frame has been received yet.
 */
int vni8200xp_get_status(const struct device *dev, struct vni8200xp_status *status);

/**
 * @brief Check a fault frame for fault conditions.
 *
 * @return true if FBOK is set and no other flag and no channel fault is set.
 */
static inline bool vni8200xp_status_ok(const struct vni8200xp_status *s)
{
	return ((s->status & VNI8200XP_STATUS_FLAGS_MASK) == VNI8200XP_STATUS_FBOK) &&
	       (s->faults == 0U);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_VNI8200XP_H_ */
