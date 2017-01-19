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

/* physical address of RAM (needed for correct __ram_phys_end symbol) */
#define PHYS_RAM_ADDR    CONFIG_PHYS_RAM_ADDR

/* physical address where the kernel is loaded */
#ifdef CONFIG_XIP
  #define PHYS_LOAD_ADDR   CONFIG_PHYS_LOAD_ADDR
#else  /* !CONFIG_XIP */
  #define PHYS_LOAD_ADDR   CONFIG_PHYS_RAM_ADDR
#endif /* CONFIG_XIP */

MEMORY
    {
#ifdef CONFIG_XIP
    ROM (rx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = CONFIG_ROM_SIZE*1K
    RAM (wx)        : ORIGIN = PHYS_RAM_ADDR, LENGTH =  CONFIG_RAM_SIZE*1K
#else  /* !CONFIG_XIP */
    RAM (wx)	    : ORIGIN = PHYS_LOAD_ADDR, LENGTH = CONFIG_RAM_SIZE*1K
#endif /* CONFIG_XIP */

    /*
     * It doesn't matter where this region goes as it is stripped from the
     * final ELF image. The address doesn't even have to be valid on the
     * target. However, it shouldn't overlap any other regions.
     */

    IDT_LIST        : ORIGIN = 512, LENGTH = 512
    }

#include <arch/x86/linker.ld>

