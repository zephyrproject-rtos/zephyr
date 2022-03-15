/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LINKER_UTILS_H_
#define ZEPHYR_INCLUDE_LINKER_UTILS_H_

#include <stdbool.h>

/**
 * @brief Check if address is in read only section.
 *
 * Note that this may return false if the address lies outside
 * the compiler's default read only sections (e.g. .rodata
 * section), depending on the linker script used. This also
 * applies to constants with explicit section attributes.
 *
 * @param addr Address.
 *
 * @return True if address identified within read only section.
 */
static inline bool linker_is_in_rodata(const void *addr)
{
#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
	extern const char lnkr_pinned_rodata_start[];
	extern const char lnkr_pinned_rodata_end[];

	if (((const char *)addr >= (const char *)lnkr_pinned_rodata_start) &&
	    ((const char *)addr < (const char *)lnkr_pinned_rodata_end)) {
		return true;
	}
#endif

#if defined(CONFIG_ARM) || defined(CONFIG_ARC) || defined(CONFIG_X86) || \
	defined(CONFIG_ARM64) || defined(CONFIG_NIOS2) || \
	defined(CONFIG_RISCV) || defined(CONFIG_SPARC) || defined(CONFIG_MIPS)
	extern char __rodata_region_start[];
	extern char __rodata_region_end[];
	#define RO_START __rodata_region_start
	#define RO_END __rodata_region_end
#elif defined(CONFIG_XTENSA)
	extern const char _rodata_start[];
	extern const char _rodata_end[];
	#define RO_START _rodata_start
	#define RO_END _rodata_end
#else
	#define RO_START 0
	#define RO_END 0
#endif

	return (((const char *)addr >= (const char *)RO_START) &&
		((const char *)addr < (const char *)RO_END));

	#undef RO_START
	#undef RO_END
}

/**
 * @brief Check if address is in log string section.
 *
 * Note that this may return false if the address lies outside
 * the compiler's __log_strings_section
 *
 * @param addr Address.
 *
 * @return True if address identified within __log_strings_section section.
 */
#if defined(CONFIG_LOG2_FMT_SECTION)
static inline bool linker_in_log_string_section(const char *addr)
{
#if defined(CONFIG_ARC) || defined(CONFIG_ARM) || defined(CONFIG_X86) \
	|| defined(CONFIG_RISCV) || defined(CONFIG_ARM64) \
	|| defined(CONFIG_NIOS2)
	extern char __log_strings_start[];
	extern char __log_strings_end[];
#define STRING_START __log_strings_start
#define STRING_END __log_strings_end
#else
#define STRING_START 0
#define STRING_END 0
#endif

	return ((addr >= (const char *)STRING_START) &&
		(addr < (const char *)STRING_END));
}
#endif

#endif /* ZEPHYR_INCLUDE_LINKER_UTILS_H_ */
