/*
 * Copyright (c) 2023, Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LLVM LLD linker defs
 *
 * This header file defines the necessary macros used by the linker script for
 * use with the LLD linker.
 */

#ifndef ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_LLD_H_
#define ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_LLD_H_

#include <zephyr/linker/linker-tool-gcc.h>

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
#define SECTION_PROLOGUE(name, options, align) \
	name options : align

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
#define SECTION_DATA_PROLOGUE(name, options, align) \
	SECTION_PROLOGUE(name, options, align)

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_LLD_H_ */
