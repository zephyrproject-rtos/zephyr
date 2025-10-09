/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief XCDSC LD linker defs
 *
 * This header file defines the necessary macros used by the linker script for
 * use with the XCDSC Linker.
 */

#ifndef ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_XCDSC_H_
#define ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_XCDSC_H_
#include <zephyr/kernel/mm.h>

OUTPUT_FORMAT("elf32-pic30")
#if defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
OUTPUT_ARCH("33AK128MC106")
#elif defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
OUTPUT_ARCH("33AK512MPS512")
#endif

/*
 * The GROUP_START() and GROUP_END() macros are used to define a group
 * of sections located in one memory area, such as RAM, ROM, etc.
 * The <where> parameter is the name of the memory area.
 */
#define GROUP_START(where)
#define GROUP_END(where)

/**
 * @def GROUP_LINK_IN
 *
 * Route memory to a specified memory area
 *
 * The GROUP_LINK_IN() macro is located at the end of the section
 * description and tells the linker that this section is located in
 * the memory area specified by 'where' argument.
 *
 * This macro is intentionally undefined for CONFIG_MMU systems when
 * CONFIG_KERNEL_VM_BASE is not the same as CONFIG_SRAM_BASE_ADDRESS,
 * as both the LMA and VMA destinations must be known for all sections
 * as this corresponds to physical vs. virtual location.
 *
 * @param where Destination memory area
 */
#define GROUP_LINK_IN(where) > where

/**
 * @def GROUP_DATA_LINK_IN
 *
 * Route memory for read-write sections that are loaded.
 *
 * Used for initialized data sections that on XIP platforms must be copied at
 * startup.
 *
 * @param vregion Output VMA
 * @param lregion Output LMA (only used if CONFIG_MMU if VMA != LMA,
 *		  or CONFIG_XIP)
 */
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion
/*
 * xcdsc linker doesn't have the following directives
 */
#define SUBALIGN(x)                          ALIGN(x)

/**
 * @def SECTION_PROLOGUE
 *
 * The SECTION_PROLOGUE() macro is used to define the beginning of a section.
 *
 * When --omagic (-N) option is provided to LLD then only the first output
 * section of given region has aligned LMA (by default, without --omagic, LLD
 * aligns LMA and VMA of every section to the same value) and the difference
 * between VMA addresses (0 is this is the first section) is added.
 * The difference between LMA and VMA is constant for every section, so this
 * emulates ALIGN_WITH_INPUT option present in GNU LD (required by XIP systems).
 *
 * The --omagic flag is defined in cmake/linker/lld/target_baremetal.cmake
 *
 * @param name Name of the output section
 * @param options Section options, such as (NOLOAD), or left blank
 * @param align Alignment directives, such as SUBALIGN(). May be blank.
 */
#undef SECTION_PROLOGUE
#define SECTION_PROLOGUE(name, options, align) name options : align

/**
 * @def SECTION_DATA_PROLOGUE
 *
 * Same as for SECTION_PROLOGUE(), except that this one must be used
 * for data sections which on XIP platforms will have differing
 * virtual and load addresses (i.e. they'll be copied into RAM at
 * program startup).  Such a section must also use
 * GROUP_DATA_LINK_IN to specify the correct output load address.
 *
 * This is equivalent to SECTION_PROLOGUE() when linking using LLD.
 *
 * @param name Name of the output section
 * @param options Section options, or left blank
 * @param align Alignment directives, such as SUBALIGN(). May be blank.
 */
#undef SECTION_DATA_PROLOGUE
#define SECTION_DATA_PROLOGUE(name, options, align) SECTION_PROLOGUE(name, options, align)

/**
 * @def GROUP_ROM_LINK_IN
 *
 * Route memory for a read-only section
 *
 * The GROUP_ROM_LINK_IN() macro is located at the end of the section
 * description and tells the linker that this a read-only section
 * that is physically placed at the 'lregion` argument.
 *
 * If CONFIG_XIP is active, the 'lregion' area is flash memory.
 *
 * If CONFIG_MMU is active, the vregion argument will be used to
 * determine where this is located in the virtual memory map, otherwise
 * it is ignored.
 *
 * @param vregion Output VMA (only used if CONFIG_MMU where LMA != VMA)
 * @param lregion Output LMA
 */
#undef GROUP_ROM_LINK_IN
#define GROUP_ROM_LINK_IN(vregion, lregion) > vregion

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_XCDSC_H_ */
