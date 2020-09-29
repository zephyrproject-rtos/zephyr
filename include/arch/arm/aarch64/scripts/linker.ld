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

#include <autoconf.h>
#include <linker/sections.h>
#include <devicetree.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

/* physical address of RAM */
#ifdef CONFIG_XIP
  #define ROMABLE_REGION FLASH
  #define RAMABLE_REGION SRAM
#else
  #define ROMABLE_REGION SRAM
  #define RAMABLE_REGION SRAM
#endif

#if defined(CONFIG_XIP)
  #define _DATA_IN_ROM __data_rom_start
#else
  #define _DATA_IN_ROM
#endif

#if !defined(CONFIG_XIP) && (CONFIG_FLASH_SIZE == 0)
  #define ROM_ADDR RAM_ADDR
#else
  #define ROM_ADDR (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET)
#endif

/*
 * MMU currently supports 4 kB translation granule size,
 * so all regions are required to be 4 kB aligned
 */
#define PAGE_SIZE	0x1000

#if CONFIG_FLASH_LOAD_SIZE > 0
  #define ROM_SIZE CONFIG_FLASH_LOAD_SIZE
#else
  #define ROM_SIZE (CONFIG_FLASH_SIZE * 1K - CONFIG_FLASH_LOAD_OFFSET)
#endif

#if defined(CONFIG_XIP)
  #if defined(CONFIG_IS_BOOTLOADER)
    #define RAM_SIZE (CONFIG_BOOTLOADER_SRAM_SIZE * 1K)
    #define RAM_ADDR (CONFIG_SRAM_BASE_ADDRESS + \
                     (CONFIG_SRAM_SIZE * 1K - RAM_SIZE))
  #else
    #define RAM_SIZE (CONFIG_SRAM_SIZE * 1K)
    #define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
  #endif
#else
  #define RAM_SIZE (CONFIG_SRAM_SIZE * 1K - CONFIG_BOOTLOADER_SRAM_SIZE * 1K)
  #define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#endif

#if defined(CONFIG_ARM_MMU)
  _region_min_align = PAGE_SIZE;
#else
  /* If building without MMU support, use default 4-byte alignment. */
  _region_min_align = 4;
#endif

#define MMU_ALIGN    . = ALIGN(_region_min_align)

MEMORY
{
    FLASH     (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
    SRAM      (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx) : ORIGIN = (RAM_ADDR + RAM_SIZE), LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{

#include <linker/rel-sections.ld>

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

    _image_rom_start = ROM_ADDR;

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
    {
        _image_text_start = .;

        _vector_start = .;
        KEEP(*(.exc_vector_table))
        KEEP(*(".exc_vector_table.*"))

        KEEP(*(.vectors))

        _vector_end = .;

        *(.text)
        *(".text.*")
        *(.gnu.linkonce.t.*)

        /*
         * These are here according to 'arm-zephyr-elf-ld --verbose',
         * after .gnu.linkonce.t.*
         */
        *(.glue_7t) *(.glue_7) *(.vfp11_veneer) *(.v4_bx)

#include <linker/kobject-text.ld>

        MMU_ALIGN;
    } GROUP_LINK_IN(ROMABLE_REGION)

    _image_text_end = .;
    _image_text_size = _image_text_end - _image_text_start;

#if defined (CONFIG_CPLUSPLUS)
    SECTION_PROLOGUE(.ARM.extab,,)
    {
        /*
         * .ARM.extab section containing exception unwinding information.
         */
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } GROUP_LINK_IN(ROMABLE_REGION)
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
    } GROUP_LINK_IN(ROMABLE_REGION)

        _image_rodata_start = .;

#include <linker/common-rom.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
    {
        KEEP(*(_IRQ_VECTOR_TABLE_SECTION_NAME))

        KEEP(*(.openocd_dbg))
        KEEP(*(".openocd_dbg.*"))

        *(.rodata)
        *(".rodata.*")
        *(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#include <linker/kobject-rom.ld>

    } GROUP_LINK_IN(ROMABLE_REGION)

#include <linker/cplusplus-rom.ld>
    MMU_ALIGN;

    _image_rodata_end = .;
    _image_rodata_size = _image_rodata_end - _image_rodata_start;
    _image_rom_end = .;

    /*
     * These are here according to 'arm-zephyr-elf-ld --verbose',
     * before data section.
     */
    /DISCARD/ :
    {
        *(.got.plt)
        *(.igot.plt)
        *(.got)
        *(.igot)
    }

    GROUP_END(ROMABLE_REGION)

    GROUP_START(RAMABLE_REGION)

    . = RAM_ADDR;
    /* Align the start of image SRAM with the
     * minimum granularity required by MMU.
     */
    . = ALIGN(_region_min_align);
    _image_ram_start = .;

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#if defined(CONFIG_USERSPACE)
#define APP_SHARED_ALIGN . = ALIGN(_region_min_align);
#define SMEM_PARTITION_ALIGN MMU_ALIGN

#include <app_smem.ld>

    _app_smem_size = _app_smem_end - _app_smem_start;
    _app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
#endif  /* CONFIG_USERSPACE */

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
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

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_bss_relocate.ld>
#endif

        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
                 */
        __bss_end = ALIGN(4);
    } GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <linker/common-noinit.ld>

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
    {
        __data_ram_start = .;
        *(.data)
        *(".data.*")
        *(".kernel.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_data_relocate.ld>
#endif

    } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

    __data_rom_start = LOADADDR(_DATA_SECTION_NAME);

#include <linker/common-ram.ld>
#include <linker/kobject.ld>
#include <linker/cplusplus-ram.ld>

    __data_ram_end = .;


    /* Define linker symbols */

    _image_ram_end = .;
    _end = .; /* end of image */

    __kernel_ram_end = RAM_ADDR + RAM_SIZE;
    __kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

    GROUP_END(RAMABLE_REGION)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#include <linker/debug-sections.ld>

    SECTION_PROLOGUE(.ARM.attributes, 0,)
    {
        KEEP(*(.ARM.attributes))
        KEEP(*(.gnu.attributes))
    }

    /DISCARD/ : { *(.note.GNU-stack) }


    /* Must be last in romable region */
    SECTION_PROLOGUE(.last_section,(NOLOAD),)
    {
    } GROUP_LINK_IN(ROMABLE_REGION)

    /* To provide the image size as a const expression,
     * calculate this value here. */
    _flash_used = LOADADDR(.last_section) - _image_rom_start;

}
