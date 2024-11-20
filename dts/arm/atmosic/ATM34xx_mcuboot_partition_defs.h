/**
 *******************************************************************************
 *
 * @file ATM34xx_mcuboot_partition_defs.h
 *
 * @brief Atmosic ATM34 partition definitions for use with mcuboot
 *
 * Copyright (C) Atmosic 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *******************************************************************************
 */

#ifndef _ATMOSIC_ATM_ATM34XX_PARTITION_MCUBOOT_DEFS_H_
#define _ATMOSIC_ATM_ATM34XX_PARTITION_MCUBOOT_DEFS_H_

// include the base definitions
#include <arm/atmosic/ATM34xx_partition_defs.h>

// override the base offsets/size, these will be adjusted based on partition
// layout
#undef ATM_SPE_OFFSET
#undef ATM_NSPE_OFFSET
#undef ATM_NSPE_SIZE
#undef ATM_FAST_CODE_OFFSET
#undef ATM_FAST_CODE_SIZE
#undef ATM_FACTORY_OFFSET
#undef ATM_STORAGE_OFFSET

#define ATM_FLASH_BLOCK_SIZE 4096
#define ROUND_DOWN_FLASH_BLK(s) \
	(((s) / ATM_FLASH_BLOCK_SIZE) * ATM_FLASH_BLOCK_SIZE)

// MCUBOOT starts at the beginning of RRAM
#define ATM_MCUBOOT_OFFSET 0x0
#ifndef ATM_MCUBOOT_SIZE
#define ATM_MCUBOOT_SIZE 0x0C000
#endif
#if ((ATM_MCUBOOT_SIZE % ATM_RRAM_BLOCK_SIZE) != 0)
#error "MCUBOOT size must be aligned"
#endif

#ifndef ATM_MCUBOOT_SCRATCH_SIZE
// scratch area in external flash
#define ATM_MCUBOOT_SCRATCH_SIZE 0x4000
#endif
#if (ATM_MCUBOOT_SCRATCH_SIZE && \
	(ATM_MCUBOOT_SCRATCH_SIZE % ATM_FLASH_BLOCK_SIZE) != 0)
#error "MCUBOOT scratch size must be aligned"
#endif

// reservation for the image trailer in slot0 in RRAM (NSPE or FastCode)
// NOTE: this has to be aligned to the trailer alignment in FLASH
#ifndef ATM_SLOT0_TRAILER_RSVD_SIZE
#define ATM_SLOT0_TRAILER_RSVD_SIZE ATM_FLASH_BLOCK_SIZE
#endif
// slot trailer reservation for slot2 in flash, (taken from NSPE)
#ifndef ATM_SLOT2_TRAILER_RSVD_SIZE
#define ATM_SLOT2_TRAILER_RSVD_SIZE ATM_FLASH_BLOCK_SIZE
#endif

#if (RUN_IN_FLASH == 2) && (FLASH_SIZE < 0x100000)
// internal testing of systems with only 512KB of FLASH
// reduce slot0/1 size and give the rest to FLASH
#define SLOT0_FLASH_RESERVE (24 * 1024)
#else
#define SLOT0_FLASH_RESERVE 0
#endif

// slot 0 in RRAM follows MCUBOOT
#define ATM_SLOT0_OFFSET (ATM_MCUBOOT_OFFSET + ATM_MCUBOOT_SIZE)
// SPE is the first image in SLOT0
#define ATM_SPE_OFFSET ATM_SLOT0_OFFSET
// define the usable RRAM area for images in slot 0
#define ATM_RRAM_USABLE_AREA_SIZE (ATM_RRAM_AVAIL_SIZE - ATM_SLOT0_OFFSET \
	- ATM_TOTAL_STORAGE_SIZE - SLOT0_FLASH_RESERVE)
// slot0 size has to align with the flash block/erase size
#define ATM_SLOT0_SIZE ROUND_DOWN_FLASH_BLK(ATM_RRAM_USABLE_AREA_SIZE)
// MCUBOOT slots must be of equal size
#define ATM_SLOT1_SIZE ATM_SLOT0_SIZE

// additional sanity checks
#if ((ATM_SLOT0_OFFSET % ATM_RRAM_BLOCK_SIZE) != 0)
#error "SLOT0 offset must be aligned"
#endif

#if ((ATM_SLOT0_SIZE % ATM_RRAM_BLOCK_SIZE) != 0)
#error "SLOT0 size must be aligned to RRAM"
#endif

#if ((ATM_SLOT0_SIZE % ATM_FLASH_BLOCK_SIZE) != 0)
#error "SLOT0 size must be aligned to flash"
#endif

// storage in RRAM follows slot 0
#define ATM_DATA_START_OFFSET (ATM_SLOT0_OFFSET + ATM_SLOT0_SIZE)
#define ATM_FACTORY_OFFSET ATM_DATA_START_OFFSET
#define ATM_STORAGE_OFFSET (ATM_FACTORY_OFFSET + ATM_FACTORY_SIZE)

#ifdef RUN_IN_FLASH
// multi-image layouts
#if (RUN_IN_FLASH == 2)
// fast code in RRAM placed after the SPE
#define ATM_FAST_CODE_OFFSET (ATM_SPE_OFFSET + ATM_SPE_SIZE)
// Fast code takes up the remainder of the slot less any trailer
// reservation
#define ATM_FAST_CODE_SIZE \
    (ATM_SLOT0_SIZE - ATM_SPE_SIZE - ATM_SLOT0_TRAILER_RSVD_SIZE)
