/*
 * Copyright (c) 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NVMEM_MMIO_H_
#define _NVMEM_MMIO_H_

#include <zephyr/nvmem.h>

#if defined(CONFIG_NVMEM_MMIO)

int nvmem_mmio_read(const struct nvmem_cell *cell, void *buf, off_t off, size_t len);

int nvmem_mmio_write(const struct nvmem_cell *cell, const void *buf, off_t off, size_t len);

#else /* defined(CONFIG_NVMEM_MMIO) */

#define nvmem_mmio_read(...) -ENXIO
#define nvmem_mmio_write(...) -ENXIO

#endif /* defined(CONFIG_NVMEM_MMIO) */

#endif /* _NVMEM_MMIO_H_ */
