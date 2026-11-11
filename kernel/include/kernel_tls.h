/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel Thread Local Storage APIs.
 *
 * Kernel APIs related to thread local storage.
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_TLS_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_TLS_H_

#include <zephyr/linker/linker-defs.h>

/**
 * @brief Return the total size of TLS data/bss areas
 *
 * This returns the total size of thread local storage (TLS)
 * data and bss areas as defined in the linker script.
 * Note that this does not include any architecture specific
 * bits required for proper functionality of TLS.
 *
 * @return Total size of TLS data/bss areas
 */
static inline FUNC_NO_STACK_PROTECTOR size_t z_tls_data_size(void)
{
	size_t size = LINKER_SECTION_SIZE(__tdata_start, __tdata_end) +
		      LINKER_SECTION_SIZE(__tbss_start, __tbss_end);

#if defined(CONFIG_STACK_CANARIES_TLS_PREPEND)
	size += LINKER_SECTION_SIZE(__stack_chk_start, __stack_chk_end);
#endif
	return size;
}

/**
 * @brief Copy the TLS data/bss areas into destination
 *
 * This copies the TLS data into destination and clear the area
 * of TLS bss size after the data section.
 *
 * @param dest Pointer to destination
 */
static inline FUNC_NO_STACK_PROTECTOR void z_tls_copy(char *dest)
{
#if defined(CONFIG_STACK_CANARIES_TLS_PREPEND)
	/* Copy .stack_chk.guard and .tdata separately since the linker may insert
	 * padding between sections.
	 */
	size_t stack_chk_size = LINKER_SECTION_SIZE(__stack_chk_start, __stack_chk_end);

	memcpy(dest, __stack_chk_start, stack_chk_size);
	dest += stack_chk_size;

	size_t tdata_size = LINKER_SECTION_SIZE(__tdata_start, __tdata_end);

	memcpy(dest, __tdata_start, tdata_size);
	dest += tdata_size;
#else
	/* Copy initialized data (tdata) */
	size_t tdata_size = LINKER_SECTION_SIZE(__tdata_start, __tdata_end);

	memcpy(dest, __tdata_start, tdata_size);
	dest += tdata_size;
#endif
	/* Clear BSS data (tbss) */
	memset(dest, 0, LINKER_SECTION_SIZE(__tbss_start, __tbss_end));
}

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_TLS_H_ */
