/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <metal/io.h>

/* Address translation table entries */
struct tlb_entries {
	const metal_phys_addr_t phys_start;	/* device address */
	const metal_phys_addr_t bus_start;	/* device address */
	const size_t size;
};

/* I/O regions */
struct regions {
	size_t no_pages;		/* number of pages */
	struct tlb_entries *pages;	/* I/O pages */
};

/**
 * @brief Return generic I/O operations
 *
 * @param	dev	Driver instance pointer
 * @return	metal_io_ops struct
 */
const struct metal_io_ops *addr_translation_get_ops(void);
