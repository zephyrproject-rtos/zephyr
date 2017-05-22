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

#ifdef CONFIG_X86_MMU

#ifdef CONFIG_XIP
MMU_BOOT_REGION(CONFIG_PHYS_LOAD_ADDR, CONFIG_ROM_SIZE*1024, MMU_ENTRY_READ);

MMU_BOOT_REGION(CONFIG_PHYS_RAM_ADDR, CONFIG_RAM_SIZE*1024, MMU_ENTRY_WRITE);
#else
MMU_BOOT_REGION(CONFIG_PHYS_LOAD_ADDR, CONFIG_RAM_SIZE*1024, MMU_ENTRY_WRITE);
#endif /*CONFIG_XIP */

MMU_BOOT_REGION(CONFIG_LOAPIC_BASE_ADDRESS, 4*1024, MMU_ENTRY_WRITE);
MMU_BOOT_REGION(CONFIG_IOAPIC_BASE_ADDRESS, 1024*1024, MMU_ENTRY_WRITE);

/*HPET */
MMU_BOOT_REGION(CONFIG_HPET_TIMER_BASE_ADDRESS, 4*1024, MMU_ENTRY_WRITE);

#endif /* CONFIG_X86_MMU*/
