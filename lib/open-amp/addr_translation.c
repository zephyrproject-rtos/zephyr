/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <addr_translation.h>
/*
 * In this file addr_map needs to be defined,
 * see example in nxp_addr_translation.h
 */
#ifdef CONFIG_OPENAMP_VENDOR_ADDR_TRANSLATION
#include CONFIG_OPENAMP_VENDOR_ADDR_TRANSLATION_FILE
struct regions *addr_map = &vendor_addr_map;
#else
struct regions *addr_map;
#endif

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
	metal_phys_addr_t tmp_phys;
	size_t tmp_size;
	unsigned long tmp_offset = 0;

	if (!addr_map) {
		return METAL_BAD_PHYS;
	}

	for (i = 0; i < addr_map->no_pages; i++) {
		tmp_phys = addr_map->pages[i].phys_start;
		tmp_size = addr_map->pages[i].size;
		if ((tmp_offset <= offset) && (offset < tmp_offset + tmp_size)) {
			return tmp_phys + (offset - tmp_offset);
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
	metal_phys_addr_t tmp_phys;
	size_t tmp_size;
	unsigned long offset = 0;

	if (!addr_map) {
		return METAL_BAD_OFFSET;
	}

	for (i = 0; i < addr_map->no_pages; i++) {
		tmp_phys = addr_map->pages[i].phys_start;
		tmp_size = addr_map->pages[i].size;
		if ((tmp_phys <= phys) && (phys < tmp_phys + tmp_size)) {
			return offset + (phys - tmp_phys);
		}
		offset += tmp_size;
	}

	/* if not found in physical addr, search in bus addr */
	offset = 0;
	for (i = 0; i < addr_map->no_pages; i++) {
		tmp_phys = addr_map->pages[i].bus_start;
		tmp_size = addr_map->pages[i].size;
		if ((phys >= tmp_phys) && (phys < tmp_phys + tmp_size)) {
			return offset + (phys - tmp_phys);
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
 * @param	dev	Driver instance pointer
 * @return	metal_io_ops struct
 */
const struct metal_io_ops *addr_translation_get_ops(void)
{
	if (!addr_map) {
		return NULL;
	}

	return &openamp_addr_translation_ops;
}
