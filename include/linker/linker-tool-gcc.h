/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GCC toolchain linker defs
 *
 * This header file defines the necessary macros used by the linker script for
 * use with the GCC linker.
 */

#ifndef ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_GCC_H_
#define ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_GCC_H_

#if defined(CONFIG_ARM)
	#if defined(CONFIG_ARM64)
		#define OUTPUT_FORMAT_ "elf64-littleaarch64"
	#else
		#if defined(CONFIG_BIG_ENDIAN)
			#define OUTPUT_FORMAT_ "elf32-bigarm"
		#else
			#define OUTPUT_FORMAT_ "elf32-littlearm"
		#endif
	#endif
	OUTPUT_FORMAT(OUTPUT_FORMAT_)
#elif defined(CONFIG_ARC)
	OUTPUT_FORMAT("elf32-littlearc", "elf32-bigarc", "elf32-littlearc")
#elif defined(CONFIG_X86)
	#if defined(CONFIG_X86_64)
		OUTPUT_FORMAT("elf64-x86-64")
		OUTPUT_ARCH("i386:x86-64")
	#else
		OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
		OUTPUT_ARCH("i386")
	#endif
#elif defined(CONFIG_NIOS2)
	OUTPUT_FORMAT("elf32-littlenios2", "elf32-bignios2", "elf32-littlenios2")
#elif defined(CONFIG_RISCV)
	OUTPUT_ARCH("riscv")
#ifdef CONFIG_64BIT
	OUTPUT_FORMAT("elf64-littleriscv")
#else
	OUTPUT_FORMAT("elf32-littleriscv")
#endif
#elif defined(CONFIG_XTENSA)
	/* Not needed */
#elif defined(CONFIG_ARCH_POSIX)
	/* Not needed */
#elif defined(CONFIG_SPARC)
	OUTPUT_FORMAT("elf32-sparc")
#else
	#error Arch not supported.
#endif

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
#if defined(CONFIG_ARCH_POSIX)
#define GROUP_LINK_IN(where)
#else
#define GROUP_LINK_IN(where) > where
#endif

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
#if defined(CONFIG_ARCH_POSIX)
#define GROUP_DATA_LINK_IN(vregion, lregion)
#else
#ifdef CONFIG_XIP
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion AT> lregion
#else
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion
#endif
#endif /*CONFIG_ARCH_POSIX*/

/*
 * The GROUP_FOLLOWS_AT() macro is located at the end of the section
 * and indicates that the section does not specify an address at which
 * it is to be loaded, but that it follows a section which did specify
 * such an address
 */
#ifdef CONFIG_ARCH_POSIX
#define GROUP_FOLLOWS_AT(where)
#else
#define GROUP_FOLLOWS_AT(where) AT > where
#endif

/*
 * The SECTION_PROLOGUE() macro is used to define the beginning of a section.
 * The <name> parameter is the name of the section, and the <option> parameter
 * is to include any special options such as (NOLOAD). Page alignment has its
 * own parameter since it needs abstraction across the different toolchains.
 * If not required, the <options> and <align> parameters should be left blank.
 */

#define SECTION_PROLOGUE(name, options, align) name options : align

/*
 * As for SECTION_PROLOGUE(), except that this one must (!) be used
 * for data sections which on XIP platforms will have differing
 * virtual and load addresses (i.e. they'll be copied into RAM at
 * program startup).  Such a section must (!) also use
 * GROUP_LINK_IN_LMA to specify the correct output load address.
 */
#ifdef CONFIG_XIP
#define SECTION_DATA_PROLOGUE(name, options, align) \
	name options : ALIGN_WITH_INPUT align
#else
#define SECTION_DATA_PROLOGUE(name, options, align) name options : align
#endif

#define COMMON_SYMBOLS *(COMMON)

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_GCC_H_ */
