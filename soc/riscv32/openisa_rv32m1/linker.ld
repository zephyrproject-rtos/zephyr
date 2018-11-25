/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2016-2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * This file is based on:
 *
 * - include/arch/arm/cortex_m/scripts/linker.ld
 * - include/arch/riscv32/common/linker.ld
 * - include/arch/riscv32/pulpino/linker.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>

#include <linker/sections.h>
#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

/*
 * Extra efforts would need to be taken to ensure the IRQ handlers are within
 * jumping distance of the vector table in non-XIP builds, so avoid them.
 */
#define ROMABLE_REGION              ROM
#define RAMABLE_REGION              RAM

#define ROM_BASE CONFIG_RISCV32_RV32M1_ROM_BASE_ADDR
#define ROM_SIZE CONFIG_RISCV32_RV32M1_ROM_SIZE
#define RAM_BASE CONFIG_RISCV32_RV32M1_RAM_BASE_ADDR
#define RAM_SIZE CONFIG_RISCV32_RV32M1_RAM_SIZE
#define VECTOR_BASE CONFIG_RISCV32_RV32M1_VECTOR_BASE_ADDR
#define VECTOR_SIZE CONFIG_RISCV32_RV32M1_VECTOR_SIZE

MEMORY
    {
    ROM (rx)      : ORIGIN = ROM_BASE,    LENGTH = ROM_SIZE
    /*
     * Each RISC-V core on this chip (RI5CY and ZERO-RISCY) has
     * a vector table at the end of its flash bank. They are relocatable
     * at runtime, but we need to put the reset vectors in hardcoded places.
     *
     * (The Arm core vector tables are at the beginning of each
     * flash bank.)
     */
    VECTORS (rx)  : ORIGIN = VECTOR_BASE, LENGTH = VECTOR_SIZE
    RAM (rwx)     : ORIGIN = RAM_BASE,    LENGTH = RAM_SIZE
    /*
     * Special section, not included in the final binary, used
     * to generate interrupt tables. See include/linker/intlist.ld.
     */
    IDT_LIST (wx) : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
    }

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
    {

#include <linker/rel-sections.ld>

    SECTION_PROLOGUE(.plt,,)
	{
		*(.plt)
	}

    SECTION_PROLOGUE(.iplt,,)
	{
		*(.iplt)
	}

    GROUP_START(ROM)
    _image_rom_start = ROM_BASE;

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	. = CONFIG_TEXT_SECTION_OFFSET;
	. = ALIGN(4);

	/*
	 * Respect for CONFIG_TEXT_SECTION_OFFSET is mandatory
	 * for MCUboot support, so .reset.* and .exception.*
	 * must come after that offset from ROM_BASE.
	 */
	KEEP(*(.reset.*))
	KEEP(*(".exception.entry.*")) /* contains __irq_wrapper */
	*(".exception.other.*")

	KEEP(*(.openocd_debug))
	KEEP(*(".openocd_debug.*"))

	_image_text_start = .;
	*(.text .text.*)
	*(.gnu.linkonce.t.*)
	*(.eh_frame)
	} GROUP_LINK_IN(ROM)

    _image_text_end = .;

    _image_rodata_start = .;

#include <linker/common-rom.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
	. = ALIGN(4);
	*(.rodata)
	*(.rodata.*)
	*(.gnu.linkonce.r.*)
	} GROUP_LINK_IN(ROMABLE_REGION)

    _image_rodata_end = .;
    _image_rom_end = .;

    /* The vector table goes into core-dependent flash locations. */
    SECTION_PROLOGUE(vectors,,)
	{
	_vector_start = .;
	KEEP(*(.vectors.*))
	} GROUP_LINK_IN(VECTORS)
    _vector_end = .;

    GROUP_END(ROM)

    GROUP_START(RAM)

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
	. = ALIGN(4);
	_image_ram_start = .;
	__data_ram_start = .;

	*(.data)
	*(.data.*)
	*(.gnu.linkonce.s.*)

	/* https://groups.google.com/a/groups.riscv.org/d/msg/sw-dev/60IdaZj27dY/TKT3hbNlAgAJ */
	*(.sdata .sdata.* .gnu.linkonce.s.*)
	*(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <linker/common-ram.ld>

    __data_ram_end = .;
    __data_rom_start = LOADADDR(_DATA_SECTION_NAME);

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
	/*
	 * For performance, BSS section is assumed to be 4 byte aligned and
	 * a multiple of 4 bytes, so it can be cleared in words.
	 */
	 . = ALIGN(4);
	 __bss_start = .;

	 *(.bss .bss.*)
	 *(.sbss .sbss.*)
	 COMMON_SYMBOLS

	 /* Ensure 4 byte alignment for the entire section. */
	 . = ALIGN(4);
	  __bss_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

    SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
	{
	/*
	 * This section is used for non-initialized objects that
	 * will not be cleared during the boot process.
	 */
	 *(.noinit .noinit.*)
	} GROUP_LINK_IN(RAMABLE_REGION)

     _image_ram_end = .;
     _end = .; /* end of image */

     GROUP_END(RAM)

#ifdef CONFIG_CUSTOM_SECTIONS_LD
/* Located in project source directory */
#include <custom-sections.ld>
#endif

#ifdef CONFIG_GEN_ISR_TABLES
/* Bogus section, post-processed during the build to initialize interrupts. */
#include <linker/intlist.ld>
#endif

#include <linker/debug-sections.ld>

    /*
     * Pulpino toolchains emit these sections; we don't care about them,
     * but need to avoid build system warnings about orphaned sections.
     */
    SECTION_PROLOGUE(.Pulp_Chip.Info,,)
	{
		*(.Pulp_Chip.*)
	}

    }
