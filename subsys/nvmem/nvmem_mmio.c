/*
 * Copyright (c) 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/device_mmio.h>

#include "nvmem_mmio.h"

int nvmem_mmio_read(const struct nvmem_cell *cell, void *buf, off_t off, size_t len)
{
	mm_reg_t addr;

	device_map(&addr, cell->phys_addr + cell->offset, cell->size, K_MEM_CACHE_NONE);
	memcpy(buf, (const void *)(addr + off), len);
	device_unmap(addr, cell->size);

	return 0;
}

int nvmem_mmio_write(const struct nvmem_cell *cell, const void *buf, off_t off, size_t len)
{
	mm_reg_t addr;

	device_map(&addr, cell->phys_addr + cell->offset, cell->size, K_MEM_CACHE_NONE);
	memcpy((void *)(addr + off), buf, len);
	device_unmap(addr, cell->size);

	return 0;
}
