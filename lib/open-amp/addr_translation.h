/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <metal/io.h>

/*
 * Description of a single physical page from the I/O region.
 * Local address is remap to remote address -
 * according to the memory map from the reference manual.
 */
struct phys_page_info {
	const metal_phys_addr_t addr;		/* local address */
	const metal_phys_addr_t remote_addr;	/* remote address */
	const size_t size;
};

/*
 * Table of base physical addresses, local and remote,
 * of the pages in the I/O region, along with size (no_pages)
 */
struct phys_pages {
	size_t no_pages;			/* number of pages */
	const struct phys_page_info *map;	/* table of pages */
};

/**
 * @brief Return generic I/O operations
 *
 * @param	phys	Physical base address of the I/O region
 * @return	metal_io_ops struct
 */
const struct metal_io_ops *addr_translation_get_ops(metal_phys_addr_t phys);
