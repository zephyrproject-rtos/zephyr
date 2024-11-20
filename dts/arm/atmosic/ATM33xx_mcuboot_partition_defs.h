/**
 *******************************************************************************
 *
 * @file ATM33xx_mcuboot_partition_defs.h
 *
 * @brief Atmosic ATM33 image partition definitions for use with mcuboot
 *
 * Copyright (C) Atmosic 2023-2024
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *******************************************************************************
 */

#ifndef _ATMOSIC_ATM_ATM33XX_PARTITION_MCUBOOT_DEFS_H_
#define _ATMOSIC_ATM_ATM33XX_PARTITION_MCUBOOT_DEFS_H_

// include the base definitions
#include <arm/atmosic/ATM33xx_partition_defs.h>

// override the base definitions
#undef ATM_SPE_OFFSET
#undef ATM_NSPE_OFFSET
#undef ATM_NSPE_SIZE
#undef ATM_FACTORY_OFFSET
#undef ATM_STORAGE_OFFSET

#define ATM_FLASH_BLOCK_SIZE 4096
#define ROUND_DOWN_FLASH_BLK(s) (((s) / ATM_FLASH_BLOCK_SIZE) * ATM_FLASH_BLOCK_SIZE)

// MCUBOOT starts at the beginning of RRAM
#define ATM_MCUBOOT_OFFSET 0x0
#ifndef ATM_MCUBOOT_SIZE
#define ATM_MCUBOOT_SIZE 0x0C000
#endif
#if ((ATM_MCUBOOT_SIZE % ATM_RRAM_BLOCK_SIZE) != 0)
#error "MCUBOOT size must be aligned"
#endif

#ifndef ATM_MCUBOOT_SCRATCH_SIZE
#ifdef DFU_IN_FLASH
// larger scratch area for external flash
#define ATM_MCUBOOT_SCRATCH_SIZE 0x4000
#else
#define ATM_MCUBOOT_SCRATCH_SIZE 0x2000
#endif // DFU_IN_FLASH
#endif // ATM_MCUBOOT_SCRATCH_SIZE
#if ((ATM_MCUBOOT_SCRATCH_SIZE % ATM_RRAM_BLOCK_SIZE) != 0)
#error "MCUBOOT scratch size must be aligned"
#endif

// reservation for the image trailer (taken from NSPE)
#ifndef ATM_SLOT0_TRAILER_RSVD_SIZE
#ifdef DFU_IN_FLASH
// trailer aligned to FLASH
#define ATM_SLOT0_TRAILER_RSVD_SIZE ATM_FLASH_BLOCK_SIZE
#else
#define ATM_SLOT0_TRAILER_RSVD_SIZE ATM_RRAM_BLOCK_SIZE
#endif // DFU_IN_FLASH
#endif // ATM_SLOT0_TRAILER_RSVD_SIZE

#ifdef DFU_IN_FLASH
// scratch located at beginning of flash
#define ATM_MCUBOOT_SCRATCH_OFFSET 0x0
// slot 0 in RRAM follows MCUBOOT
#define ATM_SLOT0_OFFSET (ATM_MCUBOOT_OFFSET + ATM_MCUBOOT_SIZE)
// slot 1 in flash, follows SCRATCH
#define ATM_SLOT1_OFFSET (ATM_MCUBOOT_SCRATCH_OFFSET + ATM_MCUBOOT_SCRATCH_SIZE)
#else
// SCRATCH located in RRAM after MCUBOOT
#define ATM_MCUBOOT_SCRATCH_OFFSET (ATM_MCUBOOT_OFFSET + ATM_MCUBOOT_SIZE)
// slot 0 in RRAM follows SCRATCH
#define ATM_SLOT0_OFFSET (ATM_MCUBOOT_SCRATCH_OFFSET + ATM_MCUBOOT_SCRATCH_SIZE)
// slot 1 in RRAM follows slot 0
#define ATM_SLOT1_OFFSET (ATM_SLOT0_OFFSET + ATM_SLOT0_SIZE)
#endif // DFU_IN_FLASH

// compute the slot size needed
#define ATM_RRAM_IMAGE_AREA_SIZE (ATM_RRAM_AVAIL_SIZE - ATM_SLOT0_OFFSET - \
    ATMWSTK_SIZE - ATM_FACTORY_SIZE - ATM_STORAGE_SIZE)

