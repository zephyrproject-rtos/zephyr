/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static const metal_phys_addr_t nxp_dev_addr[] = {0x1bef0000};
static const metal_phys_addr_t nxp_drv_addr[] = {0x8fef0000};

static struct tlb_entries physmap_addr = {
	.dev_addr = nxp_dev_addr,
	.drv_addr = nxp_drv_addr
};
