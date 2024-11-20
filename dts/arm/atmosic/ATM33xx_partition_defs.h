/**
 *******************************************************************************
 *
 * @file ATM33xx_partition_defs.h
 *
 * @brief Atmosic ATM33 image partition definitions
 *
 * Copyright (C) Atmosic 2023-2024
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *******************************************************************************
 */

#ifndef _ATMOSIC_ATM_ATM33XX_PARTITION_DEFS_H_
#define _ATMOSIC_ATM_ATM33XX_PARTITION_DEFS_H_

#ifdef ATM_APP_PART_DEFS
#include ATM_APP_PART_DEFS
#endif

#define ATM_RRAM_BLOCK_SIZE 2048
#define ROUND_DOWN_RRAM_BLK(s) (((s) / ATM_RRAM_BLOCK_SIZE) * ATM_RRAM_BLOCK_SIZE)

#ifndef ATM_RRAM_AVAIL_SIZE
#define ATM_RRAM_AVAIL_SIZE (510*1024)
#endif

#ifndef ATM_SPE_SIZE
#define ATM_SPE_SIZE (22 * 1024)
#endif
#if ((ATM_SPE_SIZE % ATM_RRAM_BLOCK_SIZE) != 0)
#error "SPE size must be aligned"
#endif

#ifndef ATM_FACTORY_SIZE
#define ATM_FACTORY_SIZE 0x800
#endif
#if ((ATM_FACTORY_SIZE % ATM_RRAM_BLOCK_SIZE) != 0)
#error "Factory size must be aligned"
#endif

#ifndef ATM_STORAGE_SIZE
#define ATM_STORAGE_SIZE 0x800
#endif
#if ((ATM_STORAGE_SIZE % ATM_RRAM_BLOCK_SIZE) != 0)
#error "Storage size must be aligned"
#endif

#define _MAKE_ATMWSTK_FLAVOR(f) ATMWSTK_FLAVOR_##f
#define MAKE_ATMWSTK_FLAVOR(f) _MAKE_ATMWSTK_FLAVOR(f)
#define ATMWSTK_FLAVOR MAKE_ATMWSTK_FLAVOR(ATMWSTK)
#define ATMWSTK_FLAVOR_LL 1
#define ATMWSTK_FLAVOR_PD50LL 2

#ifdef ATMWSTK
#if (ATMWSTK_FLAVOR == ATMWSTK_FLAVOR_PD50LL)
#define ATMWSTK_SIZE  0x16800
#define ATMWSTK_OFFSET 0x69000
#elif (ATMWSTK_FLAVOR == ATMWSTK_FLAVOR_LL)
// LL flavor
#define ATMWSTK_SIZE  0x21800
#define ATMWSTK_OFFSET 0x5e000
#else
#error "Valid ATMWSTK flavor must be set"
#endif // ATMWSTK_FLAVOR
#else
#define ATMWSTK_SIZE 0
#undef ATMWSTK_OFFSET
#endif // ATMWSTK

#define ATM_SPE_OFFSET 0x0
#define ATM_NSPE_OFFSET (ATM_SPE_OFFSET + ATM_SPE_SIZE)
#define ATM_NSPE_SIZE (ATM_RRAM_AVAIL_SIZE - ATM_SPE_SIZE - ATMWSTK_SIZE \
    - ATM_FACTORY_SIZE - ATM_STORAGE_SIZE)
#define ATM_FACTORY_OFFSET (ATM_NSPE_OFFSET + ATM_NSPE_SIZE)
#define ATM_STORAGE_OFFSET (ATM_FACTORY_OFFSET + ATM_FACTORY_SIZE)

// NODE unit addresses, these are arbitrary and only need be unique
#define ATM_SPE_NODE_ID     cece0011
#define ATM_NSPE_NODE_ID    cece0012
#define ATM_FACTORY_NODE_ID cece0050
#define ATM_STORAGE_NODE_ID cece0051
#define ATMWSTK_NODE_ID     cece0060

#endif // _ATMOSIC_ATM_ATM33XX_PARTITION_DEFS_H_