#ifdef DFU_IN_FLASH
// slot0 size has to align with the flash block/erase size
#define ATM_SLOT0_SIZE ROUND_DOWN_FLASH_BLK(ATM_RRAM_IMAGE_AREA_SIZE)
// storage in RRAM follows slot 0
#define ATM_FACTORY_OFFSET (ATM_SLOT0_OFFSET + ATM_SLOT0_SIZE)
#define ATM_STORAGE_OFFSET (ATM_FACTORY_OFFSET + ATM_FACTORY_SIZE)
#else
// split available RRAM in half, round down to RRAM block size
#define ATM_SLOT0_SIZE ROUND_DOWN_RRAM_BLK(ATM_RRAM_IMAGE_AREA_SIZE / 2)
// storage in RRAM follows slot 1
#define ATM_FACTORY_OFFSET (ATM_SLOT1_OFFSET + ATM_SLOT0_SIZE)
#define ATM_STORAGE_OFFSET (ATM_FACTORY_OFFSET + ATM_FACTORY_SIZE)
#endif

#if ((ATM_FACTORY_OFFSET % ATM_RRAM_BLOCK_SIZE) != 0)
#error "Factory offset must be aligned"
#endif

#if ((ATM_STORAGE_OFFSET % ATM_RRAM_BLOCK_SIZE) != 0)
#error "Storage offset must be aligned"
#endif

// additional sanity checks
#if ((ATM_SLOT0_OFFSET % ATM_RRAM_BLOCK_SIZE) != 0)
#error "SLOT0 offset must be aligned"
#endif

#if ((ATM_SLOT0_SIZE % ATM_RRAM_BLOCK_SIZE) != 0)
#error "SLOT0 size must be aligned to RRAM"
#endif

#ifdef DFU_IN_FLASH
#if ((ATM_SLOT0_SIZE % ATM_FLASH_BLOCK_SIZE) != 0)
#error "SLOT0 size must be aligned to flash"
#endif // ATM_SLOT0_SIZE
#endif // DFU_IN_FLASH

// MCUBOOT slots must be of equal size
#define ATM_SLOT1_SIZE ATM_SLOT0_SIZE
#define ATM_SPE_OFFSET ATM_SLOT0_OFFSET
#define ATM_NSPE_OFFSET (ATM_SPE_OFFSET + ATM_SPE_SIZE)
#define ATM_NSPE_SIZE \
    (ATM_SLOT0_SIZE - ATM_SPE_SIZE - ATM_SLOT0_TRAILER_RSVD_SIZE)
#define ATM_SLOT0_TRAILER_RSVD_OFFSET (ATM_NSPE_OFFSET + ATM_NSPE_SIZE)

// sanity check the trailer reservation
#if ATM_SLOT0_TRAILER_RSVD_SIZE
#ifdef DFU_IN_FLASH
#if ((ATM_SLOT0_TRAILER_RSVD_SIZE % ATM_FLASH_BLOCK_SIZE) != 0)
#error "SLOT0 trailer size must be aligned to FLASH block size"
#endif
#if ((ATM_SLOT0_TRAILER_RSVD_OFFSET % ATM_FLASH_BLOCK_SIZE) != 0)
#error "SLOT0 trailer offset must be aligned to FLASH block size"
#endif
#else // DFU_IN_FLASH
#if ((ATM_SLOT0_TRAILER_RSVD_SIZE % ATM_RRAM_BLOCK_SIZE) != 0)
#error "SLOT0 trailer size must be aligned to RRAM block size"
#endif
#if ((ATM_SLOT0_TRAILER_RSVD_OFFSET % ATM_RRAM_BLOCK_SIZE) != 0)
#error "SLOT0 trailer offset must be aligned to RRAM block size"
#endif
#endif // DFU_IN_FLASH
#endif // ATM_SLOT0_TRAILER_RSVD_SIZE

// NODE unit addresses, these are arbitrary and only need be unique
#define ATM_MCUBOOT_NODE_ID         cece0000
#define ATM_MCUBOOT_SCRATCH_NODE_ID cece0001
#define ATM_SLOT0_NODE_ID           cece0010
#define ATM_SLOT0_TRAILER_NODE_ID   cece0015
#define ATM_SLOT1_NODE_ID           cece0040

#endif // _ATMOSIC_ATM_ATM33XX_PARTITION_MCUBOOT_DEFS_H_
