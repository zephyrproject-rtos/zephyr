/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for boot mode interface
 */

#ifndef ZEPHYR_INCLUDE_RETENTION_BLINFO_
#define ZEPHYR_INCLUDE_RETENTION_BLINFO_

#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_RETENTION_BOOTLOADER_INFO_TYPE_MCUBOOT)
#include <bootutil/boot_status.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bootloader info interface
 * @defgroup bootloader_info_interface Bootloader info interface
 * @ingroup retention_api
 * @{
 */

#if IS_ENABLED(CONFIG_RETENTION_BOOTLOADER_INFO_OUTPUT_FUNCTION) || defined(__DOXYGEN__)
/**
 * @brief		Returns bootinfo information.
 *
 * @param key		The information to return (for MCUboot: minor TLV).
 * @param val		Where the return information will be placed.
 * @param val_len_max	The maximum size of the provided buffer.
 *
 * @retval 0		If successful.
 * @retval -EOVERFLOW	If the data is too large to fit the supplied buffer.
 * @retval -EIO		If the requested key was not found.
 * @retval -errno	Error code.
 */
int blinfo_lookup(uint16_t key, char *val, int val_len_max);
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RETENTION_BLINFO_ */
