/*
 * Copyright (c) 2020, Synopsys, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Metware toolchain linker defs
 *
 * This header file defines the necessary macros used by the linker script for
 * use with the metware linker.
 */

#ifndef ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_MWDT_H_
#define ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_MWDT_H_

/*
 * mwdt linker doesn't have the following directives
 */
#define ASSERT(x, y)
#define SUBALIGN(x) ALIGN(x)

#define LOG2CEIL(x) ((((x) <= 2048) ? 11 : (((x) <= 4096)?12:(((x) <= 8192) ? \
	13 : (((x) <= 16384) ? 14 : (((x) <= 32768) ? 15:(((x) <= 65536) ? \
	16 : (((x) <= 131072) ? 17 : (((x) <= 262144) ? 18:(((x) <= 524288) ? \
	19 : (((x) <= 1048576) ? 20 : (((x) <= 2097152) ? \
	21 : (((x) <= 4194304) ? 22 : (((x) <= 8388608) ? \
	23 : (((x) <= 16777216) ? 24 : (((x) <= 33554432) ? \
	25 : (((x) <= 67108864) ? 26 : (((x) <= 134217728) ? \
	27 : (((x) <= 268435456) ? 28 : (((x) <= 536870912) ? \
	29 : (((x) <= 1073741824) ? 30 : (((x) <= 2147483648) ? \
	31 : 32))))))))))))))))))))))
/*
 * The GROUP_START() and GROUP_END() macros are used to define a group
 * of sections located in one memory area, such as RAM, ROM, etc.
 * The <where> parameter is the name of the memory area.
 */
#define GROUP_START(where)
#define GROUP_END(where)

/*
 * The GROUP_LINK_IN() macro is located at the end of the section
 * description and tells the linker that this section is located in
 * the memory area specified by <where> argument.
 */
#define GROUP_LINK_IN(where) > where

/*
 * As GROUP_LINK_IN(), but takes a second argument indicating the
 * memory region (e.g. "ROM") for the load address.  Used for
 * initialized data sections that on XIP platforms must be copied at
 * startup.
 *
 * And, because output directives in GNU ld are "sticky", this must
 * also be used on the first section *after* such an initialized data
 * section, specifying the same memory region (e.g. "RAM") for both
 * vregion and lregion.
 */
#ifdef CONFIG_XIP
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion AT > lregion
#else
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion
#endif

/*
 * The GROUP_FOLLOWS_AT() macro is located at the end of the section
 * and indicates that the section does not specify an address at which
 * it is to be loaded, but that it follows a section which did specify
 * such an address
 */
#define GROUP_FOLLOWS_AT(where) AT > where

/*
 * The SECTION_PROLOGUE() macro is used to define the beginning of a section.
 * The <name> parameter is the name of the section, and the <option> parameter
 * is to include any special options such as (NOLOAD). Page alignment has its
 * own parameter since it needs abstraction across the different toolchains.
 * If not required, the <options> and <align> parameters should be left blank.
 */

#define SECTION_PROLOGUE(name, options, align) name options align :

/*
 * As for SECTION_PROLOGUE(), except that this one must (!) be used
 * for data sections which on XIP platforms will have differing
 * virtual and load addresses (i.e. they'll be copied into RAM at
 * program startup).  Such a section must (!) also use
 * GROUP_LINK_IN_LMA to specify the correct output load address.
 */
#ifdef CONFIG_XIP
#define SECTION_DATA_PROLOGUE(name, options, align) \
	name options ALIGN_WITH_INPUT align :
#else
#define SECTION_DATA_PROLOGUE(name, options, align) name options align :
#endif

#define SORT_BY_NAME(x) SORT(x)

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_MWDT_H_ */
