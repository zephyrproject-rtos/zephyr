/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for boot mode interface
 */

#ifndef ZEPHYR_INCLUDE_RETENTION_BOOTMODE_
#define ZEPHYR_INCLUDE_RETENTION_BOOTMODE_

#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Boot mode interface
 * @defgroup boot_mode_interface Boot mode interface
 * @ingroup retention_api
 * @{
 */

enum BOOT_MODE_TYPES {
	/** Default (normal) boot, to user application */
	BOOT_MODE_TYPE_NORMAL = 0x00,

	/** Bootloader boot mode (e.g. serial recovery for MCUboot) */
	BOOT_MODE_TYPE_BOOTLOADER,
};

/**
 * @brief		Checks if the boot mode of the device is set to a specific value.
 *
 * @param boot_mode	Expected boot mode to check.
 *
 * @retval 1		If successful and boot mode matches.
 * @retval 0		If boot mode does not match.
 * @retval -errno	Error code code.
 */
int bootmode_check(uint8_t boot_mode);

/**
 * @brief		Sets boot mode of device.
 *
 * @param boot_mode	Boot mode value to set.
 *
 * @retval 0		If successful.
 * @retval -errno	Error code code.
 */
int bootmode_set(uint8_t boot_mode);

/**
 * @brief		Clear boot mode value (sets to 0) - which corresponds to
 *			#BOOT_MODE_TYPE_NORMAL.
 *
 * @retval 0		If successful.
 * @retval -errno	Error code code.
 */
int bootmode_clear(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RETENTION_BOOTMODE_ */
