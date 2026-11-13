/*
 * SPDX-FileCopyrightText: Copyright (c) 2013-2014 Wind River Systems, Inc.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker script for the primary core (CPU0) of Renesas RZ/T and RZ/N
 *        Cortex-R52 SoCs booting from flash
 *
 *   Flash (load)                      Run
 *   FLASH_LOADER  .loader.param       read in place by the boot ROM
 *                 .loader.program ->  BTCM  loader_program, kernel text
 *                                     BTCM  interrupt, main and idle stacks
 *                 .loader.copy_table  read in place by loader_program
 *   FLASH         ATCM image      ->  ATCM  vectors, critical text
 *                 relocated text  ->  ATCM, BTCM  (CONFIG_CODE_DATA_RELOCATION)
 *                                     ATCM  CPU mode stacks
 *                 RAM read-only   ->  RAM   text, rodata
 *                 RAM data        ->  RAM   nocache, data, then bss and noinit
 *
 * The boot ROM copies .loader.program to the BTCM as set by the SoC
 * loader_param.c and enters loader_program, which copies each block of
 * .loader.copy_table and jumps to __start.
 *
 * The ATCM and BTCM keep critical routines and critical data structures
 * such as vectors, CPU mode stacks, interrupt stacks.
 *
 * The NOLOAD nocache and nocache_pad sections (CONFIG_NOCACHE_MEMORY) take
 * no flash, so .got, nocache_load, app_smem (CONFIG_USERSPACE) and the data
 * are each copied as a block of their own. arch_bss_zero() clears nocache.
 *
 * Everything a DMA may access stays in the system SRAM (RAM), such that host IP (GMAC, USB, etc.)
 * can access them directly without via AXIS regions.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#if defined(CONFIG_XIP)
#error "This linker script runs the image from the TCMs and requires CONFIG_XIP=n"
#endif

/* Memory */
#define LOADER_PARAM_NODE   DT_NODELABEL(loader_param)
#define LOADER_PROGRAM_NODE DT_NODELABEL(loader_program)

#define LOADER_FLASH_ADDR DT_REG_ADDR(LOADER_PARAM_NODE)
#define LOADER_FLASH_SIZE (DT_REG_SIZE(LOADER_PARAM_NODE) + DT_REG_SIZE(LOADER_PROGRAM_NODE))

#define ROM_ADDR DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition))
#define ROM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_code_partition))

#define RAM_ADDR DT_CHOSEN_SRAM_ADDR
#define RAM_SIZE DT_CHOSEN_SRAM_SIZE

/* LDR_SIZE_NML of loader_param.c: bytes the boot ROM copies to the BTCM */
#define LOADER_BOOT_COPY_SIZE 0xC000

/* Aliases */
#define ROMABLE_REGION FLASH
#define RAMABLE_REGION RAM

/*
 * Sections run from their vregion and load from their lregion, at the same
 * VMA to LMA offset from start to end, so that the loader program copies each
 * run of sections as one block.
 */
#undef GROUP_ROM_LINK_IN
#define GROUP_ROM_LINK_IN(vregion, lregion) > vregion AT > lregion

#undef GROUP_DATA_LINK_IN
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion AT > lregion

#undef SECTION_DATA_PROLOGUE
#define SECTION_DATA_PROLOGUE(name, options, align) name options : ALIGN_WITH_INPUT

#undef SECTION_PROLOGUE
#define SECTION_PROLOGUE(name, options, align) name options : ALIGN_WITH_INPUT align

/* MPU regions: 64 B granularity, no power of two size */
_region_min_align = CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE;

/* For the common fragments, such as nocache.ld, and linker_relocate.ld */
#define MPU_ALIGN(region_size) . = ALIGN(_region_min_align)

