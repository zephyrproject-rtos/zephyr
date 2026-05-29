/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is for the memory I/O region
 * of the shared memory between HiFi4 DSP and A35 system
 */
static const struct phys_page_info physmap[] = {
	{
		.addr = 0x1bef0000,
		.remote_addr = 0x8fef0000,
		.size = 0x10000000
	}
};

static const struct phys_pages vendor_phys_map = {
	.no_pages = 1,
	.map = physmap
};

/*
 * Return table of base physical addresses for the I/O region
 * that starts with the specified physical base address (phys)
 */
static inline const struct phys_pages *get_phys_map(metal_phys_addr_t phys)
{
	if (phys == physmap[0].addr) {
		return &vendor_phys_map;
	}
	return NULL;
}
