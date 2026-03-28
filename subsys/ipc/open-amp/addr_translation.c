/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <addr_translation.h>

#ifdef CONFIG_OPENAMP_VENDOR_ADDR_TRANSLATION_FILE
/*
 * In this file get_phys_pages() needs to be defined,
 * see example in nxp_addr_translation.h
 */
#include CONFIG_OPENAMP_VENDOR_ADDR_TRANSLATION_FILE
#else
/*
 * Return table of base physical addresses for the I/O region
 * that starts with the specified physical base address (phys)
 */
static inline const struct phys_pages *get_phys_map(metal_phys_addr_t phys)
{
	(void)phys;
	return NULL;
}
#endif

static const struct phys_pages *phys_pages;

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
	(void)io;
	size_t i;
	metal_phys_addr_t tmp_addr;
	size_t tmp_size;
	unsigned long tmp_offset = 0;

	if (offset > io->size) {
		return METAL_BAD_PHYS;
	}

	for (i = 0; i < phys_pages->no_pages; i++) {
		tmp_addr = phys_pages->map[i].addr;
		tmp_size = phys_pages->map[i].size;
		if ((tmp_offset <= offset) && (offset < tmp_offset + tmp_size)) {
			return tmp_addr + (offset - tmp_offset);
		}
		tmp_offset += tmp_size;
	}

	return METAL_BAD_PHYS;
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
	(void)io;
	size_t i;
	metal_phys_addr_t tmp_addr;
	size_t tmp_size;
	unsigned long offset = 0;

	for (i = 0; i < phys_pages->no_pages; i++) {
		tmp_addr = phys_pages->map[i].addr;
		tmp_size = phys_pages->map[i].size;
		if ((tmp_addr <= phys) && (phys < tmp_addr + tmp_size)) {
			offset += (phys - tmp_addr);
			return (offset < io->size ? offset : METAL_BAD_OFFSET);
		}
		offset += tmp_size;
	}

	/* if not found in local addr, search in remote addr */
	offset = 0;
	for (i = 0; i < phys_pages->no_pages; i++) {
		tmp_addr = phys_pages->map[i].remote_addr;
		tmp_size = phys_pages->map[i].size;
		if ((phys >= tmp_addr) && (phys < tmp_addr + tmp_size)) {
			offset += (phys - tmp_addr);
			return (offset < io->size ? offset : METAL_BAD_OFFSET);
		}
		offset += tmp_size;
	}

	return METAL_BAD_OFFSET;
}

/* Address translation operations for OpenAMP */
static const struct metal_io_ops openamp_addr_translation_ops = {
	.phys_to_offset = translate_phys_to_offset,
	.offset_to_phys = translate_offset_to_phys,
};

/**
 * @brief Return generic I/O operations
 *
 * @param	phys	Physical base address of the I/O region
 * @return	metal_io_ops struct
 */
const struct metal_io_ops *addr_translation_get_ops(metal_phys_addr_t phys)
{
	phys_pages = get_phys_map(phys);
	if (!phys_pages) {
		return NULL;
	}

	return &openamp_addr_translation_ops;
}
