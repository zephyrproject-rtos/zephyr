/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FLASH_MAP_H_
#define ZEPHYR_INCLUDE_FLASH_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * Provides abstraction of flash regions for type of use.
 * I.e. dude where's my image?
 *
 * System will contain a map which contains flash areas. Every
 * region will contain flash identifier, offset within flash and length.
 *
 * 1. This system map could be in a file within filesystem (Initializer
 * must know/figure out where the filesystem is at).
 * 2. Map could be at fixed location for project (compiled to code)
 * 3. Map could be at specific place in flash (put in place at mfg time).
 *
 * Note that the map you use must be valid for BSP it's for,
 * match the linker scripts when platform executes from flash,
 * and match the target offset specified in download script.
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>

struct flash_area {
	u8_t fa_id;
	u8_t fa_device_id;
	u16_t pad16;
	off_t fa_off;
	size_t fa_size;
};

struct flash_sector {
	off_t fs_off;
	size_t fs_size;
};

/*
 * Start using flash area.
 */
int flash_area_open(u8_t id, const struct flash_area **fa);

void flash_area_close(const struct flash_area *fa);

/*
 * Read/write/erase. Offset is relative from beginning of flash area.
 */
int flash_area_read(const struct flash_area *fa, off_t off, void *dst,
		    size_t len);
int flash_area_write(const struct flash_area *fa, off_t off, const void *src,
		     size_t len);
int flash_area_erase(const struct flash_area *fa, off_t off, size_t len);
/* int flash_area_is_empty(const struct flash_area *, bool *); */

/*
 * Alignment restriction for flash writes.
 */
u8_t flash_area_align(const struct flash_area *fa);

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
/*
 * Given flash area ID, return info about sectors within the area.
 */
int flash_area_get_sectors(int fa_id, u32_t *count,
			   struct flash_sector *sectors);

/**
 * Check whether given flash area has supporting flash driver
 * in the system.
 *
 * @param fa Flash area.
 *
 * @return 1 On success. -ENODEV if no driver match.
 */
int flash_area_has_driver(const struct flash_area *fa);

/**
 *  @brief  Get the size and offset of flash sector at certain flash offset.
 *
 *  @param  fa_id flash area ID
 *  @param  offset Offset within the sector
 *  @param  sector Sector Info structure to be filled
 *
 *  @return  0 on success,
 *           -EINVAL  if page of the index doesn't exist,
 *           -ENODEV  if flash area doesn't exist.
 */
int flash_area_get_sector_info_by_offs(int fa_id, off_t offset,
				       struct flash_sector *sector);

/**
 *  @brief  Get the size and offset of flash sector at certain index.
 *
 *  @param  fa_id flash area ID
 *  @param  sector_index Index of the sector. Index are counted from 0.
 *  @param  info Page Info structure to be filled
 *
 *  @return  0 on success,
 *           -EINVAL  if page of the index doesn't exist,
 *           -ENODEV  if flash area doesn't exist.
 */
int flash_area_get_sector_info_by_idx(int fa_id, u32_t sector_index,
				      struct flash_sector *sector);

/**
 *  @brief  Get the total number of flash area sectors.
 *
 *  @param  fa_id flash area ID
 *
 *  @return  Number of flash area sectors.
 */
size_t flash_area_get_sector_count(int fa_id);
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FLASH_MAP_H_ */
