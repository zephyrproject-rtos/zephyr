/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for HW Image Swapping
 *
 * HW Image Swapping API for both bootloader and application side
 */

#ifndef ZEPHYR_INCLUDE_MEMC_HWIMAGESWAP_H_
#define ZEPHYR_INCLUDE_MEMC_HWIMAGESWAP_H_

#include <sys/types.h>

#if defined(CONFIG_BOOT_HW_IMAGE_SWAP)
/**
 * @brief Configures HW image swapping based on offset of image selected for boot
 *
 * @param image_offset Offset of image bootloader chose to boot
 * @param offset_fix   Image offset adjustment bootloader must do to run selected image
 *
 * @return  0  No swapping action taken
 *         >0  HW swapping is active and return value indicates the type of HW image swapping
 *         <0  Error occurred
 */

int boot_hwimageswap_setup(uint32_t image_offset, uint32_t *offset_fix);
#endif

/**
 * @brief Returns slot number of image being executed
 *
 * @return 0 if running from slot0 (Swap is not active)
 *         1 if running from slot1 (Swap is active)
 */
uint8_t boot_hwimageswap_get_active_slot(void);

/**
 * @brief Enumeration of HW image swap types
 */
enum hwimageswap_type {

	/** Value for no action, must be zero */
	HWIMAGESWAP_NONE = 0,

	/** NXP FlexSPI address remapping */
	HWIMAGESWAP_MCUX_FLEXSPI_OVERLAY = 1,

	/** NXP MCXN Flash Swapping */
	HWIMAGESWAP_MCUX_MCXN_SWAP
};

#endif /* ZEPHYR_INCLUDE_MEMC_HWIMAGESWAP_H_ */
