/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for both standard images and XIP images.
 *
 * If XIP is turned on, board-level DTS must specify a flash region
 * that doesn't overlap with sram0, so that DT_PHYS_LOAD_ADDR is set.
 */

#include <autoconf.h>
#include <devicetree.h>

/* physical address where the kernel is loaded */
/* physical address of RAM */
#define PHYS_RAM_ADDR		DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define PHYS_RAM_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

#define KERNEL_BASE_ADDR (PHYS_RAM_ADDR + CONFIG_X86_KERNEL_OFFSET)
#define KERNEL_RAM_SIZE  (PHYS_RAM_SIZE - CONFIG_X86_KERNEL_OFFSET)

#ifdef CONFIG_XIP
  #define PHYS_LOAD_ADDR	DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#else  /* !CONFIG_XIP */
  #define PHYS_LOAD_ADDR	KERNEL_BASE_ADDR
#endif /* CONFIG_XIP */

MEMORY
    {
#ifdef CONFIG_XIP
    ROM (rx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#endif /* CONFIG_XIP */
    RAM (wx)        : ORIGIN = KERNEL_BASE_ADDR, LENGTH = KERNEL_RAM_SIZE

    /*
     * It doesn't matter where this region goes as it is stripped from the
     * final ELF image. The address doesn't even have to be valid on the
     * target. However, it shouldn't overlap any other regions.
     */

    IDT_LIST        : ORIGIN = 0xFFFF1000, LENGTH = 2K
    }

#ifdef CONFIG_X86_64
#include <arch/x86/intel64/linker.ld>
#else
#include <arch/x86/ia32/linker.ld>
#endif
