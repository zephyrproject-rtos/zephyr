/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <addr_translation.h>

/* Address translation table entries */
struct tlb_entries {
	const metal_phys_addr_t *dev_addr;	/* device address */
	const metal_phys_addr_t *drv_addr;	/* driver address */
};

/*
 * In this file physmap_addr needs to be defined,
 * see example in nxp_addr_translation.h
 */
#ifdef CONFIG_OPENAMP_VENDOR_ADDR_TRANSLATION_FILE
#include CONFIG_OPENAMP_VENDOR_ADDR_TRANSLATION_FILE
#endif

/**
 * @brief Converts an offset within an I/O region to a physical address.
 *
 * This helper function calculates the corresponding physical address
 * for a given offset within the memory region based on the provided physical
 * address map.
 *
 * @param io Pointer to the I/O region.
 * @param offset Offset within the I/O region.
 * @param phys Pointer to the array of physical addresses.
 *
 * @return physical address if valid, otherwise METAL_BAD_PHYS.
 */
static metal_phys_addr_t offset_to_phys_helper(struct metal_io_region *io,
					       unsigned long offset,
					       const metal_phys_addr_t *phys)
{
	unsigned long page = (io->page_shift >= sizeof(offset) * CHAR_BIT ?
			     0 : offset >> io->page_shift);

	return (phys && offset < io->size
		? phys[page] + (offset & io->page_mask)
		: METAL_BAD_PHYS);
}

/**
 * @brief Translates an offset within an I/O region to a physical address.
 *
 * This function first attempts to translate the offset using the driver's
 * physical address map. If no valid mapping is found, it falls back to the
 * device physical address map.
 *
 * @param io Pointer to the I/O region.
 * @param offset Offset within the I/O region.
 *
 * @return physical address if valid, otherwise METAL_BAD_PHYS.
 */
static metal_phys_addr_t translate_offset_to_phys(struct metal_io_region *io,
						  unsigned long offset)
{
	metal_phys_addr_t tmp_phys = offset_to_phys_helper(io, offset, physmap_addr.drv_addr);

	if (tmp_phys != METAL_BAD_PHYS) {
		return tmp_phys;
	}

	return offset_to_phys_helper(io, offset, physmap_addr.dev_addr);
}

/**
 * @brief Converts a physical address to an offset within an I/O region.
 *
 * This helper function determines the offset corresponding to a given
 * physical address within the memory region using the provided address map.
 *
 * @param io Pointer to the I/O region.
 * @param phys Physical address to translate.
 * @param map Pointer to the array of physical addresses.
 *
 * @return offset if valid, otherwise METAL_BAD_OFFSET.
 */
static unsigned long phys_to_offset_helper(struct metal_io_region *io,
					   metal_phys_addr_t phys,
					   const metal_phys_addr_t *map)
{
	unsigned long offset =
		(io->page_mask == (metal_phys_addr_t)(-1) ?
		phys - map[0] :  phys & io->page_mask);

	do {
		if (offset_to_phys_helper(io, offset, map) == phys) {
			return offset;
		}
		offset += io->page_mask + 1;
	} while (offset < io->size);

	return METAL_BAD_OFFSET;
}

/**
 * @brief Translates a physical address to an offset within an I/O region.
 *
 * This function first attempts to translate the physical address using the
 * driver's address map. If no valid mapping is found, it falls back to the
 * device address map.
 *
 * @param io Pointer to the I/O region.
 * @param phys Physical address to translate.
 *
 * @return offset if valid, otherwise METAL_BAD_OFFSET.
 */
static unsigned long translate_phys_to_offset(struct metal_io_region *io,
					      metal_phys_addr_t phys)
{
	unsigned long tmp_offset = phys_to_offset_helper(io, phys, physmap_addr.drv_addr);

	if (tmp_offset != METAL_BAD_OFFSET) {
		return tmp_offset;
	}

	return phys_to_offset_helper(io, phys, physmap_addr.dev_addr);
}

/* Address translation operations for OpenAMP */
static const struct metal_io_ops openamp_addr_translation_ops = {
	.phys_to_offset = translate_phys_to_offset,
	.offset_to_phys = translate_offset_to_phys,
};

/**
 * @brief Return generic I/O operations
 *
 * @param	dev	Driver instance pointer
 * @return	metal_io_ops struct
 */
const struct metal_io_ops *metal_io_get_ops(void)
{
	return &openamp_addr_translation_ops;
}