#elif (RUN_IN_FLASH == 1)
#error "Unsupported OTA configuration with SPE only in slot0"
#endif // (RUN_IN_FLASH == 2)

// slot 2 is the NSPE in FLASH
#define ATM_SLOT2_OFFSET 0
// compute usable flash for slot2/slot3
#define ATM_FLASH_USABLE_AREA_SIZE (FLASH_SIZE - ATM_MCUBOOT_SCRATCH_SIZE \
	- ATM_SLOT1_SIZE)
// split the above into equal parts and align to flash sector
#define ATM_SLOT2_SIZE ROUND_DOWN_FLASH_BLK(ATM_FLASH_USABLE_AREA_SIZE/2)

#if (ATM_SLOT2_SIZE <= 0)
#error "SLOT2 does not fit in specified FLASH_SIZE"
#endif

#if ((ATM_SLOT2_SIZE % ATM_FLASH_BLOCK_SIZE) != 0)
#error "SLOT2 size must be aligned to flash"
#endif

#define ATM_NSPE_OFFSET  ATM_SLOT2_OFFSET
// NSPE image size in external flash less any trailer reservation
#define ATM_NSPE_SIZE (ATM_SLOT2_SIZE - ATM_SLOT2_TRAILER_RSVD_SIZE)
// scratch located after slot 2
#define ATM_MCUBOOT_SCRATCH_OFFSET (ATM_SLOT2_OFFSET + ATM_SLOT2_SIZE)
// slot 1 is the upgrade slot for slot 0, follows scratch
#define ATM_SLOT1_OFFSET (ATM_MCUBOOT_SCRATCH_OFFSET + ATM_MCUBOOT_SCRATCH_SIZE)
// slot 3 is the upgrade slot for slot 2, follows slot 1
#define ATM_SLOT3_OFFSET (ATM_SLOT1_OFFSET + ATM_SLOT1_SIZE)
// upgrade slot must be equal in size
#define ATM_SLOT3_SIZE ATM_SLOT2_SIZE

#else // RUN_IN_FLASH
// single image layout
// scratch located at beginning of flash
#define ATM_MCUBOOT_SCRATCH_OFFSET 0x0
#define ATM_NSPE_OFFSET (ATM_SLOT0_OFFSET + ATM_SPE_SIZE)
#define ATM_NSPE_SIZE \
    (ATM_SLOT0_SIZE - ATM_SPE_SIZE - ATM_SLOT0_TRAILER_RSVD_SIZE)
// slot 1 follows SCRATCH
#define ATM_SLOT1_OFFSET (ATM_MCUBOOT_SCRATCH_OFFSET + ATM_MCUBOOT_SCRATCH_SIZE)
#endif // RUN_IN_FLASH

#ifdef ATM_FAST_CODE_SIZE
#define ATM_SLOT0_TRAILER_RSVD_OFFSET \
    (ATM_FAST_CODE_OFFSET + ATM_FAST_CODE_SIZE)
#define ATM_SLOT2_TRAILER_RSVD_OFFSET (ATM_NSPE_OFFSET + ATM_NSPE_SIZE)
#else
#define ATM_SLOT0_TRAILER_RSVD_OFFSET (ATM_NSPE_OFFSET + ATM_NSPE_SIZE)
#endif

#if ATM_SLOT0_TRAILER_RSVD_SIZE
#if ((ATM_SLOT0_TRAILER_RSVD_OFFSET % ATM_FLASH_BLOCK_SIZE) != 0)
#error "SLOT0 trailer offset must be aligned to FLASH block size"
#endif
#if ((ATM_SLOT0_TRAILER_RSVD_SIZE % ATM_FLASH_BLOCK_SIZE) != 0)
#error "SLOT0 trailer size must be aligned to FLASH block size"
#endif
#endif // ATM_SLOT0_TRAILER_RSVD_SIZE

#if ATM_SLOT2_TRAILER_RSVD_SIZE
#if ((ATM_SLOT2_TRAILER_RSVD_OFFSET % ATM_FLASH_BLOCK_SIZE) != 0)
#error "SLOT2 trailer offset must be aligned to FLASH block size"
#endif
#if ((ATM_SLOT2_TRAILER_RSVD_SIZE % ATM_FLASH_BLOCK_SIZE) != 0)
#error "SLOT2 trailer size must be aligned to FLASH block size"
#endif
#endif // ATM_SLOT2_TRAILER_RSVD_SIZE

// final checks against FLASH_SIZE
#ifdef RUN_IN_FLASH
#if ((ATM_SLOT3_OFFSET + ATM_SLOT3_SIZE) > FLASH_SIZE)
#error "Last partition does not fit in specified FLASH_SIZE"
#endif
#else
#if ((ATM_SLOT1_OFFSET + ATM_SLOT1_SIZE) > FLASH_SIZE)
#error "Last partition does not fit in specified FLASH_SIZE"
#endif
#endif

// NODE unit addresses, these are arbitrary and only need be unique
#define ATM_MCUBOOT_NODE_ID         cece0000
#define ATM_MCUBOOT_SCRATCH_NODE_ID cece0001
#define ATM_SLOT0_NODE_ID           cece0010
#define ATM_SLOT0_TRAILER_NODE_ID   cece0015
#define ATM_SLOT1_NODE_ID           cece0040
#define ATM_SLOT2_NODE_ID           cece0060
#define ATM_SLOT2_TRAILER_NODE_ID   cece0065
#define ATM_SLOT3_NODE_ID           cece0070

#endif // _ATMOSIC_ATM_ATM34XX_PARTITION_MCUBOOT_DEFS_H_
