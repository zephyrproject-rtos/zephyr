/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-A platforms.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* physical address of RAM */
#ifdef CONFIG_XIP
  #define ROMABLE_REGION FLASH
#else
  #define ROMABLE_REGION RAM
#endif
#define RAMABLE_REGION RAM

#if !defined(CONFIG_XIP) && (CONFIG_FLASH_SIZE == 0)
  #define ROM_ADDR RAM_ADDR
#else
  #define ROM_ADDR (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET)
#endif

#if defined(CONFIG_ROM_END_OFFSET)
#define ROM_END_OFFSET CONFIG_ROM_END_OFFSET
#else
#define ROM_END_OFFSET 0
#endif

#if CONFIG_FLASH_LOAD_SIZE > 0
  #define ROM_SIZE (CONFIG_FLASH_LOAD_SIZE - ROM_END_OFFSET)
#else
  #define ROM_SIZE (CONFIG_FLASH_SIZE * 1K - CONFIG_FLASH_LOAD_OFFSET - ROM_END_OFFSET)
#endif

#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K)
#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS

#if defined(CONFIG_ARM_MMU)
  _region_min_align = CONFIG_MMU_PAGE_SIZE;
#elif defined(CONFIG_ARM_MPU)
  _region_min_align = CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE;
  #define BSS_ALIGN ALIGN(_region_min_align)
#else
  /* If building without MMU support, use default 4-byte alignment. */
  _region_min_align = 4;
#endif

#ifndef BSS_ALIGN
#define BSS_ALIGN
#endif

#define MMU_ALIGN    . = ALIGN(_region_min_align)

