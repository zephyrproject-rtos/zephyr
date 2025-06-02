/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct tlb_entries pages[] = {
	{
		.phys_start = 0x1bef0000,
		.bus_start = 0x8fef0000,
		.size = 0x10000000
	}
};

struct regions vendor_addr_map = {
	.no_pages = 1,
	.pages = pages
};
