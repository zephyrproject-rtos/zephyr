/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for both standard images and XIP images.
 */

#include <autoconf.h>

/*
 * KW41Z Flash configuration fields
 * These are 16 bytes, which must be loaded to address 0x400, and include
 * default protection and security settings.
 * They are loaded at reset to various Flash Memory module (FTFE) registers.
 */

/*
 * No need to account for this when running a RAM-only image since that
 * security feature resides in ROM.
 */
#if defined(CONFIG_XIP)
	#define SKIP_TO_KINETIS_FLASH_CONFIG . = 0x400;
#endif

#include <arch/arm/cortex_m/scripts/linker.ld>
