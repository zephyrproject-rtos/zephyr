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

#include <linker/linker-defs.h>

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
static inline size_t z_tls_data_size(void)
{
	return (size_t)__tls_size;
}

/**
 * @brief Copy the TLS data/bss areas into destination
 *
 * This copies the TLS data into destination and clear the area
 * of TLS bss size after the data section.
 *
 * @param dest Pointer to destination
 */
static inline void z_tls_copy(char *dest)
{
	size_t tdata_size = (size_t)__tdata_size;
	size_t tbss_size = (size_t)__tbss_size;

	/* Copy initialized data (tdata) */
	memcpy(dest, __tdata_start, tdata_size);

	/* Clear BSS data (tbss) */
	dest += tdata_size;
	memset(dest, 0, tbss_size);
}

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_TLS_H_ */
