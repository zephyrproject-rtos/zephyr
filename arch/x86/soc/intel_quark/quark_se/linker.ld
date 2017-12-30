/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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

#include <generated_dts_board.h>
/* physical address of RAM (needed for correct __ram_phys_end symbol) */
#define PHYS_RAM_ADDR    CONFIG_PHYS_RAM_ADDR

/* physical address where the kernel is loaded */
#ifdef CONFIG_XIP
  #define PHYS_LOAD_ADDR   CONFIG_PHYS_LOAD_ADDR
#else  /* !CONFIG_XIP */
  #define PHYS_LOAD_ADDR   CONFIG_PHYS_RAM_ADDR
#endif /* CONFIG_XIP */

/* total shared RAM size (restore info + gdt) */
#define BSP_SHARED_RAM_SIZE \
	(CONFIG_SHARED_GDT_RAM_SIZE+CONFIG_BSP_SHARED_RESTORE_INFO_SIZE)
/*
 * When DEEP_SLEEP is not enabled, the shared memory can be re-used after boot.
 * Since the memory regions area are at fixed address locations, only the
 * contiguous unused memory located at the end of RAM should be made available.
 */
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
#define PHYS_RAM_SIZE CONFIG_RAM_SIZE*1K - BSP_SHARED_RAM_SIZE
#else
#ifdef CONFIG_SET_GDT
#define PHYS_RAM_SIZE CONFIG_RAM_SIZE*1K
#else
#define PHYS_RAM_SIZE CONFIG_RAM_SIZE*1K - CONFIG_SHARED_GDT_RAM_SIZE
#endif
#endif

MEMORY
    {
#ifdef CONFIG_XIP
    ROM (rx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = CONFIG_ROM_SIZE*1K
    RAM (wx)        : ORIGIN = PHYS_RAM_ADDR, LENGTH =  PHYS_RAM_SIZE
#else  /* !CONFIG_XIP */
    RAM (wx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = PHYS_RAM_SIZE
#endif /* CONFIG_XIP */
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
    BSP_SHARED_RAM (rw) : ORIGIN = CONFIG_BSP_SHARED_RESTORE_INFO_RAM_ADDR,
                          LENGTH = CONFIG_BSP_SHARED_RESTORE_INFO_SIZE
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP */
#ifndef CONFIG_SET_GDT
    SHARED_GDT_RAM (rw) : ORIGIN = CONFIG_SHARED_GDT_RAM_ADDR,
                              LENGTH = CONFIG_SHARED_GDT_RAM_SIZE
#endif /* !CONFIG_SET_GDT */

    /*
     * It doesn't matter where this region goes as it is stripped from the
     * final ELF image. The address doesn't even have to be valid on the
     * target. However, it shouldn't overlap any other regions.
     */

    IDT_LIST        : ORIGIN = 2K, LENGTH = 2K
    MMU_LIST	    : ORIGIN = 4k, LENGTH = 1K
    }

#include <arch/x86/linker.ld>

