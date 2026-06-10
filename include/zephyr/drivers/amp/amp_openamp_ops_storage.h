/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AMP OpenAMP storage_ops implementations
 * @ingroup amp_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AMP_OPENAMP_OPS_MEMORY_H_
#define ZEPHYR_INCLUDE_DRIVERS_AMP_OPENAMP_OPS_MEMORY_H_

#include <zephyr/drivers/amp/amp_openamp_ops_rproc.h>
#include <openamp/remoteproc_loader.h>

#if defined(CONFIG_AMP_OPENAMP_OPS_PROVIDE_STORAGE_OPS_MEMCPY) || defined(__DOXYGEN__)

/**
 * @brief Location and size of an ELF in readable memory
 */
struct amp_memcpy_options {
	/** Start of the ELF */
	const uintptr_t start_address;
	/** Size of the ELF */
	const size_t image_size;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Internal implementaiton details that shouldn't be used by the user
 */
int zephyr_openamp_load_memcpy_open(void *store, const char *path, const void **img_data);
void zephyr_openamp_load_memcpy_close(void *store);
int zephyr_openamp_load_memcpy_load(void *store, size_t offset, size_t size, const void **data,
				    metal_phys_addr_t pa, struct metal_io_region *io,
				    char is_blocking);
/** @endcond */

/**
 * @brief image_store_ops based on an ELF being in readable memory
 *
 * This provides image_store_ops that only require the ELF for another processor
 * being in readable memory. The store argument needs to be of the type
 * amp_memcpy_options.
 *
 * @see amp_memcpy_options
 *
 * TODO: Put into a group
 */
static const struct image_store_ops zephyr_openamp_load_memcpy_ops = {
	.open = zephyr_openamp_load_memcpy_open,
	.close = zephyr_openamp_load_memcpy_close,
	.load = zephyr_openamp_load_memcpy_load,
	.features = SUPPORT_SEEK,
};

#endif /* CONFIG_AMP_OPENAMP_OPS_PROVIDE_STORAGE_OPS_MEMCPY */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_AMP_OPENAMP_OPS_MEMORY_H_ */