MEMORY
{
    FLASH     (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
    RAM       (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
    LINKER_DT_REGIONS()
    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx) : ORIGIN = 0xFFFF8000, LENGTH = 32K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

    /*
     * .plt and .iplt are here according to 'arm-zephyr-elf-ld --verbose',
     * before text section.
     */
    /DISCARD/ :
    {
        *(.plt)
    }

    /DISCARD/ :
    {
        *(.iplt)
    }

    GROUP_START(ROMABLE_REGION)

    __rom_region_start = ROM_ADDR;

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
    {
        __text_region_start = .;
#ifndef CONFIG_XIP
        z_mapped_start = .;
#endif

#ifdef CONFIG_AARCH64_IMAGE_HEADER
        KEEP(*(.image_header))
        KEEP(*(".image_header.*"))
#endif

        _vector_start = .;
        KEEP(*(.exc_vector_table))
        KEEP(*(".exc_vector_table.*"))

#if LINKER_ZEPHYR_FINAL && defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
        INCLUDE isr_tables_vt.ld
#else
        KEEP(*(.vectors))
#endif
        _vector_end = .;

        *(.text)
        *(".text.*")
        *(.gnu.linkonce.t.*)

        /*
         * These are here according to 'arm-zephyr-elf-ld --verbose',
         * after .gnu.linkonce.t.*
         */
        *(.glue_7t) *(.glue_7) *(.vfp11_veneer) *(.v4_bx)

#include <zephyr/linker/kobject-text.ld>

        MMU_ALIGN;
    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

    __text_region_end = .;
    __text_region_size = __text_region_end - __text_region_start;

#if defined (CONFIG_CPP)
    SECTION_PROLOGUE(.ARM.extab,,)
    {
        /*
         * .ARM.extab section containing exception unwinding information.
         */
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

    SECTION_PROLOGUE(.ARM.exidx,,)
    {
        /*
         * This section, related to stack and exception unwinding, is placed
         * explicitly to prevent it from being shared between multiple regions.
         * It  must be defined for gcc to support 64-bit math and avoid
         * section overlap.
         */
        __exidx_start = .;
#if defined (__GCC_LINKER_CMD__)
        *(.ARM.exidx* gnu.linkonce.armexidx.*)
#endif
        __exidx_end = .;
    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

        __rodata_region_start = .;

#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
#include <zephyr/linker/thread-local-storage.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
    {
        *(.rodata)
        *(".rodata.*")
        *(.gnu.linkonce.r.*)

        /*
         * The following is a workaround to allow compiling with GCC 12 and
         * above, which may emit "GOT indirections" for the weak symbol
         * references (see the GitHub issue zephyrproject-rtos/sdk-ng#547).
         */
        *(.got)
        *(.got.plt)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#include <zephyr/linker/kobject-rom.ld>

    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>
    MMU_ALIGN;

    __rodata_region_end = .;
    __rodata_region_size = __rodata_region_end - __rodata_region_start;
    __rom_region_end = .;

    /*
     * These are here according to 'arm-zephyr-elf-ld --verbose',
     * before data section.
     */
    /DISCARD/ :
    {
        *(.igot.plt)
        *(.igot)
    }

    GROUP_END(ROMABLE_REGION)

    GROUP_START(RAMABLE_REGION)

    . = RAM_ADDR;
    /* Align the start of image RAM with the
     * minimum granularity required by MMU.
     */
    . = ALIGN(_region_min_align);
    _image_ram_start = .;
#ifdef CONFIG_XIP
    z_mapped_start = .;
#endif

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#if defined(CONFIG_USERSPACE)
#define APP_SHARED_ALIGN . = ALIGN(_region_min_align);
#define SMEM_PARTITION_ALIGN(size) MMU_ALIGN

#if defined(CONFIG_ARM_MPU)
/*
 * When _app_smem region is empty, alignment is also needed. If there
 * is no alignment, the _app_smem_start used by arm mpu can be lower
 * than __rodata_region_end, and this two regions can overlap.
 * The Armv8-R aarch64 MPU does not allow overlapped regions.
 */
#define EMPTY_APP_SHARED_ALIGN APP_SHARED_ALIGN
#endif

#include <app_smem.ld>

    _app_smem_size = _app_smem_end - _app_smem_start;
    _app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
#endif  /* CONFIG_USERSPACE */

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD), BSS_ALIGN)
    {
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
        __bss_start = .;
        __kernel_ram_start = .;

        *(.bss)
        *(".bss.*")
        *(COMMON)
        *(".kernel_bss.*")

        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
                 */
        __bss_end = ALIGN(4);
    } GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <zephyr/linker/common-noinit.ld>

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
    {
	__data_region_start = .;
	__data_start = .;
        *(.data)
        *(".data.*")
        *(".kernel.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

        __data_end = .;

    } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
    __data_size = __data_end - __data_start;
    __data_load_start = LOADADDR(_DATA_SECTION_NAME);

    __data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/kobject-data.ld>
#include <zephyr/linker/cplusplus-ram.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

    __data_region_end = .;


    /* Define linker symbols */

    __kernel_ram_end = RAM_ADDR + RAM_SIZE;
    __kernel_ram_size = __kernel_ram_end - __kernel_ram_start;


/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#define LAST_RAM_ALIGN MMU_ALIGN;

#include <zephyr/linker/ram-end.ld>

    GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

    SECTION_PROLOGUE(.ARM.attributes, 0,)
    {
        KEEP(*(.ARM.attributes))
        KEEP(*(.gnu.attributes))
    }

    /DISCARD/ : { *(.note.GNU-stack) }

/* Output section descriptions are needed for these sections to suppress
 * warnings when "--orphan-handling=warn" is set for lld.
 */
#if defined(CONFIG_LLVM_USE_LLD)
    SECTION_PROLOGUE(.symtab, 0,) { *(.symtab) }
    SECTION_PROLOGUE(.strtab, 0,) { *(.strtab) }
    SECTION_PROLOGUE(.shstrtab, 0,) { *(.shstrtab) }
#endif

    /* Sections generated from 'zephyr,memory-region' nodes */
    LINKER_DT_SECTIONS()

    /* Must be last in romable region */
    SECTION_PROLOGUE(.last_section,,)
    {
#ifdef CONFIG_LINKER_LAST_SECTION_ID
      /* Fill last section with a word to ensure location counter and actual rom
       * region data usage match. */
      LONG(CONFIG_LINKER_LAST_SECTION_ID_PATTERN)
#endif
    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

    /* To provide the image size as a const expression,
     * calculate this value here. */
    _flash_used = LOADADDR(.last_section) + SIZEOF(.last_section) - __rom_region_start;

}
