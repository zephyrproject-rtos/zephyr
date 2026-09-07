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
#ifdef __IAR_SYSTEMS_ICC__
	size_t size = (size_t)(uintptr_t)__tdata_size + (size_t)(uintptr_t)__tbss_size;

#if defined(CONFIG_STACK_CANARIES_TLS_PREPEND)
	size += (size_t)(uintptr_t)__stack_chk_size;
#endif
	return size;
#else
	return ROUND_UP((size_t)(uintptr_t)__tls_size, (size_t)(uintptr_t)__tls_align);
#endif
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
	size_t tdata_size = (size_t)(uintptr_t)__tdata_size;
	size_t tdata_offset = 0;

#if defined(CONFIG_STACK_CANARIES_TLS_PREPEND)
	size_t stack_chk_size = (size_t)(uintptr_t)__stack_chk_size;

	/* Use runtime offsets even when initialized sections are loaded from ROM. */
	tdata_offset = (size_t)(uintptr_t)__tdata_offset;
	memcpy(dest, __stack_chk_start, stack_chk_size);
	memset(dest + stack_chk_size, 0, tdata_offset - stack_chk_size);
#endif
	/* Copy initialized data at its ELF offset from the TLS image base. */
	dest += tdata_offset;
	memcpy(dest, __tdata_start, tdata_size);
	dest += tdata_size;

	/* Clear tbss and the padding around it. */
	memset(dest, 0, z_tls_data_size() - tdata_offset - tdata_size);
}

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_TLS_H_ */
