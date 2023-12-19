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
 *
 * If CONFIG_MMU is enabled, then the ROM region in MEMORY is used to set the
 * LMA for all sections relative to physical address. The virtual address VMAs
 * for all sections are relative to the base virtual address for the kernel.
 * Setting LMAs here helps let QEMU or any other ELF-aware loader know where to
 * physically load the image.
 */

#ifndef ARCH_X86_MEMORY_LD
#define ARCH_X86_MEMORY_LD

#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/kernel/mm.h>

/* Bounds of physical RAM from DTS */
#define PHYS_RAM_ADDR		DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define PHYS_RAM_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

/* Virtual base address for the kernel; with CONFIG_MMU this is not necessarily
 * the same as its physical location, although an identity mapping for RAM
 * is still supported by setting CONFIG_KERNEL_VM_BASE=CONFIG_SRAM_BASE_ADDRESS.
 */
#ifdef Z_VM_KERNEL
#define KERNEL_BASE_ADDR  (CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_OFFSET)
#define KERNEL_RAM_SIZE   (CONFIG_KERNEL_VM_SIZE - CONFIG_KERNEL_VM_OFFSET)
#define PHYS_RAM_AVAIL    (PHYS_RAM_SIZE - CONFIG_SRAM_OFFSET)
#else
#define KERNEL_BASE_ADDR  (PHYS_RAM_ADDR + CONFIG_SRAM_OFFSET)
#define KERNEL_RAM_SIZE   (PHYS_RAM_SIZE - CONFIG_SRAM_OFFSET)
#endif

/* "kernel RAM" for linker VMA allocations starts at the offset */

#if defined(CONFIG_ROM_END_OFFSET)
#define ROM_END_OFFSET CONFIG_ROM_END_OFFSET
#else
#define ROM_END_OFFSET 0
#endif

#ifdef CONFIG_XIP
  /* "ROM" is flash, we leave rodata and text there and just copy in data.
   * Board-level DTS must specify a flash region that doesn't overlap with
   * sram0, so that DT_PHYS_LOAD_ADDR is set.
   */
  #define FLASH_ROM_SIZE      (DT_REG_SIZE(DT_CHOSEN(zephyr_flash)) - ROM_END_OFFSET)
  #define PHYS_LOAD_ADDR      DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#else
  /* Physical RAM location where the kernel image is loaded */
  #define PHYS_LOAD_ADDR      (PHYS_RAM_ADDR + CONFIG_SRAM_OFFSET)
#endif /* CONFIG_XIP */

#ifdef CONFIG_X86_64
/* Locore must be addressable by real mode and so cannot extend past 64K.
 * Skip reserved stuff in the first page
 */
#define LOCORE_BASE         0x1000
#define LOCORE_SIZE         (0x10000 - LOCORE_BASE)

#if PHYS_RAM_ADDR != CONFIG_KERNEL_VM_BASE
#error Virtual kernel linking is not yet implemented for 64-bit
#endif
#endif /* CONFIG_X86_64 */

MEMORY
    {
#if defined(CONFIG_XIP)
    /* Address range where the kernel will be installed on a flash part (XIP),
     * or copied into physical RAM by a loader (MMU)
     */
    ROM (rx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = FLASH_ROM_SIZE
#elif defined(Z_VM_KERNEL)
    ROM (rx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = PHYS_RAM_AVAIL
#endif
    /* Linear address range to link the kernel. If non-XIP, everything is
     * linked in this space. Otherwise, rodata and text are linked at their
     * physical ROM locations
     */
    RAM (wx)        : ORIGIN = KERNEL_BASE_ADDR, LENGTH = KERNEL_RAM_SIZE
    LINKER_DT_REGIONS()

#ifdef CONFIG_X86_64
    /* Special low-memory area for bootstrapping other CPUs from real mode */
    LOCORE (wx)     : ORIGIN = LOCORE_BASE, LENGTH = LOCORE_SIZE
#else
    /*
     * On 32-bit x86, fake memory area for build-time IDT generation data.
     * 64-bit doesn't use this, interrupts are all managed at runtime.
     *
     * It doesn't matter where this region goes as it is stripped from the
     * final ELF image. The address doesn't even have to be valid on the
     * target. However, it shouldn't overlap any other regions.
     */

    IDT_LIST        : ORIGIN = 0xFFFF1000, LENGTH = 2K
#endif /* !CONFIG_X86_64 */
    }
#endif /* ARCH_X86_MEMORY_LD */
