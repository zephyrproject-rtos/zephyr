/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_METAL_METAL_IO_H_
#define ZEPHYR_INCLUDE_METAL_METAL_IO_H_

#include <stddef.h>
#include <stdint.h>
#include <metal/io.h>
#include <zephyr/devicetree.h>

/** @brief Devicetree description of a libmetal I/O mapping. */
struct metal_io_dt_spec {
	/** Locally accessible base address, including protocol headers. */
	uintptr_t region_addr;
	/** Size of the entire backing region in bytes. */
	size_t region_size;
	/** Page addresses in the address space shared with the peer. */
	const metal_phys_addr_t *phys_map;
	/** Number of entries in phys_map. */
	size_t phys_map_size;
	/** Memory flags passed unchanged to libmetal. */
	unsigned int mem_flags;
	/** Log2 of the page size, or -1 for one contiguous page. */
	int page_shift;
};

/** @cond INTERNAL_HIDDEN */
#define Z_METAL_IO_PHYS_ADDR(node_id, prop, idx) \
	((metal_phys_addr_t)(uint32_t)DT_PROP_BY_IDX(node_id, prop, idx))

#define Z_METAL_IO_DT_SPEC_GET(node_id, addr) \
	{ \
		.region_addr = (addr), \
		.region_size = DT_REG_SIZE(DT_PHANDLE(node_id, region)), \
		.phys_map = (const metal_phys_addr_t[]){ \
			DT_FOREACH_PROP_ELEM_SEP(node_id, physmap, Z_METAL_IO_PHYS_ADDR, (,)) \
		}, \
		.phys_map_size = DT_PROP_LEN(node_id, physmap), \
		.mem_flags = DT_PROP_OR(node_id, mem_flags, 0), \
		.page_shift = DT_PROP_OR(node_id, page_shift, -1), \
	}
/** @endcond */

/**
 * @brief Get a libmetal I/O specification from a zephyr,metal-io node.
 *
 * Use at file scope so the compound literal containing the physmap has static
 * storage duration. Libmetal retains the physmap pointer for the mapping's
 * lifetime. The reg address must already be accessible to the caller; this
 * helper does not map physical memory into the CPU's virtual address space.
 *
 * @param node_id Devicetree node identifier for the zephyr,metal-io node.
 * @return Initializer for struct metal_io_dt_spec.
 */
#define METAL_IO_DT_SPEC_GET(node_id) \
	Z_METAL_IO_DT_SPEC_GET(node_id, DT_REG_ADDR(DT_PHANDLE(node_id, region)))

#endif /* ZEPHYR_INCLUDE_METAL_METAL_IO_H_ */
