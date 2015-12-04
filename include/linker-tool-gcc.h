/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief GCC toolchain linker defs
 *
 * This header file defines the necessary macros used by the linker script for
 * use with the GCC linker.
 */

#ifndef __LINKER_TOOL_GCC_H
#define __LINKER_TOOL_GCC_H

#if defined(CONFIG_CPU_CORTEX_M)
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
#elif defined(CONFIG_ARC)
OUTPUT_FORMAT("elf32-littlearc", "elf32-bigarc", "elf32-littlearc")
#else
#if  defined(__IAMCU)
OUTPUT_FORMAT("elf32-iamcu")
OUTPUT_ARCH(iamcu:intel)
#else
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
#endif
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
#define GROUP_LINK_IN(where) > where

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

#define SECTION_PROLOGUE(name, options, align) name options : align

/*
 * The SECTION_AT_PROLOGUE() macro is similar to SECTION_PROLOGUE() except
 * that, in addition, the address at which the section is to be loaded is
 * specified.
 */

#define SECTION_AT_PROLOGUE(name, options, align, addr) \
	name options : align AT(addr)

#define SORT_BY_NAME(x) SORT(x)
#define OPTIONAL

#define COMMON_SYMBOLS *(COMMON)

#endif /* !__LINKER_TOOL_GCC_H */
