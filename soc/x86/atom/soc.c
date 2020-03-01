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

#ifdef CONFIG_X86_MMU
/* loapic */
MMU_BOOT_REGION(CONFIG_LOAPIC_BASE_ADDRESS, 4*1024, MMU_ENTRY_WRITE);

/*ioapic */
MMU_BOOT_REGION(DT_IOAPIC_BASE_ADDRESS, 1024*1024, MMU_ENTRY_WRITE);

/* peripherals */
MMU_BOOT_REGION(0xB0000000, 128*1024, MMU_ENTRY_WRITE);

/* SCSS system control subsystem */
MMU_BOOT_REGION(0xB0800000, 16*1024, MMU_ENTRY_WRITE);

/* DMA */
MMU_BOOT_REGION(0xB0700000, 4*1024, MMU_ENTRY_WRITE);

/* USB */
MMU_BOOT_REGION(0xB0500000, 256*1024, MMU_ENTRY_WRITE);

#ifdef CONFIG_HPET_TIMER
MMU_BOOT_REGION(DT_INST_0_INTEL_HPET_BASE_ADDRESS, KB(4), MMU_ENTRY_WRITE);
#endif	/* CONFIG_HPET_TIMER */

#endif /* CONFIG_X86_MMU */
