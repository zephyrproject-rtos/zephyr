/*
 * Copyright (c) 2011-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for the ia32 platform
 *
 * This module provides routines to initialize and support board-level hardware
 * for the ia32 platform.
 */

#include <kernel.h>
#include "soc.h"
#include <uart.h>
#include <device.h>
#include <init.h>
#include <mmustructs.h>
#include <linker/linker-defs.h>

#ifdef CONFIG_X86_MMU
/* Mark text and rodata as read-only, for XIP rest of flash unmapped */
MMU_BOOT_REGION((u32_t)&_image_rom_start, (u32_t)&_image_rom_size,
		MMU_ENTRY_READ);

/* From _image_ram_start to the end of all RAM, read/write */
MMU_BOOT_REGION((u32_t)&_image_ram_start, (u32_t)&_image_ram_all,
		MMU_ENTRY_WRITE);

MMU_BOOT_REGION(CONFIG_LOAPIC_BASE_ADDRESS, KB(4), MMU_ENTRY_WRITE);
MMU_BOOT_REGION(CONFIG_IOAPIC_BASE_ADDRESS, MB(1), MMU_ENTRY_WRITE);
MMU_BOOT_REGION(CONFIG_HPET_TIMER_BASE_ADDRESS, KB(4), MMU_ENTRY_WRITE);

#endif /* CONFIG_X86_MMU*/
