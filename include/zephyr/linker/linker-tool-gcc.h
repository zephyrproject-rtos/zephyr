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

#include <zephyr/sys/mem_manage.h>

#if defined(CONFIG_ARM)
	#if defined(CONFIG_BIG_ENDIAN)
		#define OUTPUT_FORMAT_ "elf32-bigarm"
	#else
		#define OUTPUT_FORMAT_ "elf32-littlearm"
	#endif
	OUTPUT_FORMAT(OUTPUT_FORMAT_)
#elif defined(CONFIG_ARM64)
	OUTPUT_FORMAT("elf64-littleaarch64")
#elif defined(CONFIG_ARC)
	#if defined(CONFIG_ISA_ARCV3) && defined(CONFIG_64BIT)
		OUTPUT_FORMAT("elf64-littlearc64")
	#elif defined(CONFIG_ISA_ARCV3) && !defined(CONFIG_64BIT)
		OUTPUT_FORMAT("elf32-littlearc64")
	#else
		OUTPUT_FORMAT("elf32-littlearc", "elf32-bigarc", "elf32-littlearc")
	#endif
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
#elif defined(CONFIG_MIPS)
	OUTPUT_ARCH("mips")
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
#if defined(CONFIG_ARCH_POSIX)
#define GROUP_LINK_IN(where)
#elif !defined(Z_VM_KERNEL)
#define GROUP_LINK_IN(where) > where
#endif

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
#if defined(CONFIG_ARCH_POSIX)
#define GROUP_ROM_LINK_IN(vregion, lregion)
#elif defined(Z_VM_KERNEL)
#define GROUP_ROM_LINK_IN(vregion, lregion) > vregion AT > lregion
#else
#define GROUP_ROM_LINK_IN(vregion, lregion) > lregion
#endif

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
#if defined(CONFIG_ARCH_POSIX)
#define GROUP_DATA_LINK_IN(vregion, lregion)
#elif defined(CONFIG_XIP) || defined(Z_VM_KERNEL)
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion AT > lregion
#else
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion
#endif

/**
 * @def GROUP_NOLOAD_LINK_IN
 *
 * Route memory for read-write sections that are NOT loaded; typically this
 * is only used for 'BSS' and 'noinit'.
 *
 * @param vregion Output VMA
 * @param lregion Output LMA (only used if CONFIG_MMU if VMA != LMA,
 *		  corresponds to physical location)
 */
#if defined(CONFIG_ARCH_POSIX)
#define GROUP_NOLOAD_LINK_IN(vregion, lregion)
#elif defined(Z_VM_KERNEL)
#define GROUP_NOLOAD_LINK_IN(vregion, lregion) > vregion AT > lregion
#elif defined(CONFIG_XIP)
#define GROUP_NOLOAD_LINK_IN(vregion, lregion) > vregion AT > vregion
#else
#define GROUP_NOLOAD_LINK_IN(vregion, lregion) > vregion
#endif

/**
 * @def SECTION_PROLOGUE
 *
 * The SECTION_PROLOGUE() macro is used to define the beginning of a section.
 *
 * On MMU systems where VMA != LMA there is an implicit ALIGN_WITH_INPUT
 * specified.
 *
 * @param name Name of the output section
 * @param options Section options, such as (NOLOAD), or left blank
 * @param align Alignment directives, such as SUBALIGN(). ALIGN() itself is
 *              not allowed. May be blank.
 */
#ifdef Z_VM_KERNEL
/* If we have a virtual memory map we need ALIGN_WITH_INPUT in all sections */
#define SECTION_PROLOGUE(name, options, align) \
	name options : ALIGN_WITH_INPUT align
#else
#define SECTION_PROLOGUE(name, options, align) \
	name options : align
#endif

/**
 * @def SECTION_DATA_PROLOGUE
 *
 * Same as for SECTION_PROLOGUE(), except that this one must be used
 * for data sections which on XIP platforms will have differing
 * virtual and load addresses (i.e. they'll be copied into RAM at
 * program startup).  Such a section must also use
 * GROUP_DATA_LINK_IN to specify the correct output load address.
 *
 * This is equivalent to SECTION_PROLOGUE() on non-XIP systems.
 * On XIP systems there is an implicit ALIGN_WITH_INPUT specified.
 *
 * @param name Name of the output section
 * @param options Section options, or left blank
 * @param align Alignment directives, such as SUBALIGN(). ALIGN() itself is
 *              not allowed. May be blank.
 */
#if defined(CONFIG_XIP)
#define SECTION_DATA_PROLOGUE(name, options, align) \
	name options : ALIGN_WITH_INPUT align
#else
#define SECTION_DATA_PROLOGUE(name, options, align) \
	SECTION_PROLOGUE(name, options, align)
#endif

#define COMMON_SYMBOLS *(COMMON)

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_TOOL_GCC_H_ */
