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
#include <drivers/uart.h>
#include <device.h>
#include <init.h>
#include <arch/x86/mmustructs.h>
#include <linker/linker-defs.h>

#ifdef CONFIG_X86_MMU
MMU_BOOT_REGION(CONFIG_LOAPIC_BASE_ADDRESS, KB(4), MMU_ENTRY_WRITE);
MMU_BOOT_REGION(DT_IOAPIC_BASE_ADDRESS, MB(1), MMU_ENTRY_WRITE);

#ifdef CONFIG_HPET_TIMER
MMU_BOOT_REGION(DT_INST_0_INTEL_HPET_BASE_ADDRESS, KB(4), MMU_ENTRY_WRITE);
#endif

#ifdef CONFIG_ETH_E1000
MMU_BOOT_REGION(DT_ETH_E1000_BASE_ADDRESS, KB(128), MMU_ENTRY_WRITE);
#endif

#endif /* CONFIG_X86_MMU*/
