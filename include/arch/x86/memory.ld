/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 * Copyright (c) 2019-2020 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Directives for linker MEMORY regions for all x86
 *
 * By default, the kernel is linked at its physical address and all addresses
 * are in RAM.
 *
 * If CONFIG_XIP is enabled, then another MEMORY region is declared for ROM,
 * and this is where the Zephyr image is booted from. The linker LMAs and VMAs
 * are set up, such that read/write data/bss have their VMA addresses
 * in RAM and are copied from flash at boot. Text/rodata linked in-place in
 * flash.
 */

#ifndef ARCH_X86_MEMORY_LD
#define ARCH_X86_MEMORY_LD

#include <autoconf.h>
#include <devicetree.h>

/* Bounds of physical RAM from DTS */
#define PHYS_RAM_ADDR		DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define PHYS_RAM_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

#define KERNEL_BASE_ADDR  (PHYS_RAM_ADDR + CONFIG_X86_KERNEL_OFFSET)
#define KERNEL_RAM_SIZE   (PHYS_RAM_SIZE - CONFIG_X86_KERNEL_OFFSET)

#ifdef CONFIG_XIP
  /* "ROM" is flash, we leave rodata and text there and just copy in data.
   * Board-level DTS must specify a flash region that doesn't overlap with
   * sram0, so that DT_PHYS_LOAD_ADDR is set.
   */
  #define FLASH_ROM_SIZE      DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
  #define PHYS_LOAD_ADDR      DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#endif

#ifdef CONFIG_X86_64
/* Locore must be addressable by real mode and so cannot extend past 64K.
 * Skip reserved stuff in the first page
 */
#define LOCORE_BASE         0x1000
#define LOCORE_SIZE         (0x10000 - LOCORE_BASE)
#endif

MEMORY
    {
#if defined(CONFIG_XIP)
    /* Address range where the kernel will be installed on a flash part (XIP),
     * or copied into physical RAM by a loader (MMU)
     */
    ROM (rx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = FLASH_ROM_SIZE
#endif
    /* Linear address range to link the kernel. If non-XIP, everything is
     * linked in this space. Otherwise, rodata and text are linked at their
     * physical ROM locations
     */
    RAM (wx)        : ORIGIN = KERNEL_BASE_ADDR, LENGTH = KERNEL_RAM_SIZE

#ifdef CONFIG_X86_64
    /* Special low-memory area for bootstrapping other CPUs from real mode */
    LOCORE (wx)     : ORIGIN = LOCORE_BASE, LENGTH = LOCORE_SIZE
#else
    /*
     * On 32-bit x86, fake memory area for build-time IDT generation data.
     * 64-bit doesn't use this, interrupts are all manaaged at runtime.
     *
     * It doesn't matter where this region goes as it is stripped from the
     * final ELF image. The address doesn't even have to be valid on the
     * target. However, it shouldn't overlap any other regions.
     */

    IDT_LIST        : ORIGIN = 0xFFFF1000, LENGTH = 2K
#endif /* !CONFIG_X86_64 */
    }
#endif /* ARCH_X86_MEMORY_LD */