MEMORY
{
    FLASH_LOADER (rx) : ORIGIN = LOADER_FLASH_ADDR, LENGTH = LOADER_FLASH_SIZE
    FLASH        (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
    RAM          (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
    /* zephyr,memory-region nodes, such as ATCM and BTCM */
    LINKER_DT_REGIONS()
    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST     (wx) : ORIGIN = 0xFFFF8000, LENGTH = 32K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{
#include <zephyr/linker/rel-sections.ld>
#include <zephyr/linker/llext-sections.ld>

    /* Read in place by the boot ROM */
    SECTION_PROLOGUE(.loader.param,,)
    {
        __loader_param_start = .;
        KEEP(*(.loader_param))
        __loader_param_end = .;
        . = DT_REG_SIZE(LOADER_PARAM_NODE);
    } GROUP_LINK_IN(FLASH_LOADER)

    /* Copied by the boot ROM and entered at loader_program, which comes first */
    SECTION_PROLOGUE(.loader.program,,)
    {
        __loader_program_start = .;
        KEEP(*(.loader_text.*))
        __loader_program_end = .;

        . = ALIGN(_region_min_align);
        __btcm_text_start = .;
        /*
         * Text of the kernel, loaded with the loader program by the boot
         * ROM; its read-only data is in the RAM.
         */
        *libkernel.a:(.text .text.*)
        . = ALIGN(_region_min_align);
        __btcm_text_end = .;
    } GROUP_ROM_LINK_IN(BTCM, FLASH_LOADER)

    /* Read in place by loader_program */
    SECTION_PROLOGUE(.loader.copy_table,,)
    {
        . = ALIGN(4);
        __loader_copy_table_start = .;

        /* ATCM load:
         * - Flash address (LMA) of the ATCM image
         * - Destination address (VMA) in the ATCM
         * - Size of the ATCM image
         */
        LONG(LOADADDR(rom_start))
        LONG(ADDR(rom_start))
        LONG(__atcm_text_end - ADDR(rom_start))

        /* RAM read-only load:
         * - Flash address (LMA) of the text and read-only data
         * - Destination address (VMA) in the system RAM
         * - Size of the text and read-only data
         */
        LONG(LOADADDR(_TEXT_SECTION_NAME))
        LONG(ADDR(_TEXT_SECTION_NAME))
        LONG(__rodata_region_end - ADDR(_TEXT_SECTION_NAME))

        /* RAM data load:
         * - Flash address (LMA) of the data
         * - Destination address (VMA) in the system RAM
         * - Size of the data
         */
        LONG(__data_region_load_start)
        LONG(__data_region_start)
        LONG(__data_region_end - __data_region_start)

        /* GOT load:
         * - Flash address (LMA) of .got
         * - Destination address (VMA) in the system RAM
         * - Size of .got
         */
        LONG(__got_load_start)
        LONG(__got_start)
        LONG(__got_end - __got_start)

#if defined(CONFIG_NOCACHE_MEMORY)
        /* Non-cacheable RAM load:
         * - Flash address (LMA) of nocache_load
         * - Destination address (VMA) in the system RAM
         * - Size of nocache_load
         */
        LONG(_nocache_load_rom_start)
        LONG(_nocache_load_ram_start)
        LONG(_nocache_load_ram_size)
#endif

#if defined(CONFIG_USERSPACE)
        /* Application shared memory load:
         * - Flash address (LMA) of app_smem
         * - Destination address (VMA) in the system RAM
         * - Size of app_smem
         */
        LONG(_app_smem_rom_start)
        LONG(_app_smem_start)
        LONG(_app_smem_size)
#endif

#if defined(CONFIG_CODE_DATA_RELOCATION)
        /* Relocated ATCM text load, zero size if none:
         * - Flash address (LMA) of the relocated text
         * - Destination address (VMA) in the ATCM
         * - Size of the relocated text
         */
        LONG(__atcm_reloc_rom_start)
        LONG(__atcm_reloc_start)
        LONG(__atcm_reloc_size)

        /* Relocated BTCM text load, zero size if none:
         * - Flash address (LMA) of the relocated text
         * - Destination address (VMA) in the BTCM
         * - Size of the relocated text
         */
        LONG(__btcm_reloc_rom_start)
        LONG(__btcm_reloc_start)
        LONG(__btcm_reloc_size)
#endif /* CONFIG_CODE_DATA_RELOCATION */

        __loader_copy_table_end = .;
    } GROUP_LINK_IN(FLASH_LOADER)

    /*
     * From kernel/init.c, the interrupt stack with the main and idle thread
     * stacks, after .loader.program. After the copy table in the script, so that
     * this NOLOAD section takes no room in FLASH_LOADER.
     *
     * Note: DMA-capable peripherals cannot reach the BTCM at this address: buffers
     * handed to a DMA must not be on these stacks.
     */
    SECTION_DATA_PROLOGUE(.btcm.noinit,(NOLOAD),)
    {
        __btcm_noinit_start = .;
        *libkernel.a:init.c.obj(.noinit.*)
        __btcm_noinit_end = .;
    } GROUP_LINK_IN(BTCM)

    /* Vector table, alone in a read-only MPU region */
    SECTION_PROLOGUE(rom_start,,)
    {
        __rom_region_start = .;
        z_mapped_start = .;

/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_START ...). This typically contains the vector
 * table and debug information.
 */
#include <snippets-rom-start.ld>

    } GROUP_ROM_LINK_IN(ATCM, ROMABLE_REGION)

    /*
     * Text of the architecture libraries (reset, exception and interrupt
     * entry and exit, context switch, MPU and cache handling) and critical
     * text. Before the RAM text in the script, so that its *(.text)
     * does not take these input sections.
     *
     * The reset code must run from the ATCM: a CPU0 software reset restarts
     * the core from the reset vector without the boot ROM, which leaves the
     * BTCM disabled until soc_reset_hook() enables it.
     */
    SECTION_PROLOGUE(.atcm.text,,)
    {
        . = ALIGN(_region_min_align);
        __atcm_text_start = .;
        *libarch*.a:(.text .text.*)
        *libzephyr.a:(.text .text.*)
        . = ALIGN(_region_min_align);
        __atcm_text_end = .;
    } GROUP_ROM_LINK_IN(ATCM, ROMABLE_REGION)

#ifdef CONFIG_CODE_DATA_RELOCATION
/*
 * Without XIP, the generated script links the relocated sections at the same
 * load and run address. Load them from the flash, then loader_program
 * copies them with the copy table.
 */
#undef GROUP_DATA_LINK_IN
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion AT > ROMABLE_REGION

#include <linker_relocate.ld>

#undef GROUP_DATA_LINK_IN
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion AT > lregion

    /*
     * The copy table only loads text relocated to the ATCM and the BTCM.
     * Relocated bss is zeroed by bss_zeroing_relocation(), relocated data to
     * the system RAM is linked as regular data.
     */
    __atcm_reloc_rom_start = DEFINED(__atcm_text_rom_start) ? __atcm_text_rom_start : 0;
    __atcm_reloc_start = DEFINED(__atcm_text_reloc_start) ? __atcm_text_reloc_start : 0;
    __atcm_reloc_size = DEFINED(__atcm_text_reloc_size) ? __atcm_text_reloc_size : 0;

    __btcm_reloc_rom_start = DEFINED(__btcm_text_rom_start) ? __btcm_text_rom_start : 0;
    __btcm_reloc_start = DEFINED(__btcm_text_reloc_start) ? __btcm_text_reloc_start : 0;
    __btcm_reloc_size = DEFINED(__btcm_text_reloc_size) ? __btcm_text_reloc_size : 0;

    ASSERT(!DEFINED(__atcm_rodata_reloc_start) && !DEFINED(__atcm_data_reloc_start) &&
           !DEFINED(__btcm_rodata_reloc_start) && !DEFINED(__btcm_data_reloc_start) &&
           !DEFINED(__ram_text_reloc_start) && !DEFINED(__ram_rodata_reloc_start),
           "Only text can be relocated to the ATCM and BTCM, nothing else is loaded")
#endif /* CONFIG_CODE_DATA_RELOCATION */

    /*
     * Stacks of the CPU modes (FIQ, abort, undefined, SVC, SYS), after the
     * ATCM image and not loaded. Before the RAM noinit in the script, so that
     * its *(.noinit.*) does not take these input sections.
     */
    SECTION_DATA_PROLOGUE(.atcm.noinit,(NOLOAD),)
    {
        __atcm_noinit_start = .;
        *libarch__arm__core__cortex_a_r.a:stacks.c.obj(.noinit.*)
        __atcm_noinit_end = .;
    } GROUP_LINK_IN(ATCM)

    /* At the start of the RAM, in its own read-only MPU region */
    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
    {
        . = ALIGN(_region_min_align);
        __text_region_start = .;

#include <zephyr/linker/kobject-text.ld>

        *(.text)
        *(".text.*")

#include <snippets-text-sections.ld>

        *(.gnu.linkonce.t.*)

        /*
         * These are here according to 'arm-zephyr-elf-ld --verbose',
         * after .gnu.linkonce.t.*
         */
        *(.plt) *(.iplt)
        *(.glue_7t) *(.glue_7) *(.vfp11_veneer) *(.v4_bx)

        __text_region_end = .;
        . = ALIGN(_region_min_align);
    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

    SECTION_PROLOGUE(rodata_start,,)
    {
        . = ALIGN(_region_min_align);
        __rodata_region_start = .;
    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_CPP) || defined(CONFIG_ARCH_STACKWALK)
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
#if defined(__GCC_LINKER_CMD__)
        *(.ARM.exidx* gnu.linkonce.armexidx.*)
#endif
        __exidx_end = .;
    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

/* The common fragments link their read-only sections in RAMABLE_REGION */
#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
#include <zephyr/linker/thread-local-storage.ld>
#include <zephyr/linker/cplusplus-rom.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
    {
        *(.rodata)
        *(".rodata.*")
        *(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#include <zephyr/linker/kobject-rom.ld>

        . = ALIGN(_region_min_align);
    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

    __rodata_region_end = .;
    __rom_region_end = .;

    _image_ram_start = .;

    /*
     * These are here according to 'arm-zephyr-elf-ld --verbose',
     * before data section.
     */
    SECTION_PROLOGUE(.got,,)
    {
        . = ALIGN(_region_min_align);
        __got_start = .;
        *(.got.plt)
        *(.igot.plt)
        *(.got)
        *(.igot)
        __got_end = .;
    } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
    __got_load_start = LOADADDR(.got) + (__got_start - ADDR(.got));

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#if defined(CONFIG_USERSPACE)
#define APP_SHARED_ALIGN . = ALIGN(_region_min_align);
#define SMEM_PARTITION_ALIGN MPU_ALIGN

#include <app_smem.ld>

    _app_smem_size = _app_smem_end - _app_smem_start;
    _app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
#endif  /* CONFIG_USERSPACE */

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
    {
        /*
         * After the nocache sections: the MPU driver splits the SRAM region
         * around the non-cacheable RAM and needs a single region holding
         * [__kernel_ram_start, __kernel_ram_end) for the dynamic regions.
         */
        __kernel_ram_start = .;
        __data_region_start = .;
        __data_start = .;
        *(.data)
        *(".data.*")
        *(".kernel.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

    } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/kobject-data.ld>
#include <zephyr/linker/cplusplus-ram.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

    __data_region_end = .;
    __data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#include <zephyr/linker/debug-sections.ld>

    SECTION_PROLOGUE(.ARM.attributes, 0,)
    {
        KEEP(*(.ARM.attributes))
        KEEP(*(.gnu.attributes))
    }

    /* Sections generated from 'zephyr,memory-region' nodes */
    LINKER_DT_SECTIONS()

    /* Must be last in the flash image */
    SECTION_PROLOGUE(.last_section,,)
    {
      /* .last_section contains a fixed word to ensure location counter and actual
       * rom region data usage match when CONFIG_LINKER_LAST_SECTION_ID=y. */
      KEEP(*(.last_section))
    } GROUP_LINK_IN(ROMABLE_REGION)

    /* To provide the image size as a const expression,
     * calculate this value here. */
    _flash_used = LOADADDR(.last_section) + SIZEOF(.last_section) - ROM_ADDR;

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
    {
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
        __bss_start = .;

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-bss.ld>

        *(.bss)
        *(".bss.*")
        *(COMMON)
        *(".kernel_bss.*")

        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
         */
        __bss_end = ALIGN(4);
    } GROUP_LINK_IN(RAMABLE_REGION)

#include <zephyr/linker/common-noinit.ld>

    /* Define linker symbols */

    __kernel_ram_end = RAM_ADDR + RAM_SIZE;
    __kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

#define LAST_RAM_ALIGN . = ALIGN(_region_min_align);

#include <zephyr/linker/ram-end.ld>

    /* Loader parameters: LDR_ADDR_NML, DEST_ADDR_NML and LDR_SIZE_NML */
    ASSERT(SIZEOF(.loader.param) == DT_REG_SIZE(LOADER_PARAM_NODE),
           "Loader parameters do not fill their partition")
    ASSERT(LOADADDR(.loader.program) == DT_REG_ADDR(LOADER_PROGRAM_NODE),
           "Loader program is not at the start of its partition")
    ASSERT(ADDR(.loader.program) == ORIGIN(BTCM),
           "Loader program does not run from the start of the BTCM")
    ASSERT(loader_program == ADDR(.loader.program),
           "loader_program is not where the boot ROM enters .loader.program")
    ASSERT(SIZEOF(.loader.program) <= LOADER_BOOT_COPY_SIZE,
           ".loader.program is larger than what the boot ROM copies (LDR_SIZE_NML)")
    ASSERT(LOADER_BOOT_COPY_SIZE <= DT_REG_SIZE(LOADER_PROGRAM_NODE),
           "The boot ROM copies more than the loader program partition (LDR_SIZE_NML)")

    /* The rest of the BTCM gets its own MPU region, which must not be empty */
    ASSERT(__btcm_text_end < ORIGIN(BTCM) + LENGTH(BTCM), "No BTCM left after .loader.program")
}
